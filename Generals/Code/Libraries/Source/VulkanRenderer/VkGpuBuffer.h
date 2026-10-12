#pragma once

#include <windows.h>
#include <vulkan/vulkan.h>
#include <cstdint>

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

	void Upload(const void *data, VkDeviceSize size, VkDeviceSize dstOffset = 0);
	void *Map();
	void Unmap();
	bool IsMapped() const { return m_mapped != nullptr; }

	VkBuffer GetBuffer() const { return m_handle; }
	VkDeviceSize GetSize() const { return m_size; }
	VkDeviceSize GetDeviceAddressOffset() const { return 0; }

private:
	VulkanDevice *m_device;
	VkBuffer m_handle;
	VkDeviceMemory m_memory;
	VkDeviceSize m_size;
	void *m_mapped;
};
