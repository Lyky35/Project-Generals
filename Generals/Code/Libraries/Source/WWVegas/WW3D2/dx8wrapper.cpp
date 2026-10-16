/*
**	Command & Conquer Generals(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : WW3D                                                         *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/ww3d2/dx8wrapper.cpp                          $*
 *                                                                                             *
 *              Original Author:: Hector Yee                                                   *
 *                                                                                             *
 *                     $Modtime:: 12/13/01 2:52p                                               $*
 *                                                                                             *
 *                    $Revision:: 46                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "dx8wrapper.h"
#include "dx8fvf.h"
#include "dx8vertexbuffer.h"
#define DEFAULT_RESOLUTION_WIDTH 1024
#define DEFAULT_RESOLUTION_HEIGHT 768
#define DEFAULT_BIT_DEPTH 32
#define DEFAULT_TEXTURE_BIT_DEPTH 32
static bool DX8Wrapper_IsWindowed = false;
#include "dx8indexbuffer.h"
#include "dx8renderer.h"
#include "ww3d.h"
#include "camera.h"
#include "wwstring.h"
#include "matrix4.h"
#include "vertmaterial.h"
#include "rddesc.h"
#include "lightenvironment.h"
#include "statistics.h"
#include "registry.h"
#include "boxrobj.h"
#include "pointgr.h"
#include "render2d.h"
#include "sortingrenderer.h"
#include "shattersystem.h"
#include "light.h"
#include "assetmgr.h"
#include "textureloader.h"
#include "missingtexture.h"
#include "thread.h"
#include <stdio.h>
#include "d3d8types.h"
#include "pot.h"
#include "wwprofile.h"
#include "ffactory.h"
#include "dx8caps.h"
#include "formconv.h"
#include "dx8texman.h"
#include "bound.h"
#include "dx8webbrowser.h"
#include "surfaceclass.h"
#include <math.h>
#include <string.h>
#include "VkTexture.h"
#include "VkGpuBuffer.h"
#include "VkShader.h"
#include "VkDescriptors.h"
#include "VkDevice.h"

static HWND						_Hwnd															= NULL;
bool								DX8Wrapper::IsInitted									= false;
bool								DX8Wrapper::_EnableTriangleDraw						= true;

int								DX8Wrapper::CurRenderDevice							= -1;
int								DX8Wrapper::ResolutionWidth							= DEFAULT_RESOLUTION_WIDTH;
int								DX8Wrapper::ResolutionHeight							= DEFAULT_RESOLUTION_HEIGHT;
int								DX8Wrapper::BitDepth										= DEFAULT_BIT_DEPTH;
int								DX8Wrapper::TextureBitDepth							= DEFAULT_TEXTURE_BIT_DEPTH;
bool								DX8Wrapper::IsWindowed									= false;
D3DFORMAT					DX8Wrapper::DisplayFormat	= D3DFMT_UNKNOWN;

D3DMATRIX						DX8Wrapper::old_world;
D3DMATRIX						DX8Wrapper::old_view;
D3DMATRIX						DX8Wrapper::old_prj;

bool								DX8Wrapper::world_identity;
unsigned							DX8Wrapper::RenderStates[256];
unsigned							DX8Wrapper::TextureStageStates[MAX_TEXTURE_STAGES][32];
TextureClass *				DX8Wrapper::BoundTextures[MAX_TEXTURE_STAGES];
RenderStateStruct				DX8Wrapper::render_state;
unsigned							DX8Wrapper::render_state_changed;

bool								DX8Wrapper::FogEnable									= false;
D3DCOLOR							DX8Wrapper::FogColor										= 0;

VkShaderManager				DX8Wrapper::ShaderManager;
VkDescriptorAllocator		DX8Wrapper::DescriptorAllocator;
bool								DX8Wrapper::VulkanFrameOpen							= false;

VkTexture *						DX8Wrapper::CurrentRenderTarget						= NULL;
bool								DX8Wrapper::UsingDefaultTarget							= true;
float								DX8Wrapper::ClipPlanes[6][4];
Matrix4							DX8Wrapper::CachedProjection;
Matrix4							DX8Wrapper::CachedTexTransform[8];
D3DMATERIAL8						DX8Wrapper::CachedMaterial;
D3DVIEWPORT8						DX8Wrapper::CachedViewport;
bool								DX8Wrapper::ViewportValid								= false;
float								DX8Wrapper::GammaValue									= 1.0f;
float								DX8Wrapper::BrightnessValue								= 0.0f;
float								DX8Wrapper::ContrastValue								= 1.0f;
int								DX8Wrapper::PolygonModeValue							= 0;

unsigned							DX8Wrapper::matrix_changes								= 0;
unsigned							DX8Wrapper::material_changes							= 0;
unsigned							DX8Wrapper::vertex_buffer_changes					= 0;
unsigned							DX8Wrapper::index_buffer_changes                = 0;
unsigned							DX8Wrapper::light_changes								= 0;
unsigned							DX8Wrapper::texture_changes							= 0;
unsigned							DX8Wrapper::render_state_changes						= 0;
unsigned							DX8Wrapper::texture_stage_state_changes			= 0;
unsigned							DX8Wrapper::_MainThreadID								= 0;
bool								DX8Wrapper::CurrentDX8LightEnables[4];

D3DADAPTER_IDENTIFIER8		DX8Wrapper::CurrentAdapterIdentifier;

unsigned long DX8Wrapper::FrameCount = 0;

static unsigned				last_frame_matrix_changes								= 0;
static unsigned				last_frame_material_changes							= 0;
static unsigned				last_frame_vertex_buffer_changes						= 0;
static unsigned				last_frame_index_buffer_changes						= 0;
static unsigned				last_frame_light_changes								= 0;
static unsigned				last_frame_texture_changes							= 0;
static unsigned				last_frame_render_state_changes						= 0;
static unsigned				last_frame_texture_stage_state_changes				= 0;
static unsigned				last_frame_number_of_DX8_calls						= 0;

static D3DPRESENT_PARAMETERS								_PresentParameters;
static DynamicVectorClass<StringClass>					_RenderDeviceNameTable;
static DynamicVectorClass<StringClass>					_RenderDeviceShortNameTable;
static DynamicVectorClass<RenderDeviceDescClass>	_RenderDeviceDescriptionTable;

unsigned number_of_DX8_calls = 0;
bool _DX8SingleThreaded = true;

DX8_CleanupHook	 *DX8Wrapper::m_pCleanupHook=NULL;

#ifdef EXTENDED_STATS
DX8_Stats	 DX8Wrapper::stats;
#endif

static const int VULKAN_UBO_RING_DRAWS = 2048;
// Ring stride MUST be a multiple of the device's minUniformBufferOffsetAlignment
// (it applies to descriptor bufferInfo.offset too). AMD can report > 512 while
// lavapipe reports 16 - a fixed 512 stride then makes every non-zero ring slot
// an invalid descriptor offset (UB -> garbage mvp -> world clipped -> black).
static uint32_t VulkanUboStride = 512;
// Stride MUST be >= sizeof(VkFrameUniforms) (352 bytes!) rounded up to 256.
// It used to be a flat 256, so every UBO entry overwrote the previous entry's
// tail (worldViewProj/world) and the last entries overran the buffer - only
// survivable because the shaders happen to read the first 224 bytes.
static const int VULKAN_UBO_ALIGN = ((int)((sizeof(VkFrameUniforms) + 255) / 256) * 256);

#define	VALUE_NAME_RENDER_DEVICE_NAME					"RenderDeviceName"
#define	VALUE_NAME_RENDER_DEVICE_WIDTH				"RenderDeviceWidth"
#define	VALUE_NAME_RENDER_DEVICE_HEIGHT				"RenderDeviceHeight"
#define	VALUE_NAME_RENDER_DEVICE_DEPTH				"RenderDeviceDepth"
#define	VALUE_NAME_RENDER_DEVICE_WINDOWED			"RenderDeviceWindowed"
#define	VALUE_NAME_RENDER_DEVICE_TEXTURE_DEPTH		"RenderDeviceTextureDepth"

inline DWORD F2DW(float f) { return *((unsigned*)&f); }

static VkFormat VulkanTranslateWW3DFormat(WW3DFormat format)
{
	switch (format) {
	// Port fix (validation log, RX 7900 XTX): R8G8B8 and the PACK16 formats are
	// NOT supported with OPTIMAL tiling on modern AMD (vkCreateImage ->
	// FORMAT_NOT_SUPPORTED). All CPU-side sources are 32-bit A8R8G8B8 shadows,
	// so promote to BGRA8 (same as VkTexture::TranslateFormat).
	case WW3D_FORMAT_R8G8B8:
	case WW3D_FORMAT_R5G6B5:
	case WW3D_FORMAT_X1R5G5B5:
	case WW3D_FORMAT_A1R5G5B5:
	case WW3D_FORMAT_A4R4G4B4:
	case WW3D_FORMAT_X4R4G4B4:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case WW3D_FORMAT_A8R8G8B8:
	case WW3D_FORMAT_X8R8G8B8:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case WW3D_FORMAT_R3G3B2:
	case WW3D_FORMAT_A8R3G3B2:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case WW3D_FORMAT_A8:
	case WW3D_FORMAT_L8:
		return VK_FORMAT_R8_UNORM;
	case WW3D_FORMAT_A8L8:
	case WW3D_FORMAT_U8V8:
		return VK_FORMAT_R8G8_UNORM;
	case WW3D_FORMAT_A4L4:
	case WW3D_FORMAT_P8:
	case WW3D_FORMAT_A8P8:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case WW3D_FORMAT_L6V5U5:
	case WW3D_FORMAT_X8L8V8U8:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case WW3D_FORMAT_DXT1:
		return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
	case WW3D_FORMAT_DXT2:
	case WW3D_FORMAT_DXT3:
		return VK_FORMAT_BC2_UNORM_BLOCK;
	case WW3D_FORMAT_DXT4:
	case WW3D_FORMAT_DXT5:
		return VK_FORMAT_BC3_UNORM_BLOCK;
	default:
		break;
	}
	return VK_FORMAT_B8G8R8A8_UNORM;
}
static VkGpuBuffer VulkanUboBuffers[VulkanDevice::MAX_FRAMES_IN_FLIGHT];
static uint32_t VulkanUboDrawIndex = 0;

static VkGpuBuffer VulkanDummyVB;
static VkTexture VulkanWhiteTexture;
// Per-frame submission counters (read/reset by the W3DScene STATS line to
// answer "is the shell map actually submitting anything?").
unsigned long long VulkanFrameDraws = 0;
unsigned long long VulkanFrameIndices = 0;
// Terrain (fvf 0x242) texture-binding census: are the big world draws sampling
// a real texture or the white fallback? (black-on-AMD with real shader while
// inDiffuse/white-fallback are proven white => real textures sample black).
unsigned long long TerrainDrawsNullTex = 0;
unsigned long long TerrainDrawsValidTex = 0;
static bool VulkanSharedResourcesReady = false;

struct VulkanRtEntry {
	VkTexture *texture;
	VkTexture *depth;
	VkFramebuffer framebuffer;
};
static VulkanRtEntry VulkanRtCache[8];
static int VulkanRtCacheCount = 0;

static void VulkanReleaseRtCache()
{
	VulkanDevice &device = VulkanDevice::Instance();
	if (!device.IsInitialized())
		return;
	VkDevice dev = device.GetDevice();
	for (int i = 0; i < VulkanRtCacheCount; i++) {
		if (VulkanRtCache[i].framebuffer != VK_NULL_HANDLE)
			vkDestroyFramebuffer(dev, VulkanRtCache[i].framebuffer, nullptr);
		if (VulkanRtCache[i].depth) {
			delete VulkanRtCache[i].depth;
			VulkanRtCache[i].depth = nullptr;
		}
		VulkanRtCache[i].texture = nullptr;
		VulkanRtCache[i].framebuffer = VK_NULL_HANDLE;
	}
	VulkanRtCacheCount = 0;
}

static VkFramebuffer VulkanGetRtFramebuffer(VkTexture *texture)
{
	for (int i = 0; i < VulkanRtCacheCount; i++) {
		if (VulkanRtCache[i].texture == texture)
			return VulkanRtCache[i].framebuffer;
	}
	if (VulkanRtCacheCount >= 8 || !texture)
		return VK_NULL_HANDLE;
	VulkanDevice &device = VulkanDevice::Instance();
	VkTexture *depth = new VkTexture();
	if (!depth->CreateDepth(&device, texture->GetWidth(), texture->GetHeight())) {
		delete depth;
		return VK_NULL_HANDLE;
	}
	VkImageView attachments[] = { texture->GetView(), depth->GetView() };
	VkFramebufferCreateInfo fbInfo = {};
	fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbInfo.renderPass = device.GetRtRenderPass();
	fbInfo.attachmentCount = 2;
	fbInfo.pAttachments = attachments;
	fbInfo.width = texture->GetWidth();
	fbInfo.height = texture->GetHeight();
	fbInfo.layers = 1;
	VkFramebuffer fb = VK_NULL_HANDLE;
	if (vkCreateFramebuffer(device.GetDevice(), &fbInfo, nullptr, &fb) != VK_SUCCESS) {
		delete depth;
		return VK_NULL_HANDLE;
	}
	VulkanRtCache[VulkanRtCacheCount].texture = texture;
	VulkanRtCache[VulkanRtCacheCount].depth = depth;
	VulkanRtCache[VulkanRtCacheCount].framebuffer = fb;
	VulkanRtCacheCount++;
	return fb;
}

void DX8_Assert()
{
	WWASSERT(VulkanDevice::Instance().IsInitialized());
	DX8_THREAD_ASSERT();
}

void Log_DX8_ErrorCode(unsigned res)
{
	WWDEBUG_SAY(("DX8 Error code: %08X\n", res));
}

bool DX8Wrapper::Init(void * hwnd)
{
	WWASSERT(!IsInitted);

	/*
	** Initialize all variables!
	*/
	_Hwnd = (HWND)hwnd;
	_MainThreadID=ThreadClass::_Get_Current_Thread_ID();
	CurRenderDevice = -1;
	ResolutionWidth = DEFAULT_RESOLUTION_WIDTH;
	ResolutionHeight = DEFAULT_RESOLUTION_HEIGHT;
	Render2DClass::Set_Screen_Resolution( RectClass( 0, 0, ResolutionWidth, ResolutionHeight ) );
	BitDepth = DEFAULT_BIT_DEPTH;
	IsWindowed = false;
	DX8Wrapper_IsWindowed = false;

	for (int light=0;light<4;++light) CurrentDX8LightEnables[light]=false;

	::ZeroMemory(&old_world, sizeof(D3DMATRIX));
	::ZeroMemory(&old_view, sizeof(D3DMATRIX));
	::ZeroMemory(&old_prj, sizeof(D3DMATRIX));

	CachedProjection.Make_Identity();
	for (int t=0;t<8;++t) CachedTexTransform[t].Make_Identity();
	::ZeroMemory(&CachedMaterial, sizeof(CachedMaterial));
	CachedMaterial.Diffuse.r = CachedMaterial.Diffuse.g = CachedMaterial.Diffuse.b = CachedMaterial.Diffuse.a = 1.0f;
	::ZeroMemory(&CachedViewport, sizeof(CachedViewport));
	ViewportValid = false;
	for (int b=0;b<MAX_TEXTURE_STAGES;++b) BoundTextures[b]=NULL;

	Reset_Statistics();

	Invalidate_Cached_Render_States();

	IsInitted = true;

	/*
	** Enumerate the available devices
	*/
	Enumerate_Devices();
	return true;
}

void DX8Wrapper::Shutdown(void)
{
	Set_Render_Target((TextureClass *)NULL);
	Release_Device();

	for (int i = 0; i < MAX_TEXTURE_STAGES; i++) {
		BoundTextures[i] = NULL;
	}

	_RenderDeviceNameTable.Clear();
	_RenderDeviceShortNameTable.Clear();
	_RenderDeviceDescriptionTable.Clear();

	IsInitted = false;
}

void DX8Wrapper::Do_Onetime_Device_Dependent_Inits(void)
{
	/*
	** Set Global render states (some of which depend on caps)
	*/

	Compute_Caps(DisplayFormat, D3DFMT_D24S8);

   /*
	** Initalize any other subsystems inside of WW3D
	*/
	MissingTexture::_Init();
	TextureClass::_Init_Filters();
	TheDX8MeshRenderer.Init();
	BoxRenderObjClass::Init();
	VertexMaterialClass::Init();
	PointGroupClass::_Init();
	ShatterSystem::Init();
	TextureLoader::Init();

#ifdef WW3D_DX8

	FileClass * dazzle_ini_file = _TheFileFactory->Get_File(DAZZLE_INI_FILENAME);
	if (dazzle_ini_file) {
		INIClass dazzle_ini(*dazzle_ini_file);
		DazzleRenderObjClass::Init_From_INI(&dazzle_ini);
		_TheFileFactory->Return_File(dazzle_ini_file);
	}
#endif //WW3D_DX8

	Set_Default_Global_Render_States();
}

// inline DWORD F2DW(float f) { return *((unsigned*)&f); }
void DX8Wrapper::Set_Default_Global_Render_States(void)
{
	DX8_THREAD_ASSERT();
	const D3DCAPS8 &caps = DX8Caps::Get_Default_Caps();

	Set_DX8_Render_State(D3DRS_RANGEFOGENABLE, (caps.RasterCaps & D3DPRASTERCAPS_FOGRANGE) ? TRUE : FALSE);
	Set_DX8_Render_State(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
	Set_DX8_Render_State(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
	Set_DX8_Render_State(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
	Set_DX8_Render_State(D3DRS_COLORVERTEX, TRUE);
	Set_DX8_Render_State(D3DRS_ZBIAS,0);
	Set_DX8_Texture_Stage_State(1, D3DTSS_BUMPENVLSCALE, F2DW(1.0f));
	Set_DX8_Texture_Stage_State(1, D3DTSS_BUMPENVLOFFSET, F2DW(0.0f));
	Set_DX8_Texture_Stage_State(0, D3DTSS_BUMPENVMAT00,F2DW(1.0f));
	Set_DX8_Texture_Stage_State(0, D3DTSS_BUMPENVMAT01,F2DW(0.0f));
	Set_DX8_Texture_Stage_State(0, D3DTSS_BUMPENVMAT10,F2DW(0.0f));
	Set_DX8_Texture_Stage_State(0, D3DTSS_BUMPENVMAT11,F2DW(1.0f));
}

//MW: I added this for 'Generals'.
bool DX8Wrapper::Validate_Device(void)
{
	return IsInitted && VulkanDevice::Instance().IsInitialized();
}

void DX8Wrapper::Invalidate_Cached_Render_States(void)
{
	render_state_changed = 0;

	int a;
	for (int a =0;a<sizeof(RenderStates)/sizeof(unsigned);++a) {
		RenderStates[a]=0x12345678;
	}
	for (int a =0;a<MAX_TEXTURE_STAGES;++a) {
		for (int b=0; b<32;b++) {
			TextureStageStates[a][b]=0x12345678;
		}
		BoundTextures[a]=NULL;
	}
	ShaderClass::Invalidate();
	Release_Render_State();
}

void DX8Wrapper::Do_Onetime_Device_Dependent_Shutdowns(void)
{
	TextureLoader::Deinit();
	// PointGroupClass::Shutdown();  // Not available in Vulkan backend
	// ShatterSystem::Shutdown();  // Not available in Vulkan backend
	// VertexMaterialClass::Shutdown();  // Not available in Vulkan backend
	// BoxRenderObjClass::Shutdown();  // Not available in Vulkan backend
	// TheDX8MeshRenderer.Shutdown();  // Not available in Vulkan backend
	// TextureClass::Shutdown_Filters();  // Not available in Vulkan backend
	// MissingTexture::_Shutdown();  // Not available in Vulkan backend
}

bool DX8Wrapper::Create_Device(void)
{
	VulkanDevice &device = VulkanDevice::Instance();
	if (device.IsInitialized())
		return true;

	if (!device.Init(_Hwnd))
	{
		FILE *lf = fopen("RTS_START.LOG", "a");
		if (lf) { fprintf(lf, "VK: device init FAILED\n"); fclose(lf); }
		return false;
	}
	{
		FILE *lf = fopen("RTS_START.LOG", "a");
		if (lf) { fprintf(lf, "VK: device created\n"); fclose(lf); }
	}

	char shaderDir[260];
	::GetModuleFileName(NULL, shaderDir, sizeof(shaderDir));
	char *slash = strrchr(shaderDir, '\\');
	if (slash)
		*slash = '\0';
	strcat(shaderDir, "\\shaders");
	if (!ShaderManager.Init(&device, shaderDir)) {
		char fallback[260];
		strcpy(fallback, ".\\shaders");
		if (!ShaderManager.Init(&device, fallback))
		{
			FILE *lf = fopen("RTS_START.LOG", "a");
			if (lf) { fprintf(lf, "VK: shader init FAILED (need shaders\\*.spv next to RTS.exe)\n"); fclose(lf); }
			// leave no half-initialized device behind (IsInitialized() would
			// otherwise report true and every onetime init would be skipped)
			device.Shutdown();
			return false;
		}
	}
	if (!DescriptorAllocator.Init(&device, 8192))
	{
		device.Shutdown();
		return false;
	}

	unsigned char white[4] = { 255, 255, 255, 255 };
	if (!VulkanWhiteTexture.CreateFromData(&device, 1, 1, VK_FORMAT_B8G8R8A8_UNORM, 1, white))
	{
		device.Shutdown();
		return false;
	}

	unsigned char dummyData[64];
	memset(dummyData, 0, sizeof(dummyData));
	dummyData[12] = 255;
	dummyData[13] = 255;
	dummyData[14] = 255;
	dummyData[15] = 255;
	if (!VulkanDummyVB.Create(&device, sizeof(dummyData),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
		return false;
	VulkanDummyVB.Upload(dummyData, sizeof(dummyData));

	{
		uint32_t stride = (uint32_t)VULKAN_UBO_ALIGN;
		uint64_t devAlign = device.GetMinUboAlign();
		if (devAlign > stride)
			stride = (uint32_t)((stride + devAlign - 1) / devAlign * devAlign);
		VulkanUboStride = stride;
		FILE *lf = fopen("RTS_START.LOG", "a");
		if (lf) { fprintf(lf, "VK: ubo ring stride=%u (base=%d devAlign=%llu)\n",
			VulkanUboStride, VULKAN_UBO_ALIGN, (unsigned long long)devAlign); fclose(lf); }
	}
	for (int i = 0; i < VulkanDevice::MAX_FRAMES_IN_FLIGHT; i++) {
		if (!VulkanUboBuffers[i].Create(&device, VULKAN_UBO_RING_DRAWS * VulkanUboStride,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			return false;
		VulkanUboBuffers[i].Map();
	}
	VulkanSharedResourcesReady = true;

	::ZeroMemory(&CurrentAdapterIdentifier, sizeof(D3DADAPTER_IDENTIFIER8));
	strcpy(CurrentAdapterIdentifier.Description, "Vulkan Default Adapter");
	strcpy(CurrentAdapterIdentifier.Driver, "vulkan-1.dll");
	CurrentAdapterIdentifier.WHQLLevel = 0;

	_PresentParameters.BackBufferWidth = device.GetSwapExtent().width;
	_PresentParameters.BackBufferHeight = device.GetSwapExtent().height;
	_PresentParameters.BackBufferFormat = D3DFMT_A8R8G8B8;
	_PresentParameters.BackBufferCount = 1;
	_PresentParameters.MultiSampleType = D3DMULTISAMPLE_NONE;
	_PresentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	_PresentParameters.hDeviceWindow = _Hwnd;
	_PresentParameters.Windowed = IsWindowed ? TRUE : FALSE;
	_PresentParameters.EnableAutoDepthStencil = TRUE;
	_PresentParameters.AutoDepthStencilFormat = D3DFMT_D24S8;
	_PresentParameters.Flags = 0;
	_PresentParameters.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	_PresentParameters.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

	/*
	** Initialize all subsystems
	*/
	Do_Onetime_Device_Dependent_Inits();
	return true;
}

bool DX8Wrapper::Reset_Device(bool reload_assets)
{
	DX8_THREAD_ASSERT();
	if ((IsInitted) && (VulkanDevice::Instance().IsInitialized())) {
		{
			FILE *lf = fopen("RTS_START.LOG", "a");
			if (lf) { fprintf(lf, "VK: Reset_Device(reload=%d) - waiting for in-flight frames\n", (int)reload_assets); fclose(lf); }
		}
		// Port fix: this path destroys pipelines/textures/RT caches while up to
		// MAX_FRAMES_IN_FLIGHT command buffers may still reference them (the
		// driver then uses freed objects -> amdvlk/lavapipe AV). Wait first.
		vkDeviceWaitIdle(VulkanDevice::Instance().GetDevice());
		Set_Vertex_Buffer (NULL);
		Set_Index_Buffer (NULL, 0);
		if (m_pCleanupHook) {
			m_pCleanupHook->ReleaseResources();
		}
		DynamicVBAccessClass::_Deinit();
		DynamicIBAccessClass::_Deinit();
		DX8TextureManagerClass::Release_Textures();

		VulkanReleaseRtCache();
		ShaderManager.ClearPipelineCache();
		if (!VulkanDevice::Instance().RecreateSwapchain())
			return false;

		_PresentParameters.BackBufferWidth = VulkanDevice::Instance().GetSwapExtent().width;
		_PresentParameters.BackBufferHeight = VulkanDevice::Instance().GetSwapExtent().height;

		if (reload_assets)
		{
			DX8TextureManagerClass::Recreate_Textures();
			if (m_pCleanupHook) {
				m_pCleanupHook->ReAcquireResources();
			}
		}
		Invalidate_Cached_Render_States();
		Set_Default_Global_Render_States();
		return true;
	}
	return false;
}

void DX8Wrapper::Release_Device(void)
{
	VulkanDevice &device = VulkanDevice::Instance();
	if (device.IsInitialized()) {
		vkDeviceWaitIdle(device.GetDevice());

		for (int a=0;a<MAX_TEXTURE_STAGES;++a)
		{
			BoundTextures[a]=NULL;
		}

		/*
		** Release the current vertex and index buffers
		*/
		if (render_state.vertex_buffer) render_state.vertex_buffer->Release_Engine_Ref();
		REF_PTR_RELEASE(render_state.vertex_buffer);
		if (render_state.index_buffer) render_state.index_buffer->Release_Engine_Ref();
		REF_PTR_RELEASE(render_state.index_buffer);

		/*
		** Shutdown all subsystems
		*/
		Do_Onetime_Device_Dependent_Shutdowns();

		VulkanReleaseRtCache();
		ShaderManager.ClearPipelineCache();
		DescriptorAllocator.Shutdown();
		ShaderManager.Shutdown();
		VulkanWhiteTexture.Destroy();
		VulkanDummyVB.Destroy();
		for (int i = 0; i < VulkanDevice::MAX_FRAMES_IN_FLIGHT; i++)
			VulkanUboBuffers[i].Destroy();
		VulkanSharedResourcesReady = false;
		device.Shutdown();
	}
}

void DX8Wrapper::Enumerate_Devices()
{
	DX8_Assert();

	_RenderDeviceNameTable.Clear();
	_RenderDeviceShortNameTable.Clear();
	_RenderDeviceDescriptionTable.Clear();

	StringClass device_name = "Vulkan Default Adapter";
	_RenderDeviceNameTable.Add(device_name);
	_RenderDeviceShortNameTable.Add(device_name);

	RenderDeviceDescClass desc;
	desc.set_device_name("Vulkan Default Adapter");
	desc.set_driver_name("vulkan-1.dll");
	desc.set_driver_version("1.3");

	desc.reset_resolution_list();
	static const int modes[][2] = {
		{640, 480}, {800, 600}, {1024, 768}, {1152, 864}, {1280, 720},
		{1280, 960}, {1280, 1024}, {1366, 768}, {1440, 900}, {1600, 900},
		{1600, 1200}, {1680, 1050}, {1920, 1080}, {1920, 1200}, {2560, 1440},
		{3840, 2160}
	};
	for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i++) {
		desc.add_resolution(modes[i][0], modes[i][1], 32);
	}

	int desktopW = GetSystemMetrics(SM_CXSCREEN);
	int desktopH = GetSystemMetrics(SM_CYSCREEN);
	desc.add_resolution(desktopW, desktopH, 32);

	_RenderDeviceDescriptionTable.Add(desc);
}

bool DX8Wrapper::Set_Any_Render_Device(void)
{
	for (int dev_number = 0; dev_number < _RenderDeviceNameTable.Count(); dev_number++) {
		if (Set_Render_Device(dev_number,-1,-1,-1,1,false)) {
			return true;
		}
	}

	for (int dev_number = 0; dev_number < _RenderDeviceNameTable.Count(); dev_number++) {
		if (Set_Render_Device(dev_number,-1,-1,-1,0,false)) {
			return true;
		}
	}

	return false;
}

bool DX8Wrapper::Set_Render_Device
(
	const char * dev_name,
	int width,
	int height,
	int bits,
	int windowed,
	bool resize_window
)
{
	for ( int dev_number = 0; dev_number < _RenderDeviceNameTable.Count(); dev_number++) {
		if ( strcmp( dev_name, _RenderDeviceNameTable[dev_number]) == 0) {
			return Set_Render_Device( dev_number, width, height, bits, windowed, resize_window );
		}

		if ( strcmp( dev_name, _RenderDeviceShortNameTable[dev_number]) == 0) {
			return Set_Render_Device( dev_number, width, height, bits, windowed, resize_window );
		}
	}
	return false;
}

void DX8Wrapper::Get_Format_Name(unsigned int format, StringClass *tex_format)
{
		*tex_format="Unknown";
		switch (format) {
		case D3DFMT_A8R8G8B8: *tex_format="D3DFMT_A8R8G8B8"; break;
		case D3DFMT_R8G8B8: *tex_format="D3DFMT_R8G8B8"; break;
		case D3DFMT_A4R4G4B4: *tex_format="D3DFMT_A4R4G4B4"; break;
		case D3DFMT_A1R5G5B5: *tex_format="D3DFMT_A1R5G5B5"; break;
		case D3DFMT_R5G6B5: *tex_format="D3DFMT_R5G6B5"; break;
		case D3DFMT_L8: *tex_format="D3DFMT_L8"; break;
		case D3DFMT_A8: *tex_format="D3DFMT_A8"; break;
		case D3DFMT_P8: *tex_format="D3DFMT_P8"; break;
		case D3DFMT_X8R8G8B8: *tex_format="D3DFMT_X8R8G8B8"; break;
		case D3DFMT_X1R5G5B5: *tex_format="D3DFMT_X1R5G5B5"; break;
		case D3DFMT_R3G3B2: *tex_format="D3DFMT_R3G3B2"; break;
		case D3DFMT_A8R3G3B2: *tex_format="D3DFMT_A8R3G3B2"; break;
		case D3DFMT_X4R4G4B4: *tex_format="D3DFMT_X4R4G4B4"; break;
		case D3DFMT_A8P8: *tex_format="D3DFMT_A8P8"; break;
		case D3DFMT_A8L8: *tex_format="D3DFMT_A8L8"; break;
		case D3DFMT_A4L4: *tex_format="D3DFMT_A4L4"; break;
		case D3DFMT_V8U8: *tex_format="D3DFMT_V8U8"; break;
		case D3DFMT_L6V5U5: *tex_format="D3DFMT_L6V5U5"; break;
		case D3DFMT_X8L8V8U8: *tex_format="D3DFMT_X8L8V8U8"; break;
		case D3DFMT_Q8W8V8U8: *tex_format="D3DFMT_Q8W8V8U8"; break;
		case D3DFMT_V16U16: *tex_format="D3DFMT_V16U16"; break;
		case D3DFMT_W11V11U10: *tex_format="D3DFMT_W11V11U10"; break;
		case D3DFMT_UYVY: *tex_format="D3DFMT_UYVY"; break;
		case D3DFMT_YUY2: *tex_format="D3DFMT_YUY2"; break;
		case D3DFMT_DXT1: *tex_format="D3DFMT_DXT1"; break;
		case D3DFMT_DXT2: *tex_format="D3DFMT_DXT2"; break;
		case D3DFMT_DXT3: *tex_format="D3DFMT_DXT3"; break;
		case D3DFMT_DXT4: *tex_format="D3DFMT_DXT4"; break;
		case D3DFMT_DXT5: *tex_format="D3DFMT_DXT5"; break;
		case D3DFMT_D16_LOCKABLE: *tex_format="D3DFMT_D16_LOCKABLE"; break;
		case D3DFMT_D32: *tex_format="D3DFMT_D32"; break;
		case D3DFMT_D15S1: *tex_format="D3DFMT_D15S1"; break;
		case D3DFMT_D24S8: *tex_format="D3DFMT_D24S8"; break;
		case D3DFMT_D16: *tex_format="D3DFMT_D16"; break;
		case D3DFMT_D24X8: *tex_format="D3DFMT_D24X8"; break;
		case D3DFMT_D24X4S4: *tex_format="D3DFMT_D24X4S4"; break;
		default:	break;
		}
}

bool DX8Wrapper::Set_Render_Device(int dev, int width, int height, int bits, int windowed,
	bool resize_window, bool reset_device, bool restore_assets)
{
	if (dev < 0 || dev >= _RenderDeviceNameTable.Count())
		return false;

	if (width > 0) ResolutionWidth = width;
	if (height > 0) ResolutionHeight = height;
	if (bits > 0) BitDepth = bits;
	if (windowed >= 0) {
		IsWindowed = (windowed != 0);
		DX8Wrapper_IsWindowed = IsWindowed;
	}
	CurRenderDevice = dev;
	DisplayFormat = D3DFMT_A8R8G8B8;

	Render2DClass::Set_Screen_Resolution(RectClass(0, 0, ResolutionWidth, ResolutionHeight));

	if (resize_window && _Hwnd) {
		DWORD style = WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN;
		RECT rect;
		if (IsWindowed) {
			style |= WS_DLGFRAME | WS_CAPTION | WS_SYSMENU;
			rect.left = 0;
			rect.top = 0;
			rect.right = ResolutionWidth;
			rect.bottom = ResolutionHeight;
			AdjustWindowRect(&rect, style, FALSE);
			int x = (GetSystemMetrics(SM_CXSCREEN) - (rect.right - rect.left)) / 2;
			int y = (GetSystemMetrics(SM_CYSCREEN) - (rect.bottom - rect.top)) / 2;
			SetWindowPos(_Hwnd, HWND_TOP, x, y, rect.right - rect.left, rect.bottom - rect.top, 0);
		} else {
			SetWindowPos(_Hwnd, HWND_TOPMOST, 0, 0, ResolutionWidth, ResolutionHeight, 0);
		}
		SetWindowLong(_Hwnd, GWL_STYLE, style);
	}

	VulkanDevice &device = VulkanDevice::Instance();
	if (!device.IsInitialized()) {
		if (!Create_Device())
			return false;
	} else if (reset_device) {
		if (!Reset_Device(restore_assets))
			return false;
	}
	return true;
}

bool DX8Wrapper::Set_Next_Render_Device(void)
{
	int next = CurRenderDevice + 1;
	if (next >= _RenderDeviceNameTable.Count())
		next = 0;
	return Set_Render_Device(next, -1, -1, -1, -1, false);
}

bool DX8Wrapper::Toggle_Windowed(void)
{
	return Set_Render_Device(CurRenderDevice, -1, -1, -1, IsWindowed ? 0 : 1, true);
}

void DX8Wrapper::Set_Swap_Interval(int swap)
{
	_PresentParameters.FullScreen_PresentationInterval = swap ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
}

int DX8Wrapper::Get_Swap_Interval(void)
{
	return (_PresentParameters.FullScreen_PresentationInterval == D3DPRESENT_INTERVAL_ONE) ? 1 : 0;
}

bool DX8Wrapper::Has_Stencil(void)
{
	return VulkanDevice::Instance().HasStencil();
}

int DX8Wrapper::Get_Render_Device_Count(void)
{
	return _RenderDeviceNameTable.Count();
}

int DX8Wrapper::Get_Render_Device(void)
{
	return CurRenderDevice;
}

const RenderDeviceDescClass & DX8Wrapper::Get_Render_Device_Desc(int deviceidx)
{
	WWASSERT(deviceidx >= 0 && deviceidx < _RenderDeviceDescriptionTable.Count());
	return _RenderDeviceDescriptionTable[deviceidx];
}

const char * DX8Wrapper::Get_Render_Device_Name(int device_index)
{
	WWASSERT(device_index >= 0 && device_index < _RenderDeviceNameTable.Count());
	return _RenderDeviceNameTable[device_index];
}

bool DX8Wrapper::Set_Device_Resolution(int width,int height,int bits,int windowed, bool resize_window)
{
	return Set_Render_Device(CurRenderDevice, width, height, bits, windowed, resize_window, true, true);
}

void DX8Wrapper::Get_Device_Resolution(int & set_w,int & set_h,int & set_bits,bool & set_windowed)
{
	set_w = ResolutionWidth;
	set_h = ResolutionHeight;
	set_bits = BitDepth;
	set_windowed = IsWindowed;
}

void DX8Wrapper::Get_Render_Target_Resolution(int & set_w,int & set_h,int & set_bits,bool & set_windowed)
{
	if (CurrentRenderTarget && !UsingDefaultTarget && CurrentRenderTarget) {
		set_w = (int)CurrentRenderTarget->GetWidth();
		set_h = (int)CurrentRenderTarget->GetHeight();
	} else {
		set_w = ResolutionWidth;
		set_h = ResolutionHeight;
	}
	set_bits = BitDepth;
	set_windowed = IsWindowed;
}

bool DX8Wrapper::Registry_Save_Render_Device( const char * sub_key )
{
	int	width, height, depth;
	bool	windowed;
	Get_Device_Resolution(width, height, depth, windowed);
	return Registry_Save_Render_Device(sub_key, CurRenderDevice, ResolutionWidth, ResolutionHeight, BitDepth, IsWindowed, TextureBitDepth);
}

bool DX8Wrapper::Registry_Save_Render_Device( const char *sub_key, int device, int width, int height, int depth, bool windowed, int texture_depth)
{
	RegistryClass * registry = W3DNEW RegistryClass( sub_key );
	WWASSERT( registry );

	if ( !registry->Is_Valid() ) {
		delete registry;
		WWDEBUG_SAY(( "Error getting Registry\n" ));
		return false;
	}

	registry->Set_String( VALUE_NAME_RENDER_DEVICE_NAME,
		_RenderDeviceShortNameTable[device] );
	registry->Set_Int( VALUE_NAME_RENDER_DEVICE_WIDTH,	width );
	registry->Set_Int( VALUE_NAME_RENDER_DEVICE_HEIGHT, height );
	registry->Set_Int( VALUE_NAME_RENDER_DEVICE_DEPTH, depth );
	registry->Set_Int( VALUE_NAME_RENDER_DEVICE_WINDOWED, windowed );
	registry->Set_Int( VALUE_NAME_RENDER_DEVICE_TEXTURE_DEPTH, texture_depth );

	delete registry;
	return true;
}

bool DX8Wrapper::Registry_Load_Render_Device( const char * sub_key, bool resize_window )
{
	char	name[ 200 ];
	int	width,height,depth,windowed;

	if (	Registry_Load_Render_Device(	sub_key,
													name,
													sizeof(name),
													width,
													height,
													depth,
													windowed,
													TextureBitDepth) &&
			(*name != 0))
	{
		WWDEBUG_SAY(( "Device %s (%d X %d) %d bit windowed:%d\n", name,width,height,depth,windowed));

		if (TextureBitDepth==16 || TextureBitDepth==32) {
		} else {
			WWDEBUG_SAY(( "Invalid texture depth %d, switching to 16 bits\n", TextureBitDepth));
			TextureBitDepth=16;
		}

		if ( Set_Render_Device( name, width,height,depth,windowed, resize_window ) != true) {
			return Set_Any_Render_Device();
		}

		return true;
	}
	return false;
}

bool DX8Wrapper::Registry_Load_Render_Device( const char * sub_key, char *device, int device_len, int &width, int &height, int &depth, int &windowed, int &texture_depth)
{
	RegistryClass * registry = W3DNEW RegistryClass( sub_key );
	WWASSERT( registry );

	if ( !registry->Is_Valid() ) {
		delete registry;
		WWDEBUG_SAY(( "Error getting Registry\n" ));
		return false;
	}

	StringClass device_name;
	registry->Get_String( VALUE_NAME_RENDER_DEVICE_NAME, device_name, "" );
	strncpy( device, device_name.Peek_Buffer(), device_len );
	width = registry->Get_Int( VALUE_NAME_RENDER_DEVICE_WIDTH, 800 );
	height = registry->Get_Int( VALUE_NAME_RENDER_DEVICE_HEIGHT, 600 );
	depth = registry->Get_Int( VALUE_NAME_RENDER_DEVICE_DEPTH, 32 );
	windowed = registry->Get_Int( VALUE_NAME_RENDER_DEVICE_WINDOWED, 0 );
	texture_depth = registry->Get_Int( VALUE_NAME_RENDER_DEVICE_TEXTURE_DEPTH, 32 );

	delete registry;
	return true;
}

bool DX8Wrapper::Find_Color_And_Z_Mode(int resx,int resy,int bitdepth,D3DFORMAT * set_colorbuffer,D3DFORMAT * set_backbuffer, D3DFORMAT * set_zmode)
{
	*set_colorbuffer = D3DFMT_A8R8G8B8;
	*set_backbuffer = D3DFMT_A8R8G8B8;
	*set_zmode = D3DFMT_D24S8;
	return true;
}

bool DX8Wrapper::Find_Color_Mode(D3DFORMAT colorbuffer, int resx, int resy, UINT *mode)
{
	*mode = 0;
	return true;
}

bool DX8Wrapper::Find_Z_Mode(D3DFORMAT colorbuffer,D3DFORMAT backbuffer, D3DFORMAT *zmode)
{
	*zmode = D3DFMT_D24S8;
	return true;
}

bool DX8Wrapper::Test_Z_Mode(D3DFORMAT colorbuffer,D3DFORMAT backbuffer, D3DFORMAT zmode)
{
	return (zmode == D3DFMT_D24S8 || zmode == D3DFMT_D32 || zmode == D3DFMT_D24X8 ||
		zmode == D3DFMT_D24X4S4 || zmode == D3DFMT_D16 || zmode == D3DFMT_D15S1);
}

void DX8Wrapper::Compute_Caps(D3DFORMAT display_format,D3DFORMAT depth_stencil_format)
{
	// was an empty stub - DX8Caps::Init_Caps never ran, MaxTextureWidth/Height
	// stayed 0, and TextureLoader::Validate_Texture_Size hit an integer 0/0
	// divide during terrain texture loading.
	DX8Caps::Compute_Caps(display_format, depth_stencil_format, NULL);
}

void DX8Wrapper::Begin_Scene(void)
{
	DX8_THREAD_ASSERT();
	VulkanDevice &device = VulkanDevice::Instance();
	if (!device.BeginFrame())
		return;
	VulkanFrameOpen = true;
	VulkanUboDrawIndex = 0;
	DescriptorAllocator.ResetFrame();
	DX8WebBrowser::Update();
}

void DX8Wrapper::End_Scene(bool flip_frames)
{
	DX8_THREAD_ASSERT();
	VulkanDevice &device = VulkanDevice::Instance();
	if (VulkanFrameOpen) {
		DX8Wrapper::EndTargetPass();
		device.EndFrame(flip_frames);
		VulkanFrameOpen = false;
	}

	DX8WebBrowser::Render(0);

	if (flip_frames) {
		FrameCount++;
	}

	Set_Vertex_Buffer(NULL);
	Set_Index_Buffer(NULL,0);
	for (unsigned i=0;i<MAX_TEXTURE_STAGES;++i) Set_Texture(i,NULL);
	Set_Material(NULL);
}

void DX8Wrapper::Flip_To_Primary(void)
{
}

static bool VulkanTargetPassOpen = false;
static VkTexture *VulkanPassTarget = NULL;
static float VulkanPassClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
static float VulkanPassClearDepth = 1.0f;
static unsigned VulkanPassClearStencil = 0;

static void VulkanBeginTargetPass(VkTexture *target, const float *clearColor, float clearDepth, unsigned clearStencil)
{
	VulkanDevice &device = VulkanDevice::Instance();
	VkCommandBuffer cmd = device.GetCommandBuffer();
	if (cmd == VK_NULL_HANDLE) {
		static bool logged = false;
		if (!logged) {
			FILE *lf = fopen("RTS_START.LOG", "a");
			if (lf) { fprintf(lf, "VK: BeginTargetPass with NULL command buffer\n"); fclose(lf); }
			logged = true;
		}
		return;
	}
	if (target == NULL) {
		if (device.GetRenderPass() == VK_NULL_HANDLE || device.GetFramebuffer() == VK_NULL_HANDLE)
			return;
		{
			// Breadcrumb: first 40 pass begins - brackets where a driver AV
			// (e.g. the mission-start amdvlk crash) happened relative to passes.
			static int loggedRP = 0;
			loggedRP++;
			if (loggedRP <= 40 || (loggedRP % 100) == 0) {
				char buf[256];
				snprintf(buf, sizeof(buf),
					"VK: BeginTargetPass#%d cmd=%p rp=%p fb=%p extent=%ux%u clearDepth=%.4f clearStencil=%u clearCol=(%.2f,%.2f,%.2f,%.2f)",
					loggedRP, (void*)cmd, (void*)device.GetRenderPass(), (void*)device.GetFramebuffer(),
					(unsigned)device.GetSwapExtent().width, (unsigned)device.GetSwapExtent().height,
					(float)clearDepth, (unsigned)clearStencil,
					(float)clearColor[0], (float)clearColor[1], (float)clearColor[2], (float)clearColor[3]);
				FILE *lf = fopen("RTS_START.LOG", "a");
				if (lf) { fprintf(lf, "%s\n", buf); fclose(lf); }
			}
		}
		VkClearValue clearValues[2];
		clearValues[0].color.float32[0] = clearColor[0];
		clearValues[0].color.float32[1] = clearColor[1];
		clearValues[0].color.float32[2] = clearColor[2];
		clearValues[0].color.float32[3] = clearColor[3];
		clearValues[1].depthStencil.depth = clearDepth;
		clearValues[1].depthStencil.stencil = clearStencil;
		VkRenderPassBeginInfo rpBegin = {};
		rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rpBegin.renderPass = device.GetRenderPass();
		rpBegin.framebuffer = device.GetFramebuffer();
		rpBegin.renderArea.offset.x = 0;
		rpBegin.renderArea.offset.y = 0;
		rpBegin.renderArea.extent = device.GetSwapExtent();
		rpBegin.clearValueCount = 2;
		rpBegin.pClearValues = clearValues;
		vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
	} else {
		VkFramebuffer fb = VulkanGetRtFramebuffer(target);
		if (fb == VK_NULL_HANDLE)
			return;
		VkClearValue clearValues[2];
		clearValues[0].color.float32[0] = clearColor[0];
		clearValues[0].color.float32[1] = clearColor[1];
		clearValues[0].color.float32[2] = clearColor[2];
		clearValues[0].color.float32[3] = clearColor[3];
		clearValues[1].depthStencil.depth = clearDepth;
		clearValues[1].depthStencil.stencil = clearStencil;
		VkRenderPassBeginInfo rpBegin = {};
		rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rpBegin.renderPass = device.GetRtRenderPass();
		rpBegin.framebuffer = fb;
		rpBegin.renderArea.offset.x = 0;
		rpBegin.renderArea.offset.y = 0;
		rpBegin.renderArea.extent.width = target->GetWidth();
		rpBegin.renderArea.extent.height = target->GetHeight();
		rpBegin.clearValueCount = 2;
		rpBegin.pClearValues = clearValues;
		vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
	}
	VulkanTargetPassOpen = true;
	VulkanPassTarget = target;
}

static void VulkanEndTargetPass()
{
	if (!VulkanTargetPassOpen)
		return;
	VulkanDevice &device = VulkanDevice::Instance();
	vkCmdEndRenderPass(device.GetCommandBuffer());
	VulkanTargetPassOpen = false;
	VulkanPassTarget = NULL;
}

void DX8Wrapper::EndTargetPass()
{
	VulkanEndTargetPass();
}

void DX8Wrapper::EnsureTargetPass()
{
	VkTexture *want = DX8Wrapper::UsingDefaultTarget ? NULL : DX8Wrapper::CurrentRenderTarget;
	if (VulkanTargetPassOpen && VulkanPassTarget == want)
		return;
	VulkanEndTargetPass();
	VulkanBeginTargetPass(want, VulkanPassClearColor, VulkanPassClearDepth, VulkanPassClearStencil);
}

void DX8Wrapper::Clear(bool clear_color, bool clear_z_stencil, const Vector3 &color, float dest_alpha, float z, unsigned int stencil)
{
	DX8_THREAD_ASSERT();
	bool has_stencil = VulkanDevice::Instance().HasStencil();
	VulkanPassClearColor[0] = color.X;
	VulkanPassClearColor[1] = color.Y;
	VulkanPassClearColor[2] = color.Z;
	VulkanPassClearColor[3] = dest_alpha;
	VulkanPassClearDepth = z;
	VulkanPassClearStencil = (has_stencil && clear_z_stencil) ? stencil : 0;
	if (!clear_color && !clear_z_stencil)
		return;
	VulkanEndTargetPass();
	VkTexture *want = UsingDefaultTarget ? NULL : CurrentRenderTarget;
	VulkanBeginTargetPass(want, VulkanPassClearColor, VulkanPassClearDepth, VulkanPassClearStencil);
	(void)has_stencil;
}

void DX8Wrapper::Set_Viewport(CONST D3DVIEWPORT8* pViewport)
{
	DX8_THREAD_ASSERT();
	if (pViewport) {
		CachedViewport = *pViewport;
		ViewportValid = true;
	}
}

void DX8Wrapper::Set_Vertex_Buffer(const VertexBufferClass* vb)
{
	render_state.vba_offset=0;
	render_state.vba_count=0;
	if (render_state.vertex_buffer) {
		render_state.vertex_buffer->Release_Engine_Ref();
	}
	REF_PTR_SET(render_state.vertex_buffer,const_cast<VertexBufferClass*>(vb));
	if (vb) {
		vb->Add_Engine_Ref();
		render_state.vertex_buffer_type=vb->Type();
	}
	else {
		render_state.index_buffer_type=BUFFER_TYPE_INVALID;
	}
	render_state_changed|=VERTEX_BUFFER_CHANGED;
}

void DX8Wrapper::Set_Index_Buffer(const IndexBufferClass* ib,unsigned short index_base_offset)
{
	render_state.iba_offset=0;
	if (render_state.index_buffer) {
		render_state.index_buffer->Release_Engine_Ref();
	}
	REF_PTR_SET(render_state.index_buffer,const_cast<IndexBufferClass*>(ib));
	render_state.index_base_offset=index_base_offset;
	if (ib) {
		ib->Add_Engine_Ref();
		render_state.index_buffer_type=ib->Type();
	}
	else {
		render_state.index_buffer_type=BUFFER_TYPE_INVALID;
	}
	render_state_changed|=INDEX_BUFFER_CHANGED;
}

void DX8Wrapper::Set_Vertex_Buffer(const DynamicVBAccessClass& vba_)
{
	if (render_state.vertex_buffer) render_state.vertex_buffer->Release_Engine_Ref();

	DynamicVBAccessClass& vba=const_cast<DynamicVBAccessClass&>(vba_);
	render_state.vertex_buffer_type=vba.Get_Type();
	render_state.vba_offset=vba.VertexBufferOffset;
	render_state.vba_count=vba.Get_Vertex_Count();
	REF_PTR_SET(render_state.vertex_buffer,vba.VertexBuffer);
	render_state.vertex_buffer->Add_Engine_Ref();
	render_state_changed|=VERTEX_BUFFER_CHANGED;
	render_state_changed|=INDEX_BUFFER_CHANGED;
}

void DX8Wrapper::Set_Index_Buffer(const DynamicIBAccessClass& iba_,unsigned short index_base_offset)
{
	if (render_state.index_buffer) render_state.index_buffer->Release_Engine_Ref();

	DynamicIBAccessClass& iba=const_cast<DynamicIBAccessClass&>(iba_);
	render_state.index_base_offset=index_base_offset;
	render_state.index_buffer_type=iba.Get_Type();
	render_state.iba_offset=iba.IndexBufferOffset;
	REF_PTR_SET(render_state.index_buffer,iba.IndexBuffer);
	render_state.index_buffer->Add_Engine_Ref();
	render_state_changed|=INDEX_BUFFER_CHANGED;
}

void DX8Wrapper::Set_DX8_Transform_Cached(D3DTRANSFORMSTATETYPE transform, const Matrix4& m)
{
	switch ((int)transform) {
	case D3DTS_WORLD:
	case D3DTS_VIEW:
	case D3DTS_PROJECTION:
		if (transform == D3DTS_PROJECTION)
			CachedProjection = *(const Matrix4*)&m;
		break;
	default:
		if (transform >= D3DTS_TEXTURE0 && transform <= D3DTS_TEXTURE7)
			CachedTexTransform[transform - D3DTS_TEXTURE0] = *(const Matrix4*)&m;
		break;
	}
	DX8_RECORD_MATRIX_CHANGE();
	number_of_DX8_calls++;
}

void DX8Wrapper::Set_Light(unsigned index,const LightClass &light)
{
	D3DLIGHT8 dlight;
	Vector3 temp;
	memset(&dlight,0,sizeof(D3DLIGHT8));

	switch (light.Get_Type())
	{
	case LightClass::POINT:
		{
			dlight.Type=D3DLIGHT_POINT;
		}
		break;
	case LightClass::DIRECTIONAL:
		{
			dlight.Type=D3DLIGHT_DIRECTIONAL;
		}
		break;
	case LightClass::SPOT:
		{
			dlight.Type=D3DLIGHT_SPOT;
		}
		break;
	}

	light.Get_Diffuse(&temp);
	temp*=light.Get_Intensity();
	dlight.Diffuse.r=temp.X;
	dlight.Diffuse.g=temp.Y;
	dlight.Diffuse.b=temp.Z;
	dlight.Diffuse.a=1.0f;

	light.Get_Specular(&temp);
	temp*=light.Get_Intensity();
	dlight.Specular.r=temp.X;
	dlight.Specular.g=temp.Y;
	dlight.Specular.b=temp.Z;
	dlight.Specular.a=1.0f;

	light.Get_Ambient(&temp);
	temp*=light.Get_Intensity();
	dlight.Ambient.r=temp.X;
	dlight.Ambient.g=temp.Y;
	dlight.Ambient.b=temp.Z;
	dlight.Ambient.a=1.0f;

	temp=light.Get_Position();
	dlight.Position=*(D3DVECTOR*) &temp;

	light.Get_Spot_Direction(temp);
	dlight.Direction=*(D3DVECTOR*) &temp;

	dlight.Range=light.Get_Attenuation_Range();
	dlight.Falloff=light.Get_Spot_Exponent();
	dlight.Theta=light.Get_Spot_Angle();
	dlight.Phi=light.Get_Spot_Angle();

	double a,b;
	light.Get_Far_Attenuation_Range(a,b);
	dlight.Attenuation0=1.0f;
	if (fabs(a-b)<1e-5)
		dlight.Attenuation1=0.0f;
	else
		dlight.Attenuation1=(float) 1.0/a;
	dlight.Attenuation2=0.0f;

	Set_Light(index,&dlight);
}

bool DX8Wrapper::VulkanDrawIndexed(
	VkGpuBuffer *vb, uint32_t vbOffsetBytes, unsigned fvf, const FVFInfoClass &fvfInfo,
	VkGpuBuffer *ib, uint32_t firstIndex, uint32_t indexCount, uint32_t vertexOffset,
	unsigned topology)
{
	VulkanDevice &device = VulkanDevice::Instance();
	if (!vb || !ib || indexCount == 0)
		return false;

	DX8Wrapper::EnsureTargetPass();
	VkCommandBuffer cmd = device.GetCommandBuffer();

	VkPipelineState state;
	DX8Wrapper::VulkanBuildPipelineState(&state, fvf, fvfInfo, topology);
	VkPipeline pipeline = DX8Wrapper::ShaderManager.GetPipeline(state);
	if (pipeline == VK_NULL_HANDLE)
		return false;
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	D3DVIEWPORT8 vp;
	VkExtent2D targetExtent = device.GetSwapExtent();
	VkTexture *rt = DX8Wrapper::UsingDefaultTarget ? NULL : DX8Wrapper::CurrentRenderTarget;
	if (rt) {
		targetExtent.width = rt->GetWidth();
		targetExtent.height = rt->GetHeight();
	}
	if (DX8Wrapper::ViewportValid) {
		vp = DX8Wrapper::CachedViewport;
	} else {
		vp.X = 0;
		vp.Y = 0;
		vp.Width = targetExtent.width;
		vp.Height = targetExtent.height;
		vp.MinZ = 0.0f;
		vp.MaxZ = 1.0f;
	}
	VkViewport viewport = {};
	viewport.x = (float)vp.X;
	viewport.y = (float)(vp.Y + vp.Height);
	viewport.width = (float)vp.Width;
	viewport.height = -(float)vp.Height;
	viewport.minDepth = vp.MinZ;
	viewport.maxDepth = vp.MaxZ;
	{
		static int drawLog = 0;
		drawLog++;
		if (drawLog <= 60 || (drawLog % 2000) == 0) {
			FILE *lf = fopen("RTS_START.LOG", "a");
			if (lf) {
				fprintf(lf, "VK: Draw3D #%d fvf=0x%x vp=%dx%d@%d,%d idx=%u\n",
					drawLog, (unsigned)fvf, (int)viewport.width, (int)(-viewport.height),
					(int)viewport.x, (int)viewport.y, (unsigned)indexCount);
				fclose(lf);
			}
		}
	}
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor = {};
	scissor.offset.x = (int32_t)vp.X;
	scissor.offset.y = (int32_t)(targetExtent.height - (vp.Y + vp.Height));
	scissor.extent.width = vp.Width;
	scissor.extent.height = vp.Height;
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	VkBuffer buffers[2] = { vb->GetBuffer(), VulkanDummyVB.GetBuffer() };
	VkDeviceSize offsets[2] = { vbOffsetBytes, 0 };
	vkCmdBindVertexBuffers(cmd, 0, 2, buffers, offsets);
	vkCmdBindIndexBuffer(cmd, ib->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

	Matrix4 world = DX8Wrapper::render_state.world;
	Matrix4 view = DX8Wrapper::render_state.view;
	Matrix4 proj = DX8Wrapper::CachedProjection;
	// Port fix (transform order): WW matrices are row-vector (v*M, translation
	// in the last row) and GLSL reads the memory as M^T, so gl_Position =
	// M^T*v == v*M - the raw upload is correct. But the CHAIN must follow
	// row-vector order: v * world * view * proj. The old proj*view*world
	// applied the projection FIRST to world-space coordinates (garbage clip:
	// diag showed m22=-320 -> ndc_z=320 -> depth-rejected everywhere, and
	// negative m00/m11 flipped winding). Menus (identity world/view) were
	// unaffected because proj*view*world == world*view*proj for them.
	Matrix4 mvp = world * view * proj;
	{
		VkTexture *t0 = DX8Wrapper::BoundTextures[0] ? DX8Wrapper::BoundTextures[0]->Peek_Vulkan_Texture() : NULL;
		if (fvf == 0x242) {
			if (t0) TerrainDrawsValidTex++;
			else TerrainDrawsNullTex++;
		}
		static int mvLog = 0;
		if (fvf == 0x242 && mvLog < 16) {
			mvLog++;
			FILE *lf = fopen("RTS_START.LOG", "a");
			if (lf && mvLog == 1) {
				fprintf(lf, "VK: PROJ ");
				for (int r = 0; r < 4; r++)
					for (int c = 0; c < 4; c++)
						fprintf(lf, "%.4f ", proj[r][c]);
				fprintf(lf, "\nVK: VIEW ");
				for (int r = 0; r < 4; r++)
					for (int c = 0; c < 4; c++)
						fprintf(lf, "%.4f ", view[r][c]);
				fprintf(lf, "\nVK: MVP-FULL ");
				for (int r = 0; r < 4; r++)
					for (int c = 0; c < 4; c++)
						fprintf(lf, "%.4f ", mvp[r][c]);
				fprintf(lf, "\n");
			}
			if (lf) {
				fprintf(lf, "VK: TERRAIN-DRAW #%d mvp=[%.3f %.3f %.3f %.3f] tex0=%p %ux%u idx=%u blend=%d/%d/%d z=%d/%d nullTex=%llu validTex=%llu\n",
					mvLog, mvp[0][0], mvp[1][1], mvp[2][2], mvp[3][3],
					(void *)t0, t0 ? t0->GetWidth() : 0, t0 ? t0->GetHeight() : 0,
					(unsigned)indexCount,
					(int)state.blendEnable, (int)state.srcBlend, (int)state.dstBlend,
					(int)state.depthTest, (int)state.depthWrite,
					(unsigned long long)TerrainDrawsNullTex,
					(unsigned long long)TerrainDrawsValidTex);
				fclose(lf);
			}
		}
	}
	// (same convention fix as mvp above - pass the raw WW matrix)
	Matrix4 worldForNormals = world;
	struct PushConsts {
		float mvp[16];
		float world[16];
	} push;
	memcpy(push.mvp, &mvp[0][0], sizeof(push.mvp));
	memcpy(push.world, &worldForNormals[0][0], sizeof(push.world));
	vkCmdPushConstants(cmd, DX8Wrapper::ShaderManager.GetPipelineLayout(),
		VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);

	VkTexture *tex0 = DX8Wrapper::BoundTextures[0] ? DX8Wrapper::BoundTextures[0]->Peek_Vulkan_Texture() : NULL;
	const bool tex0Missing = (tex0 == NULL);
	VkTexture *tex1 = DX8Wrapper::BoundTextures[1] ? DX8Wrapper::BoundTextures[1]->Peek_Vulkan_Texture() : NULL;
	// AllocateTextureSet requires BOTH stages; single-stage draws (most GUI
	// widgets) used to be silently skipped -> invisible menu over black bg.
	if (!tex0) tex0 = &VulkanWhiteTexture;
	if (!tex1) tex1 = &VulkanWhiteTexture;
	if (!tex0) tex0 = &VulkanWhiteTexture;
	if (!tex1) tex1 = &VulkanWhiteTexture;
	bool filt0 = true, rep0 = true, filt1 = true, rep1 = true;
	unsigned min0 = DX8Wrapper::TextureStageStates[0][D3DTSS_MINFILTER];
	unsigned mag0 = DX8Wrapper::TextureStageStates[0][D3DTSS_MAGFILTER];
	if (min0 == D3DTEXF_POINT && mag0 == D3DTEXF_POINT) filt0 = false;
	if (DX8Wrapper::TextureStageStates[0][D3DTSS_ADDRESSU] != D3DTADDRESS_WRAP &&
		DX8Wrapper::TextureStageStates[0][D3DTSS_ADDRESSV] != D3DTADDRESS_WRAP) rep0 = false;
	unsigned min1 = DX8Wrapper::TextureStageStates[1][D3DTSS_MINFILTER];
	unsigned mag1 = DX8Wrapper::TextureStageStates[1][D3DTSS_MAGFILTER];
	if (min1 == D3DTEXF_POINT && mag1 == D3DTEXF_POINT) filt1 = false;
	if (DX8Wrapper::TextureStageStates[1][D3DTSS_ADDRESSU] != D3DTADDRESS_WRAP &&
		DX8Wrapper::TextureStageStates[1][D3DTSS_ADDRESSV] != D3DTADDRESS_WRAP) rep1 = false;
	VkDescriptorSet texSet = DX8Wrapper::DescriptorAllocator.AllocateTextureSet(
		DX8Wrapper::ShaderManager.GetTextureSetLayout(), tex0, filt0, rep0, tex1, filt1, rep1);

	VkFrameUniforms ubo;
	DX8Wrapper::VulkanFillFrameUniforms(&ubo);
	ubo.drawFlags = tex0Missing ? 1u : 0u;
	uint32_t uboOffset = DX8Wrapper::VulkanAllocUbo(&ubo);
	VkDescriptorSet uboSet = VK_NULL_HANDLE;
	if (uboOffset != UINT32_MAX) {
		uboSet = DX8Wrapper::DescriptorAllocator.AllocateFrameSet(
			DX8Wrapper::ShaderManager.GetFrameSetLayout(),
			VulkanUboBuffers[device.GetFrameIndex()].GetBuffer(), uboOffset, sizeof(ubo));
	}
	if (texSet == VK_NULL_HANDLE || uboSet == VK_NULL_HANDLE) {
		static unsigned skips = 0;
		if (skips < 30) { skips++; FILE *lf = fopen("RTS_START.LOG", "a"); if (lf) { fprintf(lf, "VulkanDrawIndexed SKIP #%u texSet=%p uboSet=%p\n", (unsigned)skips, (void*)texSet, (void*)uboSet); fclose(lf); } }
		return false;
	}
	VkDescriptorSet sets[] = { texSet, uboSet };
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
		DX8Wrapper::ShaderManager.GetPipelineLayout(), 0, 2, sets, 0, NULL);

	vkCmdDrawIndexed(cmd, indexCount, 1, firstIndex, (int32_t)vertexOffset, 0);
	number_of_DX8_calls++;
	VulkanFrameDraws++;
	VulkanFrameIndices += indexCount;
	return true;
}

void DX8Wrapper::Draw_Sorting_IB_VB(
	unsigned primitive_type,
	unsigned short start_index,
	unsigned short polygon_count,
	unsigned short min_vertex_index,
	unsigned short vertex_count)
{
	WWASSERT(render_state.vertex_buffer_type==BUFFER_TYPE_SORTING || render_state.vertex_buffer_type==BUFFER_TYPE_DYNAMIC_SORTING);
	WWASSERT(render_state.index_buffer_type==BUFFER_TYPE_SORTING || render_state.index_buffer_type==BUFFER_TYPE_DYNAMIC_SORTING);

	DynamicVBAccessClass dyn_vb_access(BUFFER_TYPE_DYNAMIC_DX8,dynamic_fvf_type,vertex_count);
	{
		DynamicVBAccessClass::WriteLockClass lock(&dyn_vb_access);
		VertexFormatXYZNDUV2* src = static_cast<SortingVertexBufferClass*>(render_state.vertex_buffer)->VertexBuffer;
		VertexFormatXYZNDUV2* dest= lock.Get_Formatted_Vertex_Array();
		src += render_state.vba_offset + render_state.index_base_offset + min_vertex_index;
		unsigned  size = dyn_vb_access.FVF_Info().Get_FVF_Size()*vertex_count/sizeof(unsigned);
		unsigned *dest_u =(unsigned*) dest;
		unsigned *src_u = (unsigned*) src;

		for (unsigned i=0;i<size;++i) {
			*dest_u++=*src_u++;
		}
	}

	unsigned index_count=0;
	switch (primitive_type) {
	case D3DPT_TRIANGLELIST: index_count=polygon_count*3; break;
	case D3DPT_TRIANGLESTRIP: index_count=polygon_count+2; break;
	case D3DPT_TRIANGLEFAN: index_count=polygon_count+2; break;
	default: WWASSERT(0); break;
	}

	DynamicIBAccessClass dyn_ib_access(BUFFER_TYPE_DYNAMIC_DX8,index_count);
	{
		DynamicIBAccessClass::WriteLockClass lock(&dyn_ib_access);
		unsigned short* dest=lock.Get_Index_Array();
		unsigned short* src=NULL;
		src=static_cast<SortingIndexBufferClass*>(render_state.index_buffer)->index_buffer;
		src+=render_state.iba_offset+start_index;

		for (unsigned short i=0;i<index_count;++i) {
			unsigned short index=*src++;
			index-=min_vertex_index;
			WWASSERT(index<vertex_count);
			*dest++=index;
		}
	}

	VkGpuBuffer *vb = static_cast<DX8VertexBufferClass*>(dyn_vb_access.VertexBuffer)->Get_Vulkan_Vertex_Buffer();
	VkGpuBuffer *ib = static_cast<DX8IndexBufferClass*>(dyn_ib_access.IndexBuffer)->Get_Vulkan_Index_Buffer();
	const FVFInfoClass &fvfInfo = dyn_vb_access.FVF_Info();
	VulkanDrawIndexed(vb, 0, fvfInfo.Get_FVF(), fvfInfo,
		ib, dyn_ib_access.IndexBufferOffset, index_count, dyn_vb_access.VertexBufferOffset,
		D3DPT_TRIANGLELIST);

	DX8_RECORD_RENDER(polygon_count,vertex_count,render_state.shader);
}

void DX8Wrapper::Draw(
	unsigned primitive_type,
	unsigned short start_index,
	unsigned short polygon_count,
	unsigned short min_vertex_index,
	unsigned short vertex_count)
{
	DX8_THREAD_ASSERT();
	SNAPSHOT_SAY(("DX8 - draw\n"));

	Apply_Render_State_Changes();

	if (!_Is_Triangle_Draw_Enabled()) return;

	SNAPSHOT_SAY(("DX8 - draw %s polygons (%d vertices)\n",polygon_count,vertex_count));

	if (vertex_count<3) {
		min_vertex_index=0;
		switch (render_state.vertex_buffer_type) {
		case BUFFER_TYPE_DX8:
		case BUFFER_TYPE_SORTING:
			vertex_count=render_state.vertex_buffer->Get_Vertex_Count()-render_state.index_base_offset-render_state.vba_offset-min_vertex_index;
			break;
		case BUFFER_TYPE_DYNAMIC_DX8:
		case BUFFER_TYPE_DYNAMIC_SORTING:
			vertex_count=render_state.vba_count;
			break;
		}
	}

	switch (render_state.vertex_buffer_type) {
	case BUFFER_TYPE_DX8:
	case BUFFER_TYPE_DYNAMIC_DX8:
		switch (render_state.index_buffer_type) {
		case BUFFER_TYPE_DX8:
		case BUFFER_TYPE_DYNAMIC_DX8:
			{
				unsigned index_count=0;
				switch (primitive_type) {
				case D3DPT_TRIANGLELIST: index_count=polygon_count*3; break;
				case D3DPT_TRIANGLESTRIP: index_count=polygon_count+2; break;
				case D3DPT_TRIANGLEFAN: index_count=polygon_count+2; break;
				default: WWASSERT(0); break;
				}
				DX8_RECORD_RENDER(polygon_count,vertex_count,render_state.shader);
				DX8VertexBufferClass *dxvb = static_cast<DX8VertexBufferClass*>(render_state.vertex_buffer);
				DX8IndexBufferClass *dxib = static_cast<DX8IndexBufferClass*>(render_state.index_buffer);
				const FVFInfoClass &fvfInfo = render_state.vertex_buffer->FVF_Info();
				VulkanDrawIndexed(dxvb->Get_Vulkan_Vertex_Buffer(), 0,
					fvfInfo.Get_FVF(), fvfInfo,
					dxib->Get_Vulkan_Index_Buffer(),
					(unsigned)start_index + render_state.iba_offset, index_count,
					(unsigned)render_state.index_base_offset + render_state.vba_offset,
					primitive_type);
			}
			break;
		case BUFFER_TYPE_SORTING:
		case BUFFER_TYPE_DYNAMIC_SORTING:
			WWASSERT_PRINT(0,"VB and IB must of same type (sorting or dx8)");
			break;
		case BUFFER_TYPE_INVALID:
			WWASSERT(0);
			break;
		}
		break;
	case BUFFER_TYPE_SORTING:
	case BUFFER_TYPE_DYNAMIC_SORTING:
		switch (render_state.index_buffer_type) {
		case BUFFER_TYPE_DX8:
		case BUFFER_TYPE_DYNAMIC_DX8:
			WWASSERT_PRINT(0,"VB and IB must of same type (sorting or dx8)");
			break;
		case BUFFER_TYPE_SORTING:
		case BUFFER_TYPE_DYNAMIC_SORTING:
			Draw_Sorting_IB_VB(primitive_type,start_index,polygon_count,min_vertex_index,vertex_count);
			break;
		case BUFFER_TYPE_INVALID:
			WWASSERT(0);
			break;
		}
		break;
	case BUFFER_TYPE_INVALID:
		WWASSERT(0);
		break;
	}
}

void DX8Wrapper::Draw_Triangles(
	unsigned buffer_type,
	unsigned short start_index,
	unsigned short polygon_count,
	unsigned short min_vertex_index,
	unsigned short vertex_count)
{
	if (buffer_type==BUFFER_TYPE_SORTING || buffer_type==BUFFER_TYPE_DYNAMIC_SORTING) {
		SortingRendererClass::Insert_Triangles(start_index,polygon_count,min_vertex_index,vertex_count);
	}
	else {
		Draw(D3DPT_TRIANGLELIST,start_index,polygon_count,min_vertex_index,vertex_count);
	}
}

void DX8Wrapper::Draw_Triangles(
	unsigned short start_index,
	unsigned short polygon_count,
	unsigned short min_vertex_index,
	unsigned short vertex_count)
{
	Draw(D3DPT_TRIANGLELIST,start_index,polygon_count,min_vertex_index,vertex_count);
}

void DX8Wrapper::Draw_Strip(
	unsigned short start_index,
	unsigned short index_count,
	unsigned short min_vertex_index,
	unsigned short vertex_count)
{
	Draw(D3DPT_TRIANGLESTRIP,start_index,index_count,min_vertex_index,vertex_count);
}

void DX8Wrapper::Apply_Render_State_Changes()
{
	if (!render_state_changed) return;
	if (render_state_changed&SHADER_CHANGED) {
		SNAPSHOT_SAY(("DX8 - apply shader\n"));
		render_state.shader.Apply();
	}

	unsigned mask=TEXTURE0_CHANGED;
	for (unsigned i=0;i<MAX_TEXTURE_STAGES;++i,mask<<=1) {
		if (render_state_changed&mask) {
			SNAPSHOT_SAY(("DX8 - apply texture %d\n",i));
			if (render_state.Textures[i]) render_state.Textures[i]->Apply(i);
			else TextureClass::Apply_Null(i);
		}
	}

	if (render_state_changed&MATERIAL_CHANGED) {
		SNAPSHOT_SAY(("DX8 - apply material\n"));
		VertexMaterialClass* material=const_cast<VertexMaterialClass*>(render_state.material);
		if (material) {
			material->Apply();
		}
		else VertexMaterialClass::Apply_Null();
	}

	if (render_state_changed&LIGHTS_CHANGED)
	{
		unsigned lmask=LIGHT0_CHANGED;
		for (unsigned index=0;index<4;++index,lmask<<=1) {
			if (render_state_changed&lmask) {
				SNAPSHOT_SAY(("DX8 - apply light %d\n",index));
				if (render_state.LightEnable[index]) {
					Set_DX8_Light(index,&render_state.Lights[index]);
				}
				else {
					Set_DX8_Light(index,NULL);

				}
			}
		}
	}

	if (render_state_changed&WORLD_CHANGED) {
		SNAPSHOT_SAY(("DX8 - apply world matrix\n"));
		_Set_DX8_Transform(D3DTS_WORLD,render_state.world);
	}
	if (render_state_changed&VIEW_CHANGED) {
		SNAPSHOT_SAY(("DX8 - apply view matrix\n"));
		_Set_DX8_Transform(D3DTS_VIEW,render_state.view);
	}
	if (render_state_changed&VERTEX_BUFFER_CHANGED) {
		SNAPSHOT_SAY(("DX8 - apply vb change\n"));
		if (render_state.vertex_buffer) {
			switch (render_state.vertex_buffer_type) {
			case BUFFER_TYPE_DX8:
			case BUFFER_TYPE_DYNAMIC_DX8:
				DX8_RECORD_VERTEX_BUFFER_CHANGE();
				break;
			case BUFFER_TYPE_SORTING:
			case BUFFER_TYPE_DYNAMIC_SORTING:
				break;
			default:
				WWASSERT(0);
			}
		}
	}
	if (render_state_changed&INDEX_BUFFER_CHANGED) {
		SNAPSHOT_SAY(("DX8 - apply ib change\n"));
		if (render_state.index_buffer) {
			switch (render_state.index_buffer_type) {
			case BUFFER_TYPE_DX8:
			case BUFFER_TYPE_DYNAMIC_DX8:
				DX8_RECORD_INDEX_BUFFER_CHANGE();
				break;
			case BUFFER_TYPE_SORTING:
			case BUFFER_TYPE_DYNAMIC_SORTING:
				break;
			default:
				WWASSERT(0);
			}
		}
	}

	render_state_changed&=((unsigned)WORLD_IDENTITY|(unsigned)VIEW_IDENTITY);
}

void DX8Wrapper::Reset_Statistics()
{
	matrix_changes	= 0;
	material_changes = 0;
	vertex_buffer_changes = 0;
	index_buffer_changes = 0;
	light_changes = 0;
	texture_changes = 0;
	render_state_changes =0;
	texture_stage_state_changes =0;

	number_of_DX8_calls = 0;
	last_frame_matrix_changes = 0;
	last_frame_material_changes = 0;
	last_frame_vertex_buffer_changes = 0;
	last_frame_index_buffer_changes = 0;
	last_frame_light_changes = 0;
	last_frame_texture_changes = 0;
	last_frame_render_state_changes = 0;
	last_frame_texture_stage_state_changes = 0;
	last_frame_number_of_DX8_calls = 0;
}

void DX8Wrapper::Begin_Statistics()
{
	matrix_changes=0;
	material_changes=0;
	vertex_buffer_changes=0;
	index_buffer_changes=0;
	light_changes=0;
	texture_changes = 0;
	render_state_changes =0;
	texture_stage_state_changes =0;
	number_of_DX8_calls=0;
}

void DX8Wrapper::End_Statistics()
{
	last_frame_matrix_changes=matrix_changes;
	last_frame_material_changes=material_changes;
	last_frame_vertex_buffer_changes=vertex_buffer_changes;
	last_frame_index_buffer_changes=index_buffer_changes;
	last_frame_light_changes=light_changes;
	last_frame_texture_changes = texture_changes;
	last_frame_render_state_changes = render_state_changes;
	last_frame_texture_stage_state_changes = texture_stage_state_changes;
	last_frame_number_of_DX8_calls=number_of_DX8_calls;
}

unsigned DX8Wrapper::Get_Last_Frame_Matrix_Changes()			{ return last_frame_matrix_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Material_Changes()		{ return last_frame_material_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Vertex_Buffer_Changes()	{ return last_frame_vertex_buffer_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Index_Buffer_Changes()	{ return last_frame_index_buffer_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Light_Changes()			{ return last_frame_light_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Texture_Changes()			{ return last_frame_texture_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Render_State_Changes()	{ return last_frame_render_state_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Texture_Stage_State_Changes()	{ return last_frame_texture_stage_state_changes; }
unsigned DX8Wrapper::Get_Last_Frame_DX8_Calls()					{ return last_frame_number_of_DX8_calls; }
unsigned long DX8Wrapper::Get_FrameCount(void) {return FrameCount;}

VkTexture * DX8Wrapper::_Create_Vulkan_Texture(
	unsigned int width,
	unsigned int height,
	WW3DFormat format,
	TextureClass::MipCountType mip_level_count,
	D3DPOOL pool,
	bool rendertarget)
{
	// Sampled initiator log: names WHO creates textures repeatedly (the churn
	// that may leave in-flight descriptors pointing at destroyed images).
	{
		static unsigned n = 0;
		n++;
		if ((n % 64u) == 0) {
			FILE *lf = fopen("RTS_START.LOG", "a");
			if (lf) {
				fprintf(lf, "VK: tex create init %ux%u fmt=%d from=%p n=%u\n",
					width, height, (int)format, __builtin_return_address(0), n);
				fclose(lf);
			}
		}
	}
	VulkanDevice &device = VulkanDevice::Instance();
	VkFormat vkFormat = VulkanTranslateWW3DFormat(format);
	unsigned mipLevels = 1;
	if (mip_level_count == TextureClass::MIP_LEVELS_ALL)
		mipLevels = 0;
	else if (mip_level_count > 1)
		mipLevels = (unsigned)mip_level_count;
	if (mipLevels == 0) {
		mipLevels = 1;
		unsigned w = width, h = height;
		while (w > 1 || h > 1) {
			if (w > 1) w /= 2;
			if (h > 1) h /= 2;
			mipLevels++;
		}
	}
	VkTexture *texture = new VkTexture();
	if (!texture->Create(&device, width, height, vkFormat, mipLevels, rendertarget)) {
		delete texture;
		return NULL;
	}
	return texture;
}

VkTexture * DX8Wrapper::_Create_Vulkan_Texture(const char *filename, TextureClass::MipCountType mip_level_count)
{
	return NULL;
}

VkTexture * DX8Wrapper::_Create_Vulkan_Texture(VkTexture *surface, TextureClass::MipCountType mip_level_count)
{
	return surface;
}

VkTexture * DX8Wrapper::_Create_Vulkan_Image(unsigned int width, unsigned int height, WW3DFormat format)
{
	return _Create_Vulkan_Texture(width, height, format, TextureClass::MIP_LEVELS_1, D3DPOOL_MANAGED, false);
}

VkTexture * DX8Wrapper::_Create_Vulkan_Image(const char *filename_)
{
	return _Create_Vulkan_Texture(filename_, TextureClass::MIP_LEVELS_1);
}

bool DX8Wrapper::_Read_Front_Buffer(unsigned char *dest, unsigned *width, unsigned *height)
{
	VulkanDevice &device = VulkanDevice::Instance();
	if (!dest || !width || !height)
		return false;
	VulkanEndTargetPass();
	VkExtent2D extent = device.GetSwapExtent();
	*width = extent.width;
	*height = extent.height;

	VkImage srcImage = device.GetSwapImages()[device.GetImageIndex()];
	VkBuffer staging;
	VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
	VkDeviceSize imageSize = (VkDeviceSize)extent.width * extent.height * 4;
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = imageSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkDevice dev = device.GetDevice();
	if (vkCreateBuffer(dev, &bufferInfo, nullptr, &staging) != VK_SUCCESS)
		return false;
	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(dev, staging, &memReq);
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = device.FindMemoryType(memReq.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (allocInfo.memoryTypeIndex == UINT32_MAX) {
		vkDestroyBuffer(dev, staging, nullptr);
		return false;
	}
	if (vkAllocateMemory(dev, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
		vkDestroyBuffer(dev, staging, nullptr);
		return false;
	}
	vkBindBufferMemory(dev, staging, stagingMemory, 0);

	VkCommandBuffer cmd = device.BeginOneTimeCommands();
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = srcImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier);
	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent.width = extent.width;
	region.imageExtent.height = extent.height;
	region.imageExtent.depth = 1;
	vkCmdCopyImageToBuffer(cmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		staging, 1, &region);
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier);
	device.EndOneTimeCommands(cmd);

	void *mapped = nullptr;
	vkMapMemory(dev, stagingMemory, 0, imageSize, 0, &mapped);
	memcpy(dest, mapped, (size_t)imageSize);
	vkUnmapMemory(dev, stagingMemory);
	vkDestroyBuffer(dev, staging, nullptr);
	vkFreeMemory(dev, stagingMemory, nullptr);
	return true;
}

void DX8Wrapper::_Update_Texture(TextureClass *system, TextureClass *video)
{
}

void DX8Wrapper::_Copy_Vulkan_Rects(
	VkTexture* pSourceTexture,
	CONST RECT* pSourceRectsArray,
	UINT cRects,
	VkTexture* pDestinationTexture,
	CONST POINT* pDestPointsArray)
{
	if (!pSourceTexture || !pDestinationTexture)
		return;
	VulkanDevice &device = VulkanDevice::Instance();
	VkCommandBuffer cmd = device.BeginOneTimeCommands();
	if (cRects == 0 || !pSourceRectsArray) {
		VkImageCopy copy = {};
		copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy.srcSubresource.layerCount = 1;
		copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy.dstSubresource.layerCount = 1;
		copy.extent.width = pSourceTexture->GetWidth();
		copy.extent.height = pSourceTexture->GetHeight();
		copy.extent.depth = 1;
		if (copy.extent.width > pDestinationTexture->GetWidth())
			copy.extent.width = pDestinationTexture->GetWidth();
		if (copy.extent.height > pDestinationTexture->GetHeight())
			copy.extent.height = pDestinationTexture->GetHeight();
		vkCmdCopyImage(cmd, pSourceTexture->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			pDestinationTexture->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
	} else {
		for (UINT i = 0; i < cRects; i++) {
			VkImageCopy copy = {};
			copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copy.srcSubresource.layerCount = 1;
			copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copy.dstSubresource.layerCount = 1;
			copy.srcOffset.x = pSourceRectsArray[i].left;
			copy.srcOffset.y = pSourceRectsArray[i].top;
			if (pDestPointsArray) {
				copy.dstOffset.x = pDestPointsArray[i].x;
				copy.dstOffset.y = pDestPointsArray[i].y;
			}
			copy.extent.width = (uint32_t)(pSourceRectsArray[i].right - pSourceRectsArray[i].left);
			copy.extent.height = (uint32_t)(pSourceRectsArray[i].bottom - pSourceRectsArray[i].top);
			copy.extent.depth = 1;
			vkCmdCopyImage(cmd, pSourceTexture->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				pDestinationTexture->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
		}
	}
	device.EndOneTimeCommands(cmd);
}

TextureClass * DX8Wrapper::Create_Render_Target(int width, int height, bool alpha)
{
	VkTexture *image = _Create_Vulkan_Texture((unsigned)width, (unsigned)height,
		WW3D_FORMAT_A8R8G8B8, TextureClass::MIP_LEVELS_1, D3DPOOL_DEFAULT, true);
	if (!image)
		return NULL;
	TextureClass *tex = W3DNEW TextureClass(image);
	return tex;
}

void DX8Wrapper::Set_Render_Target(TextureClass * texture)
{
	VkTexture *rt = texture ? texture->Peek_Vulkan_Texture() : NULL;
	VkTexture *want = rt;
	VkTexture *cur = UsingDefaultTarget ? NULL : CurrentRenderTarget;
	if (want == cur)
		return;
	VulkanEndTargetPass();
	CurrentRenderTarget = rt;
	UsingDefaultTarget = (rt == NULL);
}

void DX8Wrapper::Set_Gamma(float gamma,float bright,float contrast,bool calibrate,bool uselimit)
{
	GammaValue = gamma;
	BrightnessValue = bright;
	ContrastValue = contrast;
}

void DX8Wrapper::Set_Polygon_Mode(int mode)
{
	PolygonModeValue = mode;
}

WW3DFormat	DX8Wrapper::getBackBufferFormat( void )
{
	VulkanDevice &device = VulkanDevice::Instance();
	switch (device.GetSwapFormat()) {
	case VK_FORMAT_B8G8R8A8_UNORM:
		return WW3D_FORMAT_A8R8G8B8;
	case VK_FORMAT_R8G8B8A8_UNORM:
		return WW3D_FORMAT_A8R8G8B8;
	case VK_FORMAT_R5G6B5_UNORM_PACK16:
		return WW3D_FORMAT_R5G6B5;
	default:
		break;
	}
	return WW3D_FORMAT_UNKNOWN;
}

void DX8Wrapper::Set_Light_Environment(LightEnvironmentClass* light_env)
{
	if (light_env) {

		int light_count = light_env->Get_Light_Count();
		unsigned int color=Convert_Color(light_env->Get_Equivalent_Ambient(),0.0f);
		if (RenderStates[D3DRS_AMBIENT]!=color)
		{
			Set_DX8_Render_State(D3DRS_AMBIENT,color);
#if 1
			render_state_changed|=MATERIAL_CHANGED;
#endif
		}

		D3DLIGHT8 light;
		for (int l=0;l<light_count;++l) {
			::ZeroMemory(&light, sizeof(D3DLIGHT8));
			light.Type=D3DLIGHT_DIRECTIONAL;
			(Vector3&)light.Diffuse=light_env->Get_Light_Diffuse(l);
			Vector3 dir=-light_env->Get_Light_Direction(l);
			light.Direction=(const D3DVECTOR&)(dir);
			if (light_env->isPointLight(l)) {
				light.Type = D3DLIGHT_POINT;
				(Vector3&)light.Diffuse=light_env->getPointDiffuse(l);
				(Vector3&)light.Ambient=light_env->getPointAmbient(l);
				light.Position = (const D3DVECTOR&)light_env->getPointCenter(l);
				light.Range = light_env->getPointOrad(l);
				double a,b;
				b = light_env->getPointOrad(l);
				a = light_env->getPointIrad(l);
				light.Attenuation0=0.01f;
				if (fabs(a-b)<1e-5)
					light.Attenuation1=0.0f;
				else
					light.Attenuation1=(float) 0.1/a;
				light.Attenuation2=8.0f/(b*b);
			}
			Set_Light(l,&light);
		}

		for (int l=0;l<4;++l) {
			Set_Light(l,NULL);
		}
	}
}

static void VulkanDecodeColor(unsigned argb, float *out)
{
	out[0] = ((argb >> 16) & 0xff) / 255.0f;
	out[1] = ((argb >> 8) & 0xff) / 255.0f;
	out[2] = (argb & 0xff) / 255.0f;
	out[3] = ((argb >> 24) & 0xff) / 255.0f;
}

static int VulkanMapTexOp(unsigned colorOp, unsigned arg1)
{
	switch (colorOp) {
	case D3DTOP_DISABLE:
		return 0;
	case D3DTOP_SELECTARG1:
		if (arg1 == D3DTA_TEXTURE)
			return 1;
		if (arg1 == D3DTA_TFACTOR)
			return 4;
		return 0;
	case D3DTOP_SELECTARG2:
		return 4;
	case D3DTOP_MODULATE:
	case D3DTOP_MODULATE2X:
	case D3DTOP_MODULATE4X:
	case D3DTOP_DOTPRODUCT3:
	case D3DTOP_BUMPENVMAP:
	case D3DTOP_BUMPENVMAPLUMINANCE:
		return 2;
	case D3DTOP_ADD:
	case D3DTOP_SUBTRACT:
	case D3DTOP_ADDSMOOTH:
		return 3;
	default:
		break;
	}
	return 2;
}

void DX8Wrapper::VulkanFillFrameUniforms(VkFrameUniforms *ubo)
{
	memset(ubo, 0, sizeof(*ubo));
	{
		static int uboLog = 0;
		uboLog++;
		if (uboLog <= 12 || (uboLog % 600) == 0) {
			FILE *lf = fopen("RTS_START.LOG", "a");
			if (lf) {
				unsigned amb = DX8Wrapper::RenderStates[D3DRS_AMBIENT];
				Vector3 md(1,1,1);
				if (DX8Wrapper::render_state.material)
					((VertexMaterialClass *)DX8Wrapper::render_state.material)->Get_Diffuse(&md);
				bool interesting = (md.X != 1.0f || md.Y != 1.0f || md.Z != 1.0f) || (uboLog % 500) == 0;
				static unsigned matLog = 0;
				if (interesting && matLog < 20) {
					matLog++;
					fprintf(lf, "VK: FillUBO #%d light0=%d fogEn=%d matDiffuse=(%.3f,%.3f,%.3f)%s\n",
						uboLog, (int)DX8Wrapper::render_state.LightEnable[0],
						(int)DX8Wrapper::RenderStates[D3DRS_FOGENABLE],
						(float)md.X, (float)md.Y, (float)md.Z,
						(md.X != 1.0f || md.Y != 1.0f || md.Z != 1.0f) ? " NONWHITE" : "");
				}
				fclose(lf);
			}
		}
	}
	unsigned ambient = DX8Wrapper::RenderStates[D3DRS_AMBIENT];
	if (ambient == 0x12345678)
		ambient = 0x00ffffff;
	VulkanDecodeColor(ambient, ubo->ambientColor);
	ubo->ambientColor[3] = 1.0f;

	unsigned fogColor = DX8Wrapper::RenderStates[D3DRS_FOGCOLOR];
	if (fogColor == 0x12345678)
		fogColor = 0;
	VulkanDecodeColor(fogColor, ubo->fogColor);
	ubo->fogColor[3] = 1.0f;

	// Port fix: when the game never set fog (states still the 0x12345678
	// sentinel), the bit-cast float is a denormal ~0 -> fogEnd becomes ~1 ->
	// factor=clamp((viewDist-0)/1) = 1 for EVERY fragment -> fogColor(black)
	// over the whole world = pitch-black background. Sentinel now = fog OFF.
	if (DX8Wrapper::RenderStates[D3DRS_FOGSTART] == 0x12345678 ||
		DX8Wrapper::RenderStates[D3DRS_FOGEND] == 0x12345678) {
		ubo->fogParams[0] = 1e30f;
		ubo->fogParams[1] = 1e30f + 1.0f;
	} else {
		ubo->fogParams[0] = *(const float *)&DX8Wrapper::RenderStates[D3DRS_FOGSTART];
		ubo->fogParams[1] = *(const float *)&DX8Wrapper::RenderStates[D3DRS_FOGEND];
		if (ubo->fogParams[1] <= ubo->fogParams[0])
			ubo->fogParams[1] = ubo->fogParams[0] + 1.0f;
	}

	const VertexMaterialClass *material = DX8Wrapper::render_state.material;
	Vector3 diffuse(1.0f, 1.0f, 1.0f);
	Vector3 emissive(0.0f, 0.0f, 0.0f);
	float opacity = 1.0f;
	if (material) {
		((VertexMaterialClass *)material)->Get_Diffuse(&diffuse);
		((VertexMaterialClass *)material)->Get_Emissive(&emissive);
		opacity = ((VertexMaterialClass *)material)->Get_Opacity();
	}
	ubo->materialDiffuse[0] = diffuse.X;
	ubo->materialDiffuse[1] = diffuse.Y;
	ubo->materialDiffuse[2] = diffuse.Z;
	ubo->materialDiffuse[3] = opacity;

	unsigned alphaRef = DX8Wrapper::RenderStates[D3DRS_ALPHAREF];
	if (alphaRef == 0x12345678)
		alphaRef = 0x60;
	ubo->miscParams[0] = (alphaRef & 0xff) / 255.0f;

	unsigned tfactor = DX8Wrapper::RenderStates[D3DRS_TEXTUREFACTOR];
	VulkanDecodeColor(tfactor, ubo->tfactorColor);

	for (int i = 0; i < 4; i++) {
		if (DX8Wrapper::render_state.LightEnable[i]) {
			const D3DLIGHT8 &light = DX8Wrapper::render_state.Lights[i];
			float len = (float)sqrt(light.Direction.x * light.Direction.x +
				light.Direction.y * light.Direction.y + light.Direction.z * light.Direction.z);
			if (len > 1e-6f) {
				ubo->lightDirs[i][0] = light.Direction.x / len;
				ubo->lightDirs[i][1] = light.Direction.y / len;
				ubo->lightDirs[i][2] = light.Direction.z / len;
			}
			ubo->lightDirs[i][3] = 1.0f;
			ubo->lightColors[i][0] = light.Diffuse.r;
			ubo->lightColors[i][1] = light.Diffuse.g;
			ubo->lightColors[i][2] = light.Diffuse.b;
			ubo->lightColors[i][3] = 1.0f;
		} else {
			ubo->lightDirs[i][3] = 0.0f;
		}
	}
}

uint32_t DX8Wrapper::VulkanAllocUbo(VkFrameUniforms *ubo)
{
	VulkanDevice &device = VulkanDevice::Instance();
	uint32_t frame = device.GetFrameIndex();
	if (VulkanUboDrawIndex >= VULKAN_UBO_RING_DRAWS)
		return UINT32_MAX;
	uint32_t offset = VulkanUboDrawIndex * VulkanUboStride;
	VulkanUboDrawIndex++;
	void *mapped = VulkanUboBuffers[frame].Map();
	if (!mapped)
		return UINT32_MAX;
	memcpy((unsigned char *)mapped + offset, ubo, sizeof(*ubo));
	{
		static unsigned offLog = 0;
		if (offLog < 8) {
			offLog++;
			FILE *lf = fopen("RTS_START.LOG", "a");
			if (lf) { fprintf(lf, "VK: ubo slot=%u offset=%u stride=%u\n",
				(unsigned)(VulkanUboDrawIndex - 1), offset, VulkanUboStride); fclose(lf); }
		}
	}
	return offset;
}

void DX8Wrapper::VulkanBuildPipelineState(VkPipelineState *state, unsigned fvf, const FVFInfoClass &fvfInfo,
	unsigned topology)
{
	memset(state, 0, sizeof(*state));
	unsigned stage0op = DX8Wrapper::TextureStageStates[0][D3DTSS_COLOROP];
	unsigned stage0arg1 = DX8Wrapper::TextureStageStates[0][D3DTSS_COLORARG1];
	unsigned stage1op = DX8Wrapper::TextureStageStates[1][D3DTSS_COLOROP];
	unsigned stage1arg1 = DX8Wrapper::TextureStageStates[1][D3DTSS_COLORARG1];
	if (stage0op == 0x12345678) { stage0op = D3DTOP_DISABLE; stage0arg1 = D3DTA_CURRENT; }
	if (stage1op == 0x12345678) { stage1op = D3DTOP_DISABLE; stage1arg1 = D3DTA_CURRENT; }
	state->tex0Op = VulkanMapTexOp(stage0op, stage0arg1);
	state->tex1Op = VulkanMapTexOp(stage1op, stage1arg1);
	state->alphaTest = (DX8Wrapper::RenderStates[D3DRS_ALPHATESTENABLE] != 0) ? 1 : 0;
	state->fog = (DX8Wrapper::RenderStates[D3DRS_FOGENABLE] != 0) ? 1 : 0;
	state->lighting = (DX8Wrapper::RenderStates[D3DRS_LIGHTING] != 0) ? 1 : 0;
	unsigned alphaFunc = DX8Wrapper::RenderStates[D3DRS_ALPHAFUNC];
	state->alphaFunc = (alphaFunc == D3DCMP_LESSEQUAL) ? 1 : 0;
	state->cullMode = DX8Wrapper::RenderStates[D3DRS_CULLMODE];
	if (state->cullMode == 0x12345678)
		state->cullMode = D3DCULL_CCW;
	state->fillMode = DX8Wrapper::RenderStates[D3DRS_FILLMODE];
	if (state->fillMode == 0x12345678)
		state->fillMode = D3DFILL_SOLID;
	state->depthTest = (DX8Wrapper::RenderStates[D3DRS_ZENABLE] == 0x12345678)
		? 1 : (DX8Wrapper::RenderStates[D3DRS_ZENABLE] ? 1 : 0);
	state->depthWrite = (DX8Wrapper::RenderStates[D3DRS_ZWRITEENABLE] != 0) ? 1 : 0;
	state->depthCompare = DX8Wrapper::RenderStates[D3DRS_ZFUNC];
	if (state->depthCompare == 0x12345678)
		state->depthCompare = D3DCMP_LESSEQUAL;
	state->blendEnable = (DX8Wrapper::RenderStates[D3DRS_ALPHABLENDENABLE] != 0) ? 1 : 0;
	state->srcBlend = DX8Wrapper::RenderStates[D3DRS_SRCBLEND];
	if (state->srcBlend == 0x12345678)
		state->srcBlend = D3DBLEND_ONE;
	state->dstBlend = DX8Wrapper::RenderStates[D3DRS_DESTBLEND];
	if (state->dstBlend == 0x12345678)
		state->dstBlend = D3DBLEND_ZERO;
	unsigned writeMask = DX8Wrapper::RenderStates[D3DRS_COLORWRITEENABLE];
	if (writeMask == 0x12345678)
		writeMask = D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA;
	state->colorWriteMask = 0;
	if (writeMask & D3DCOLORWRITEENABLE_RED) state->colorWriteMask |= 1;
	if (writeMask & D3DCOLORWRITEENABLE_GREEN) state->colorWriteMask |= 2;
	if (writeMask & D3DCOLORWRITEENABLE_BLUE) state->colorWriteMask |= 4;
	if (writeMask & D3DCOLORWRITEENABLE_ALPHA) state->colorWriteMask |= 8;
	state->stencilEnable = (DX8Wrapper::RenderStates[D3DRS_STENCILENABLE] != 0) ? 1 : 0;
	state->stencilFailOp = DX8Wrapper::RenderStates[D3DRS_STENCILFAIL];
	state->stencilPassOp = DX8Wrapper::RenderStates[D3DRS_STENCILPASS];
	state->stencilDepthFailOp = DX8Wrapper::RenderStates[D3DRS_STENCILZFAIL];
	state->stencilCompare = DX8Wrapper::RenderStates[D3DRS_STENCILFUNC];
	if (state->stencilCompare == 0x12345678)
		state->stencilCompare = D3DCMP_ALWAYS;
	state->stencilRef = DX8Wrapper::RenderStates[D3DRS_STENCILREF] & 0xff;
	state->stencilReadMask = DX8Wrapper::RenderStates[D3DRS_STENCILMASK];
	if (state->stencilReadMask == 0x12345678)
		state->stencilReadMask = 0xff;
	state->stencilWriteMask = DX8Wrapper::RenderStates[D3DRS_STENCILWRITEMASK];
	if (state->stencilWriteMask == 0x12345678)
		state->stencilWriteMask = 0xff;
	state->topology = topology;
	state->vertexFVF = fvf;
	state->vertexSize = fvfInfo.Get_FVF_Size();
	state->normalOffset = (fvf & D3DFVF_NORMAL) ? fvfInfo.Get_Normal_Offset() : UINT32_MAX;
	state->diffuseOffset = (fvf & D3DFVF_DIFFUSE) ? fvfInfo.Get_Diffuse_Offset() : UINT32_MAX;
	unsigned texCount = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
	state->uv0Offset = (texCount >= 1) ? fvfInfo.Get_Tex_Offset(0) : UINT32_MAX;
	state->uv1Offset = (texCount >= 2) ? fvfInfo.Get_Tex_Offset(1) : UINT32_MAX;
}

// ---------------------------------------------------------------------------
// Legacy D3D8-era helpers — no-op stubs for partially-ported shadow code.
// The Vulkan backend does not expose a raw IDirect3DDevice8.
// ---------------------------------------------------------------------------
IDirect3DDevice8* DX8Wrapper::_Get_D3D_Device8() { return 0; }
void DX8Wrapper::Set_StreamSource(unsigned, void*, unsigned, unsigned) {}
void DX8Wrapper::Set_AlphaEnable(bool) {}
void DX8Wrapper::Set_StencilEnable(bool) {}
void DX8Wrapper::Set_StencilFunc(unsigned) {}
void DX8Wrapper::Set_StencilRef(unsigned) {}
void DX8Wrapper::Set_StencilWriteMask(unsigned) {}
void DX8Wrapper::Set_StencilReadMask(unsigned) {}
void DX8Wrapper::Set_StencilFailOp(unsigned) {}
void DX8Wrapper::Set_StencilPassOp(unsigned) {}
void DX8Wrapper::Set_Lighting(bool) {}
void DX8Wrapper::Set_BlendFunc(unsigned, unsigned) {}
void DX8Wrapper::Set_ZEnable(bool enable) { RenderStates[D3DRS_ZENABLE] = enable ? 1 : 0; }
void DX8Wrapper::Set_ShadeMode(unsigned) {}

IDirect3D8* DX8Wrapper::_Get_D3D8() { return 0; }
