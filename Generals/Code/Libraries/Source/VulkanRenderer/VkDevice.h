#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define VK_USE_PLATFORM_WIN32_KHR 0
#endif

#include <cstdint>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include <windows.h>
#include <vector>
#include <cassert>

#define VK_CHECK(x) do { VkResult _r = (x); assert(_r == VK_SUCCESS); (void)_r; } while (0)

class VulkanDevice
{
public:
    static const int MAX_FRAMES_IN_FLIGHT = 2;

    VulkanDevice();
    ~VulkanDevice();

    VulkanDevice(const VulkanDevice &) = delete;
    VulkanDevice &operator=(const VulkanDevice &) = delete;

    bool Init(HWND hwnd);
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
    uint32_t GetFrameIndex() const { return m_frameIndex; }
    VkCommandBuffer GetCommandBuffer() const { return m_commandBuffers[m_frameIndex]; }

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
    bool RecreateSwapchain();

    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice;
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

    // Additional members used in implementation
    uint32_t m_graphicsFamily;
    uint32_t m_presentFamily;
    VkFormat m_swapFormat;
    VkImage m_depthImage;
    VkDeviceMemory m_depthMemory;
    VkImageView m_depthView;
    VkFormat m_depthFormat;
    VkRenderPass m_rtRenderPass;
    VkCommandPool m_commandPool;
    uint32_t m_imageIndex;
    bool m_framebufferResized;
    VkExtent2D m_swapExtent;
    std::vector<VkImage> m_swapImages;
    std::vector<VkImageView> m_swapImageViews;
    std::vector<VkFramebuffer> m_framebuffers;
    VkImageView m_depthView;
    VkRenderPass m_rtRenderPass;
    VkCommandPool m_commandPool;
    std::vector<VkSemaphore> m_imageAvailable;
    std::vector<VkSemaphore> m_renderFinished;
    std::vector<VkFence> m_inFlight;
    std::vector<VkFence> m_imagesInFlight;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
    VkSwapchainKHR m_swapchain;
    VkFormat m_swapchainFormat;
    VkExtent2D m_swapchainExtent;
    VkRenderPass m_renderPass;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkSurfaceKHR m_surface;
    HWND m_hwnd;
    uint32_t m_frameIndex;
    VkCommandPool m_commandPool;
    std::vector<VkSemaphore> m_imageAvailable;
    std::vector<VkSemaphore> m_renderFinished;
    std::vector<VkFence> m_inFlight;
    std::vector<VkFence> m_imagesInFlight;
    bool m_framebufferResized;
    uint32_t m_imageIndex;
    VkCommandPool m_commandPool;
    VkFormat m_swapFormat;
    VkImage m_depthImage;
    VkDeviceMemory m_depthMemory;
    VkImageView m_depthView;
    VkFormat m_depthFormat;
    VkRenderPass m_rtRenderPass;
    VkCommandPool m_commandPool;
    VkExtent2D m_swapExtent;
    std::vector<VkImage> m_swapImages;
    std::vector<VkImageView> m_swapImageViews;
    std::vector<VkFramebuffer> m_framebuffers;
    VkImageView m_depthView;
    VkRenderPass m_rtRenderPass;
    VkCommandPool m_commandPool;
    std::vector<VkSemaphore> m_imageAvailable;
    std::vector<VkSemaphore> m_renderFinished;
    std::vector<VkFence> m_inFlight;
    std::vector<VkFence> m_imagesInFlight;
    uint32_t m_graphicsFamily;
    uint32_t m_presentFamily;
    VkFormat m_swapFormat;
    VkExtent2D m_swapExtent;
    std::vector<VkImage> m_swapImages;
    std::vector<VkImageView> m_swapImageViews;
    std::vector<VkFramebuffer> m_framebuffers;
    VkImageView m_depthView;
    VkRenderPass m_rtRenderPass;
    VkCommandPool m_commandPool;
    std::vector<VkSemaphore> m_imageAvailable;
    std::vector<VkSemaphore> m_renderFinished;
    std::vector<VkFence> m_inFlight;
    std::vector<VkFence> m_imagesInFlight;
    bool m_framebufferResized;
    uint32_t m_imageIndex;
    VkCommandPool m_commandPool;
};
