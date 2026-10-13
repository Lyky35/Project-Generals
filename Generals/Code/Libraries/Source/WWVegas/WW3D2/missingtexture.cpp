#include "always.h"
#include "missingtexture.h"
#include "dx8wrapper.h"
#define missing_image_width 64
#define missing_image_height 64
#define missing_image_width 64
#define missing_image_height 64

static VkTexture * _MissingTexture = NULL;

VkTexture* MissingTexture::_Get_Missing_Texture()
{
	WWASSERT(_MissingTexture);
	return _MissingTexture;
}

void MissingTexture::_Init()
{
	WWASSERT(!_MissingTexture);
	// Vulkan backend - create a simple missing texture
	_MissingTexture = DX8Wrapper::_Create_Vulkan_Texture(
		missing_image_width,
		missing_image_height,
		WW3D_FORMAT_A8R8G8B8,
		TextureClass::MIP_LEVELS_1);
}

void MissingTexture::_Shutdown()
{
	if (_MissingTexture) {
		delete _MissingTexture;
		_MissingTexture = NULL;
	}
}
