#include "VkDescriptors.h"
#include "VkDevice.h"
#include "VkTexture.h"

VkDescriptorAllocator::VkDescriptorAllocator()
	: m_device(nullptr)
	, m_textureLayout(VK_NULL_HANDLE)
	, m_maxSets(4096)
{
}

VkDescriptorAllocator::~VkDescriptorAllocator()
{
	Shutdown();
}

bool VkDescriptorAllocator::Init(VulkanDevice *device, uint32_t maxSets)
{
	m_device = device;
	m_maxSets = maxSets;

	VkDescriptorSetLayoutBinding binding = {};
	binding.binding = 0;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &binding;
	if (vkCreateDescriptorSetLayout(device->GetDevice(), &layoutInfo, nullptr, &m_textureLayout) != VK_SUCCESS)
		return false;
	return EnsurePool();
}

void VkDescriptorAllocator::Shutdown()
{
	if (!m_device)
		return;
	VkDevice dev = m_device->GetDevice();
	for (size_t i = 0; i < m_pools.size(); i++)
		vkDestroyDescriptorPool(dev, m_pools[i], nullptr);
	m_pools.clear();
	if (m_textureLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(dev, m_textureLayout, nullptr);
		m_textureLayout = VK_NULL_HANDLE;
	}
	m_device = nullptr;
}

bool VkDescriptorAllocator::EnsurePool()
{
	if (!m_pools.empty())
		return true;
	VkDescriptorPoolSize poolSizes[2] = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[0].descriptorCount = m_maxSets * 2;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = m_maxSets;
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.maxSets = m_maxSets;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolSizes;
	VkDescriptorPool pool = VK_NULL_HANDLE;
	if (vkCreateDescriptorPool(m_device->GetDevice(), &poolInfo, nullptr, &pool) != VK_SUCCESS)
		return false;
	m_pools.push_back(pool);
	return true;
}

void VkDescriptorAllocator::ResetFrame()
{
	if (!m_device)
		return;
	for (size_t i = 0; i < m_pools.size(); i++)
		vkResetDescriptorPool(m_device->GetDevice(), m_pools[i], 0);
}

VkDescriptorSet VkDescriptorAllocator::AllocateTextureSet(VkDescriptorSetLayout layout,
	VkTexture *tex0, bool filtered0, bool repeat0,
	VkTexture *tex1, bool filtered1, bool repeat1)
{
	if (!m_device || !tex0 || !tex1)
		return VK_NULL_HANDLE;
	if (!EnsurePool())
		return VK_NULL_HANDLE;

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_pools.back();
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;
	VkDescriptorSet set = VK_NULL_HANDLE;
	if (vkAllocateDescriptorSets(m_device->GetDevice(), &allocInfo, &set) != VK_SUCCESS)
		return VK_NULL_HANDLE;

	VkDescriptorImageInfo imageInfos[2] = {};
	imageInfos[0].sampler = tex0->GetSampler(filtered0, repeat0);
	imageInfos[0].imageView = tex0->GetView();
	imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfos[1].sampler = tex1->GetSampler(filtered1, repeat1);
	imageInfos[1].imageView = tex1->GetView();
	imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkWriteDescriptorSet writes[2] = {};
	for (int i = 0; i < 2; i++) {
		writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[i].dstSet = set;
		writes[i].dstBinding = (uint32_t)i;
		writes[i].descriptorCount = 1;
		writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[i].pImageInfo = &imageInfos[i];
	}
	vkUpdateDescriptorSets(m_device->GetDevice(), 2, writes, 0, nullptr);
	return set;
}

VkDescriptorSet VkDescriptorAllocator::AllocateFrameSet(VkDescriptorSetLayout layout, VkBuffer buffer,
	uint32_t offset, uint32_t range)
{
	if (!m_device)
		return VK_NULL_HANDLE;
	if (!EnsurePool())
		return VK_NULL_HANDLE;

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_pools.back();
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;
	VkDescriptorSet set = VK_NULL_HANDLE;
	if (vkAllocateDescriptorSets(m_device->GetDevice(), &allocInfo, &set) != VK_SUCCESS)
		return VK_NULL_HANDLE;

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = buffer;
	bufferInfo.offset = offset;
	bufferInfo.range = range;
	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = set;
	write.dstBinding = 0;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.pBufferInfo = &bufferInfo;
	vkUpdateDescriptorSets(m_device->GetDevice(), 1, &write, 0, nullptr);
	return set;
}
