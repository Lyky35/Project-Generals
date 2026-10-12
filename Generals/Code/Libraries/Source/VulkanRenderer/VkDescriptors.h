#pragma once

#include <cstdint>
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

	VkDescriptorSet Allocate(VkDescriptorSetLayout layout);

private:
	VulkanDevice *m_device;
	VkDescriptorPool m_pool;
};

class VkDescriptorSetLayoutBuilder
{
public:
	VkDescriptorSetLayoutBuilder();
	~VkDescriptorSetLayoutBuilder();

	VkDescriptorSetLayoutBuilder &AddBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stages, uint32_t count = 1);
	VkDescriptorSetLayout Build(VulkanDevice *device);

private:
	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
};
