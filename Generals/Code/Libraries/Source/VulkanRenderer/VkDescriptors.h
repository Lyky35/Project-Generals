#pragma once

#include <windows.h>
#include <vulkan/vulkan.h>
#include <vector>

class VulkanDevice;
class VkTexture;

class VkDescriptorAllocator
{
public:
	VkDescriptorAllocator();
	~VkDescriptorAllocator();

	VkDescriptorAllocator(const VkDescriptorAllocator &) = delete;
	VkDescriptorAllocator &operator=(const VkDescriptorAllocator &) = delete;

	bool Init(VulkanDevice *device, uint32_t maxSets = 4096);
	void Shutdown();
	void ResetFrame();

	VkDescriptorSet AllocateTextureSet(VkDescriptorSetLayout layout,
		VkTexture *tex0, bool filtered0, bool repeat0,
		VkTexture *tex1, bool filtered1, bool repeat1);
	VkDescriptorSet AllocateFrameSet(VkDescriptorSetLayout layout, VkBuffer buffer,
		uint32_t offset, uint32_t range);

	VkDescriptorSetLayout GetTextureSetLayout() const { return m_textureLayout; }

private:
	bool EnsurePool();

	VulkanDevice *m_device;
	VkDescriptorSetLayout m_textureLayout;
	std::vector<VkDescriptorPool> m_pools;
	uint32_t m_maxSets;
};
