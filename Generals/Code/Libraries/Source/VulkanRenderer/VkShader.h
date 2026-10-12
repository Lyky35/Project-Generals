/*
** VkShader.h - Vulkan shader management
*/

#pragma once

#include "VulkanDevice.h"
#include "VkPipeline.h"
#include <vector>
#include <unordered_map>

struct VkPipelineState {
	int32_t tex0Op;
	int32_t tex1Op;
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

struct VkVertexLayoutDesc {
	VkVertexInputBindingDescription bindings[2];
	uint32_t bindingCount;
	VkVertexInputAttributeDescription attributes[5];
	uint32_t attributeCount;
};

class VkShaderManager
{
public:
	VkShaderManager();
	~VkShaderManager();

	VkShaderManager(const VkShaderManager &) = delete;
	VkShaderManager &operator=(const VkShaderManager &) = delete;

	bool Init(VulkanDevice *device, const char *shaderDir);
	void Shutdown();

	VkPipeline_T* GetPipeline(const VkPipelineState &state);
	void SetVertexLayout(const VkVertexLayoutDesc &layout);

private:
	VulkanDevice *m_device;
	std::unordered_map<VkPipelineState, VkPipeline_T*, VkPipelineStateHash> m_pipelines;
	VkPipelineCache m_pipelineCache;
	VkVertexLayoutDesc m_vertexLayout;
};

};

struct VkVertexLayoutDesc {
	VkVertexInputBindingDescription bindings[2];
	uint32_t bindingCount;
	VkVertexInputAttributeDescription attributes[5];
	uint32_t attributeCount;
};

class VkShaderManager
{
public:
	VkShaderManager();
	~VkShaderManager();

	VkShaderManager(const VkShaderManager &) = delete;
	VkShaderManager &operator=(const VkShaderManager &) = delete;

	bool Init(VulkanDevice *device, const char *shaderDir);
	void Shutdown();

	VkPipeline GetPipeline(const VkPipelineState &state);
	void ClearPipelineCache();
	VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }
	VkDescriptorSetLayout GetTextureSetLayout() const { return m_textureSetLayout; }
	VkDescriptorSetLayout GetFrameSetLayout() const { return m_frameSetLayout; }

	static VkVertexLayoutDesc GetVertexLayout(uint32_t fvf, uint32_t fvfSize,
		uint32_t normalOffset, uint32_t diffuseOffset,
		uint32_t uv0Offset, uint32_t uv1Offset);
	static uint32_t FanToList(const uint16_t *fanIndices, uint32_t fanCount, uint16_t *outList);

	enum TexOp {
		TEXOP_DISABLE = 0,
		TEXOP_SELECTARG1 = 1,
		TEXOP_MODULATE = 2,
		TEXOP_ADD = 3
	};

private:
	VkShaderModule LoadModule(const char *path);
	bool CreateDescriptorLayouts();
	bool CreatePipelineLayout();
	VkPipeline BuildPipeline(const VkPipelineState &state);

	VulkanDevice *m_device;
	char m_shaderDir[260];
	VkShaderModule m_vertModule;
	VkShaderModule m_fragModule;
	VkDescriptorSetLayout m_textureSetLayout;
	VkDescriptorSetLayout m_frameSetLayout;
	VkPipelineLayout m_pipelineLayout;
	std::unordered_map<VkPipelineState, VkPipeline, VkPipelineStateHash> m_pipelineCache;
};
