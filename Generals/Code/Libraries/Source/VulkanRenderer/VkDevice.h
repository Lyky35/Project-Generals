#pragma once

#include <windows.h>
#include <vulkan/vulkan.h>
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
	bool IsInitialized() const { return m_device != VK_NULL_HANDLE; }

	bool BeginFrame();
	void EndFrame(bool present = true);
	bool RecreateSwapchain();

	VkInstance GetInstance() const { return m_instance; }
	VkDevice GetDevice() const { return m_device; }
	VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
	VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
	VkQueue GetPresentQueue() const { return m_presentQueue; }
	uint32_t GetGraphicsQueueFamily() const { return m_graphicsFamily; }
	VkExtent2D GetSwapExtent() const { return m_swapExtent; }
	VkFormat GetSwapFormat() const { return m_swapFormat; }
	VkFormat GetDepthFormat() const { return m_depthFormat; }
	VkRenderPass GetRenderPass() const { return m_renderPass; }
	VkRenderPass GetRtRenderPass() const { return m_rtRenderPass; }
	VkCommandBuffer GetCommandBuffer() const { return m_commandBuffers[m_frameIndex]; }
	uint32_t GetFrameIndex() const { return m_frameIndex; }
	uint32_t GetImageIndex() const { return m_imageIndex; }
	VkFramebuffer GetFramebuffer() const { return m_framebuffers[m_imageIndex]; }
	const std::vector<VkImage> &GetSwapImages() const { return m_swapImages; }
	VkCommandBuffer GetFrameCommandBuffer() const { return m_commandBuffers[m_frameIndex]; }
	bool HasStencil() const { return m_depthFormat == VK_FORMAT_D24_UNORM_S8_UINT; }

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkCommandBuffer BeginOneTimeCommands();
	void EndOneTimeCommands(VkCommandBuffer cmd);

	static VulkanDevice &Instance();

private:
	bool CreateInstance();
	bool CreateSurface(HWND hwnd);
	bool PickPhysicalDevice();
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

	int RatePhysicalDevice(VkPhysicalDevice device);

	HWND m_hwnd;
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkSurfaceKHR m_surface;
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_device;
	uint32_t m_graphicsFamily;
	uint32_t m_presentFamily;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	VkSwapchainKHR m_swapchain;
	std::vector<VkImage> m_swapImages;
	std::vector<VkImageView> m_swapImageViews;
	VkFormat m_swapFormat;
	VkExtent2D m_swapExtent;

	VkImage m_depthImage;
	VkDeviceMemory m_depthMemory;
	VkImageView m_depthView;
	VkFormat m_depthFormat;

	VkRenderPass m_renderPass;
	VkRenderPass m_rtRenderPass;
	std::vector<VkFramebuffer> m_framebuffers;

	VkCommandPool m_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;

	std::vector<VkSemaphore> m_imageAvailable;
	std::vector<VkSemaphore> m_renderFinished;
	std::vector<VkFence> m_inFlight;
	std::vector<VkFence> m_imagesInFlight;

	uint32_t m_frameIndex;
	uint32_t m_imageIndex;
	bool m_framebufferResized;
};
