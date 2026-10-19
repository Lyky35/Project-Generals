#version 450

// Native terrain fragment shader. MODE (spec constant):
// 0 = base: atlas(uv0) x vertexDiffuse, opaque
// 1 = alpha: atlas(uv1), alpha-blended (pipeline: SRC_ALPHA/INV_SRC_ALPHA)
// 2 = cloud x noise multiply (pipeline: DST_COLOR/ZERO)
// 3 = cloud only multiply    4 = noise only multiply
// Noise/cloud uv = worldPos.xy * stretch - algebraically identical to the
// legacy TCI_CAMERASPACEPOSITION + inv(view)*STRETCH texture matrix (the
// view cancels exactly), with zero state-translation machinery.

layout(location = 0) in vec4 vDiff;
layout(location = 1) in vec2 vUV0;
layout(location = 2) in vec2 vUV1;
layout(location = 3) in vec3 vWorld;
layout(location = 4) in float vStretch;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texA;
layout(set = 0, binding = 1) uniform sampler2D texB;
layout(set = 0, binding = 2) uniform sampler2D shadowMap;	// unused; matches texture-set layout

layout(constant_id = 0) const int MODE = 0;

void main()
{
	if (MODE == 0) {
		vec4 t = texture(texA, vUV0);
		outColor = vec4(t.rgb * vDiff.rgb, 1.0);
	} else if (MODE == 1) {
		vec4 t = texture(texA, vUV1);
		outColor = vec4(t.rgb * vDiff.rgb, t.a * vDiff.a);
	} else if (MODE == 2) {
		vec2 uv = vWorld.xy * vStretch;
		vec4 c = texture(texA, uv);
		vec4 n = texture(texB, uv);
		outColor = vec4(c.rgb * n.rgb, 1.0);
	} else if (MODE == 3) {
		vec4 c = texture(texA, vWorld.xy * vStretch);
		outColor = vec4(c.rgb, 1.0);
	} else {
		vec4 n = texture(texA, vWorld.xy * vStretch);
		outColor = vec4(n.rgb, 1.0);
	}
}
