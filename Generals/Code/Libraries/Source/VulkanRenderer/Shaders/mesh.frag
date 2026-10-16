#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inDiffuse;
layout(location = 3) in vec2 inUV0;
layout(location = 4) in vec2 inUV1;
layout(location = 5) in float inViewDist;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texStage0;
layout(set = 0, binding = 1) uniform sampler2D texStage1;

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
} frame;

layout(constant_id = 0) const int TEX0_OP = 1;
layout(constant_id = 1) const int TEX1_OP = 0;
layout(constant_id = 2) const int ALPHATEST = 0;
layout(constant_id = 3) const int FOG = 0;
layout(constant_id = 4) const int LIGHTING = 0;
layout(constant_id = 5) const int ALPHAFUNC = 0;

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
		// Port fix: draws without a bound texture get the white-fallback
		// substitution on the CPU side; sampling that 1x1 white image returns
		// BLACK on AMD (menus' real textures sample fine) -> bright diffuse
		// becomes black terrain. Skip the sample entirely for flagged draws.
		vec4 texel = ((frame.drawFlags & 1u) != 0u) ? vec4(1.0) : texture(texStage0, inUV0);
		color = applyTexOp(color, alpha, texel, TEX0_OP, alpha);
	}
	if (TEX1_OP > 0) {
		vec4 texel = ((frame.drawFlags & 256u) != 0u) ? vec4(1.0) : texture(texStage1, inUV1);
		color = applyTexOp(color, alpha, texel, TEX1_OP, alpha);
	}

	if (LIGHTING == 1) {
		vec3 n = normalize(inNormal);
		vec3 lit = frame.ambientColor.rgb;
		for (int i = 0; i < 4; i++) {
			float ndl = max(dot(n, -frame.lightDirs[i].xyz), 0.0);
			lit += frame.lightColors[i].rgb * (ndl * frame.lightDirs[i].w);
		}
		color *= lit;
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
