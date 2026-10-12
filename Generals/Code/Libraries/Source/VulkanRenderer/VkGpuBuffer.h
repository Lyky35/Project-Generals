#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>
#include <cassert>

class VulkanDevice;

class VkGpuBuffer
{
public:
	VkGpuBuffer();
	~VkGpuBuffer();

	VkGpuBuffer(const VkGpuBuffer &) = delete;
	VkGpuBuffer &operator=(const VkGpuBuffer &) = delete;

	bool Create(VulkanDevice *device, VkDeviceSize size, VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties);
	void Destroy();

	void *Map();
	void Unmap();
	void Upload(const void *data, VkDeviceSize size, VkDeviceSize offset = 0);

	VkBuffer GetBuffer() const { return m_handle; }
	VkDeviceMemory GetMemory() const { return m_memory; }
	VkDeviceSize GetSize() const { return m_size; }

private:
	VulkanDevice *m_device;
	VkBuffer m_handle;
	VkDeviceMemory m_memory;
	VkDeviceSize m_size;
	void *m_mapped;
};
