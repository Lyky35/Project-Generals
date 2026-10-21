#include "W3DDevice/GameClient/TerrainClipmap.h"
#include "W3DDevice/GameClient/HeightMap.h"
#include "W3DDevice/GameClient/WorldHeightMap.h"
#include "Common/GlobalData.h"
#include "Common/MapObject.h"
#include "VkTexture.h"
#include "VkDevice.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>

TerrainClipmap::~TerrainClipmap()
{
	Reset();
}

void TerrainClipmap::Reset()
{
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

	FILE *lf = fopen("RTS_START.LOG", "a");
	if (lf) {
		fprintf(lf, "VK: ClipMap %s cells=%dx%d border=%d cellSize=%.1f height=[%u..%u] colorNonz=%d/%d htex=%p ctex=%p\n",
			m_built ? "built" : "FAILED", w, h, m_border, m_cellSize,
			(unsigned)hmin, (unsigned)hmax, colorNonz, w * h,
			(void*)m_heightTex, (void*)m_colorTex);
		fclose(lf);
	}
}
