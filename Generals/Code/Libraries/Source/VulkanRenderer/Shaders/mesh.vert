#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inDiffuse;
layout(location = 3) in vec2 inUV0;
layout(location = 4) in vec2 inUV1;
layout(location = 5) in vec2 inUV2;
layout(location = 6) in vec2 inUV3;

layout(push_constant) uniform PushConsts {
	mat4 mvp;
	mat4 world;
} pc;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 outDiffuse;
layout(location = 3) out vec2 outUV0;
layout(location = 4) out vec2 outUV1;
layout(location = 5) out float outViewDist;
layout(location = 6) out vec2 outUV2;
layout(location = 7) out vec2 outUV3;

void main()
{
	vec4 worldPos = pc.world * vec4(inPos, 1.0);
	gl_Position = pc.mvp * vec4(inPos, 1.0);
	outWorldPos = worldPos.xyz;
	outNormal = normalize((pc.world * vec4(inNormal, 0.0)).xyz);
	outDiffuse = inDiffuse;
	outUV0 = inUV0;
	outUV1 = inUV1;
	outUV2 = inUV2;
	outUV3 = inUV3;
	outViewDist = length(worldPos.xyz);
}
