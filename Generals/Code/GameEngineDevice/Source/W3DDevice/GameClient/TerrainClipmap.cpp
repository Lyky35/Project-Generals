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

void TerrainClipmap::BuildMesh(WorldHeightMap *map)
{
	const int w = m_cellsX, h = m_cellsY;
	// verts: w*h cells * 4 corners; guard uint32-safe but keep sane (< 1M)
	if (w * h > 250000) {
		FILE *lf = fopen("RTS_START.LOG", "a");
		if (lf) { fprintf(lf, "VK: ClipMap mesh SKIPPED (map %dx%d too big for M2a full-grid)\n", w, h); fclose(lf); }
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
				corners[k].diff = 0;              // VS fetches ColorTex
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
	std::vector<unsigned int> idx((size_t)w * h * 6);
	size_t ii = 0;
	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) {
			unsigned int b = (unsigned int)((j * w + i) * 4);
			idx[ii++] = b;     idx[ii++] = b + 2; idx[ii++] = b + 3;
			idx[ii++] = b;     idx[ii++] = b + 1; idx[ii++] = b + 2;
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
}
