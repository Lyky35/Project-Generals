#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>
#include <vector>

class VulkanDevice;

struct SamplerEntry {
    bool filtered;
    bool repeat;
    VkSampler sampler;
};

class VkTexture
{
public:
    VkTexture();
    ~VkTexture();

    VkTexture(const VkTexture &) = delete;
    VkTexture &operator=(const VkTexture &) = delete;

    VkFormat TranslateFormat(unsigned d3dFormat);
    bool Create(VulkanDevice *device, uint32_t width, uint32_t height, VkFormat format,
        uint32_t mipLevels, bool renderTarget = false);
    bool CreateFromData(VulkanDevice *device, uint32_t width, uint32_t height, VkFormat format,
        uint32_t mipLevels, const void *pixels);
    bool CreateDepth(VulkanDevice *device, uint32_t width, uint32_t height);
    void Destroy();

    void TransitionLayout(VkCommandBuffer cmd, VkImageLayout oldLayout, VkImageLayout newLayout,
        uint32_t baseMip, uint32_t mipCount);
    void UploadLevel(uint32_t mip, uint32_t mipWidth, uint32_t mipHeight, const void *pixels);
    void GenerateMipmaps();
    // Synchronous GPU->CPU readback of mip 0 into dst (pitch bytes/row).
    // 32-bit formats only. Image must be a regular color texture.
    bool ReadbackLevel(void *dst, uint32_t pitchBytes);

    VkSampler FindOrCreateSampler(bool filtered, bool repeat);
    VkSampler GetSampler(bool filtered, bool repeat);

    VkImage GetImage() const { return m_image; }
    VkImageView GetView() const { return m_view; }
    VkSampler GetSampler() const { return m_samplerCount > 0 ? m_samplers[0].sampler : VK_NULL_HANDLE; }
    VkFormat GetFormat() const { return m_format; }
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    uint32_t GetMipLevels() const { return m_mips; }

private:
    VulkanDevice *m_device;
    VkImage m_image;
    VkImageView m_view;
    VkDeviceMemory m_memory;
    VkFormat m_format;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_mips;
    uint64_t m_allocBytes;
    SamplerEntry m_samplers[4];
    int m_samplerCount;
    VkSampler m_sampler;
};
