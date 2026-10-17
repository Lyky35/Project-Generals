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
 *                     $Archive:: /VSS_Sync/ww3d2/surfaceclass.cpp                            $*
 *                                                                                             *
 *              Original Author:: Nathaniel Hoffman                                            *
 *                                                                                             *
 *                      $Author:: Vss_sync                                                    $*
 *                                                                                             *
 *                     $Modtime:: 8/29/01 7:29p                                               $*
 *                                                                                             *
 *                    $Revision:: 24                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   SurfaceClass::Clear -- Clears a surface to 0                                              *
 *   SurfaceClass::Copy -- Copies a region from one surface to another of the same format      *
 *   SurfaceClass::FindBBAlpha -- Finds the bounding box of non zero pixels in the region (x0, *
 *   PixelSize -- Helper Function to find the size in bytes of a pixel                         *
 *   SurfaceClass::Is_Transparent_Column -- Tests to see if the column is transparent or not   *
 *   SurfaceClass::Copy -- Copies from a byte array to the surface                             *
 *   SurfaceClass::CreateCopy -- Creates a byte array copy of the surface                      *
 *   SurfaceClass::DrawHLine -- draws a horizontal line                                        *
 *   SurfaceClass::DrawPixel -- draws a pixel                                                  *
 *   SurfaceClass::Copy -- Copies a block of system ram to the surface                         *
 *   SurfaceClass::Hue_Shift -- changes the hue of the surface                                 *
 *   SurfaceClass::Is_Monochrome -- Checks if surface is monochrome or not                     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "surfaceclass.h"
#include "formconv.h"
#include "dx8wrapper.h"
#include "vector2i.h"
#include "colorspace.h"
#include "bound.h"
#include "d3d8types.h"

/***********************************************************************************************
 * PixelSize -- Helper Function to find the size in bytes of a pixel                           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/13/2001  hy : Created.                                                                  *
 *=============================================================================================*/

unsigned int PixelSize(const SurfaceClass::SurfaceDescription &sd)
{
	unsigned int size=0;

	switch (sd.Format)
	{	
	case WW3D_FORMAT_A8R8G8B8:
	case WW3D_FORMAT_X8R8G8B8:
		size=4;
		break;
	case WW3D_FORMAT_R8G8B8:
		size=3;
		break;
	case WW3D_FORMAT_R5G6B5:
	case WW3D_FORMAT_X1R5G5B5:
	case WW3D_FORMAT_A1R5G5B5:
	case WW3D_FORMAT_A4R4G4B4:
	case WW3D_FORMAT_A8R3G3B2:
	case WW3D_FORMAT_X4R4G4B4:
	case WW3D_FORMAT_A8P8:	
	case WW3D_FORMAT_A8L8:
		size=2;
		break;
	case WW3D_FORMAT_R3G3B2:
	case WW3D_FORMAT_A8:
	case WW3D_FORMAT_P8:
	case WW3D_FORMAT_L8:
	case WW3D_FORMAT_A4L4:
		size=1;
		break;
	}

	return size;
}

void Convert_Pixel(Vector3 &rgb, const SurfaceClass::SurfaceDescription &sd, const unsigned char * pixel)
{
	const float scale=1/255.0f;
	switch (sd.Format)
	{	
	case WW3D_FORMAT_A8R8G8B8:
	case WW3D_FORMAT_X8R8G8B8:
	case WW3D_FORMAT_R8G8B8:
		{
			rgb.X=pixel[2]; // R
			rgb.Y=pixel[1]; // G
			rgb.Z=pixel[0]; // B
		}
		break;
	case WW3D_FORMAT_A4R4G4B4:
		{
			unsigned short tmp;
			tmp=*(unsigned short*)&pixel[0];
			rgb.X=((tmp&0x0f00)>>4);   // R
			rgb.Y=((tmp&0x00f0));		// G
			rgb.Z=((tmp&0x000f)<<4);	// B			
		}
		break;
	case WW3D_FORMAT_A1R5G5B5:
		{
			unsigned short tmp;
			tmp=*(unsigned short*)&pixel[0];			
			rgb.X=(tmp>>7)&0xf8; // R
			rgb.Y=(tmp>>2)&0xf8; // G
			rgb.Z=(tmp<<3)&0xf8; // B			
		}
		break;
	case WW3D_FORMAT_R5G6B5:
		{
			unsigned short tmp;
			tmp=*(unsigned short*)&pixel[0];			
			rgb.X=(tmp>>8)&0xf8;
			rgb.Y=(tmp>>3)&0xfc;
			rgb.Z=(tmp<<3)&0xf8;
		}
		break;

	default:
		// TODO: Implement other pixel formats
		WWASSERT(0);
	}
	rgb*=scale;
}

// Note: This function must never overwrite the original alpha
void Convert_Pixel(unsigned char * pixel,const SurfaceClass::SurfaceDescription &sd, const Vector3 &rgb)
{
	unsigned char r,g,b;
	r=(unsigned char) (rgb.X*255.0f);
	g=(unsigned char) (rgb.Y*255.0f);
	b=(unsigned char) (rgb.Z*255.0f);
	switch (sd.Format)
	{	
	case WW3D_FORMAT_A8R8G8B8:
	case WW3D_FORMAT_X8R8G8B8:
	case WW3D_FORMAT_R8G8B8:
		pixel[0]=b;
		pixel[1]=g;
		pixel[2]=r;
		break;
	case WW3D_FORMAT_A4R4G4B4:
		{
			unsigned short tmp;
			tmp=*(unsigned short*)&pixel[0];
			tmp&=0xF000;
			tmp|=(r&0xF0) << 4;
			tmp|=(g&0xF0);
			tmp|=(b&0xF0) >> 4;			
			*(unsigned short*)&pixel[0]=tmp;
		}
		break;
	case WW3D_FORMAT_A1R5G5B5:
		{
			unsigned short tmp;
			tmp=*(unsigned short*)&pixel[0];
			tmp&=0x8000;
			tmp|=(r&0xF8) << 7;
			tmp|=(g&0xF8) << 2;
			tmp|=(b&0xF8) >> 3;			
			*(unsigned short*)&pixel[0]=tmp;
		}
		break;
	case WW3D_FORMAT_R5G6B5:
		{
			unsigned short tmp;			
			tmp=(r&0xf8) << 8;
			tmp|=(g&0xfc) << 3;
			tmp|=(b&0xf8) >> 3;
			*(unsigned short*)&pixel[0]=tmp;
		}
		break;
	default:
		// TODO: Implement other pixel formats
		WWASSERT(0);
	}
}

/*************************************************************************
**                             SurfaceClass
*************************************************************************/
SurfaceClass::SurfaceClass(unsigned width, unsigned height, WW3DFormat format):
	VulkanSurface(NULL),
	SurfaceFormat(format),
	ShadowMemory(NULL),
	ShadowPitch(0),
	OwnsVulkanSurface(true)
{
	WWASSERT(width);
	WWASSERT(height);
	// Promote 16-bit packed formats: PACK16 support is spotty on modern
	// drivers (vkCreateImage failed on AMD = no fonts + blinking), and our
	// CPU shadow buffer is 32-bit pitched anyway.
	if (format == WW3D_FORMAT_A4R4G4B4 || format == WW3D_FORMAT_X4R4G4B4 ||
		 format == WW3D_FORMAT_A1R5G5B5 || format == WW3D_FORMAT_X1R5G5B5 ||
		 format == WW3D_FORMAT_R5G6B5 || format == WW3D_FORMAT_R8G8B8) {
		format = WW3D_FORMAT_A8R8G8B8;
		SurfaceFormat = format;
	}
	// Create VkTexture-backed surface. NOTE: TranslateFormat expects D3DFMT ids,
	// not WW3D ids - convert first or most formats map to the wrong VkFormat.
	VulkanSurface = new VkTexture();
	bool created = VulkanSurface->Create(&VulkanDevice::Instance(), width, height,
		VulkanSurface->TranslateFormat(WW3DFormat_To_D3DFormat(format)), 1, false);
	if (!created) {
		char buf[128];
		snprintf(buf, sizeof(buf), "SurfaceClass: Create FAILED %ux%u fmt=%d", (unsigned)width, (unsigned)height, (int)format);
		FILE *lf = fopen("RTS_START.LOG", "a");
		if (lf) { fprintf(lf, "%s\n", buf); fclose(lf); }
	}
	ShadowPitch = width * 4;
}

SurfaceClass::SurfaceClass(const char *filename):
	VulkanSurface(NULL),
	ShadowMemory(NULL),
	ShadowPitch(0),
	OwnsVulkanSurface(true)
{
	// TODO: Load VkTexture from file (DDS support)
	// For now create blank surface
	VulkanSurface = new VkTexture();
	bool created = VulkanSurface->Create(&VulkanDevice::Instance(), 64, 64,
		VK_FORMAT_R8G8B8A8_UNORM, 1, false);
	WWASSERT(created);
	ShadowPitch = 64 * 4;
	// TODO: Read surface description from file
	SurfaceFormat = WW3D_FORMAT_UNKNOWN;
}

SurfaceClass::SurfaceClass(VkTexture *vk_texture):
	VulkanSurface(vk_texture),
	ShadowMemory(NULL),
	ShadowPitch(0),
	OwnsVulkanSurface(false)	// borrowed from a TextureClass - do not delete
{
	SurfaceFormat = WW3D_FORMAT_UNKNOWN;
	if (VulkanSurface) {
		ShadowPitch = VulkanSurface->GetWidth() * 4;
	}
}

SurfaceClass::~SurfaceClass(void)
{
	if (ShadowMemory) {
		delete[] ShadowMemory;
		ShadowMemory = NULL;
	}
	if (VulkanSurface && OwnsVulkanSurface) {
		VulkanSurface->Destroy();
		delete VulkanSurface;
	}
	VulkanSurface = NULL;
}

void SurfaceClass::Get_Description(SurfaceDescription &surface_desc)
{
	if (VulkanSurface) {
		surface_desc.Format = SurfaceFormat;
		surface_desc.Width = VulkanSurface->GetWidth();
		surface_desc.Height = VulkanSurface->GetHeight();
	} else {
		// Fallback
		surface_desc.Format = WW3D_FORMAT_UNKNOWN;
		surface_desc.Width = 0;
		surface_desc.Height = 0;
	}
}

void * SurfaceClass::Lock(int * pitch)
{
	if (!VulkanSurface) {
		*pitch = 0;
		return NULL;
	}
	unsigned w = VulkanSurface->GetWidth();
	unsigned h = VulkanSurface->GetHeight();
	if (!ShadowMemory) {
		ShadowPitch = w * 4;
		ShadowMemory = new unsigned char[ShadowPitch * h];
		::ZeroMemory(ShadowMemory, ShadowPitch * h);
	}
	*pitch = ShadowPitch;
	return ShadowMemory;
}

void SurfaceClass::Unlock(void)
{
	if (VulkanSurface && ShadowMemory) {
		// Port fix: 16-bit surfaces (shroud R5G6B5 etc.) were uploaded raw
		// into the promoted 32-bit BGRA image = garbage. Repack first.
		if (ShadowPitch == VulkanSurface->GetWidth() * 2 &&
			VulkanSurface->GetFormat() == VK_FORMAT_B8G8R8A8_UNORM) {
			unsigned w = VulkanSurface->GetWidth();
			unsigned h = VulkanSurface->GetHeight();
			unsigned char *tmp = new unsigned char[(size_t)w * h * 4];
			const unsigned short *src16 = (const unsigned short *)ShadowMemory;
			bool fmt565 = (SurfaceFormat == WW3D_FORMAT_R5G6B5);
			for (size_t i = 0; i < (size_t)w * h; i++) {
				unsigned short p = src16[i];
				unsigned r, g, b;
				if (fmt565) {
					r = ((p >> 11) & 31) * 255 / 31;
					g = ((p >> 5) & 63) * 255 / 63;
					b = (p & 31) * 255 / 31;
				} else {
					// A4R4G4B4-style
					b = ((p & 0xf) * 255) / 15;
					g = (((p >> 4) & 0xf) * 255) / 15;
					r = (((p >> 8) & 0xf) * 255) / 15;
				}
				tmp[i * 4 + 0] = (unsigned char)b;
				tmp[i * 4 + 1] = (unsigned char)g;
				tmp[i * 4 + 2] = (unsigned char)r;
				tmp[i * 4 + 3] = 255;
			}
			VulkanSurface->UploadLevel(0, w, h, tmp);
			delete[] tmp;
			return;
		}
		VulkanSurface->UploadLevel(0, VulkanSurface->GetWidth(), VulkanSurface->GetHeight(), ShadowMemory);
	}
}

/***********************************************************************************************
 * SurfaceClass::Clear -- Clears a surface to 0                                                *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/13/2001  hy : Created.                                                                  *
 *=============================================================================================*/
void SurfaceClass::Clear()
{
	{
		static unsigned n = 0;
		n++;
		if (n <= 30 || (n % 100u) == 0) {
			SurfaceDescription sd0; Get_Description(sd0);
			FILE *lf = fopen("RTS_START.LOG", "a");
			if (lf) {
				fprintf(lf, "VK: SurfaceClass::Clear #%u %ux%u fmt=%d caller=%p\n",
					n, (unsigned)sd0.Width, (unsigned)sd0.Height, (int)sd0.Format,
					__builtin_return_address(0));
				fclose(lf);
			}
		}
	}
	SurfaceDescription sd;
	Get_Description(sd);

	// size of each pixel in bytes
	unsigned int size=PixelSize(sd);

	int pitch=0;
	unsigned char *mem=(unsigned char *)Lock(&pitch);
	if (!mem) return;

	for (unsigned int i=0; i<sd.Height; i++)
	{
		memset(mem,0,size*sd.Width);
		mem+=pitch;
	}
	
	Unlock();
}


/***********************************************************************************************
 * SurfaceClass::Copy -- Copies from a byte array to the surface                               *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   3/15/2001  hy : Created.                                                                  *
 *=============================================================================================*/
void SurfaceClass::Copy(const unsigned char *other)
{
	SurfaceDescription sd;
	Get_Description(sd);

	// size of each pixel in bytes
	unsigned int size=PixelSize(sd);

	int pitch=0;
	unsigned char *mem=(unsigned char *)Lock(&pitch);
	if (!mem) return;

	for (unsigned int i=0; i<sd.Height; i++)
	{
		memcpy(mem,&other[i*sd.Width*size],size*sd.Width);		
		mem+=pitch;
	}
	
	Unlock();
}


/***********************************************************************************************
 * SurfaceClass::Copy -- Copies a block of system ram to the surface                           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   5/2/2001   hy : Created.                                                                  *
 *=============================================================================================*/
void SurfaceClass::Copy(Vector2i &min,Vector2i &max, const unsigned char *other)
{
	SurfaceDescription sd;
	Get_Description(sd);

	// size of each pixel in bytes
	unsigned int size=PixelSize(sd);

	int pitch=0;
	unsigned char *full=(unsigned char *)Lock(&pitch);
	if (!full) return;
	unsigned char *mem=full+min.J*pitch+min.I*size;
	int dx=max.I-min.I;

	for (int i=min.J; i<max.J; i++)
	{
		memcpy(mem,&other[(i*sd.Width+min.I)*size],size*dx);		
		mem+=pitch;
	}
	
	Unlock();
}


/***********************************************************************************************
 * SurfaceClass::CreateCopy -- Creates a byte array copy of the surface                        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   3/16/2001  hy : Created.                                                                  *
 *=============================================================================================*/
unsigned char *SurfaceClass::CreateCopy(int *width,int *height,int*size,bool flip)
{
	SurfaceDescription sd;
	Get_Description(sd);

	// size of each pixel in bytes
	unsigned int mysize=PixelSize(sd);

	*width=sd.Width;
	*height=sd.Height;
	*size=mysize;

	unsigned char *other=W3DNEWARRAY unsigned char [sd.Height*sd.Width*mysize];

	int pitch=0;
	unsigned char *mem=(unsigned char *)Lock(&pitch);
	if (!mem) return other;

	for (unsigned int i=0; i<sd.Height; i++)
	{
		if (flip)
		{
			memcpy(&other[(sd.Height-i-1)*sd.Width*mysize],mem,mysize*sd.Width);		
		} else
		{
			memcpy(&other[i*sd.Width*mysize],mem,mysize*sd.Width);		
		}
		mem+=pitch;
	}
	
	Unlock();

	return other;
}


/***********************************************************************************************
 * SurfaceClass::Copy -- Copies a region from one surface to another                           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/13/2001  hy : Created.                                                                  *
 *=============================================================================================*/
void SurfaceClass::Copy(
	unsigned int dstx, unsigned int dsty,
	unsigned int srcx, unsigned int srcy, 
	unsigned int width, unsigned int height,
	const SurfaceClass *other)
{
	WWASSERT(other);
	WWASSERT(width);
	WWASSERT(height);

	SurfaceDescription sd,osd;
	Get_Description(sd);
	const_cast <SurfaceClass*>(other)->Get_Description(osd);

	RECT src;
	src.left=srcx;
	src.right=srcx+width;
	src.top=srcy;
	src.bottom=srcy+height;

	if (src.right>int(osd.Width)) src.right=int(osd.Width);
	if (src.bottom>int(osd.Height)) src.bottom=int(osd.Height);	

	if (sd.Format==osd.Format && sd.Width==osd.Width && sd.Height==osd.Height)
	{
		POINT dst;
		dst.x=dstx;
		dst.y=dsty;	
		DX8Wrapper::_Copy_Vulkan_Rects(other->VulkanSurface,&src,1,VulkanSurface,&dst);
	}
	else
	{
		RECT dest;
		dest.left=dstx;
		dest.right=dstx+width;
		dest.top=dsty;
		dest.bottom=dsty+height;

		if (dest.right>int(sd.Width)) dest.right=int(sd.Width);
		if (dest.bottom>int(sd.Height)) dest.bottom=int(sd.Height);

		// CPU fallback: copy via shadow buffers (no D3DX on Vulkan backend)
		int src_pitch=0, dst_pitch=0;
		unsigned char *src_mem=(unsigned char *)const_cast<SurfaceClass*>(other)->Lock(&src_pitch);
		unsigned char *dst_mem=(unsigned char *)Lock(&dst_pitch);
		if (src_mem && dst_mem) {
			unsigned int psize=PixelSize(sd);
			for (int y=0; y<dest.bottom-dest.top && srcy+y<osd.Height; ++y) {
				unsigned char *s=src_mem+(srcy+y)*src_pitch+srcx*psize;
				unsigned char *d=dst_mem+(dest.top+y)*dst_pitch+dest.left*psize;
				memcpy(d,s,psize*(dest.right-dest.left));
			}
		}
		if (src_mem) const_cast<SurfaceClass*>(other)->Unlock();
		if (dst_mem) Unlock();
	}
}

/***********************************************************************************************
 * SurfaceClass::Copy -- Copies a region from one surface to another                           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/13/2001  hy : Created.                                                                  *
 *=============================================================================================*/
void SurfaceClass::Stretch_Copy(
	unsigned int dstx, unsigned int dsty, unsigned int dstwidth, unsigned int dstheight,
	unsigned int srcx, unsigned int srcy, unsigned int srcwidth, unsigned int srcheight,
	const SurfaceClass *other)
{
	WWASSERT(other);

	SurfaceDescription sd,osd;
	Get_Description(sd);
	const_cast <SurfaceClass*>(other)->Get_Description(osd);

	RECT src;
	src.left=srcx;
	src.right=srcx+srcwidth;
	src.top=srcy;	
	src.bottom=srcy+srcheight;

	RECT dest;
	dest.left=dstx;
	dest.right=dstx+dstwidth;
	dest.top=dsty;
	dest.bottom=dsty+dstheight;

	// CPU fallback: nearest-sample stretch copy via shadow buffers
	int src_pitch=0, dst_pitch=0;
	unsigned char *src_mem=(unsigned char *)const_cast<SurfaceClass*>(other)->Lock(&src_pitch);
	unsigned char *dst_mem=(unsigned char *)Lock(&dst_pitch);
	if (src_mem && dst_mem) {
		SurfaceDescription sd,osd;
		Get_Description(sd);
		const_cast<SurfaceClass*>(other)->Get_Description(osd);
		unsigned int psize=PixelSize(sd);
		for (unsigned int y=0; y<dstheight; ++y) {
			unsigned int sy=srcy+y*srcheight/dstheight;
			if (sy>=osd.Height) break;
			for (unsigned int x=0; x<dstwidth; ++x) {
				unsigned int sx=srcx+x*srcwidth/dstwidth;
				if (sx>=osd.Width) break;
				memcpy(dst_mem+(dsty+y)*dst_pitch+(dstx+x)*psize,
					src_mem+sy*src_pitch+sx*psize, psize);
			}
		}
	}
	if (src_mem) const_cast<SurfaceClass*>(other)->Unlock();
	if (dst_mem) Unlock();
}

/***********************************************************************************************
 * SurfaceClass::FindBB -- Finds the bounding box of non zero pixels in the region             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/13/2001  hy : Created.                                                                  *
 *=============================================================================================*/
void SurfaceClass::FindBB(Vector2i *min,Vector2i*max)
{
	SurfaceDescription sd;
	Get_Description(sd);

	WWASSERT(Has_Alpha(sd.Format));

	int alphabits=Alpha_Bits(sd.Format);
	int mask=0;
	switch (alphabits)
	{
	case 1: mask=1;
		break;
	case 4: mask=0xf;
		break;
	case 8: mask=0xff;
		break;
	}

	int pitch=0;
	unsigned char *base=(unsigned char *)Lock(&pitch);
	if (!base) return;

	int x,y;
	unsigned int size=PixelSize(sd);
	Vector2i realmin=*max;
	Vector2i realmax=*min;	
	
	// the assumption here is that whenever a pixel has alpha it's in the MSB
	for (y = min->J; y < max->J; y++) {
		for (x = min->I; x < max->I; x++) {
			unsigned char *alpha=base+y*pitch+x*size;
			#pragma MESSAGE("HY - this is not endian safe")
			unsigned char myalpha=alpha[size-1];
			myalpha=(myalpha>>(8-alphabits)) & mask;
			if (myalpha) {
				realmin.I = MIN(realmin.I, x);
				realmax.I = MAX(realmax.I, x);
				realmin.J = MIN(realmin.J, y);
				realmax.J = MAX(realmax.J, y);
			}
		}
	}

	Unlock();

	*max=realmax;
	*min=realmin;
}


/***********************************************************************************************
 * SurfaceClass::Is_Transparent_Column -- Tests to see if the column is transparent or not     *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/13/2001  hy : Created.                                                                  *
 *=============================================================================================*/
bool SurfaceClass::Is_Transparent_Column(unsigned int column)
{
	SurfaceDescription sd;
	Get_Description(sd);

	WWASSERT(column<sd.Width);
	WWASSERT(Has_Alpha(sd.Format));

	int alphabits=Alpha_Bits(sd.Format);
	int mask=0;
	switch (alphabits)
	{
	case 1: mask=1;
		break;
	case 4: mask=0xf;
		break;
	case 8: mask=0xff;
		break;
	}

	unsigned int size=PixelSize(sd);

	int pitch=0;
	unsigned char *base=(unsigned char *)Lock(&pitch);
	if (!base) return true;

	int y;	
	
	// the assumption here is that whenever a pixel has alpha it's in the MSB
	for (y = 0; y < (int) sd.Height; y++)
	{
		unsigned char *alpha=base+y*pitch+column*size;
		#pragma MESSAGE("HY - this is not endian safe")
		unsigned char myalpha=alpha[size-1];		
		myalpha=(myalpha>>(8-alphabits)) & mask;		
		if (myalpha) {
			Unlock();
			return false;			
		}		
	}

	Unlock();
	return true;
}

/***********************************************************************************************
 * SurfaceClass::Get_Pixel -- Returns the pixel's RGB valus to the caller							  *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/13/2001  hy : Created.                                                                  *
 *=============================================================================================*/
void SurfaceClass::Get_Pixel(Vector3 &rgb, int x,int y)
{
	SurfaceDescription sd;
	Get_Description(sd);

	x = (x < (int)sd.Width - 1) ? x : (int)sd.Width - 1;
	y = (y < (int)sd.Height - 1) ? y : (int)sd.Height - 1;

	int pitch=0;
	unsigned char *base=(unsigned char *)Lock(&pitch);
	if (!base) return;
	Convert_Pixel(rgb,sd,base+y*pitch+x*PixelSize(sd));
	Unlock();
}

/***********************************************************************************************
 * SurfaceClass::Attach -- Attaches a surface pointer to the object, releasing the current ptr.*
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   3/27/2001  pds : Created.                                                                 *
 *=============================================================================================*/
void SurfaceClass::Attach (VkTexture *texture)
{
	Detach ();
	VulkanSurface = texture;
	return ;
}


/***********************************************************************************************
 * SurfaceClass::Detach -- Releases the reference on the internal surface ptr, and NULLs it.	 .*
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   3/27/2001  pds : Created.                                                                 *
 *=============================================================================================*/
void SurfaceClass::Detach (void)
{
	VulkanSurface = NULL;
	return ;
}


/***********************************************************************************************
 * SurfaceClass::DrawPixel -- draws a pixel                                                    *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *=============================================================================================*/
void SurfaceClass::DrawPixel(const unsigned int x,const unsigned int y, unsigned int color)
{
	SurfaceDescription sd;
	Get_Description(sd);

	unsigned int size=PixelSize(sd);

	int pitch=0;
	unsigned char *base=(unsigned char *)Lock(&pitch);
	if (!base) return;
	unsigned char *cptr=base+y*pitch+x*size;

	switch (size)
	{
	case 1:
		*cptr=(unsigned char) (color & 0xFF);
		break;
	case 2:
		*(unsigned short *)cptr=(unsigned short) (color & 0xFFFF);
		break;
	case 4:
		*(unsigned int *)cptr=color;
		break;
	}

	Unlock();
}

void SurfaceClass::DrawPixelFast(const unsigned int x,const unsigned int y, unsigned int color)
{
	SurfaceDescription sd;
	Get_Description(sd);

	unsigned int size=PixelSize(sd);

	int pitch=0;
	unsigned char *base=(unsigned char *)Lock(&pitch);
	if (!base) return;
	unsigned char *cptr=base+y*pitch+x*size;

	switch (size)
	{
	case 1:
		*cptr=(unsigned char) (color & 0xFF);
		break;
	case 2:
		*(unsigned short *)cptr=(unsigned short) (color & 0xFFFF);
		break;
	case 4:
		*(unsigned int *)cptr=color;
		break;
	}
	// no Unlock on purpose - the caller uploads once with Unlock()
}

/***********************************************************************************************
 * SurfaceClass::DrawHLine -- draws a horizontal line                                          *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   4/9/2001   hy : Created.                                                                  *
 *   4/9/2001   hy : Created.                                                                  *
 *=============================================================================================*/
void SurfaceClass::DrawHLine(const unsigned int y,const unsigned int x1, const unsigned int x2, unsigned int color)
{ 
	SurfaceDescription sd;
	Get_Description(sd);

	unsigned int size=PixelSize(sd);

	int pitch=0;
	unsigned char *base=(unsigned char *)Lock(&pitch);
	if (!base) return;
	unsigned char *cptr=base+y*pitch+x1*size;

	unsigned int x;
	// the assumption here is that whenever a pixel has alpha it's in the MSB
	for (x=x1; x<=x2; x++)
	{		
		switch (size)
		{
		case 1:
			*cptr++=(unsigned char) (color & 0xFF);
			break;
		case 2:
			*(unsigned short *)cptr=(unsigned short) (color & 0xFFFF);
			cptr+=2;
			break;
		case 4:
			*(unsigned int *)cptr=color;
			cptr+=4;
			break;
		}
	}

	Unlock();
}


/***********************************************************************************************
 * SurfaceClass::Is_Monochrome -- Checks if surface is monochrome or not                       *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   7/5/2001   hy : Created.                                                                  *
 *=============================================================================================*/
bool SurfaceClass::Is_Monochrome(void)
{
	unsigned int x,y;
	SurfaceDescription sd;
	Get_Description(sd);

	switch (sd.Format)
	{
		case WW3D_FORMAT_A8L8:	
		case WW3D_FORMAT_A8:		
		case WW3D_FORMAT_L8:
		case WW3D_FORMAT_A4L4:
			return true;
		break;
	}

	int pitch,size;

	size=PixelSize(sd);
	unsigned char *bits=(unsigned char*) Lock(&pitch);

	Vector3 rgb;
	bool mono=true;

	for (y=0; y<sd.Height; y++)
	{
		for (x=0; x<sd.Width; x++)
		{
			Convert_Pixel(rgb,sd,&bits[x*size]);
			mono&=(rgb.X==rgb.Y);
			mono&=(rgb.X==rgb.Z);
			mono&=(rgb.Z==rgb.Y);
			if (!mono)
			{
				Unlock();
				return false;
			}
		}
		bits+=pitch;
	}

	Unlock();

	return true;
}

/***********************************************************************************************
 * SurfaceClass::Hue_Shift -- changes the hue of the surface                                   *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   7/3/2001   hy : Created.                                                                  *
 *=============================================================================================*/
void SurfaceClass::Hue_Shift(const Vector3 &hsv_shift)
{
	unsigned int x,y;
	SurfaceDescription sd;
	Get_Description(sd);
	int pitch,size;

	size=PixelSize(sd);
	unsigned char *bits=(unsigned char*) Lock(&pitch);

	Vector3 rgb;

	for (y=0; y<sd.Height; y++)
	{
		for (x=0; x<sd.Width; x++)
		{
			Convert_Pixel(rgb,sd,&bits[x*size]);
			Recolor(rgb,hsv_shift);
			rgb.X=Bound(rgb.X,0.0f,1.0f);
			rgb.Y=Bound(rgb.Y,0.0f,1.0f);
			rgb.Z=Bound(rgb.Z,0.0f,1.0f);
			Convert_Pixel(&bits[x*size],sd,rgb);
		}
		bits+=pitch;
	}

	Unlock();
}