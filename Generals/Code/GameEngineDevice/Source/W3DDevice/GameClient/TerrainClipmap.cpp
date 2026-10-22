#include "W3DDevice/GameClient/TerrainClipmap.h"
#include "W3DDevice/GameClient/HeightMap.h"
#include "W3DDevice/GameClient/WorldHeightMap.h"
#include "Common/GlobalData.h"
#include "Common/MapObject.h"
#include "VkTexture.h"
#include "VkDevice.h"
#include "vkgpubuffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>

TerrainClipmap::~TerrainClipmap()
{
	Reset();
}

void TerrainClipmap::Reset()
{
	if (m_vb) { m_vb->Destroy(); delete m_vb; m_vb = 0; }
	if (m_ib) { m_ib->Destroy(); delete m_ib; m_ib = 0; }
	m_idxCount = 0;
	m_chunks.clear();
	delete m_heightTex;
	m_heightTex = 0;
	delete m_colorTex;
	m_colorTex = 0;
	m_map = 0;
	m_built = false;
	m_cellsX = m_cellsY = m_border = 0;
	m_cellSize = 0.0f;
}

void TerrainClipmap::WorldToUV(float wx, float wy, float *u, float *v) const
{
	*u = (wx + (float)m_border * m_cellSize) / (m_cellSize * (float)m_cellsX);
	*v = (wy + (float)m_border * m_cellSize) / (m_cellSize * (float)m_cellsY);
}

void TerrainClipmap::Ensure(WorldHeightMap *map, HeightMapRenderObjClass *heightMap, bool force)
{
	{	static unsigned ep = 0; ep++;
		if (ep <= 12 || (ep % 300) == 0) {
			FILE *lf = fopen("RTS_START.LOG", "a");
			if (lf) { fprintf(lf, "VK: ClipMap Ensure enter #%u force=%d built=%d map=%p hm=%p\n",
				ep, (int)force, (int)m_built, (void*)map, (void*)heightMap); fclose(lf); }
		}
	}
	if (!map || !heightMap) {
		Reset();
		return;
	}
	int w = map->getXExtent();
	int h = map->getYExtent();
	if (w <= 0 || h <= 0) {
		Reset();
		return;
	}
	if (!force && m_built && map == m_map && w == m_cellsX && h == m_cellsY)
		return;   // already valid

	Reset();
	m_map = map;
	m_cellsX = w;
	m_cellsY = h;
	m_border = map->getBorderSize();
	m_cellSize = (float)MAP_XY_FACTOR;

	// ---- heights: raw byte height -> R16 (byte * 257 so /65535 == /255) ----
	std::vector<unsigned short> heights((size_t)w * (size_t)h);
	unsigned short hmin = 0xffff, hmax = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			unsigned short v = (unsigned short)map->getHeight(x, y) * 257u;
			heights[(size_t)y * w + x] = v;
			if (v < hmin) hmin = v;
			if (v > hmax) hmax = v;
		}
	}

	// ---- static lighting colors: same packed value updateVB writes ----
	std::vector<unsigned int> colors((size_t)w * (size_t)h);
	int colorNonz = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			unsigned int c = 0xff000000u | ((unsigned int)heightMap->getStaticDiffuse(x, y) & 0x00ffffffu);
			colors[(size_t)y * w + x] = c;
			if (c & 0x00ffffff) colorNonz++;
		}
	}

	bool ok = false;
	VulkanDevice &dev = VulkanDevice::Instance();

	m_heightTex = new VkTexture();
	if (m_heightTex->Create(&dev, (uint32_t)w, (uint32_t)h, VK_FORMAT_R16_UNORM, 1, false)) {
		m_heightTex->UploadLevel(0, (uint32_t)w, (uint32_t)h, heights.data());
		ok = true;
	} else {
		delete m_heightTex; m_heightTex = 0;
	}

	m_colorTex = new VkTexture();
	if (m_colorTex->Create(&dev, (uint32_t)w, (uint32_t)h, VK_FORMAT_B8G8R8A8_UNORM, 1, false)) {
		m_colorTex->UploadLevel(0, (uint32_t)w, (uint32_t)h, colors.data());
	} else {
		delete m_colorTex; m_colorTex = 0;
	}
	if (!m_colorTex) ok = false;

	m_built = ok;
	if (m_built)
		BuildMesh(map);   // M2a grid (legacy quad-corner topology, baked UVs)

	FILE *lf = fopen("RTS_START.LOG", "a");
	if (lf) {
		fprintf(lf, "VK: ClipMap %s cells=%dx%d border=%d cellSize=%.1f height=[%u..%u] colorNonz=%d/%d htex=%p ctex=%p mesh=%ux%u idx=%u\n",
			m_built ? "built" : "FAILED", w, h, m_border, m_cellSize,
			(unsigned)hmin, (unsigned)hmax, colorNonz, w * h,
			(void*)m_heightTex, (void*)m_colorTex,
			HasMesh() ? (unsigned)m_cellsX : 0, HasMesh() ? (unsigned)m_cellsY : 0, m_idxCount);
		fclose(lf);
	}
}

// ---------------------------------------------------------------------------
// M2a: whole-map quad-corner grid - EXACT legacy topology from
// HeightMapRenderObjClass initHeightData (4 verts/cell: TL,TR,BR,BL; indices
// [i,i+2,i+3],[i,i+1,i+2]; FLIP_TRIANGLES rotates the vertex payload) with
// atlas/alpha UVs from getUVData/getAlphaUVData. Pos.z is filled in the VS
// from HeightTex; diffuse attr is unused (ColorTex in VS).
// ---------------------------------------------------------------------------
struct ClipmapVert {
	float x, y, z;
	unsigned int diff;
	float u0, v0;   // atlas (legacy uv set0 / FVF TEXCOORD0)
	float u1, v1;   // alpha-map (uv set1)
};

// M5: index-buffer chunk size in cells (32x32 = 2048 idx/chunk). Chunks get
// bounding spheres computed from the height data for per-frame frustum culling.
static const int CLIPMAP_CHUNK_CELLS = 32;

void TerrainClipmap::BuildMesh(WorldHeightMap *map)
{
	const int w = m_cellsX, h = m_cellsY;
	// verts: w*h cells * 4 corners. M5 raised the guard (chunked culling makes
	// big maps cheap to draw; memory = cells*4*32B ~ 64MB at 500k cells).
	if (w * h > 500000) {
		FILE *lf = fopen("RTS_START.LOG", "a");
		if (lf) { fprintf(lf, "VK: ClipMap mesh SKIPPED (map %dx%d too big for full-grid)\n", w, h); fclose(lf); }
		return;
	}
	std::vector<ClipmapVert> verts((size_t)w * h * 4);
	const float cs = m_cellSize;
	const int border = m_border;
	size_t vi = 0;
	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) {
			float U[4], V[4], UA[4], VA[4];
			unsigned char alpha[4];
			Bool flip = false;
			map->getUVData(i, j, U, V, false);
			map->getAlphaUVData(i, j, UA, VA, alpha, &flip, false);
			ClipmapVert corners[4];
			// TL, TR, BR, BL (matches legacy corner order + IB topology)
			for (int k = 0; k < 4; k++) {
				int cx = i + (k == 1 || k == 2 ? 1 : 0);
				int cy = j + (k >= 2 ? 1 : 0);
				corners[k].x = (float)(cx - border) * cs;
				corners[k].y = (float)(cy - border) * cs;
				corners[k].z = 0.0f;              // VS fetches HeightTex
				// blend alpha in the high byte (legacy updateVB wrote
				// alpha[k]<<24 here); ColorTex supplies the RGB. The alpha
				// pass (terrain mode1) blends with t.a * vDiff.a - without
				// this the per-vertex weight was forced to1 and adjacent
				// terrain tiles met with a hard edge (no blending).
				corners[k].diff = ((unsigned int)alpha[k] << 24);
				corners[k].u0 = U[k];
				corners[k].v0 = V[k];
				corners[k].u1 = UA[k];
				corners[k].v1 = VA[k];
			}
			if (flip) { // FLIPTriangles: rotate payload TL<-TR<-BR<-BL<-TL
				ClipmapVert tmp = corners[0];
				corners[0] = corners[1];
				corners[1] = corners[2];
				corners[2] = corners[3];
				corners[3] = tmp;
			}
			verts[vi++] = corners[0];
			verts[vi++] = corners[1];
			verts[vi++] = corners[2];
			verts[vi++] = corners[3];
		}
	}
	// M5: emit the IB CHUNK-MAJOR so each chunk owns a contiguous index range
	// (vertex IDs stay cell-major: cell (i,j) -> verts (j*w+i)*4 .. +3).
	std::vector<unsigned int> idx((size_t)w * h * 6);
	size_t ii = 0;
	{
		const int chunkW = CLIPMAP_CHUNK_CELLS;
		const int chunksX = (w + chunkW - 1) / chunkW;
		const int chunksY = (h + chunkW - 1) / chunkW;
		for (int cy = 0; cy < chunksY; cy++) {
			for (int cx = 0; cx < chunksX; cx++) {
				int x0 = cx * chunkW, y0 = cy * chunkW;
				int x1 = x0 + chunkW; if (x1 > w) x1 = w;
				int y1 = y0 + chunkW; if (y1 > h) y1 = h;
				for (int j = y0; j < y1; j++) {
					for (int i = x0; i < x1; i++) {
						unsigned int b = (unsigned int)((j * w + i) * 4);
						idx[ii++] = b;     idx[ii++] = b + 2; idx[ii++] = b + 3;
						idx[ii++] = b;     idx[ii++] = b + 1; idx[ii++] = b + 2;
					}
				}
			}
		}
	}
	// M5: partition the (already cell-major) index buffer into chunks and give
	// each a bounding sphere (world AABB of its cells x min/max height).
	const int chunkW = CLIPMAP_CHUNK_CELLS;
	const int chunksX = (w + chunkW - 1) / chunkW;
	const int chunksY = (h + chunkW - 1) / chunkW;
	m_chunks.clear();
	m_chunks.reserve((size_t)chunksX * chunksY);
	// (cs / border already in scope from the vertex loop above)
	for (int cy = 0; cy < chunksY; cy++) {
		for (int cx = 0; cx < chunksX; cx++) {
			int x0 = cx * chunkW, y0 = cy * chunkW;
			int x1 = x0 + chunkW; if (x1 > w) x1 = w;
			int y1 = y0 + chunkW; if (y1 > h) y1 = h;
			Chunk ck;
			ck.indexCount = (unsigned)((size_t)(y1 - y0) * (x1 - x0) * 6);
			ck.firstIndex = 0;	// filled below (IB is chunk-major)
			// world XY of the cell-corner block
			float minX = (float)(x0 - border) * cs;
			float maxX = (float)(x1 - border) * cs;
			float minY = (float)(y0 - border) * cs;
			float maxY = (float)(y1 - border) * cs;
			float minZ = 1e9f, maxZ = -1e9f;
			for (int y = y0; y < y1; y++) {
				for (int x = x0; x < x1; x++) {
					// cell (x,y) corners span heights at (x,y)..(x+1,y+1)
					for (int yy = y; yy <= y + 1 && yy < h; yy++) {
						for (int xx = x; xx <= x + 1 && xx < w; xx++) {
							float z = (float)map->getHeight(xx, yy) * (cs / 16.0f);
							if (z < minZ) minZ = z;
							if (z > maxZ) maxZ = z;
						}
					}
				}
			}
			if (minZ > maxZ) { minZ = 0.0f; maxZ = 0.0f; }
			ck.cx = (minX + maxX) * 0.5f;
			ck.cy = (minY + maxY) * 0.5f;
			ck.cz = (minZ + maxZ) * 0.5f;
			float ex = (maxX - minX) * 0.5f;
			float ey = (maxY - minY) * 0.5f;
			float ez = (maxZ - minZ) * 0.5f;
			ck.radius = sqrtf(ex * ex + ey * ey + ez * ez);
			m_chunks.push_back(ck);
		}
	}
	// assign contiguous firstIndex in the same chunk-major order as the IB
	{
		unsigned acc = 0;
		for (size_t i = 0; i < m_chunks.size(); i++) {
			m_chunks[i].firstIndex = acc;
			acc += m_chunks[i].indexCount;
		}
	}

	VulkanDevice &dev = VulkanDevice::Instance();
	m_vb = new VkGpuBuffer();
	if (!m_vb->Create(&dev, verts.size() * sizeof(ClipmapVert),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
		delete m_vb; m_vb = 0;
		FILE *lf = fopen("RTS_START.LOG", "a");
		if (lf) { fprintf(lf, "VK: ClipMap VB create FAILED\n"); fclose(lf); }
		return;
	}
	m_vb->Upload(verts.data(), verts.size() * sizeof(ClipmapVert));
	m_ib = new VkGpuBuffer();
	if (!m_ib->Create(&dev, idx.size() * sizeof(unsigned int),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
		delete m_ib; m_ib = 0;
		return;
	}
	m_ib->Upload(idx.data(), idx.size() * sizeof(unsigned int));
	m_idxCount = (unsigned)idx.size();
	{
		FILE *lf = fopen("RTS_START.LOG", "a");
		if (lf) {
			fprintf(lf, "VK: ClipMap chunks=%d (%dx%d cells each) totalIdx=%u\n",
				(int)m_chunks.size(), chunkW, chunkW, m_idxCount);
			fclose(lf);
		}
	}
}
