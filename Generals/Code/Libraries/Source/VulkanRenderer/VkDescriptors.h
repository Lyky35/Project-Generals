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

    bool EnsurePool();
	VkDescriptorPool CreatePool();
    void ResetFrame();

    VkDescriptorSet Allocate(VkDescriptorSetLayout layout);

    VkDescriptorSet AllocateTextureSet(VkDescriptorSetLayout layout,
        VkTexture *tex0, bool filtered0, bool repeat0,
        VkTexture *tex1, bool filtered1, bool repeat1);
    VkDescriptorSet AllocateFrameSet(VkDescriptorSetLayout layout, VkBuffer buffer,
        uint32_t offset, uint32_t range);

private:
    // One pool chain per swapchain frame slot (MAX_FRAMES_IN_FLIGHT=2):
    // resetting ALL pools each frame destroyed descriptor sets the previous
    // in-flight frame was still rendering with -> menu elements blinked.
    int FrameSlot() const;
    VulkanDevice *m_device;
    VkDescriptorPool m_pool;
    std::vector<VkDescriptorPool> m_pools[2];
    VkDescriptorSetLayout m_textureLayout;
    uint32_t m_maxSets;
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
