#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inDiffuse;
layout(location = 3) in vec2 inUV0;
layout(location = 4) in vec2 inUV1;
layout(location = 5) in float inViewDist;
layout(location = 6) in vec2 inUV2;
layout(location = 7) in vec2 inUV3;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texStage0;
layout(set = 0, binding = 1) uniform sampler2D texStage1;
layout(set = 0, binding = 2) uniform sampler2D shadowMap;
layout(set = 0, binding = 3) uniform sampler2D texStage2;

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

layout(constant_id = 0) const int TEX0_OP = 1;
layout(constant_id = 1) const int TEX1_OP = 0;
layout(constant_id = 2) const int ALPHATEST = 0;
layout(constant_id = 3) const int FOG = 0;
layout(constant_id = 4) const int LIGHTING = 0;
layout(constant_id = 5) const int ALPHAFUNC = 0;
layout(constant_id = 6) const int ALPHAOP0 = 0;
layout(constant_id = 7) const int ALPHAOP1 = 0;
layout(constant_id = 8) const int UVSEL0 = 0;
layout(constant_id = 9) const int UVSEL1 = 0;
// PORT (round-6 water env-bump): stage-2 noise sampler feeding op7 below;
// engine sets TEX2_OP>0 only on the trapezoidWater main pass.
layout(constant_id = 10) const int TEX2_OP = 0;
layout(constant_id = 11) const int UVSEL2 = 0;

// UV selector: 0..3 = passthrough UV channels (W3D FVF tex slots);
// 4 = TCI_CAMERASPACEPOSITION, 5 = TCI_CAMERASPACENORMAL,
// 6 = TCI_CAMERASPACEREFLECTIONVECTOR (D3D8 texture-stage texgen).
vec2 selectUV(int sel, mat4 texMat)
{
	if (sel == 1) return inUV1;
	if (sel == 2) return inUV2;
	if (sel == 3) return inUV3;
	if (sel >= 4) {
		vec4 vp = frame.viewMat * vec4(inWorldPos, 1.0);
		if (sel == 4)
			return (texMat * vp).xy;
		vec3 nv = normalize((frame.viewMat * vec4(inNormal, 0.0)).xyz);
		if (sel == 5)
			return (texMat * vec4(nv, 1.0)).xy;
		// sel == 6: reflection vector, eye at camera-space origin
		vec3 r = reflect(normalize(vp.xyz), nv);
		return (texMat * vec4(r, 1.0)).xy;
	}
	return inUV0;
}

vec3 applyTexOp(vec3 current, float currentAlpha, vec4 texel, int op, out float outAlpha)
{
	outAlpha = currentAlpha;
	if (op == 1) {
		outAlpha = texel.a;
		return texel.rgb;
	} else if (op == 2) {
		outAlpha = currentAlpha * texel.a;
		return current * texel.rgb;
	} else if (op == 3) {
		return current + texel.rgb;
	} else if (op == 4) {
		outAlpha = frame.tfactorColor.a;
		return frame.tfactorColor.rgb;
	} else if (op == 5) {
		// SELECTARG2: ARG2 is the diffuse = the shader's current color
		return current;
	}
	return current;
}

void main()
{

	// Port fix: D3D8 only folds the material diffuse into the color for LIT
	// passes; unlit/prelit meshes (terrain etc. - material diffuse often 0)
	// use the vertex color alone. Multiplying unconditionally made the whole
	// shell map render black.
	// Port fix 2: COLORVERTEX off / DIFFUSEMATERIALSOURCE=MATERIAL => the
	// diffuse comes from the MATERIAL, not the (black) vertex attribute.
	vec4 base = inDiffuse;
	if ((frame.drawFlags & 2u) != 0u)
		base = frame.materialDiffuse;
	vec3 color = base.rgb;
	float alpha = base.a;
	if (LIGHTING == 1 && (frame.drawFlags & 2u) == 0u) {
		// bit2 already substituted the material as the diffuse source - folding
		// it in again would square it
		color *= frame.materialDiffuse.rgb;
		alpha *= frame.materialDiffuse.a;
	}

	if (TEX0_OP > 0) {
		vec2 uv0sel = selectUV(UVSEL0, frame.texMat0);
		vec4 texel = texture(texStage0, uv0sel);
		float aIn = alpha;
		color = applyTexOp(color, alpha, texel, TEX0_OP, alpha);
		// D3DTSS_ALPHAOP: alpha channel of this stage is computed INDEPENDENTLY
		// of the color op (blend masks ride in texture alpha for terrain).
		if (ALPHAOP0 == 1) alpha = texel.a;
		else if (ALPHAOP0 == 2) alpha = base.a;
		else if (ALPHAOP0 == 3) alpha = aIn;
		else if (ALPHAOP0 == 4) alpha = frame.tfactorColor.a;
		else if (ALPHAOP0 == 5) alpha = aIn * texel.a;
		else if (ALPHAOP0 == 6) alpha = aIn + texel.a;
	}
	if (TEX1_OP > 0) {
		vec2 uv1sel = selectUV(UVSEL1, frame.texMat1);
		vec4 texel = texture(texStage1, uv1sel);
		float aIn = alpha;
		if (TEX1_OP == 7) {
			// PORT op7 = ADDPRODUCT: trapezoidWater ps.1.1 line "mad r0.rgb, t1, t2, r0"
			// - raw unsigned sparkle texel * raw unsigned noise texel ADDED to prev
			// (unsigned raw = faithful to ps.1.1; no 2x-1 signed conversion). Noise
			// sampler uses CAMERASPACEPOSITION texgen: uv = (texMat2 * view * worldPos).xy
			// fed by the engine's D3DTS_TEXTURE2 world->noise matrix.
			vec4 noise = vec4(0.0);
			if (TEX2_OP > 0)
				noise = texture(texStage2, selectUV(UVSEL2, frame.texMat2));
			color = min(color + texel.rgb * noise.rgb, vec3(1.0));
		} else {
			color = applyTexOp(color, alpha, texel, TEX1_OP, alpha);
			if (ALPHAOP1 == 1) alpha = texel.a;
			else if (ALPHAOP1 == 2) alpha = base.a;
			else if (ALPHAOP1 == 3) alpha = aIn;
			else if (ALPHAOP1 == 4) alpha = frame.tfactorColor.a;
			else if (ALPHAOP1 == 5) alpha = aIn * texel.a;
			else if (ALPHAOP1 == 6) alpha = aIn + texel.a;
		}
	}

	if (LIGHTING == 1) {
		vec3 n = normalize(inNormal);
		vec3 lit = frame.ambientColor.rgb;
		for (int i = 0; i < 4; i++) {
			if (frame.lightDirs[i].w <= 0.0)
				continue;
			if (frame.lightColors[i].a > 1.5) {
				// point light: lightDirs.xyz = position, lightAtten = att/range
				vec3 L = frame.lightDirs[i].xyz - inWorldPos;
				float d = length(L);
				if (d < 1e-4)
					continue;
				float range = frame.lightAtten[i].w;
				if (range > 0.0 && d > range)
					continue;
				float att = 1.0 / max(frame.lightAtten[i].x +
					frame.lightAtten[i].y * d + frame.lightAtten[i].z * d * d, 1e-4);
				float ndl = max(dot(n, L / d), 0.0);
				lit += frame.lightColors[i].rgb * (ndl * att);
			} else {
				float ndl = max(dot(n, -frame.lightDirs[i].xyz), 0.0);
				lit += frame.lightColors[i].rgb * ndl;
			}
		}
		// PORT FIX: ambient + up to4 unclamped light diffs could exceed1 and
		// saturate lit surfaces to white (mission sun + map point lights +
		// selection/tint adds). Clamp the light multiplier.
		lit = min(lit, vec3(1.0));
		color *= lit;
	}

	// Native shadow map: darken where the fragment is occluded in light space
	if (frame.shadowParams.z > 0.5) {
		vec4 lp = frame.lightMvp * vec4(inWorldPos, 1.0);
		if (abs(lp.w) > 1e-5) {
			vec3 lpc = lp.xyz / lp.w;
			// negative-height viewport: ndc +y maps to image row 0
			vec2 suv = vec2(lpc.x * 0.5 + 0.5, 0.5 - lpc.y * 0.5);
			if (lpc.z > 0.0 && lpc.z < 1.0 &&
				suv.x > 0.002 && suv.x < 0.998 && suv.y > 0.002 && suv.y < 0.998) {
				float d = texture(shadowMap, suv).r;
				if (lpc.z - frame.shadowParams.x > d) {
					color *= frame.shadowParams.y;
				}
			}
		}
	}

	if (ALPHATEST == 1) {
		if (ALPHAFUNC == 0 && alpha < frame.miscParams.x) {
			discard;
		} else if (ALPHAFUNC == 1 && alpha > frame.miscParams.x) {
			discard;
		}
	}

	if (FOG == 1) {
		// D3D fog uses EYE distance; inViewDist was distance from the world
		// ORIGIN (fogged out anything far from 0,0,0)
		float viewDist = length(inWorldPos - frame.miscParams.yzw);
		float factor = clamp((viewDist - frame.fogParams.x) / max(frame.fogParams.y - frame.fogParams.x, 1e-5), 0.0, 1.0);
		color = mix(color, frame.fogColor.rgb, factor);
	}

	outColor = vec4(color, alpha);
}
