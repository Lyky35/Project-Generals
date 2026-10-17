#include "VkShader.h"
#include "VkDevice.h"
#include <cstdio>
#include <cstring>

bool VkPipelineState::operator==(const VkPipelineState &other) const
{
	return tex0Op == other.tex0Op && tex1Op == other.tex1Op &&
		alphaTest == other.alphaTest && fog == other.fog &&
		lighting == other.lighting && alphaFunc == other.alphaFunc && cullMode == other.cullMode &&
		fillMode == other.fillMode && depthTest == other.depthTest &&
		depthWrite == other.depthWrite && depthCompare == other.depthCompare &&
		blendEnable == other.blendEnable && srcBlend == other.srcBlend &&
		dstBlend == other.dstBlend && colorWriteMask == other.colorWriteMask &&
		stencilEnable == other.stencilEnable && stencilFailOp == other.stencilFailOp &&
		stencilPassOp == other.stencilPassOp && stencilDepthFailOp == other.stencilDepthFailOp &&
		stencilCompare == other.stencilCompare && stencilRef == other.stencilRef &&
		stencilReadMask == other.stencilReadMask && stencilWriteMask == other.stencilWriteMask &&
		topology == other.topology &&
		vertexFVF == other.vertexFVF && vertexSize == other.vertexSize &&
		normalOffset == other.normalOffset && diffuseOffset == other.diffuseOffset &&
		uv0Offset == other.uv0Offset && uv1Offset == other.uv1Offset;
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
	, m_pipelineCache(VK_NULL_HANDLE)
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
	if (!CreatePipelineLayout())
		return false;

	// Driver pipeline cache - lets the driver reuse compiled state between
	// pipeline builds (and keeps vkCreateGraphicsPipelines cheap).
	{
		VkPipelineCacheCreateInfo pci = {};
		pci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		if (vkCreatePipelineCache(m_device->GetDevice(), &pci, nullptr, &m_pipelineCache) != VK_SUCCESS)
			m_pipelineCache = VK_NULL_HANDLE;
	}
	return true;
}

void VkShaderManager::Shutdown()
{
	if (!m_device)
		return;
	VkDevice dev = m_device->GetDevice();
	for (auto &entry : m_pipelines)
		vkDestroyPipeline(dev, entry.second, nullptr);
	m_pipelines.clear();
	if (m_pipelineCache != VK_NULL_HANDLE) {
		vkDestroyPipelineCache(dev, m_pipelineCache, nullptr);
		m_pipelineCache = VK_NULL_HANDLE;
	}
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
	if (!file) {
		static bool lf1 = false;
		if (!lf1) { lf1 = true; FILE *lf = fopen("RTS_START.LOG", "a"); if (lf) { fprintf(lf, "VK: shader OPEN FAILED '%s'\n", path); fclose(lf); } }
		return VK_NULL_HANDLE;
	}
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	// Breadcrumb: which SPIR-V is actually loaded (new build frag = 4172 B,
	// original Oct-13 = 6932 B - tells us whether the zip's shaders/ were
	// extracted over the old ones).
	{
		FILE *lf = fopen("RTS_START.LOG", "a");
		if (lf) { fprintf(lf, "VK: shader loaded '%s' bytes=%ld\n", path, size); fclose(lf); }
	}
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

VkVertexLayoutDesc VkShaderManager::GetVertexLayout(uint32_t fvf, uint32_t fvfSize,
	uint32_t normalOffset, uint32_t diffuseOffset,
	uint32_t uv0Offset, uint32_t uv1Offset)
{
	VkVertexLayoutDesc desc = {};
	desc.bindings[0].binding = 0;
	desc.bindings[0].stride = fvfSize;
	desc.bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	desc.bindings[1].binding = 1;
	desc.bindings[1].stride = 64;
	// INSTANCE rate: draws use instanceCount=1 so every vertex reads dummy
	// vertex 0. PER_VERTEX with stride 64 on a 64-byte buffer fetched out of
	// bounds for vertexIndex >= 1 (undefined attributes on strict drivers).
	desc.bindings[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
	desc.bindingCount = 2;
	desc.attributeCount = 0;
	(void)fvf;

	VkVertexInputAttributeDescription *attr = desc.attributes;
	attr->location = 0;
	attr->binding = 0;
	attr->format = VK_FORMAT_R32G32B32_SFLOAT;
	attr->offset = 0;
	attr++;
	desc.attributeCount++;

	attr->location = 1;
	attr->format = VK_FORMAT_R32G32B32_SFLOAT;
	if (normalOffset != UINT32_MAX) {
		attr->binding = 0;
		attr->offset = normalOffset;
	} else {
		attr->binding = 1;
		attr->offset = 0;
	}
	attr++;
	desc.attributeCount++;

	attr->location = 2;
	attr->format = VK_FORMAT_B8G8R8A8_UNORM;
	if (diffuseOffset != UINT32_MAX) {
		attr->binding = 0;
		attr->offset = diffuseOffset;
	} else {
		attr->binding = 1;
		attr->offset = 12;
	}
	attr++;
	desc.attributeCount++;

	attr->location = 3;
	attr->format = VK_FORMAT_R32G32_SFLOAT;
	if (uv0Offset != UINT32_MAX) {
		attr->binding = 0;
		attr->offset = uv0Offset;
	} else {
		attr->binding = 1;
		attr->offset = 16;
	}
	attr++;
	desc.attributeCount++;

	attr->location = 4;
	attr->format = VK_FORMAT_R32G32_SFLOAT;
	if (uv1Offset != UINT32_MAX) {
		attr->binding = 0;
		attr->offset = uv1Offset;
	} else {
		attr->binding = 1;
		attr->offset = 24;
	}
	attr++;
	desc.attributeCount++;
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
	// Port decision: the world MVP carries negative x/y scales (view/rotation
	// + projection conventions vs D3D) and Vulkan's negative-height viewport
	// flips screen winding - VK_CULL_MODE_BACK then discards the world's
	// front faces on AMD while menu quads (identity transform) survive.
	// Every successful render so far (wine terrain, the magenta probe) ran
	// with cull OFF; every black run had cull ON. Render double-sided.
	(void)cullMode;
	return VK_CULL_MODE_NONE;
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
	case 12:
		return VK_BLEND_FACTOR_SRC_ALPHA;			// D3DBLEND_BOTHSRCALPHA (color channel)
	case 13:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;	// D3DBLEND_BOTHINVSRCALPHA
	default:
		break;
	}
	return VK_BLEND_FACTOR_ONE;
}

static VkStencilOp TranslateStencilOp(uint32_t op)
{
	switch (op) {
	case 1:
		return VK_STENCIL_OP_KEEP;
	case 2:
		return VK_STENCIL_OP_ZERO;
	case 3:
		return VK_STENCIL_OP_REPLACE;
	case 4:
		return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
	case 5:
		return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
	case 6:
		return VK_STENCIL_OP_INVERT;
	case 7:
		return VK_STENCIL_OP_INCREMENT_AND_WRAP;
	case 8:
		return VK_STENCIL_OP_DECREMENT_AND_WRAP;
	default:
		break;
	}
	return VK_STENCIL_OP_KEEP;
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

void VkShaderManager::ClearPipelineCache()
{
	if (!m_device)
		return;
	VkDevice dev = m_device->GetDevice();
	for (auto &entry : m_pipelines)
		vkDestroyPipeline(dev, entry.second, nullptr);
	m_pipelines.clear();
}

VkPipeline VkShaderManager::GetPipeline(const VkPipelineState &state)
{
	auto it = m_pipelines.find(state);
	if (it != m_pipelines.end())
		return it->second;
	VkPipeline pipeline = BuildPipeline(state);
	if (pipeline != VK_NULL_HANDLE)
		m_pipelines[state] = pipeline;

	// Breadcrumb: unbounded pipeline builds = cache never hits (perf + GPU
	// memory death). Log the first few and every 500th with the state.
	{
		static unsigned buildCount = 0;
		buildCount++;
		if (buildCount <= 25 || (buildCount % 500) == 0)
		{
			FILE *lf = fopen("RTS_START.LOG", "a");
			if (lf)
			{
				fprintf(lf, "VK: pipeline build #%u fvf=0x%x topo=%u tex0=%d tex1=%d blend=%d cull=%u z=%d/%d zfunc=%d fog=%d light=%d alphaTest=%d sten=%d/sfunc=%d blend=%d/%d cached=%u ok=%d\n",
					buildCount, (unsigned)state.vertexFVF, (unsigned)state.topology,
					(int)state.tex0Op, (int)state.tex1Op, (int)state.blendEnable,
					(unsigned)state.cullMode, (int)state.depthTest, (int)state.depthWrite,
					(int)state.depthCompare, (int)state.fog, (int)state.lighting, (int)state.alphaTest,
					(int)state.stencilEnable, (int)state.stencilCompare,
					(int)state.srcBlend, (int)state.dstBlend,
					(unsigned)m_pipelines.size(), pipeline != VK_NULL_HANDLE);
				fclose(lf);
			}
		}
	}
	return pipeline;
}

VkPipeline VkShaderManager::BuildPipeline(const VkPipelineState &state)
{
	VkVertexLayoutDesc layout = GetVertexLayout(state.vertexFVF, state.vertexSize,
		state.normalOffset, state.diffuseOffset, state.uv0Offset, state.uv1Offset);

	VkPipelineShaderStageCreateInfo vertStage = {};
	vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertStage.module = m_vertModule;
	vertStage.pName = "main";

	VkSpecializationMapEntry specEntries[6];
	int32_t specValues[6] = {
		state.tex0Op, state.tex1Op, state.alphaTest,
		state.fog, state.lighting, state.alphaFunc
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
	vertexInput.vertexBindingDescriptionCount = layout.bindingCount;
	vertexInput.pVertexBindingDescriptions = layout.bindings;
	vertexInput.vertexAttributeDescriptionCount = layout.attributeCount;
	vertexInput.pVertexAttributeDescriptions = layout.attributes;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = TranslateTopology(state.topology);
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = TranslateFill(state.fillMode);
	rasterizer.cullMode = TranslateCull(state.cullMode);
	// D3D8 front faces are wound CLOCKWISE; the Vulkan default of CCW inverts
	// the convention and back-face-culled every D3D-front primitive (the whole
	// menu rendered as nothing). Match D3D: clockwise = front.
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisample = {};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = state.depthTest ? VK_TRUE : VK_FALSE;
	depthStencil.depthWriteEnable = state.depthWrite ? VK_TRUE : VK_FALSE;
	depthStencil.depthCompareOp = TranslateCompare(state.depthCompare);
	// Port fix (AMD black world): W3DScene enables D3DRS_STENCILENABLE with
	// FUNC=LESS/EQUAL around scene passes, so scene pipelines carry a stencil
	// test - but the depth attachment format is driver-chosen: llvmpipe picked
	// D16_UNORM_S8 (field log 129 = HAS stencil plane, engine semantics work)
	// while the AMD card picked D32_SFLOAT (field log 126 = NO stencil plane).
	// Testing stencil against a nonexistent plane is UB: AMD rejects the whole
	// world (black), lavapipe renders it. The ported stencil-driven features
	// (selection markers / behind-building selection) use engine-side
	// Has_Stencil() fallbacks, so stencil ops are cosmetic here - never test.
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front.failOp = TranslateStencilOp(state.stencilFailOp);
	depthStencil.front.passOp = TranslateStencilOp(state.stencilPassOp);
	depthStencil.front.depthFailOp = TranslateStencilOp(state.stencilDepthFailOp);
	depthStencil.front.compareOp = TranslateCompare(state.stencilCompare);
	depthStencil.front.compareMask = state.stencilReadMask;
	depthStencil.front.writeMask = state.stencilWriteMask;
	depthStencil.front.reference = state.stencilRef;
	depthStencil.back = depthStencil.front;

	VkPipelineColorBlendAttachmentState blendAttachment = {};
	blendAttachment.blendEnable = state.blendEnable ? VK_TRUE : VK_FALSE;
	blendAttachment.srcColorBlendFactor = TranslateBlend(state.srcBlend);
	blendAttachment.dstColorBlendFactor = TranslateBlend(state.dstBlend);
	blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	// D3D8 has no separate-alpha-blend: alpha uses the SAME factors as color
	// (matters for layered opacity/fade effects).
	blendAttachment.srcAlphaBlendFactor = blendAttachment.srcColorBlendFactor;
	blendAttachment.dstAlphaBlendFactor = blendAttachment.dstColorBlendFactor;
	blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	blendAttachment.colorWriteMask = 0;
	if (state.colorWriteMask & 1) blendAttachment.colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
	if (state.colorWriteMask & 2) blendAttachment.colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
	if (state.colorWriteMask & 4) blendAttachment.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
	if (state.colorWriteMask & 8) blendAttachment.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
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
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisample;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &blendState;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = m_device->GetRenderPass();
	pipelineInfo.subpass = 0;

	VkPipeline pipeline = VK_NULL_HANDLE;
	if (vkCreateGraphicsPipelines(m_device->GetDevice(), m_pipelineCache, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
		return VK_NULL_HANDLE;
	return pipeline;
}
