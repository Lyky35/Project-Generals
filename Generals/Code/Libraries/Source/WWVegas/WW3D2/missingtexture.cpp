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
	if (_MissingTexture) {
		// The old placeholder had NO pixels uploaded - it sampled UNDEFINED
		// content (black on strict drivers). Fill a magenta/black checker.
		static unsigned char checker[missing_image_width * missing_image_height * 4];
		for (int y = 0; y < missing_image_height; y++) {
			for (int x = 0; x < missing_image_width; x++) {
				unsigned char *p = &checker[(y * missing_image_width + x) * 4];
				bool on = (((x >> 3) ^ (y >> 3)) & 1) != 0;
				p[0] = on ? 0 : 255;	// B
				p[1] = on ? 0 : 0;		// G
				p[2] = on ? 255 : 0;	// R
				p[3] = 255;				// A
			}
		}
		_MissingTexture->UploadLevel(0, missing_image_width, missing_image_height, checker);
	}
}

void MissingTexture::_Shutdown()
{
	if (_MissingTexture) {
		delete _MissingTexture;
		_MissingTexture = NULL;
	}
}
