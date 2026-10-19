/*
** VkShader.h - Vulkan shader management
*/

#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>

class VulkanDevice; // Forward declaration

struct VkPipelineState {
	int32_t tex0Op;
	int32_t tex1Op;
	int32_t alphaOp0;	// internal alpha-op code (0=DISABLE/color-op alpha)
	int32_t alphaOp1;
	int32_t uvSel0;		// D3DTSS_TEXCOORDINDEX per stage (0/1)
	int32_t uvSel1;
	int32_t alphaTest;
	int32_t fog;
	int32_t lighting;
	int32_t alphaFunc;
	uint32_t cullMode;
	uint32_t fillMode;
	int32_t depthTest;
	int32_t depthWrite;
	uint32_t depthCompare;
	int32_t blendEnable;
	uint32_t srcBlend;
	uint32_t dstBlend;
	uint32_t topology;
	uint32_t vertexFVF;
	uint32_t vertexSize;
	uint32_t normalOffset;
	uint32_t diffuseOffset;
	uint32_t uv0Offset;
	uint32_t uv1Offset;
	uint32_t uv2Offset;
	uint32_t uv3Offset;

	// Stencil state
	int32_t stencilEnable;
	uint32_t stencilFailOp;
	uint32_t stencilPassOp;
	uint32_t stencilDepthFailOp;
	uint32_t stencilCompare;
	uint32_t stencilReadMask;
	uint32_t stencilWriteMask;
	uint32_t stencilRef;

	// Color write mask
	uint32_t colorWriteMask;

	bool operator==(const VkPipelineState &other) const;
};

struct VkPipelineStateHash {
	size_t operator()(const VkPipelineState &s) const;
};

struct VkFrameUniforms {
    float ambientColor[4];
    float fogColor[4];
    float fogParams[4];
    float materialDiffuse[4];
    float miscParams[4];
    float tfactorColor[4];
    float lightDirs[4][4];
    float lightColors[4][4];
    // std140 offset 224 - matches GLSL FrameData.drawFlags right after
    // lightColors; flags drawn per-frame by VulkanDrawIndexed.
    uint32_t drawFlags;
    uint32_t _pad0;
    // GLSL puts vec4 lightAtten[4] at 240 (16-aligned after the uint);
    // CPU pads to match.
    uint32_t _pad1[2];	// must be [2]: lightAtten starts at 240 (std140); [3] overshot to 244
    float lightAtten[4][4];	// (att0, att1, att2, range) per light - point lights
    // std140: 304/368/432 - texture-generation matrices for TCI_CAMERA-position
    // texgen (road feather/noise passes): uv = (texMat * view * worldPos).xy
    float texMat0[4][4];
    float texMat1[4][4];
    float viewMat[4][4];
    // std140: 496/560 - native shadow map (light view-projection + params)
    float lightMvp[4][4];
    float shadowParams[4];	// (bias, darken, enabled, unused)
    float worldViewProj[16];
    float world[16];
};

struct VkVertexLayoutDesc {
	VkVertexInputBindingDescription bindings[2];
	uint32_t bindingCount;
	VkVertexInputAttributeDescription attributes[8];
	uint32_t attributeCount;
};

class VkShaderManager
{
public:
    VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }
    VkDescriptorSetLayout GetTextureSetLayout() const { return m_textureSetLayout; }
    VkDescriptorSetLayout GetFrameSetLayout() const { return m_frameSetLayout; }
	VkShaderManager();
	~VkShaderManager();

	VkShaderManager(const VkShaderManager &) = delete;
	VkShaderManager &operator=(const VkShaderManager &) = delete;

	bool Init(VulkanDevice *device, const char *shaderDir);
	void Shutdown();

	VkPipeline_T* GetPipeline(const VkPipelineState &state);
	// Native terrain pipelines (mode:0 base,1 alpha,2 cloud+noise,3 cloud,4 noise).
	VkPipeline_T* GetTerrainPipeline(int mode, uint32_t diffuseOffset, uint32_t uv0Offset, uint32_t uv1Offset);
	void SetVertexLayout(const VkVertexLayoutDesc &layout);
	void ClearPipelineCache();

private:
	VulkanDevice *m_device;
	std::unordered_map<VkPipelineState, VkPipeline_T*, VkPipelineStateHash> m_pipelines;
	VkPipelineCache m_pipelineCache; // Vulkan pipeline cache handle
	VkVertexLayoutDesc m_vertexLayout;
	
	// Shader modules
	VkShaderModule m_vertModule;
	VkShaderModule m_fragModule;
	VkShaderModule m_terrainVertModule = VK_NULL_HANDLE;
	VkShaderModule m_terrainFragModule = VK_NULL_HANDLE;
	std::unordered_map<int, VkPipeline_T*> m_terrainPipelines;
	
	// Descriptor set layouts
	VkDescriptorSetLayout m_textureSetLayout;
	VkDescriptorSetLayout m_frameSetLayout;
	
	// Pipeline layout
	VkPipelineLayout m_pipelineLayout;
	
	// Shader directory
	char m_shaderDir[300];
	
	// Helper functions
	uint32_t FanToList(const uint16_t *fanIndices, uint32_t fanCount, uint16_t *outList);
	VkPipeline_T* BuildPipeline(const VkPipelineState &state);
	VkShaderModule LoadModule(const char *path);
	bool CreateDescriptorLayouts();
	bool CreatePipelineLayout();
	VkVertexLayoutDesc GetVertexLayout(uint32_t fvf, uint32_t fvfSize, uint32_t normalOffset, uint32_t diffuseOffset, uint32_t uv0Offset, uint32_t uv1Offset, uint32_t uv2Offset, uint32_t uv3Offset);
	VkVertexLayoutDesc GetVertexLayout(uint32_t fvf, uint32_t fvfSize);
};
