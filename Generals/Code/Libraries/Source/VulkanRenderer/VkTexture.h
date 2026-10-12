#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>
#include <vector>

class VulkanDevice;

class VkTexture
{
public:
	VkTexture();
	~VkTexture();

	VkTexture(const VkTexture &) = delete;
	VkTexture &operator=(const VkTexture &) = delete;

	bool Create(VulkanDevice *device, uint32_t width, uint32_t height, VkFormat format,
		uint32_t mipLevels, bool renderTarget = false);
	bool CreateFromData(VulkanDevice *device, uint32_t width, uint32_t height, VkFormat format,
		const void *data, uint32_t mipLevels = 1);
	void Destroy();

	VkImage GetImage() const { return m_image; }
	VkImageView GetView() const { return m_imageView; }
	VkSampler GetSampler() const { return m_sampler; }
	VkFormat GetFormat() const { return m_format; }
	uint32_t GetWidth() const { return m_width; }
	uint32_t GetHeight() const { return m_height; }
	uint32_t GetMipLevels() const { return m_mipLevels; }

private:
	VulkanDevice *m_device;
	VkImage m_image;
	VkImageView m_imageView;
	VkSampler m_sampler;
	VkDeviceMemory m_memory;
	VkFormat m_format;
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_mipLevels;
};
