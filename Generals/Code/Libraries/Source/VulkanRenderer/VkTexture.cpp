#include "VkTexture.h"
#include "VkDevice.h"
#include "DX8Compat/d3d8types.h"
#include <cstring>
#include <cstdio>
#include <cassert>

// Device-memory accounting. The field run died with
// "VK: UploadLevel vkAllocateMemory -> -2" (OUT_OF_DEVICE_MEMORY) ~20 frames
// after the Options menu opened, then every 64x64 surface create failed and the
// screen went white - so track live texture bytes and log every 256MB step.
uint64_t g_liveTexBytes = 0;
uint64_t g_liveTexCount = 0;
static uint64_t g_lastLoggedMB = 0;
static uint64_t g_totalTexCreates = 0;

static void logTexMemory(const char *why)
{
	FILE *lf = fopen("RTS_START.LOG", "a");
	if (lf) {
		fprintf(lf, "VK: VkTexture %s live=%lluMB count=%llu creates=%llu\n", why,
			(unsigned long long)(g_liveTexBytes / (1024ull * 1024ull)),
			(unsigned long long)g_liveTexCount,
			(unsigned long long)g_totalTexCreates);
		fclose(lf);
	}
}

VkTexture::VkTexture()
	: m_device(nullptr)
	, m_image(VK_NULL_HANDLE)
	, m_memory(VK_NULL_HANDLE)
	, m_view(VK_NULL_HANDLE)
	, m_format(VK_FORMAT_UNDEFINED)
	, m_width(0)
	, m_height(0)
	, m_mips(1)
	, m_allocBytes(0)
	, m_samplerCount(0)
{
	for (int i = 0; i < 4; i++) {
		m_samplers[i].filtered = false;
		m_samplers[i].repeat = false;
		m_samplers[i].sampler = VK_NULL_HANDLE;
	}
}

VkTexture::~VkTexture()
{
	Destroy();
}

VkFormat VkTexture::TranslateFormat(unsigned d3dFormat)
{
	switch (d3dFormat) {
	case D3DFMT_A8R8G8B8:
	case D3DFMT_X8R8G8B8:
		return VK_FORMAT_B8G8R8A8_UNORM;
	// Port fix (validation log, RX 7900 XTX): VK_FORMAT_R8G8B8_UNORM and the
	// PACK16 formats are NOT supported with OPTIMAL tiling on modern AMD -
	// vkCreateImage returns FORMAT_NOT_SUPPORTED, the VkTexture kept a NULL
	// image with a live device pointer, and every upload/draw through it was
	// invalid (black menu elements + driver AVs). All CPU-side sources are
	// 32-bit A8R8G8B8 shadows anyway, so promote to BGRA8.
	case D3DFMT_R8G8B8:
	case D3DFMT_R5G6B5:
	case D3DFMT_X1R5G5B5:
	case D3DFMT_A1R5G5B5:
	case D3DFMT_A4R4G4B4:
	case D3DFMT_X4R4G4B4:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case D3DFMT_R3G3B2:
	case D3DFMT_A8R3G3B2:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case D3DFMT_A8:
	case D3DFMT_L8:
		return VK_FORMAT_R8_UNORM;
	case D3DFMT_A8L8:
		return VK_FORMAT_R8G8_UNORM;
	case D3DFMT_A4L4:
	case D3DFMT_P8:
	case D3DFMT_A8P8:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case D3DFMT_DXT1:
		return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
	case D3DFMT_DXT2:
	case D3DFMT_DXT3:
		return VK_FORMAT_BC2_UNORM_BLOCK;
	case D3DFMT_DXT4:
	case D3DFMT_DXT5:
		return VK_FORMAT_BC3_UNORM_BLOCK;
	case D3DFMT_V8U8:
		return VK_FORMAT_R8G8_SNORM;
	case D3DFMT_L6V5U5:
	case D3DFMT_X8L8V8U8:
	case D3DFMT_Q8W8V8U8:
	case D3DFMT_V16U16:
	case D3DFMT_W11V11U10:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case D3DFMT_D16:
	case D3DFMT_D16_LOCKABLE:
		return VK_FORMAT_D16_UNORM;
	case D3DFMT_D24S8:
		return VK_FORMAT_D24_UNORM_S8_UINT;
	case D3DFMT_D32:
		return VK_FORMAT_D32_SFLOAT;
	default:
		break;
	}
	return VK_FORMAT_B8G8R8A8_UNORM;
}

bool VkTexture::Create(VulkanDevice *device, uint32_t width, uint32_t height, VkFormat format,
	uint32_t mipLevels, bool renderTarget)
{
	Destroy();
	if (device == nullptr || width == 0 || height == 0)
		return false;
	m_device = device;
	m_width = width;
	m_height = height;
	m_format = format;
	m_mips = mipLevels > 0 ? mipLevels : 1;

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = format;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = m_mips;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	// TRANSFER_SRC always: SurfaceClass::Lock readbacks (team-color recolor)
	// are legal on every image, not just mip chains (1-mip images were
	// missing the bit -> undefined readback on strict drivers).
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	if (renderTarget)
		imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkDevice dev = device->GetDevice();
	if (vkCreateImage(dev, &imageInfo, nullptr, &m_image) != VK_SUCCESS) {
		FILE *lf = fopen("RTS_START.LOG", "a");
		if (lf) { fprintf(lf, "VK: tex create FAILED vkCreateImage %ux%u fmt=%d\n", width, height, (int)format); fclose(lf); }
		Destroy();	// clear m_device too - a device-live texture with no image makes every upload/draw invalid
		return false;
	}
	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(dev, m_image, &memReq);
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = device->FindMemoryType(memReq.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (allocInfo.memoryTypeIndex == UINT32_MAX) {
		Destroy();
		return false;
	}
	if (vkAllocateMemory(dev, &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
		FILE *lf = fopen("RTS_START.LOG", "a");
		if (lf) {
			fprintf(lf, "VK: tex create OOM %ux%u bytes=%llu live=%lluMB\n", width, height,
				(unsigned long long)memReq.size,
				(unsigned long long)(g_liveTexBytes / (1024ull * 1024ull)));
			fclose(lf);
		}
		Destroy();
		return false;
	}
	vkBindImageMemory(dev, m_image, m_memory, 0);

	// device-memory accounting (see g_liveTexBytes at top of file)
	m_allocBytes = memReq.size;
	g_liveTexBytes += m_allocBytes;
	g_liveTexCount++;
	g_totalTexCreates++;
	// Sampled caller log for SMALL creates: live bytes stay flat while creates
	// climb (hundreds/sec) = something churns textures every frame; descriptors
	// referencing a destroyed image while it is still in flight is the leading
	// suspect for the driver AVs (amdvlk mission crash / lavapipe shell-map
	// crash). Sample so the field log names the churn source.
	if ((uint64_t)width * (uint64_t)height <= 256ull * 256ull && (g_totalTexCreates % 256ull) == 0) {
		FILE *lf = fopen("RTS_START.LOG", "a");
		if (lf) {
			fprintf(lf, "VK: small tex create %ux%u fmt=%d caller=%p creates=%llu\n",
				width, height, (int)format, __builtin_return_address(0),
				(unsigned long long)g_totalTexCreates);
			fclose(lf);
		}
	}
	{
		uint64_t mb = g_liveTexBytes / (1024ull * 1024ull);
		if (mb / 256ull != g_lastLoggedMB / 256ull) {
			g_lastLoggedMB = g_liveTexBytes;
			logTexMemory("LIVE");
		}
		if ((g_totalTexCreates % 512ull) == 0)
			logTexMemory("creates");
	}

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = m_mips;
	viewInfo.subresourceRange.layerCount = 1;
	if (vkCreateImageView(dev, &viewInfo, nullptr, &m_view) != VK_SUCCESS) {
		Destroy();
		return false;
	}

	VkCommandBuffer cmd = device->BeginOneTimeCommands();
	TransitionLayout(cmd, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, m_mips);
	device->EndOneTimeCommands(cmd);
	return true;
}

bool VkTexture::CreateFromData(VulkanDevice *device, uint32_t width, uint32_t height, VkFormat format,
	uint32_t mipLevels, const void *pixels)
{
	if (!Create(device, width, height, format, mipLevels, false))
		return false;
	UploadLevel(0, width, height, pixels);
	if (mipLevels > 1)
		GenerateMipmaps();
	return true;
}

void VkTexture::TransitionLayout(VkCommandBuffer cmd, VkImageLayout oldLayout, VkImageLayout newLayout,
	uint32_t baseMip, uint32_t mipCount)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = baseMip;
	barrier.subresourceRange.levelCount = mipCount;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VkTexture::UploadLevel(uint32_t mip, uint32_t width, uint32_t height, const void *pixels)
{
	// A 0x0 upload or NULL source means the caller's DDS/header parse failed;
	// proceeding creates invalid Vulkan objects and crashes GPU drivers.
	if (width == 0 || height == 0 || pixels == nullptr)
		return;
	if (m_device == nullptr) {
		static bool logged = false;
		if (!logged) {
			char buf[96];
			snprintf(buf, sizeof(buf), "VK: UploadLevel on texture %p with NULL device (w=%u h=%u)", (void*)this, (unsigned)width, (unsigned)height);
			FILE *lf = fopen("RTS_START.LOG", "a");
			if (lf) { fprintf(lf, "%s\n", buf); fclose(lf); }
			logged = true;
		}
		return;
	}
	VkDevice dev = m_device->GetDevice();
	VkDeviceSize rowPitch = 0;
	VkDeviceSize imageSize = 0;
	bool compressed = false;
	switch (m_format) {
	case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
		compressed = true;
		rowPitch = ((width + 3) / 4) * 8;
		imageSize = rowPitch * ((height + 3) / 4);
		break;
	case VK_FORMAT_BC2_UNORM_BLOCK:
	case VK_FORMAT_BC3_UNORM_BLOCK:
		compressed = true;
		rowPitch = ((width + 3) / 4) * 16;
		imageSize = rowPitch * ((height + 3) / 4);
		break;
	default:
		break;
	}
	if (!compressed) {
		uint32_t texelSize = 4;
		switch (m_format) {
		case VK_FORMAT_R8_UNORM:
			texelSize = 1;
			break;
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8_SNORM:
		case VK_FORMAT_R5G6B5_UNORM_PACK16:
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
		case VK_FORMAT_A4R4G4B4_UNORM_PACK16:
			texelSize = 2;
			break;
		case VK_FORMAT_R8G8B8_UNORM:
			texelSize = 3;
			break;
		default:
			texelSize = 4;
			break;
		}
		rowPitch = (VkDeviceSize)width * texelSize;
		imageSize = rowPitch * height;
	}

	VkBuffer staging = VK_NULL_HANDLE;
	VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = imageSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	{
		static int uploadLogCount = 0;
		// always log non-standard pitches (sentence/font atlases are w*4)
		if (width >= 1024 || uploadLogCount < 8 || rowPitch != (VkDeviceSize)width * 4) {
			char buf[128];
			snprintf(buf, sizeof(buf), "VK: upload w=%u h=%u fmt=%d size=%llu pitch=%llu caller=%p",
				(unsigned)width, (unsigned)height, (int)m_format, (unsigned long long)imageSize, (unsigned long long)rowPitch,
				__builtin_return_address(0));
			FILE *lf = fopen("RTS_START.LOG", "a");
			if (lf) { fprintf(lf, "%s\n", buf); fclose(lf); }
			uploadLogCount++;
		}
	}
	VkResult res = vkCreateBuffer(dev, &bufferInfo, nullptr, &staging);
	if (res != VK_SUCCESS) {
		FILE *lf = fopen("RTS_START.LOG", "a");
		if (lf) { fprintf(lf, "VK: UploadLevel vkCreateBuffer -> %d\n", (int)res); fclose(lf); }
		return;
	}
	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(dev, staging, &memReq);
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = m_device->FindMemoryType(memReq.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (allocInfo.memoryTypeIndex == UINT32_MAX) {
		vkDestroyBuffer(dev, staging, nullptr);
		return;
	}
	res = vkAllocateMemory(dev, &allocInfo, nullptr, &stagingMemory);
	if (res != VK_SUCCESS) {
		FILE *lf = fopen("RTS_START.LOG", "a");
		if (lf) { fprintf(lf, "VK: UploadLevel vkAllocateMemory -> %d\n", (int)res); fclose(lf); }
		vkDestroyBuffer(dev, staging, nullptr);
		return;
	}
	res = vkBindBufferMemory(dev, staging, stagingMemory, 0);
	if (res != VK_SUCCESS) {
		FILE *lf = fopen("RTS_START.LOG", "a");
		if (lf) { fprintf(lf, "VK: UploadLevel vkBindBufferMemory -> %d\n", (int)res); fclose(lf); }
		vkDestroyBuffer(dev, staging, nullptr);
		vkFreeMemory(dev, stagingMemory, nullptr);
		return;
	}
	void *mapped = nullptr;
	res = vkMapMemory(dev, stagingMemory, 0, imageSize, 0, &mapped);
	if (res != VK_SUCCESS || mapped == nullptr) {
		FILE *lf = fopen("RTS_START.LOG", "a");
		if (lf) { fprintf(lf, "VK: UploadLevel vkMapMemory -> %d\n", (int)res); fclose(lf); }
		vkDestroyBuffer(dev, staging, nullptr);
		vkFreeMemory(dev, stagingMemory, nullptr);
		return;
	}
	memcpy(mapped, pixels, (size_t)imageSize);
	vkUnmapMemory(dev, stagingMemory);

	VkCommandBuffer cmd = m_device->BeginOneTimeCommands();
	if (cmd == VK_NULL_HANDLE) {
		vkDestroyBuffer(dev, staging, nullptr);
		vkFreeMemory(dev, stagingMemory, nullptr);
		return;
	}
	TransitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip, 1);
	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = mip;
	region.imageSubresource.layerCount = 1;
	region.imageOffset.x = 0;
	region.imageOffset.y = 0;
	region.imageOffset.z = 0;
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;
	vkCmdCopyBufferToImage(cmd, staging, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	TransitionLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mip, 1);
	m_device->EndOneTimeCommands(cmd);

	vkDestroyBuffer(dev, staging, nullptr);
	vkFreeMemory(dev, stagingMemory, nullptr);
}

bool VkTexture::ReadbackLevel(void *dst, uint32_t pitchBytes)
{
	if (!m_device || m_image == VK_NULL_HANDLE || !dst || m_width == 0 || m_height == 0)
		return false;
	// 32-bit texels only (B8G8R8A8/R8G8B8A8/other 4-byte formats the port uses)
	const uint32_t bpp = 4;
	const size_t rowBytes = (size_t)m_width * bpp;
	const size_t totalBytes = rowBytes * m_height;
	if (pitchBytes < rowBytes)
		return false;

	VulkanDevice &dev = *m_device;
	VkDevice device = dev.GetDevice();

	VkBufferCreateInfo bi = {};
	bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bi.size = totalBytes;
	bi.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkBuffer buf = VK_NULL_HANDLE;
	if (vkCreateBuffer(device, &bi, nullptr, &buf) != VK_SUCCESS)
		return false;
	VkMemoryRequirements mr;
	vkGetBufferMemoryRequirements(device, buf, &mr);
	VkMemoryAllocateInfo ai = {};
	ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	ai.allocationSize = mr.size;
	ai.memoryTypeIndex = dev.FindMemoryType(mr.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VkDeviceMemory mem = VK_NULL_HANDLE;
	if (ai.memoryTypeIndex == UINT32_MAX ||
		vkAllocateMemory(device, &ai, nullptr, &mem) != VK_SUCCESS) {
		vkDestroyBuffer(device, buf, nullptr);
		return false;
	}
	vkBindBufferMemory(device, buf, mem, 0);

	VkCommandBuffer cmd = dev.BeginOneTimeCommands();
	if (cmd == VK_NULL_HANDLE) {
		vkDestroyBuffer(device, buf, nullptr);
		vkFreeMemory(device, mem, nullptr);
		return false;
	}
	{
		VkImageMemoryBarrier b = {};
		b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		b.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		b.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		b.image = m_image;
		b.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		b.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
	}
	{
		VkBufferImageCopy rg = {};
		rg.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		rg.imageExtent = { m_width, m_height, 1 };
		vkCmdCopyImageToBuffer(cmd, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buf, 1, &rg);
	}
	{
		VkImageMemoryBarrier b = {};
		b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		b.image = m_image;
		b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		b.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
	}
	dev.EndOneTimeCommands(cmd);	// submits and waits idle

	void *mapped = nullptr;
	bool ok = false;
	if (vkMapMemory(device, mem, 0, VK_WHOLE_SIZE, 0, &mapped) == VK_SUCCESS && mapped) {
		const unsigned char *src = (const unsigned char *)mapped;
		unsigned char *d = (unsigned char *)dst;
		for (uint32_t y = 0; y < m_height; y++) {
			memcpy(d + (size_t)y * pitchBytes, src + (size_t)y * rowBytes, rowBytes);
		}
		vkUnmapMemory(device, mem);
		ok = true;
	}
	vkDestroyBuffer(device, buf, nullptr);
	vkFreeMemory(device, mem, nullptr);
	return ok;
}

void VkTexture::GenerateMipmaps()
{
	// Port fix (AMD black terrain): mid-loop level i was blitted into
	// TRANSFER_DST and NEVER transitioned back, while the next iteration
	// claimed oldLayout=SHADER_READ_ONLY for it (spec lie) - and only the
	// last level got a final TRANSFER_DST->SHADER_READ transition. Result:
	// levels 1..m_mips-2 stayed in TRANSFER_DST -> sampling them is
	// undefined -> AMD returns black (menus are single-mip -> worked),
	// lavapipe tolerates it (wine showed terrain). Invariant now: every
	// level ends each iteration in SHADER_READ_ONLY_OPTIMAL.
	VkCommandBuffer cmd = m_device->BeginOneTimeCommands();
	uint32_t w = m_width;
	uint32_t h = m_height;
	for (uint32_t i = 1; i < m_mips; i++) {
		TransitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, i - 1, 1);
		uint32_t nw = w > 1 ? w / 2 : 1;
		uint32_t nh = h > 1 ? h / 2 : 1;
		VkImageBlit blit = {};
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.layerCount = 1;
		blit.srcOffsets[1].x = (int32_t)w;
		blit.srcOffsets[1].y = (int32_t)h;
		blit.srcOffsets[1].z = 1;
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.layerCount = 1;
		blit.dstOffsets[1].x = (int32_t)nw;
		blit.dstOffsets[1].y = (int32_t)nh;
		blit.dstOffsets[1].z = 1;
		TransitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, i, 1);
		vkCmdBlitImage(cmd, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
		TransitionLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, i - 1, 1);
		// restore level i so the next iteration's SHADER_READ_ONLY claim is true
		TransitionLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, i, 1);
		w = nw;
		h = nh;
	}
	m_device->EndOneTimeCommands(cmd);
}

bool VkTexture::CreateDepth(VulkanDevice *device, uint32_t width, uint32_t height)
{
	Destroy();
	m_device = device;
	m_width = width;
	m_height = height;
	m_format = device->GetDepthFormat();
	m_mips = 1;

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = m_format;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkDevice dev = device->GetDevice();
	if (vkCreateImage(dev, &imageInfo, nullptr, &m_image) != VK_SUCCESS)
		return false;
	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(dev, m_image, &memReq);
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = device->FindMemoryType(memReq.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (allocInfo.memoryTypeIndex == UINT32_MAX) {
		Destroy();
		return false;
	}
	if (vkAllocateMemory(dev, &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
		FILE *lf = fopen("RTS_START.LOG", "a");
		if (lf) {
			fprintf(lf, "VK: depth create OOM %ux%u bytes=%llu\n", width, height,
				(unsigned long long)memReq.size);
			fclose(lf);
		}
		Destroy();
		return false;
	}
	vkBindImageMemory(dev, m_image, m_memory, 0);
	m_allocBytes = memReq.size;
	g_liveTexBytes += m_allocBytes;
	g_liveTexCount++;

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = m_format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	if (m_format == VK_FORMAT_D24_UNORM_S8_UINT)
		viewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;
	if (vkCreateImageView(dev, &viewInfo, nullptr, &m_view) != VK_SUCCESS) {
		Destroy();
		return false;
	}
	return true;
}

// Process-wide sampler cache: samplers used to live per-texture and were
// destroyed with it - validation caught vkDestroySampler on a sampler still
// referenced by an in-flight command buffer (the fixed-address amdvlk AV).
// Samplers are now shared (only 4 combos) and live for the process lifetime.
static VulkanDevice *g_samplerDevice = nullptr;
static SamplerEntry g_sharedSamplers[4];
static int g_sharedSamplerCount = 0;

void Vulkan_Reset_Shared_Samplers()
{
	for (int i = 0; i < g_sharedSamplerCount; i++) {
		if (g_sharedSamplers[i].sampler != VK_NULL_HANDLE && g_samplerDevice)
			vkDestroySampler(g_samplerDevice->GetDevice(), g_sharedSamplers[i].sampler, nullptr);
		g_sharedSamplers[i].sampler = VK_NULL_HANDLE;
	}
	g_sharedSamplerCount = 0;
	g_samplerDevice = nullptr;
}

VkSampler VkTexture::FindOrCreateSampler(bool filtered, bool repeat)
{
	if (m_device == nullptr) {
		static bool logged = false;
		if (!logged) {
			char buf[160];
			snprintf(buf, sizeof(buf), "VK: GetSampler on texture %p with NULL device (w=%u h=%u fmt=%d)",
				(void*)this, (unsigned)m_width, (unsigned)m_height, (int)m_format);
			FILE *lf = fopen("RTS_START.LOG", "a");
			if (lf) { fprintf(lf, "%s\n", buf); fclose(lf); }
			logged = true;
		}
		return VK_NULL_HANDLE;
	}
	for (int i = 0; i < g_sharedSamplerCount; i++) {
		if (g_sharedSamplers[i].filtered == filtered && g_sharedSamplers[i].repeat == repeat)
			return g_sharedSamplers[i].sampler;
	}
	if (g_sharedSamplerCount >= 4)
		return g_sharedSamplers[0].sampler;

	g_samplerDevice = m_device;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = filtered ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
	samplerInfo.minFilter = filtered ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.anisotropyEnable = filtered ? VK_TRUE : VK_FALSE;
	samplerInfo.maxAnisotropy = filtered ? 8.0f : 1.0f;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1000.0f;	// implicit clamp to the image's real level count (a 1-mip
	// texture was baking maxLod=1 into the SHARED sampler cache for the whole
	// process - the first texture to touch a slot was the 1x1 white fallback)
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	VkSampler sampler = VK_NULL_HANDLE;
	if (vkCreateSampler(m_device->GetDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
		samplerInfo.anisotropyEnable = VK_FALSE;	// e.g. maxAnisotropy > device limit
		samplerInfo.maxAnisotropy = 1.0f;
		if (vkCreateSampler(m_device->GetDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
			return VK_NULL_HANDLE;
	}
	g_sharedSamplers[g_sharedSamplerCount].filtered = filtered;
	g_sharedSamplers[g_sharedSamplerCount].repeat = repeat;
	g_sharedSamplers[g_sharedSamplerCount].sampler = sampler;
	g_sharedSamplerCount++;
	return sampler;
}

VkSampler VkTexture::GetSampler(bool filtered, bool repeat)
{
	return FindOrCreateSampler(filtered, repeat);
}

void VkTexture::Destroy()
{
	if (!m_device)
		return;
	VkDevice dev = m_device->GetDevice();
	// NOTE: samplers are process-wide now (g_sharedSamplers) - they must NOT be
	// destroyed here: in-flight command buffers still reference them.
	m_samplerCount = 0;
	if (m_view != VK_NULL_HANDLE) {
		vkDestroyImageView(dev, m_view, nullptr);
		m_view = VK_NULL_HANDLE;
	}
	if (m_image != VK_NULL_HANDLE) {
		vkDestroyImage(dev, m_image, nullptr);
		m_image = VK_NULL_HANDLE;
	}
	if (m_memory != VK_NULL_HANDLE) {
		vkFreeMemory(dev, m_memory, nullptr);
		m_memory = VK_NULL_HANDLE;
	}
	if (m_allocBytes) {
		g_liveTexBytes = (g_liveTexBytes > m_allocBytes) ? g_liveTexBytes - m_allocBytes : 0;
		if (g_liveTexCount > 0)
			g_liveTexCount--;
		m_allocBytes = 0;
	}
	m_device = nullptr;
}
