#include "W3DDevice/GameClient/TerrainClipmap.h"
#include "W3DDevice/GameClient/HeightMap.h"
#include "W3DDevice/GameClient/WorldHeightMap.h"
#include "Common/GlobalData.h"
#include "Common/MapObject.h"
#include "GameLogic/TerrainLogic.h"
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
	m_staticColors.clear();
	m_dynScratch.clear();
	m_dynamicLit = false;
	m_cellsX = m_cellsY = m_border = 0;
	m_cellSize = 0.0f;
}

void TerrainClipmap::WorldToUV(float wx, float wy, float *u, float *v) const
{
	*u = (wx + (float)m_border * m_cellSize) / (m_cellSize * (float)m_cellsX);
	*v = (wy + (float)m_border * m_cellSize) / (m_cellSize * (float)m_cellsY);
}

void TerrainClipmap::UploadColors(const unsigned int *pixels)
{
	if (!m_colorTex || !pixels)
		return;
	m_colorTex->UploadLevel(0, (uint32_t)m_cellsX, (uint32_t)m_cellsY, pixels);
}

void TerrainClipmap::Ensure(WorldHeightMap *map, HeightMapRenderObjClass *heightMap, bool force)
{
	{	static unsigned ep = 0; ep++;
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
	unsigned sunk = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			unsigned short v = (unsigned short)map->getHeight(x, y) * 257u;
			// PORT (round-7 water black void, root cause): ZH flattens heightmap cells
			// under water to the waterline at load, so the GPU clipmap bed sits COPLANAR
			// with the water plane; raising the water quads WATER_Z_BIAS above it cannot
			// win a LESS test against their own bed, and the legacy coplanar EQUAL only
			// worked by bit-exact luck (angle-dependent = the field 'black void'). Sink
			// underwater cells to the true lake bed (below the water plane) so LESS
			// passes deterministically and the bed reads THROUGH the translucent water.
			{	Real wx = (Real)(x - m_border) * m_cellSize + 0.5f * m_cellSize;
				Real wy = (Real)(y - m_border) * m_cellSize + 0.5f * m_cellSize;
				Real waterZ = 0.0f;
				if (TheTerrainLogic && TheTerrainLogic->isUnderwater(wx, wy, &waterZ)) {
					Real bedZ = waterZ + 12.0f;	// W3D +Z = DOWN (camera z=-998 above) => lake bed is waterZ+12
					Real byteF = bedZ * 16.0f / m_cellSize;	// inverse of VS z = byte*cellSize/16
					if (byteF < 0.0f) byteF = 0.0f;
					unsigned short bv = (unsigned short)(byteF * 257.0f);
					// PORT round-7b (empirical, ENS probe): ZH zeroes lake cells (raw byte 0 =
					// bed vertex ABOVE the water plane, z=0 vs 137) => center of every lake had
					// NO bed to lose-against: water quads LESS-fail vs that ceiling = black void
					// (field 'black void at angles' = the bed ceiling moving). Sink must RAISE the
					// byte (bigger byte = bigger z = deeper DOWN): v = max(v, bv). The old min-guard
					// was exactly backwards and never wrote anything on flattened cells.
					if (bv > v) v = bv;
					sunk++;
				} else if (v == 0) {
					// PORT round-7b (z-encode probe, BEDDEF): ZH zeroes the heightmap under the
					// ENTIRE lake body - a strictly wider region than the water PolygonTrigger,
					// so isUnderwater() misses those cells. Bed vertex z=0 then sits ABOVE the
					// water plane (camera z=-998 DOWN convention) = staircase black void inside
					// every lake (field 'black void' + 'water still broken'). Zero-byte cells ARE
					// lake beds (real land keeps byte>=1): clamp them below the waterline so the
					// water quads LESS-pass and read their color (bed texture page stays black -
					// the translucent water over black bed = the retail dark-blue look).
					v = 238u * 257u;
					sunk++;
				}
			}
			heights[(size_t)y * w + x] = v;
			if (v < hmin) hmin = v;
			if (v > hmax) hmax = v;
		}
	}

	// ---- static lighting colors: same packed value updateVB writes ----
	// Kept as m_staticColors: baseline for per-frame dynamic-light re-bakes
	// (explosions/muzzle flashes) under the clipmap - see UploadColors.
	m_staticColors.resize((size_t)w * (size_t)h);
	m_dynScratch.resize((size_t)w * (size_t)h);
	m_dynamicLit = false;
	std::vector<unsigned int> &colors = m_staticColors;
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
}
