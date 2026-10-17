#include "VkDescriptors.h"
#include "VkDevice.h"
#include "VkTexture.h"
#include <cstdio>

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

int VkDescriptorAllocator::FrameSlot() const
{
	return m_device ? (int)(VulkanDevice::Instance().GetFrameIndex() & 1) : 0;
}

void VkDescriptorAllocator::Shutdown()
{
	if (!m_device)
		return;
	VkDevice dev = m_device->GetDevice();
	for (int s = 0; s < 2; s++)
		for (size_t i = 0; i < m_pools[s].size(); i++)
			vkDestroyDescriptorPool(dev, m_pools[s][i], nullptr);
	for (int s = 0; s < 2; s++)
		m_pools[s].clear();
	if (m_textureLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(dev, m_textureLayout, nullptr);
		m_textureLayout = VK_NULL_HANDLE;
	}
	m_device = nullptr;
}

VkDescriptorPool VkDescriptorAllocator::CreatePool()
{
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
		return VK_NULL_HANDLE;
	return pool;
}

bool VkDescriptorAllocator::EnsurePool()
{
	const int slot = FrameSlot();
	if (!m_pools[slot].empty())
		return true;
	VkDescriptorPool pool = CreatePool();
	if (pool == VK_NULL_HANDLE)
		return false;
	m_pools[slot].push_back(pool);
	return true;
}

void VkDescriptorAllocator::ResetFrame()
{
	if (!m_device)
		return;
	// Only reset THIS frame slot's pools. Called after BeginFrame waited the
	// slot's previous fence, so its sets are GPU-idle; the other in-flight
	// slot's sets must survive (they were being freed mid-frame = blinking).
	const int slot = FrameSlot();
	for (size_t i = 0; i < m_pools[slot].size(); i++)
		vkResetDescriptorPool(m_device->GetDevice(), m_pools[slot][i], 0);
}

VkDescriptorSet VkDescriptorAllocator::AllocateTextureSet(VkDescriptorSetLayout layout,
	VkTexture *tex0, bool filtered0, bool repeat0,
	VkTexture *tex1, bool filtered1, bool repeat1)
{
	if (!m_device || !tex0 || !tex1) {
		static bool l1=false; if(!l1){l1=true;FILE*lf=fopen("RTS_START.LOG","a");if(lf){fprintf(lf,"AllocTexSet fail: dev=%p t0=%p t1=%p\n",(void*)m_device,(void*)tex0,(void*)tex1);fclose(lf);}}
		return VK_NULL_HANDLE;
	}
	if (!EnsurePool()) {
		static bool l2=false; if(!l2){l2=true;FILE*lf=fopen("RTS_START.LOG","a");if(lf){fprintf(lf,"AllocTexSet fail: EnsurePool\n");fclose(lf);}}
		return VK_NULL_HANDLE;
	}

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_pools[FrameSlot()].back();
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;
	VkDescriptorSet set = VK_NULL_HANDLE;
	if (vkAllocateDescriptorSets(m_device->GetDevice(), &allocInfo, &set) != VK_SUCCESS) {
		VkDescriptorPool extra = CreatePool();
		if (extra != VK_NULL_HANDLE) {
			m_pools[FrameSlot()].push_back(extra);
			allocInfo.descriptorPool = extra;
			if (vkAllocateDescriptorSets(m_device->GetDevice(), &allocInfo, &set) != VK_SUCCESS)
				set = VK_NULL_HANDLE;
		}
		if (set == VK_NULL_HANDLE) {
			static bool l3=false; if(!l3){l3=true;FILE*lf=fopen("RTS_START.LOG","a");if(lf){fprintf(lf,"AllocTexSet fail: vkAllocateDescriptorSets\n");fclose(lf);}}
			return VK_NULL_HANDLE;
		}
	}

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
	{
		static unsigned dsLog = 0;
		if (dsLog < 16) {
			dsLog++;
			FILE *lf = fopen("RTS_START.LOG", "a");
			if (lf) {
				fprintf(lf, "VK: TexSet #%u sam0=%p view0=%p sam1=%p view1=%p\n",
					dsLog, (void *)imageInfos[0].sampler, (void *)imageInfos[0].imageView,
					(void *)imageInfos[1].sampler, (void *)imageInfos[1].imageView);
				fclose(lf);
			}
		}
	}
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
	allocInfo.descriptorPool = m_pools[FrameSlot()].back();
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;
	VkDescriptorSet set = VK_NULL_HANDLE;
	if (vkAllocateDescriptorSets(m_device->GetDevice(), &allocInfo, &set) != VK_SUCCESS) {
		VkDescriptorPool extra = CreatePool();
		if (extra != VK_NULL_HANDLE) {
			m_pools[FrameSlot()].push_back(extra);
			allocInfo.descriptorPool = extra;
			if (vkAllocateDescriptorSets(m_device->GetDevice(), &allocInfo, &set) != VK_SUCCESS)
				set = VK_NULL_HANDLE;
		}
		if (set == VK_NULL_HANDLE)
			return VK_NULL_HANDLE;
	}

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
