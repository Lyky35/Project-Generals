#include "VkDevice.h"
#include <cstring>
#include <cstdio>
#include <limits>

static void vkLog(const char *what, int res)
{
	FILE *lf = fopen("RTS_START.LOG", "a");
	if (lf) { fprintf(lf, "VK: %s -> %d\n", what, res); fclose(lf); }
}

#ifdef _DEBUG
static const bool kEnableValidation = true;
#else
static const bool kEnableValidation = false;
#endif

static const char *kValidationLayers[] = {
	"VK_LAYER_KHRONOS_validation"
};

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
	void *userData)
{
	(void)severity;
	(void)type;
	(void)userData;
	OutputDebugStringA(callbackData->pMessage);
	OutputDebugStringA("\n");
	return VK_FALSE;
}

VulkanDevice::VulkanDevice()
	: m_hwnd(NULL)
	, m_instance(VK_NULL_HANDLE)
	, m_debugMessenger(VK_NULL_HANDLE)
	, m_surface(VK_NULL_HANDLE)
	, m_physicalDevice(VK_NULL_HANDLE)
	, m_device(VK_NULL_HANDLE)
	, m_graphicsFamily(UINT32_MAX)
	, m_presentFamily(UINT32_MAX)
	, m_graphicsQueue(VK_NULL_HANDLE)
	, m_presentQueue(VK_NULL_HANDLE)
	, m_swapchain(VK_NULL_HANDLE)
	, m_swapFormat(VK_FORMAT_B8G8R8A8_UNORM)
	, m_depthImage(VK_NULL_HANDLE)
	, m_depthMemory(VK_NULL_HANDLE)
	, m_depthView(VK_NULL_HANDLE)
	, m_depthFormat(VK_FORMAT_D32_SFLOAT)
	, m_renderPass(VK_NULL_HANDLE)
	, m_rtRenderPass(VK_NULL_HANDLE)
	, m_commandPool(VK_NULL_HANDLE)
	, m_frameIndex(0)
	, m_imageIndex(0)
	, m_framebufferResized(false)
{
	m_swapExtent.width = 0;
	m_swapExtent.height = 0;
}

VulkanDevice::~VulkanDevice()
{
	Shutdown();
}

VulkanDevice &VulkanDevice::Instance()
{
	static VulkanDevice instance;
	return instance;
}

bool VulkanDevice::Init(HWND hwnd)
{
	m_hwnd = hwnd;
	if (!CreateInstance()) { vkLog("Init: CreateInstance FAILED", -1); return false; }
	if (!CreateSurface(hwnd)) {
		vkLog("Init: CreateSurface FAILED", -1);
		Shutdown();
		return false;
	}
	if (!PickPhysicalDevice()) {
		vkLog("Init: PickPhysicalDevice FAILED", -1);
		Shutdown();
		return false;
	}
	if (!CreateLogicalDevice()) {
		vkLog("Init: CreateLogicalDevice FAILED", -1);
		Shutdown();
		return false;
	}
	if (!CreateSwapchain()) {
		vkLog("Init: CreateSwapchain FAILED", -1);
		Shutdown();
		return false;
	}
	if (!CreateImageViews()) {
		vkLog("Init: CreateImageViews FAILED", -1);
		Shutdown();
		return false;
	}
	if (!CreateRenderPass()) {
		vkLog("Init: CreateRenderPass FAILED", -1);
		Shutdown();
		return false;
	}
	if (!CreateDepthResources()) {
		vkLog("Init: CreateDepthResources FAILED", -1);
		Shutdown();
		return false;
	}
	if (!CreateFramebuffers()) {
		vkLog("Init: CreateFramebuffers FAILED", -1);
		Shutdown();
		return false;
	}
	if (!CreateCommandPool()) {
		vkLog("Init: CreateCommandPool FAILED", -1);
		Shutdown();
		return false;
	}
	if (!CreateCommandBuffers()) {
		vkLog("Init: CreateCommandBuffers FAILED", -1);
		Shutdown();
		return false;
	}
	if (!CreateSyncObjects()) {
		vkLog("Init: CreateSyncObjects FAILED", -1);
		Shutdown();
		return false;
	}
	vkLog("Init: Vulkan device READY", 0);
	return true;
}

void VulkanDevice::Shutdown()
{
	if (m_device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(m_device);
		CleanupSwapchain();
		if (m_commandPool != VK_NULL_HANDLE) {
			vkDestroyCommandPool(m_device, m_commandPool, nullptr);
			m_commandPool = VK_NULL_HANDLE;
		}
		m_commandBuffers.clear();
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (m_imageAvailableSemaphores.size() > i && m_imageAvailableSemaphores[i] != VK_NULL_HANDLE)
				vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
			if (m_renderFinishedSemaphores.size() > i && m_renderFinishedSemaphores[i] != VK_NULL_HANDLE)
				vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
			if (m_inFlightFences.size() > i && m_inFlightFences[i] != VK_NULL_HANDLE)
				vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
		}
		m_imageAvailableSemaphores.clear();
		m_renderFinishedSemaphores.clear();
		m_inFlightFences.clear();
		m_imagesInFlight.clear();
		vkDestroyDevice(m_device, nullptr);
		m_device = VK_NULL_HANDLE;
	}
	if (m_surface != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		m_surface = VK_NULL_HANDLE;
	}
	if (m_debugMessenger != VK_NULL_HANDLE) {
		PFN_vkDestroyDebugUtilsMessengerEXT destroyFunc =
			(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
		if (destroyFunc)
			destroyFunc(m_instance, m_debugMessenger, nullptr);
		m_debugMessenger = VK_NULL_HANDLE;
	}
	if (m_instance != VK_NULL_HANDLE) {
		vkDestroyInstance(m_instance, nullptr);
		m_instance = VK_NULL_HANDLE;
	}
	m_physicalDevice = VK_NULL_HANDLE;
}

bool VulkanDevice::CreateInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Command and Conquer Generals";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "ProjectGenerals-Vulkan";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	std::vector<const char *> extensions;
	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef _WIN32
	// was VK_KHR_surface twice (duplicate -> VK_ERROR_EXTENSION_NOT_PRESENT
	// on real Windows loaders) and win32_surface was never enabled.
	extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
	if (kEnableValidation)
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = (uint32_t)extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();
	if (kEnableValidation) {
		createInfo.enabledLayerCount = 1;
		createInfo.ppEnabledLayerNames = kValidationLayers;
	}

	VkResult instRes = vkCreateInstance(&createInfo, nullptr, &m_instance);
	vkLog("vkCreateInstance", (int)instRes);
	if (instRes != VK_SUCCESS)
		return false;

	if (kEnableValidation) {
		VkDebugUtilsMessengerCreateInfoEXT messengerInfo = {};
		messengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		messengerInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		messengerInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		messengerInfo.pfnUserCallback = DebugCallback;
		PFN_vkCreateDebugUtilsMessengerEXT createFunc =
			(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
		if (createFunc)
			createFunc(m_instance, &messengerInfo, nullptr, &m_debugMessenger);
	}
	return true;
}

bool VulkanDevice::CreateSurface(void* hwnd)
{
	if (m_instance == VK_NULL_HANDLE || hwnd == NULL)
	{
		vkLog("CreateSurface: bad args", -1);
		return false;
	}
	VkWin32SurfaceCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	info.hinstance = GetModuleHandle(NULL);
	info.hwnd = (HWND)hwnd;
	VkResult res = vkCreateWin32SurfaceKHR(m_instance, &info, nullptr, &m_surface);
	vkLog("vkCreateWin32SurfaceKHR", (int)res);
	return res == VK_SUCCESS;
}

int VulkanDevice::RatePhysicalDevice(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(device, &props);
	if (props.apiVersion < VK_API_VERSION_1_3)
		return -1;

	uint32_t queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);
	std::vector<VkQueueFamilyProperties> queues(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, queues.data());

	bool hasGraphics = false;
	bool hasPresent = false;
	for (uint32_t i = 0; i < queueCount; i++) {
		if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			hasGraphics = true;
		VkBool32 present = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &present);
		if (present)
			hasPresent = true;
	}
	if (!hasGraphics || !hasPresent)
		return -1;

	uint32_t extCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);
	std::vector<VkExtensionProperties> exts(extCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, exts.data());
	bool hasSwapchain = false;
	for (size_t i = 0; i < exts.size(); i++) {
		if (strcmp(exts[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
			hasSwapchain = true;
			break;
		}
	}
	if (!hasSwapchain)
		return -1;

	int score = 0;
	if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		score += 1000;
	if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		score += 100;
	score += (int)(props.limits.maxImageDimension2D / 1024);
	return score;
}

bool VulkanDevice::PickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	VkResult enumRes = vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
	vkLog("vkEnumeratePhysicalDevices(count)", (int)enumRes);
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "physical device count=%u", (unsigned)deviceCount);
		vkLog(buf, -2);
	}
	if (enumRes != VK_SUCCESS || deviceCount == 0)
		return false;
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	int bestScore = -1;
	for (size_t i = 0; i < devices.size(); i++) {
		int score = RatePhysicalDevice(devices[i]);
		if (score > bestScore) {
			bestScore = score;
			m_physicalDevice = devices[i];
		}
	}
	return bestScore >= 0;
}

bool VulkanDevice::CreateLogicalDevice()
{
	uint32_t queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueCount, nullptr);
	std::vector<VkQueueFamilyProperties> queues(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueCount, queues.data());

	m_graphicsFamily = UINT32_MAX;
	m_presentFamily = UINT32_MAX;
	for (uint32_t i = 0; i < queueCount; i++) {
		if ((queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && m_graphicsFamily == UINT32_MAX)
			m_graphicsFamily = i;
		VkBool32 present = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, m_surface, &present);
		if (present && m_presentFamily == UINT32_MAX)
			m_presentFamily = i;
	}
	if (m_graphicsFamily == UINT32_MAX || m_presentFamily == UINT32_MAX)
	{
		vkLog("CreateLogicalDevice: no graphics/present queue", (int)m_graphicsFamily * 1000 + (int)m_presentFamily);
		return false;
	}

	std::vector<VkDeviceQueueCreateInfo> queueInfos;
	std::vector<uint32_t> families;
	families.push_back(m_graphicsFamily);
	if (m_presentFamily != m_graphicsFamily)
		families.push_back(m_presentFamily);
	float priority = 1.0f;
	for (size_t i = 0; i < families.size(); i++) {
		VkDeviceQueueCreateInfo queueInfo = {};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = families[i];
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &priority;
		queueInfos.push_back(queueInfo);
	}

	VkPhysicalDeviceFeatures features = {};
	features.samplerAnisotropy = VK_TRUE;
	features.fillModeNonSolid = VK_TRUE;
	features.independentBlend = VK_TRUE;

	const char *deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = (uint32_t)queueInfos.size();
	createInfo.pQueueCreateInfos = queueInfos.data();
	createInfo.pEnabledFeatures = &features;
	createInfo.enabledExtensionCount = 1;
	createInfo.ppEnabledExtensionNames = deviceExtensions;

	VkResult devRes = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
	vkLog("vkCreateDevice", (int)devRes);
	if (devRes != VK_SUCCESS)
		return false;

	vkGetDeviceQueue(m_device, m_graphicsFamily, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, m_presentFamily, 0, &m_presentQueue);
	return true;
}

bool VulkanDevice::CreateSwapchain()
{
	VkSurfaceCapabilitiesKHR caps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &caps);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> formats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());

	uint32_t presentCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes(presentCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentCount, presentModes.data());

	m_swapFormat = VK_FORMAT_B8G8R8A8_UNORM;
	VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	for (size_t i = 0; i < formats.size(); i++) {
		if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM) {
			m_swapFormat = formats[i].format;
			colorSpace = formats[i].colorSpace;
			break;
		}
	}

	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (size_t i = 0; i < presentModes.size(); i++) {
		if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
	}

	if (caps.currentExtent.width != (std::numeric_limits<uint32_t>::max)()) {
		m_swapExtent = caps.currentExtent;
	} else {
		RECT rect;
		GetClientRect(m_hwnd, &rect);
		m_swapExtent.width = rect.right - rect.left;
		m_swapExtent.height = rect.bottom - rect.top;
		if (m_swapExtent.width < caps.minImageExtent.width)
			m_swapExtent.width = caps.minImageExtent.width;
		if (m_swapExtent.height < caps.minImageExtent.height)
			m_swapExtent.height = caps.minImageExtent.height;
		if (m_swapExtent.width > caps.maxImageExtent.width)
			m_swapExtent.width = caps.maxImageExtent.width;
		if (m_swapExtent.height > caps.maxImageExtent.height)
			m_swapExtent.height = caps.maxImageExtent.height;
	}

	uint32_t imageCount = caps.minImageCount + 1;
	if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
		imageCount = caps.maxImageCount;

	VkSwapchainCreateInfoKHR swapInfo = {};
	swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapInfo.surface = m_surface;
	swapInfo.minImageCount = imageCount;
	swapInfo.imageFormat = m_swapFormat;
	swapInfo.imageColorSpace = colorSpace;
	swapInfo.imageExtent = m_swapExtent;
	swapInfo.imageArrayLayers = 1;
	swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	uint32_t families[] = { m_graphicsFamily, m_presentFamily };
	if (m_graphicsFamily != m_presentFamily) {
		swapInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapInfo.queueFamilyIndexCount = 2;
		swapInfo.pQueueFamilyIndices = families;
	} else {
		swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	swapInfo.preTransform = caps.currentTransform;
	swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapInfo.presentMode = presentMode;
	swapInfo.clipped = VK_TRUE;
	swapInfo.oldSwapchain = m_swapchain;

	VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
	VkResult scRes = vkCreateSwapchainKHR(m_device, &swapInfo, nullptr, &newSwapchain);
	vkLog("vkCreateSwapchainKHR", (int)scRes);
	if (scRes != VK_SUCCESS)
		return false;
	if (m_swapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
	m_swapchain = newSwapchain;

	uint32_t count = 0;
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &count, nullptr);
	m_swapImages.resize(count);
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &count, m_swapImages.data());
	return true;
}

bool VulkanDevice::CreateImageViews()
{
	m_swapImageViews.resize(m_swapImages.size());
	for (size_t i = 0; i < m_swapImages.size(); i++) {
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_swapImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = m_swapFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;
		if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_swapImageViews[i]) != VK_SUCCESS)
			return false;
	}
	return true;
}

bool VulkanDevice::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = m_swapFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorRef = {};
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = m_depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthRef = {};
	depthRef.attachment = 1;
	depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;
	subpass.pDepthStencilAttachment = &depthRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
		return false;

	VkAttachmentDescription rtColor = colorAttachment;
	rtColor.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	rtColor.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkAttachmentDescription rtAttachments[] = { rtColor, depthAttachment };
	VkRenderPassCreateInfo rtPassInfo = {};
	rtPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rtPassInfo.attachmentCount = 2;
	rtPassInfo.pAttachments = rtAttachments;
	rtPassInfo.subpassCount = 1;
	rtPassInfo.pSubpasses = &subpass;
	rtPassInfo.dependencyCount = 1;
	rtPassInfo.pDependencies = &dependency;
	return vkCreateRenderPass(m_device, &rtPassInfo, nullptr, &m_rtRenderPass) == VK_SUCCESS;
}

bool VulkanDevice::CreateDepthResources()
{
	VkFormat candidates[] = { VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM };
	m_depthFormat = VK_FORMAT_UNDEFINED;
	for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_physicalDevice, candidates[i], &props);
		if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			m_depthFormat = candidates[i];
			break;
		}
	}
	if (m_depthFormat == VK_FORMAT_UNDEFINED)
		return false;

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = m_depthFormat;
	imageInfo.extent.width = m_swapExtent.width;
	imageInfo.extent.height = m_swapExtent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateImage(m_device, &imageInfo, nullptr, &m_depthImage) != VK_SUCCESS)
		return false;

	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(m_device, m_depthImage, &memReq);
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_depthMemory) != VK_SUCCESS)
		return false;
	vkBindImageMemory(m_device, m_depthImage, m_depthMemory, 0);

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_depthImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = m_depthFormat;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	if (m_depthFormat == VK_FORMAT_D24_UNORM_S8_UINT)
		viewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;
	return vkCreateImageView(m_device, &viewInfo, nullptr, &m_depthView) == VK_SUCCESS;
}

bool VulkanDevice::CreateFramebuffers()
{
	m_framebuffers.resize(m_swapImageViews.size());
	for (size_t i = 0; i < m_swapImageViews.size(); i++) {
		VkImageView attachments[] = { m_swapImageViews[i], m_depthView };
		VkFramebufferCreateInfo fbInfo = {};
		fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbInfo.renderPass = m_renderPass;
		fbInfo.attachmentCount = 2;
		fbInfo.pAttachments = attachments;
		fbInfo.width = m_swapExtent.width;
		fbInfo.height = m_swapExtent.height;
		fbInfo.layers = 1;
		if (vkCreateFramebuffer(m_device, &fbInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS)
			return false;
	}
	return true;
}

bool VulkanDevice::CreateCommandPool()
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = m_graphicsFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	return vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) == VK_SUCCESS;
}

bool VulkanDevice::CreateCommandBuffers()
{
	m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
	return vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) == VK_SUCCESS;
}

bool VulkanDevice::CreateSyncObjects()
{
	m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semInfo = {};
	semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(m_device, &semInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS)
			return false;
		if (vkCreateSemaphore(m_device, &semInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS)
			return false;
		if (vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
			return false;
	}
	m_imagesInFlight.resize(m_swapImages.size(), VK_NULL_HANDLE);
	return true;
}

void VulkanDevice::CleanupSwapchain()
{
	for (size_t i = 0; i < m_framebuffers.size(); i++)
		vkDestroyFramebuffer(m_device, m_framebuffers[i], nullptr);
	m_framebuffers.clear();
	if (m_depthView != VK_NULL_HANDLE) {
		vkDestroyImageView(m_device, m_depthView, nullptr);
		m_depthView = VK_NULL_HANDLE;
	}
	if (m_depthImage != VK_NULL_HANDLE) {
		vkDestroyImage(m_device, m_depthImage, nullptr);
		m_depthImage = VK_NULL_HANDLE;
	}
	if (m_depthMemory != VK_NULL_HANDLE) {
		vkFreeMemory(m_device, m_depthMemory, nullptr);
		m_depthMemory = VK_NULL_HANDLE;
	}
	if (m_renderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(m_device, m_renderPass, nullptr);
		m_renderPass = VK_NULL_HANDLE;
	}
	if (m_rtRenderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(m_device, m_rtRenderPass, nullptr);
		m_rtRenderPass = VK_NULL_HANDLE;
	}
	for (size_t i = 0; i < m_swapImageViews.size(); i++)
		vkDestroyImageView(m_device, m_swapImageViews[i], nullptr);
	m_swapImageViews.clear();
	m_swapImages.clear();
	if (m_swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
		m_swapchain = VK_NULL_HANDLE;
	}
}

bool VulkanDevice::RecreateSwapchain()
{
	vkDeviceWaitIdle(m_device);
	CleanupSwapchain();
	if (!CreateSwapchain())
		return false;
	if (!CreateImageViews())
		return false;
	if (!CreateRenderPass())
		return false;
	if (!CreateDepthResources())
		return false;
	if (!CreateFramebuffers())
		return false;
	m_imagesInFlight.clear();
	m_imagesInFlight.resize(m_swapImages.size(), VK_NULL_HANDLE);
	m_framebufferResized = false;
	return true;
}

bool VulkanDevice::BeginFrame()
{
	vkWaitForFences(m_device, 1, &m_inFlightFences[m_frameIndex], VK_TRUE, ~0ULL);

	VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, ~0ULL,
		m_imageAvailableSemaphores[m_frameIndex], VK_NULL_HANDLE, &m_imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		if (!RecreateSwapchain())
			return false;
		return BeginFrame();
	}
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		return false;

	if (m_imagesInFlight[m_imageIndex] != VK_NULL_HANDLE)
		vkWaitForFences(m_device, 1, &m_imagesInFlight[m_imageIndex], VK_TRUE, UINT64_MAX);
	m_imagesInFlight[m_imageIndex] = m_inFlightFences[m_frameIndex];

	vkResetFences(m_device, 1, &m_inFlightFences[m_frameIndex]);
	vkResetCommandBuffer(m_commandBuffers[m_frameIndex], 0);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(m_commandBuffers[m_frameIndex], &beginInfo) != VK_SUCCESS)
		return false;
	return true;
}

void VulkanDevice::EndFrame(bool present)
{
	{
		static bool ph = true;
		if (ph) { ph = false; FILE *lf = fopen("RTS_START.LOG", "a"); if (lf) { fprintf(lf, "hb phase=EndFrame begin\n"); fclose(lf); } }
	}
	if (vkEndCommandBuffer(m_commandBuffers[m_frameIndex]) != VK_SUCCESS)
		return;
	{
		static bool ph = true;
		if (ph) { ph = false; FILE *lf = fopen("RTS_START.LOG", "a"); if (lf) { fprintf(lf, "hb phase=EndFrame cmdbuffer-ended\n"); fclose(lf); } }
	}

	VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_frameIndex] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_frameIndex] };

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[m_frameIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_frameIndex]) != VK_SUCCESS)
		return;
	{
		static bool ph = true;
		if (ph) { ph = false; FILE *lf = fopen("RTS_START.LOG", "a"); if (lf) { fprintf(lf, "hb phase=EndFrame submitted\n"); fclose(lf); } }
	}

	// One-time debug capture: dump the first presented frame to RTS_FRAME1.TGA
	{
		static bool captured = false;
		if (!captured) {
			captured = true;
			vkQueueWaitIdle(m_graphicsQueue);
			uint32_t w = m_swapExtent.width, h = m_swapExtent.height;
			VkDeviceSize size = (VkDeviceSize)w * h * 4;
			VkBufferCreateInfo bi = {};
			bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bi.size = size;
			bi.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VkBuffer cbuf = VK_NULL_HANDLE;
			if (vkCreateBuffer(m_device, &bi, nullptr, &cbuf) == VK_SUCCESS) {
				VkMemoryRequirements mr;
				vkGetBufferMemoryRequirements(m_device, cbuf, &mr);
				VkMemoryAllocateInfo ai = {};
				ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				ai.allocationSize = mr.size;
				ai.memoryTypeIndex = FindMemoryType(mr.memoryTypeBits,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
				VkDeviceMemory cmem = VK_NULL_HANDLE;
				if (ai.memoryTypeIndex != UINT32_MAX &&
					vkAllocateMemory(m_device, &ai, nullptr, &cmem) == VK_SUCCESS) {
					vkBindBufferMemory(m_device, cbuf, cmem, 0);
					VkCommandBuffer ccmd = BeginOneTimeCommands();
					if (ccmd) {
						VkImageMemoryBarrier b = {};
						b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
						b.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
						b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
						b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
						b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
						b.image = m_swapImages[m_imageIndex];
						b.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
						vkCmdPipelineBarrier(ccmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
						VkBufferImageCopy region = {};
						region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
						region.imageExtent = { w, h, 1 };
						vkCmdCopyImageToBuffer(ccmd, m_swapImages[m_imageIndex], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cbuf, 1, &region);
						VkImageMemoryBarrier b2 = b;
						b2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
						b2.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
						vkCmdPipelineBarrier(ccmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &b2);
						EndOneTimeCommands(ccmd);
						void *mapped = nullptr;
						if (vkMapMemory(m_device, cmem, 0, size, 0, &mapped) == VK_SUCCESS && mapped) {
							char path[MAX_PATH] = { 0 };
							GetModuleFileNameA(NULL, path, MAX_PATH);
							char *sl = strrchr(path, '\\');
							if (sl) strcpy(sl + 1, "RTS_FRAME1.BMP");
							else strcpy(path, "RTS_FRAME1.BMP");
							FILE *f = fopen(path, "wb");
							if (f) {
								uint32_t rowBytes = (w * 3 + 3) & ~3u;
								uint32_t fileSize = 54 + rowBytes * h;
								unsigned char hdr[54] = { 'B','M', 0,0,0,0, 54,0,0,0, 40,0,0,0 };
								hdr[2] = (unsigned char)(fileSize); hdr[3] = (unsigned char)(fileSize >> 8);
								hdr[4] = (unsigned char)(fileSize >> 16); hdr[5] = (unsigned char)(fileSize >> 24);
								hdr[10] = 54;
								hdr[14] = 40;  // biSize - was 0 = unreadable BMP
								hdr[18] = (unsigned char)(w); hdr[19] = (unsigned char)(w >> 8);
								hdr[22] = (unsigned char)(h); hdr[23] = (unsigned char)(h >> 8);
								hdr[26] = 1; hdr[28] = 24;
								fwrite(hdr, 1, 54, f);
								unsigned char *p = (unsigned char *)mapped;
								unsigned char row[8192 * 3];
								for (int y = (int)h - 1; y >= 0; --y) {
									for (uint32_t x = 0; x < w && x < 8192; ++x) {
										unsigned char *q = p + ((size_t)y * w + x) * 4;
										row[x * 3 + 0] = q[0];
										row[x * 3 + 1] = q[1];
										row[x * 3 + 2] = q[2];
									}
									fwrite(row, 1, rowBytes, f);
								}
								fclose(f);
								FILE *lf = fopen("RTS_START.LOG", "a");
								if (lf) { fprintf(lf, "VK: captured RTS_FRAME1.BMP %ux%u\n", (unsigned)w, (unsigned)h); fclose(lf); }
							}
							vkUnmapMemory(m_device, cmem);
						}
					}
					vkDestroyBuffer(m_device, cbuf, nullptr);
					vkFreeMemory(m_device, cmem, nullptr);
				}
			}
		}
	}
	if (!present)
		return;

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain;
	presentInfo.pImageIndices = &m_imageIndex;
	VkResult result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
	{
		static bool ph = true;
		if (ph) { ph = false; FILE *lf = fopen("RTS_START.LOG", "a"); if (lf) { fprintf(lf, "hb phase=EndFrame presented res=%d\n", (int)result); fclose(lf); } }
	}
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized)
		RecreateSwapchain();

	m_frameIndex = (m_frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

uint32_t VulkanDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProps);
	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}
	return UINT32_MAX;
}

VkCommandBuffer VulkanDevice::BeginOneTimeCommands()
{
	if (m_device == VK_NULL_HANDLE || m_commandPool == VK_NULL_HANDLE)
		return VK_NULL_HANDLE;
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	VkCommandBuffer cmd = VK_NULL_HANDLE;
	if (vkAllocateCommandBuffers(m_device, &allocInfo, &cmd) != VK_SUCCESS)
		return VK_NULL_HANDLE;
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS)
		return VK_NULL_HANDLE;
	return cmd;
}

void VulkanDevice::EndOneTimeCommands(VkCommandBuffer cmd)
{
	vkEndCommandBuffer(cmd);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;
	vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphicsQueue);
	vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
}
