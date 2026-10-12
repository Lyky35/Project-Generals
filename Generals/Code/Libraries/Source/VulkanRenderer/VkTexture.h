#pragma once

#include <windows.h>
#include <vulkan/vulkan.h>
#include <cstdint>

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
		uint32_t mipLevels, const void *pixels);
	void Destroy();

	void UploadLevel(uint32_t mip, uint32_t width, uint32_t height, const void *pixels);
	void GenerateMipmaps();

	VkImage GetImage() const { return m_image; }
	VkImageView GetView() const { return m_view; }
	VkSampler GetSampler(bool filtered, bool repeat) const;
	VkFormat GetFormat() const { return m_format; }
	uint32_t GetWidth() const { return m_width; }
	uint32_t GetHeight() const { return m_height; }
	uint32_t GetMipLevels() const { return m_mips; }

	static VkFormat TranslateFormat(unsigned d3dFormat);
	bool CreateDepth(VulkanDevice *device, uint32_t width, uint32_t height);

private:
	void TransitionLayout(VkCommandBuffer cmd, VkImageLayout oldLayout, VkImageLayout newLayout,
		uint32_t baseMip, uint32_t mipCount);
	VkSampler FindOrCreateSampler(bool filtered, bool repeat) const;

	VulkanDevice *m_device;
	VkImage m_image;
	VkDeviceMemory m_memory;
	VkImageView m_view;
	VkFormat m_format;
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_mips;

	struct SamplerEntry {
		bool filtered;
		bool repeat;
		VkSampler sampler;
	};
	mutable SamplerEntry m_samplers[4];
	mutable int m_samplerCount;
};
