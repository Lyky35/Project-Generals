#pragma once

#include <windows.h>
#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>
#include <unordered_map>

class VulkanDevice;

struct VkPipelineState {
	int32_t tex0Op;
	int32_t tex1Op;
	int32_t alphaTest;
	int32_t fog;
	int32_t lighting;
	uint32_t cullMode;
	uint32_t fillMode;
	int32_t depthTest;
	int32_t depthWrite;
	uint32_t depthCompare;
	int32_t blendEnable;
	uint32_t srcBlend;
	uint32_t dstBlend;
	uint32_t topology;
	uint32_t vertexLayout;

	bool operator==(const VkPipelineState &other) const;
};

struct VkPipelineStateHash {
	size_t operator()(const VkPipelineState &s) const;
};

struct VkVertexLayoutDesc {
	VkVertexInputBindingDescription binding;
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
	VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }
	VkDescriptorSetLayout GetTextureSetLayout() const { return m_textureSetLayout; }
	VkDescriptorSetLayout GetFrameSetLayout() const { return m_frameSetLayout; }

	static VkVertexLayoutDesc GetVertexLayout(uint32_t layoutId);
	static uint32_t FanToList(const uint16_t *fanIndices, uint32_t fanCount, uint16_t *outList);

	enum VertexLayout {
		LAYOUT_XYZ_N_UV1 = 0,
		LAYOUT_XYZ_N_D_UV2 = 1,
		LAYOUT_XYZ_D_UV1 = 2,
		LAYOUT_COUNT = 3
	};

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
