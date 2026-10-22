#version 450

// Option C M2b: clipmap shroud multiply pass. Replaces the legacy
// ST_SHROUD_TEXTURE tile pass (camera-pos TCI + texMat). The upstream math
// (inv*offset)*scale cancels the view exactly, so uv = (world+off)*scale.

layout(push_constant) uniform PushConsts {
	mat4 mvp;
	float stretch;
	float pad0;
	float pad1;
	float pad2;
	vec4 mapInfo;   // reused as shroudParams: x=offX y=offY z=scaleX w=scaleY
} pc;

layout(location = 0) in vec4 vDiff;
layout(location = 1) in vec2 vUV0;
layout(location = 2) in vec2 vUV1;
layout(location = 3) in vec3 vWorld;
layout(location = 4) in float vStretch;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D shroudTex; // dst shroud (CLAMP)

void main()
{
	vec2 uv = (vWorld.xy + pc.mapInfo.xy) * pc.mapInfo.zw;
	outColor = texture(shroudTex, uv);
}
