#version 450

// Option C M2a: full-map grid terrain VS. Heights + static colors come from
// whole-map textures (TerrainClipmap); atlas/alpha UVs are baked per-vertex
// (identical data to legacy updateVB). Attribute layout matches terrain.vert
// so the shared vertex layout builder works. MODE0 only (base pass).

layout(location = 0) in vec3 inPos;     // world x,y ; z fetched from HeightTex
layout(location = 1) in vec3 inNormal;  // dummy binding (terrain FVF)
layout(location = 2) in vec4 inDiffuse; // unused; color fetched from ColorTex
layout(location = 3) in vec2 inUV0;     // atlas UV (baked)
layout(location = 4) in vec2 inUV1;     // alpha-map UV (baked)

layout(push_constant) uniform PushConsts {
	mat4 mvp;
	float stretch;
	float pad0;
	float pad1;
	float pad2;
	vec4 mapInfo;   // x=cellsX y=cellsY z=border w=cellSize
} pc;

layout(set = 0, binding = 0) uniform sampler2D texA;      // atlas (frag mode0)
layout(set = 0, binding = 1) uniform sampler2D heightTex; // R16: byte*257 / 65535
layout(set = 0, binding = 2) uniform sampler2D colorTex;  // A8R8G8B8 static light

layout(location = 0) out vec4 vDiff;
layout(location = 1) out vec2 vUV0;
layout(location = 2) out vec2 vUV1;
layout(location = 3) out vec3 vWorld;
layout(location = 4) out float vStretch;

void main()
{
	// texelFetch: integer cell addressing (world/cellSize + border), NO sampler
	// - lavapipe's JIT crashed on texture() in the vertex stage (diag build
	// with fetches stubbed ran215 draws stable vs crash at draw #2).
	ivec2 ic = clamp(ivec2(floor(inPos.xy / pc.mapInfo.w) + ivec2(pc.mapInfo.z)),
		ivec2(0), ivec2(pc.mapInfo.xy) - ivec2(1));
	float h = texelFetch(heightTex, ic, 0).r;
	vec3 world = vec3(inPos.x, inPos.y, h * 255.0 * (pc.mapInfo.w / 16.0));
	vec4 col = texelFetch(colorTex, ic, 0);
	// vertex alpha = per-corner terrain blend weight baked by BuildMesh
	// (legacy updateVB wrote alpha[k]<<24 into the vertex color); the alpha
	// pass multiplies it with the alpha-map texel for tile-to-tile blending
	vDiff = vec4(col.rgb, inDiffuse.a);
	vUV0 = inUV0;
	vUV1 = inUV1;
	vWorld = world;
	vStretch = pc.stretch;
	gl_Position = pc.mvp * vec4(world, 1.0);
}
