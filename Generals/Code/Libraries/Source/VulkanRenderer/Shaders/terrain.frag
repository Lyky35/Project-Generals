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
layout(set = 0, binding = 2) uniform sampler2D shadowMap;	// unused; clipmap hosts bind the page/color texture in this slot
layout(set = 0, binding = 3) uniform sampler2D clipShadowMap;	// PORT round-7: sun-shadow depth map (white when the pass is inactive)

layout(set = 1, binding = 0) uniform FrameData {
	vec4 ambientColor;
	vec4 fogColor;
	vec4 fogParams;
 vec4 materialDiffuse;
 vec4 miscParams;
 vec4 tfactorColor;
 vec4 lightDirs[4];
 vec4 lightColors[4];
 uint drawFlags;
 vec4 lightAtten[4];
 mat4 texMat0;
 mat4 texMat1;
 mat4 viewMat;
 mat4 lightMvp;
 vec4 shadowParams;	// (bias, darken, enabled, unused)
 // mesh.vert passes mvp/world via PUSH CONSTANTS, but the CPU mirror of
 // this shared ring UBO still carries worldViewProj/world at 576..703 -
 // declare the same span so texMat2 lands at 704 matching the upload.
 float vboPadWorldViewProj[16];
 float vboPadWorld[16];
 mat4 texMat2;
} frame;

layout(constant_id = 0) const int MODE = 0;

// SOFT shadow receive (round-7 directive): terrain/ground must catch the same
// PCF-penalumbrided depth-sun shadows as mesh.frag - retail blob shadows fall
// on the ground, so without this the map stays fully lit while only objects
// self-darken (field report). Base + alpha passes only; cloud/noise overlays
// keep multiply semantics untouched.
vec3 shadowTint()
{
 if (frame.shadowParams.z <= 0.5)
	 return vec3(1.0);
 vec4 lp = frame.lightMvp * vec4(vWorld, 1.0);
 if (abs(lp.w) <= 1e-5)
	 return vec3(1.0);
 vec3 lpc = lp.xyz / lp.w;
 // negative-height viewport: ndc +y maps to image row 0 (same as mesh.frag)
 vec2 suv = vec2(lpc.x * 0.5 + 0.5, 0.5 - lpc.y * 0.5);
 if (lpc.z <= 0.0 || lpc.z >= 1.0 ||
	 suv.x <= 0.002 || suv.x >= 0.998 || suv.y <= 0.002 || suv.y >= 0.998)
	 return vec3(1.0);
 float inShade = 0.0;
 const float rad = 3.0 / max(textureSize(clipShadowMap, 0).x, 1.0);
 for (int tx = -1; tx <= 1; tx++) {
	 for (int ty = -1; ty <= 1; ty++) {
	 vec2 off = vec2(float(tx), float(ty)) * rad;
	 float d = texture(clipShadowMap, suv + off).r;
	 inShade += smoothstep(0.0, frame.shadowParams.w, lpc.z - frame.shadowParams.x - d);
	 }
 }
 inShade /= 9.0;
 // ROUND 19: (a) round-14 patch collateral had DELETED the final return of this non-void
 //   function (UB = silent register lottery = lesson-7 class on strict drivers); (b) fade
 //   over the outer 30% of the light box = no hard pop-in ring at the box edge.
 // ROUND 20: radial cam-distance fade REPLACES the box-normalized edge fade above (the
 //   straight suv-guard square boundary clipping unbounded terrain/mesh shadows off-center
 //   + sweeping with rotation = 'giant square shadow / far fans vanish on rotate' class;
 //   see W3DView.cpp rig note). miscParams.yz = game cam world XY; end 3,900u is inside
 //   all coverage limits (XY 5,120u; sun-side z-cut ~3,326u beyond visible range) so no
 //   hard edge survives in-view, and r19 lesson 'coverage edges must dissolve' kept.
 float fade = 1.0 - smoothstep(2500.0, 3900.0,
   length(vWorld.xy - frame.miscParams.yz));
 return vec3(1.0 - inShade * fade * (1.0 - frame.shadowParams.y));
}

void main()
{
 if (MODE == 0) {
	 vec4 t = texture(texA, vUV0);
	 outColor = vec4(t.rgb * vDiff.rgb * shadowTint(), 1.0);
 } else if (MODE == 1) {
	 vec4 t = texture(texA, vUV1);
	 outColor = vec4(t.rgb * vDiff.rgb * shadowTint(), t.a * vDiff.a);
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
