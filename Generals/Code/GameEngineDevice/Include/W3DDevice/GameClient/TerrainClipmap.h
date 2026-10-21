#pragma once
#ifndef __TERRAINCLIPMAP_H_
#define __TERRAINCLIPMAP_H_

// ---------------------------------------------------------------------------
// Option C (beta branch): native GPU terrain via clipmap.
// M1 (this file): bakes ONE height texture (R16: byte height -> byte*257 so
//   z = r * 255 * MAP_HEIGHT_SCALE reproduces the legacy vertex height exactly)
//   and ONE static-lighting color texture (A8R8G8B8 = getStaticDiffuse with
//   forced alpha) for the WHOLE map (borders included, cell = MAP_XY_FACTOR
//   world units, index = world/cellSize + border - same convention as
//   check()/shroud). Data-only; the draw path lands in M2 behind RTS_CLIPMAP=1
//   with the legacy tile renderer remaining the default.
// ---------------------------------------------------------------------------

class VkTexture;
class VkGpuBuffer;
class WorldHeightMap;
class HeightMapRenderObjClass;

class TerrainClipmap
{
public:
	TerrainClipmap() : m_heightTex(0), m_colorTex(0), m_vb(0), m_ib(0), m_idxCount(0),
		m_map(0), m_built(false),
		m_cellsX(0), m_cellsY(0), m_border(0), m_cellSize(0.0f) {}
	~TerrainClipmap();

	// Rebuilds when the map/dims change or textures were released (device
	// loss). force=TRUE after initHeightData (map just (re)attached).
	// Cheap no-op once built - safe to call per frame.
	void Ensure(WorldHeightMap *map, HeightMapRenderObjClass *heightMap, bool force = false);
	void Reset();

	bool IsBuilt(void) const { return m_built; }
	int GetCellsX(void) const { return m_cellsX; }
	int GetCellsY(void) const { return m_cellsY; }
	int GetBorder(void) const { return m_border; }
	float GetCellSize(void) const { return m_cellSize; }

	// world -> texture UV in [0,1] over the full grid (borders included)
	void WorldToUV(float wx, float wy, float *u, float *v) const;

	VkTexture *PeekHeightTex(void) const { return m_heightTex; }
	VkTexture *PeekColorTex(void) const { return m_colorTex; }

	// M2a mesh: full-map quad-corner grid (legacy topology), atlas/alpha UVs
	// baked; drawn by VulkanDrawClipmapPass when RTS_CLIPMAP=1 (mode0 only).
	bool HasMesh(void) const { return m_vb != 0 && m_ib != 0; }
	VkGpuBuffer *GetVb(void) const { return m_vb; }
	VkGpuBuffer *GetIb(void) const { return m_ib; }
	unsigned GetIdxCount(void) const { return m_idxCount; }

private:
	void BuildMesh(WorldHeightMap *map);

	VkTexture *m_heightTex;
	VkTexture *m_colorTex;
	VkGpuBuffer *m_vb;
	VkGpuBuffer *m_ib;
	unsigned m_idxCount;
	WorldHeightMap *m_map;   // not ref-counted: callers re-Ensure every frame
	bool m_built;
	int m_cellsX, m_cellsY, m_border;
	float m_cellSize;
};

#endif // __TERRAINCLIPMAP_H_
