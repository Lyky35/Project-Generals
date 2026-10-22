#version 450

// Option C M5: cloud/noise multiply passes over the clipmap mesh (terrain
// modes 2-4). Same algebra as terrain.frag modes 2-4: uv = world * stretch.
// binding0 = texA (cloud, or noise for mode4); binding1 = heightTex (VS);
// binding2 = texB (noise, mode2 only) or colorTex (unused by this FS).
// The clipmap VS still texelFetches binding1/2 - harmless here (vDiff unused).

layout(push_constant) uniform PushConsts {
	mat4 mvp;
	float stretch;
	float pad0;
	float pad1;
	float pad2;
	vec4 mapInfo;
} pc;

layout(location = 0) in vec4 vDiff;
layout(location = 1) in vec2 vUV0;
layout(location = 2) in vec2 vUV1;
layout(location = 3) in vec3 vWorld;
layout(location = 4) in float vStretch;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texA;
layout(set = 0, binding = 2) uniform sampler2D texB;

layout(constant_id = 0) const int MODE = 2;

void main()
{
	vec2 uv = vWorld.xy * vStretch;
	if (MODE == 2) {
		vec4 c = texture(texA, uv);
		vec4 n = texture(texB, uv);
		outColor = vec4(c.rgb * n.rgb, 1.0);
	} else {
		outColor = vec4(texture(texA, uv).rgb, 1.0);
	}
}
