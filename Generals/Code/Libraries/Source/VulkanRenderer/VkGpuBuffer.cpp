#include "VkGpuBuffer.h"
#include "VkDevice.h"
#include <cstring>
#include <cassert>
#include <cstdio>


VkGpuBuffer::VkGpuBuffer()
	: m_device(nullptr)
	, m_handle(VK_NULL_HANDLE)
	, m_size(0)
	, m_allocBytes(0)
	, m_mapped(nullptr)
{
}

VkGpuBuffer::~VkGpuBuffer()
{
	Destroy();
}

bool VkGpuBuffer::Create(VulkanDevice *device, VkDeviceSize size, VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties)
{
	Destroy();
	m_device = device;
	m_size = size;

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(device->GetDevice(), &bufferInfo, nullptr, &m_handle) != VK_SUCCESS)
		return false;

	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(device->GetDevice(), m_handle, &memReq);
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = device->FindMemoryType(memReq.memoryTypeBits, properties);
	if (allocInfo.memoryTypeIndex == UINT32_MAX) {
		Destroy();
		return false;
	}
	if (vkAllocateMemory(device->GetDevice(), &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
		Destroy();
		return false;
	}
	if (vkBindBufferMemory(device->GetDevice(), m_handle, m_memory, 0) != VK_SUCCESS) {
		Destroy();
		return false;
	}
	m_allocBytes = memReq.size;
	return true;
}

void VkGpuBuffer::Destroy()
{
	if (m_mapped) {
		Unmap();
	}
	if (m_handle != VK_NULL_HANDLE && m_device) {
		vkDestroyBuffer(m_device->GetDevice(), m_handle, nullptr);
		m_handle = VK_NULL_HANDLE;
	}
	if (m_memory != VK_NULL_HANDLE && m_device) {
		vkFreeMemory(m_device->GetDevice(), m_memory, nullptr);
		m_memory = VK_NULL_HANDLE;
	}
	if (m_allocBytes) {
		m_allocBytes = 0;
	}
	m_device = nullptr;
	m_size = 0;
}

void VkGpuBuffer::Upload(const void *data, VkDeviceSize size, VkDeviceSize dstOffset)
{
	assert(dstOffset + size <= m_size);
	if (m_mapped) {
		memcpy((unsigned char *)m_mapped + dstOffset, data, (size_t)size);
		return;
	}

	VkBuffer staging;
	VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkDevice device = m_device->GetDevice();
	if (vkCreateBuffer(device, &bufferInfo, nullptr, &staging) != VK_SUCCESS)
		return;
	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(device, staging, &memReq);
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = m_device->FindMemoryType(memReq.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (allocInfo.memoryTypeIndex == UINT32_MAX) {
		vkDestroyBuffer(device, staging, nullptr);
		return;
	}
	if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
		vkDestroyBuffer(device, staging, nullptr);
		return;
	}
	vkBindBufferMemory(device, staging, stagingMemory, 0);
	void *mapped = nullptr;
	vkMapMemory(device, stagingMemory, 0, size, 0, &mapped);
	memcpy(mapped, data, (size_t)size);
	vkUnmapMemory(device, stagingMemory);

	VkCommandBuffer cmd = m_device->BeginOneTimeCommands();
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = dstOffset;
	copyRegion.size = size;
	vkCmdCopyBuffer(cmd, staging, m_handle, 1, &copyRegion);
	m_device->EndOneTimeCommands(cmd);

	vkDestroyBuffer(device, staging, nullptr);
	vkFreeMemory(device, stagingMemory, nullptr);
}

void *VkGpuBuffer::Map()
{
	if (!m_mapped && m_handle != VK_NULL_HANDLE)
		vkMapMemory(m_device->GetDevice(), m_memory, 0, m_size, 0, &m_mapped);
	return m_mapped;
}

void VkGpuBuffer::Unmap()
{
	if (m_mapped) {
		vkUnmapMemory(m_device->GetDevice(), m_memory);
		m_mapped = nullptr;
	}
}
