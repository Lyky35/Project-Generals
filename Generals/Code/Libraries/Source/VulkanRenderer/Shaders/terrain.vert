#version 450

// Native terrain vertex shader - bypasses the D3D8 texture-stage-state
// translation entirely. FVF = XYZ|DIFFUSE|TEX2 (same attribute locations as
// the mesh pipeline so the shared vertex layout builder works).

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;	// dummy binding for terrain FVF (no normal)
layout(location = 2) in vec4 inDiffuse;
layout(location = 3) in vec2 inUV0;
layout(location = 4) in vec2 inUV1;

layout(push_constant) uniform PushConsts {
	mat4 mvp;
	float stretch;	// world-space noise/cloud UV scale (legacy: inv(view)*STRETCH texgen)
} pc;

layout(location = 0) out vec4 vDiff;
layout(location = 1) out vec2 vUV0;
layout(location = 2) out vec2 vUV1;
layout(location = 3) out vec3 vWorld;
layout(location = 4) out float vStretch;

void main()
{
	gl_Position = pc.mvp * vec4(inPos, 1.0);
	vDiff = inDiffuse;
	vUV0 = inUV0;
	vUV1 = inUV1;
	vWorld = inPos;
	vStretch = pc.stretch;
}
