#include "VkShader.h"
#include "VkDevice.h"
#include <cstdio>
#include <cstring>

bool VkPipelineState::operator==(const VkPipelineState &other) const
{
	return tex0Op == other.tex0Op && tex1Op == other.tex1Op &&
		alphaTest == other.alphaTest && fog == other.fog &&
		lighting == other.lighting && cullMode == other.cullMode &&
		fillMode == other.fillMode && depthTest == other.depthTest &&
		depthWrite == other.depthWrite && depthCompare == other.depthCompare &&
		blendEnable == other.blendEnable && srcBlend == other.srcBlend &&
		dstBlend == other.dstBlend && topology == other.topology &&
		vertexLayout == other.vertexLayout;
}

size_t VkPipelineStateHash::operator()(const VkPipelineState &s) const
{
	size_t h = 1469598103934665603ULL;
	const int32_t *words = (const int32_t *)&s;
	for (size_t i = 0; i < sizeof(VkPipelineState) / sizeof(int32_t); i++) {
		h ^= (size_t)(uint32_t)words[i];
		h *= 1099511628211ULL;
	}
	return h;
}

VkShaderManager::VkShaderManager()
	: m_device(nullptr)
	, m_vertModule(VK_NULL_HANDLE)
	, m_fragModule(VK_NULL_HANDLE)
	, m_textureSetLayout(VK_NULL_HANDLE)
	, m_frameSetLayout(VK_NULL_HANDLE)
	, m_pipelineLayout(VK_NULL_HANDLE)
{
	m_shaderDir[0] = '\0';
}

VkShaderManager::~VkShaderManager()
{
	Shutdown();
}

bool VkShaderManager::Init(VulkanDevice *device, const char *shaderDir)
{
	m_device = device;
	strncpy(m_shaderDir, shaderDir, sizeof(m_shaderDir) - 1);
	m_shaderDir[sizeof(m_shaderDir) - 1] = '\0';

	char vertPath[300];
	char fragPath[300];
	snprintf(vertPath, sizeof(vertPath), "%s/mesh.vert.spv", shaderDir);
	snprintf(fragPath, sizeof(fragPath), "%s/mesh.frag.spv", shaderDir);
	m_vertModule = LoadModule(vertPath);
	m_fragModule = LoadModule(fragPath);
	if (m_vertModule == VK_NULL_HANDLE || m_fragModule == VK_NULL_HANDLE)
		return false;
	if (!CreateDescriptorLayouts())
		return false;
	return CreatePipelineLayout();
}

void VkShaderManager::Shutdown()
{
	if (!m_device)
		return;
	VkDevice dev = m_device->GetDevice();
	for (auto &entry : m_pipelineCache)
		vkDestroyPipeline(dev, entry.second, nullptr);
	m_pipelineCache.clear();
	if (m_pipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(dev, m_pipelineLayout, nullptr);
		m_pipelineLayout = VK_NULL_HANDLE;
	}
	if (m_textureSetLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(dev, m_textureSetLayout, nullptr);
		m_textureSetLayout = VK_NULL_HANDLE;
	}
	if (m_frameSetLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(dev, m_frameSetLayout, nullptr);
		m_frameSetLayout = VK_NULL_HANDLE;
	}
	if (m_vertModule != VK_NULL_HANDLE) {
		vkDestroyShaderModule(dev, m_vertModule, nullptr);
		m_vertModule = VK_NULL_HANDLE;
	}
	if (m_fragModule != VK_NULL_HANDLE) {
		vkDestroyShaderModule(dev, m_fragModule, nullptr);
		m_fragModule = VK_NULL_HANDLE;
	}
	m_device = nullptr;
}

VkShaderModule VkShaderManager::LoadModule(const char *path)
{
	FILE *file = fopen(path, "rb");
	if (!file)
		return VK_NULL_HANDLE;
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (size <= 0 || (size % 4) != 0) {
		fclose(file);
		return VK_NULL_HANDLE;
	}
	std::vector<uint32_t> code((size_t)size / 4);
	if (fread(code.data(), 1, (size_t)size, file) != (size_t)size) {
		fclose(file);
		return VK_NULL_HANDLE;
	}
	fclose(file);
	VkShaderModuleCreateInfo moduleInfo = {};
	moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleInfo.codeSize = (size_t)size;
	moduleInfo.pCode = code.data();
	VkShaderModule module = VK_NULL_HANDLE;
	if (vkCreateShaderModule(m_device->GetDevice(), &moduleInfo, nullptr, &module) != VK_SUCCESS)
		return VK_NULL_HANDLE;
	return module;
}

bool VkShaderManager::CreateDescriptorLayouts()
{
	VkDescriptorSetLayoutBinding texBindings[2] = {};
	for (int i = 0; i < 2; i++) {
		texBindings[i].binding = (uint32_t)i;
		texBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		texBindings[i].descriptorCount = 1;
		texBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	}
	VkDescriptorSetLayoutCreateInfo texLayoutInfo = {};
	texLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	texLayoutInfo.bindingCount = 2;
	texLayoutInfo.pBindings = texBindings;
	if (vkCreateDescriptorSetLayout(m_device->GetDevice(), &texLayoutInfo, nullptr, &m_textureSetLayout) != VK_SUCCESS)
		return false;

	VkDescriptorSetLayoutBinding frameBinding = {};
	frameBinding.binding = 0;
	frameBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	frameBinding.descriptorCount = 1;
	frameBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	VkDescriptorSetLayoutCreateInfo frameLayoutInfo = {};
	frameLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	frameLayoutInfo.bindingCount = 1;
	frameLayoutInfo.pBindings = &frameBinding;
	if (vkCreateDescriptorSetLayout(m_device->GetDevice(), &frameLayoutInfo, nullptr, &m_frameSetLayout) != VK_SUCCESS)
		return false;
	return true;
}

bool VkShaderManager::CreatePipelineLayout()
{
	VkPushConstantRange pushRange = {};
	pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushRange.offset = 0;
	pushRange.size = 128;
	VkDescriptorSetLayout layouts[] = { m_textureSetLayout, m_frameSetLayout };
	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 2;
	layoutInfo.pSetLayouts = layouts;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &pushRange;
	return vkCreatePipelineLayout(m_device->GetDevice(), &layoutInfo, nullptr, &m_pipelineLayout) == VK_SUCCESS;
}

VkVertexLayoutDesc VkShaderManager::GetVertexLayout(uint32_t layoutId)
{
	VkVertexLayoutDesc desc = {};
	desc.binding.binding = 0;
	desc.attributeCount = 0;
	uint32_t offset = 0;
	switch (layoutId) {
	case LAYOUT_XYZ_N_UV1:
		desc.binding.stride = 32;
		desc.binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		desc.attributes[0].location = 0;
		desc.attributes[0].binding = 0;
		desc.attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		desc.attributes[0].offset = 0;
		desc.attributes[1].location = 1;
		desc.attributes[1].binding = 0;
		desc.attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		desc.attributes[1].offset = 12;
		desc.attributes[2].location = 3;
		desc.attributes[2].binding = 0;
		desc.attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
		desc.attributes[2].offset = 24;
		desc.attributeCount = 3;
		break;
	case LAYOUT_XYZ_N_D_UV2:
		desc.binding.stride = 44;
		desc.binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		desc.attributes[0].location = 0;
		desc.attributes[0].binding = 0;
		desc.attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		desc.attributes[0].offset = 0;
		desc.attributes[1].location = 1;
		desc.attributes[1].binding = 0;
		desc.attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		desc.attributes[1].offset = 12;
		desc.attributes[2].location = 2;
		desc.attributes[2].binding = 0;
		desc.attributes[2].format = VK_FORMAT_B8G8R8A8_UNORM;
		desc.attributes[2].offset = 24;
		desc.attributes[3].location = 3;
		desc.attributes[3].binding = 0;
		desc.attributes[3].format = VK_FORMAT_R32G32_SFLOAT;
		desc.attributes[3].offset = 28;
		desc.attributes[4].location = 4;
		desc.attributes[4].binding = 0;
		desc.attributes[4].format = VK_FORMAT_R32G32_SFLOAT;
		desc.attributes[4].offset = 36;
		desc.attributeCount = 5;
		break;
	case LAYOUT_XYZ_D_UV1:
		desc.binding.stride = 24;
		desc.binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		desc.attributes[0].location = 0;
		desc.attributes[0].binding = 0;
		desc.attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		desc.attributes[0].offset = 0;
		desc.attributes[1].location = 2;
		desc.attributes[1].binding = 0;
		desc.attributes[1].format = VK_FORMAT_B8G8R8A8_UNORM;
		desc.attributes[1].offset = 12;
		desc.attributes[2].location = 3;
		desc.attributes[2].binding = 0;
		desc.attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
		desc.attributes[2].offset = 16;
		desc.attributeCount = 3;
		break;
	default:
		break;
	}
	(void)offset;
	return desc;
}

uint32_t VkShaderManager::FanToList(const uint16_t *fanIndices, uint32_t fanCount, uint16_t *outList)
{
	if (fanCount < 3 || !fanIndices || !outList)
		return 0;
	uint32_t outCount = 0;
	for (uint32_t i = 1; i + 1 < fanCount; i++) {
		outList[outCount++] = fanIndices[0];
		outList[outCount++] = fanIndices[i];
		outList[outCount++] = fanIndices[i + 1];
	}
	return outCount;
}

static VkCullModeFlags TranslateCull(uint32_t cullMode)
{
	switch (cullMode) {
	case 2:
		return VK_CULL_MODE_FRONT_BIT;
	case 3:
		return VK_CULL_MODE_BACK_BIT;
	default:
		break;
	}
	return VK_CULL_MODE_NONE;
}

static VkPolygonMode TranslateFill(uint32_t fillMode)
{
	if (fillMode == 2)
		return VK_POLYGON_MODE_LINE;
	if (fillMode == 1)
		return VK_POLYGON_MODE_POINT;
	return VK_POLYGON_MODE_FILL;
}

static VkCompareOp TranslateCompare(uint32_t cmp)
{
	switch (cmp) {
	case 1:
		return VK_COMPARE_OP_NEVER;
	case 2:
		return VK_COMPARE_OP_LESS;
	case 3:
		return VK_COMPARE_OP_EQUAL;
	case 4:
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	case 5:
		return VK_COMPARE_OP_GREATER;
	case 6:
		return VK_COMPARE_OP_NOT_EQUAL;
	case 7:
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	default:
		break;
	}
	return VK_COMPARE_OP_ALWAYS;
}

static VkBlendFactor TranslateBlend(uint32_t blend)
{
	switch (blend) {
	case 1:
		return VK_BLEND_FACTOR_ZERO;
	case 2:
		return VK_BLEND_FACTOR_ONE;
	case 3:
		return VK_BLEND_FACTOR_SRC_COLOR;
	case 4:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case 5:
		return VK_BLEND_FACTOR_SRC_ALPHA;
	case 6:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case 7:
		return VK_BLEND_FACTOR_DST_ALPHA;
	case 8:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	case 9:
		return VK_BLEND_FACTOR_DST_COLOR;
	case 10:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	case 11:
		return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
	default:
		break;
	}
	return VK_BLEND_FACTOR_ONE;
}

static VkPrimitiveTopology TranslateTopology(uint32_t topology)
{
	switch (topology) {
	case 4:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	case 5:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	case 6:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
	case 2:
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case 3:
		return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	case 1:
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	default:
		break;
	}
	return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

VkPipeline VkShaderManager::GetPipeline(const VkPipelineState &state)
{
	auto it = m_pipelineCache.find(state);
	if (it != m_pipelineCache.end())
		return it->second;
	VkPipeline pipeline = BuildPipeline(state);
	if (pipeline != VK_NULL_HANDLE)
		m_pipelineCache[state] = pipeline;
	return pipeline;
}

VkPipeline VkShaderManager::BuildPipeline(const VkPipelineState &state)
{
	VkVertexLayoutDesc layout = GetVertexLayout(state.vertexLayout);

	VkPipelineShaderStageCreateInfo vertStage = {};
	vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertStage.module = m_vertModule;
	vertStage.pName = "main";

	VkSpecializationMapEntry specEntries[6];
	int32_t specValues[6] = {
		state.tex0Op, state.tex1Op, state.alphaTest,
		state.fog, state.lighting, 0
	};
	for (int i = 0; i < 6; i++) {
		specEntries[i].constantID = (uint32_t)i;
		specEntries[i].offset = (uint32_t)(i * sizeof(int32_t));
		specEntries[i].size = sizeof(int32_t);
	}
	VkSpecializationInfo specInfo = {};
	specInfo.mapEntryCount = 6;
	specInfo.pMapEntries = specEntries;
	specInfo.dataSize = sizeof(specValues);
	specInfo.pData = specValues;

	VkPipelineShaderStageCreateInfo fragStage = {};
	fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStage.module = m_fragModule;
	fragStage.pName = "main";
	fragStage.pSpecializationInfo = &specInfo;

	VkPipelineVertexInputStateCreateInfo vertexInput = {};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.pVertexBindingDescriptions = &layout.binding;
	vertexInput.vertexAttributeDescriptionCount = layout.attributeCount;
	vertexInput.pVertexAttributeDescriptions = layout.attributes;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = TranslateTopology(state.topology);
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_device->GetSwapExtent().width;
	viewport.height = (float)m_device->GetSwapExtent().height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = m_device->GetSwapExtent();
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = TranslateFill(state.fillMode);
	rasterizer.cullMode = TranslateCull(state.cullMode);
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisample = {};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = state.depthTest ? VK_TRUE : VK_FALSE;
	depthStencil.depthWriteEnable = state.depthWrite ? VK_TRUE : VK_FALSE;
	depthStencil.depthCompareOp = TranslateCompare(state.depthCompare);

	VkPipelineColorBlendAttachmentState blendAttachment = {};
	blendAttachment.blendEnable = state.blendEnable ? VK_TRUE : VK_FALSE;
	blendAttachment.srcColorBlendFactor = TranslateBlend(state.srcBlend);
	blendAttachment.dstColorBlendFactor = TranslateBlend(state.dstBlend);
	blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendStateCreateInfo blendState = {};
	blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendState.attachmentCount = 1;
	blendState.pAttachments = &blendAttachment;

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	VkPipelineShaderStageCreateInfo stages[] = { vertStage, fragStage };
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = stages;
	pipelineInfo.pVertexInputState = &vertexInput;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisample;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &blendState;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = m_device->GetRenderPass();
	pipelineInfo.subpass = 0;

	VkPipeline pipeline = VK_NULL_HANDLE;
	if (vkCreateGraphicsPipelines(m_device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
		return VK_NULL_HANDLE;
	return pipeline;
}
