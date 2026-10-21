#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define VK_USE_PLATFORM_WIN32_KHR 0
#endif

#include <cstdint>
#include <functional>
#include <utility>
#include <windows.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include <vector>
#include <cassert>

#define VK_CHECK(x) do { VkResult _r = (x); assert(_r == VK_SUCCESS); (void)_r; } while (0)

class VulkanDevice
{
public:
    static const int MAX_FRAMES_IN_FLIGHT = 2;

    static VulkanDevice &Instance();
    uint64_t GetMinUboAlign() const { return m_minUboAlign ? m_minUboAlign : 16; }
    VulkanDevice();
    ~VulkanDevice();

    VulkanDevice(const VulkanDevice &) = delete;
    VulkanDevice &operator=(const VulkanDevice &) = delete;

    bool Init(HWND hwnd);

    bool PickPhysicalDevice();
    int RatePhysicalDevice(VkPhysicalDevice device);
    void Shutdown();

    bool BeginFrame();
    void EndFrame(bool present);

    VkDevice GetDevice() const { return m_device; }
    VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue GetPresentQueue() const { return m_presentQueue; }
    VkSwapchainKHR GetSwapchain() const { return m_swapchain; }
    VkFormat GetSwapchainFormat() const { return m_swapchainFormat; }
    VkExtent2D GetSwapchainExtent() const { return m_swapchainExtent; }
    VkRenderPass GetRenderPass() const { return m_renderPass; }
    VkFormat GetDepthFormat() const { return m_depthFormat; }
    uint32_t GetFrameIndex() const { return m_frameIndex; }
    VkCommandBuffer GetCommandBuffer() const { return m_commandBuffers[m_frameIndex]; }

    // PORT FIX (amdvlk crash at mission start): destroying textures/framebuffers
    // while frames are still in flight is spec-illegal and dereferenced freed
    // views inside the next vkCmdBeginRenderPass (crash stack:
    // VulkanBeginTargetPass -> vkCmdBeginRenderPass -> AV). Destroy work is
    // queued here and runs after MAX_FRAMES_IN_FLIGHT frames have completed.
    uint64_t FrameCounter() const { return m_frameCounter; }
    void Defer(uint64_t frame, std::function<void()> fn) { m_deferred.emplace_back(frame, std::move(fn)); }
    void DeferCurrent(std::function<void()> fn) { Defer(m_frameCounter, std::move(fn)); }
    void DrainDeferred();
    void DrainAllDeferred();

    bool CreateSurface(void* hwnd);

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkCommandBuffer BeginOneTimeCommands();
    void EndOneTimeCommands(VkCommandBuffer cmd);

private:
    bool CreateInstance();
    bool SelectPhysicalDevice();
    bool CreateLogicalDevice();
    bool CreateSwapchain();
    bool CreateImageViews();
    bool CreateRenderPass();
    bool CreateDepthResources();
    bool CreateFramebuffers();
    bool CreateCommandPool();
    bool CreateCommandBuffers();
    bool CreateSyncObjects();
    void CleanupSwapchain();
    public:
    bool RecreateSwapchain();

    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice;
    uint64_t m_minUboAlign = 16;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
    VkSwapchainKHR m_swapchain;
    VkFormat m_swapchainFormat;
    VkExtent2D m_swapchainExtent;
    VkRenderPass m_renderPass;
    std::vector<VkImageView> m_swapchainImageViews;
    std::vector<VkFramebuffer> m_swapchainFramebuffers;
    std::vector<VkCommandBuffer> m_commandBuffers;
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    std::vector<VkFence> m_imagesInFlight;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkSurfaceKHR m_surface;
    HWND m_hwnd;
    uint32_t m_frameIndex;
    uint64_t m_frameCounter = 0;
    std::vector<std::pair<uint64_t, std::function<void()>>> m_deferred;

    uint32_t m_graphicsFamily;
    uint32_t m_presentFamily;
    VkFormat m_swapFormat;
    VkImage m_depthImage;
    VkBuffer m_readbackBuf = VK_NULL_HANDLE;
    VkDeviceMemory m_readbackMem = VK_NULL_HANDLE;
    VkBuffer m_texReadbackBuf = VK_NULL_HANDLE;
    VkDeviceMemory m_texReadbackMem = VK_NULL_HANDLE;
    VkDeviceMemory m_depthMemory;
    VkImageView m_depthView;
    VkFormat m_depthFormat;
    VkRenderPass m_rtRenderPass;
    VkCommandPool m_commandPool;
    uint32_t m_imageIndex;
    bool m_framebufferResized;
    VkExtent2D m_swapExtent;

public:
    // Accessor methods
    // Must present the framebuffer of the ACQUIRED swapchain image: always
    // returning [0] rendered even frames into image0 while presenting image1
    // (stale) - the menu blinked at swapchain rate.
    VkFramebuffer GetFramebuffer() const {
        return m_framebuffers[m_imageIndex < m_framebuffers.size() ? m_imageIndex : 0];
    }
    VkExtent2D GetSwapExtent() const { return m_swapExtent; }
    VkFormat GetSwapFormat() const { return m_swapFormat; }
    // Was hard-coded `return true`, which made DX8Wrapper::Create_Device skip
    // device creation entirely - every later vk* call then used an
    // uninitialized/null VkDevice (crashed GPU drivers silently on Windows,
    // crashed winevulkan under wine).
    bool IsInitialized() const { return m_device != VK_NULL_HANDLE; }
    uint32_t GetImageIndex() const { return m_imageIndex; }
    VkRenderPass GetRtRenderPass() const { return m_rtRenderPass; }
    bool HasStencil() const {
        // Stencil shadows/occlusion depend on this - was hardcoded false,
        // silently disabling the whole stencil feature set.
        return m_depthFormat == VK_FORMAT_D24_UNORM_S8_UINT ||
               m_depthFormat == VK_FORMAT_D16_UNORM_S8_UINT ||
               m_depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT;
    }
        std::vector<VkImage> m_swapImages;
    const std::vector<VkImage>& GetSwapImages() const { return m_swapImages; }
    std::vector<VkImageView> m_swapImageViews;
    std::vector<VkFramebuffer> m_framebuffers;
};
