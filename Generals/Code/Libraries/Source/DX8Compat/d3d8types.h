/*
**	Project Generals — DX8 compatibility types
**
**	Drop-in replacement for the Direct3D 8 type/enum declarations used by
**	WW3D2 (d3d8.h / d3dx8.h / d3dx8math.h). Values match the real D3D8 SDK so
**	existing code using these constants behaves identically. COM interfaces
**	(IDirect3DDevice8 etc.) are intentionally NOT provided — the Vulkan
**	backend replaces them (see ../VulkanRenderer).
*/

#ifndef DX8COMPAT_D3D8TYPES_H
#define DX8COMPAT_D3D8TYPES_H

#include <windows.h>

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
	((DWORD)(unsigned char)(ch0) | ((DWORD)(unsigned char)(ch1) << 8) | \
	((DWORD)(unsigned char)(ch2) << 16) | ((DWORD)(unsigned char)(ch3) << 24))
#endif

typedef DWORD D3DCOLOR;
typedef enum _D3DFORMAT {
	D3DFMT_UNKNOWN = 0,
	D3DFMT_R8G8B8 = 20,
	D3DFMT_A8R8G8B8 = 21,
	D3DFMT_X8R8G8B8 = 22,
	D3DFMT_R5G6B5 = 23,
	D3DFMT_X1R5G5B5 = 24,
	D3DFMT_A1R5G5B5 = 25,
	D3DFMT_A4R4G4B4 = 26,
	D3DFMT_R3G3B2 = 27,
	D3DFMT_A8 = 28,
	D3DFMT_A8R3G3B2 = 29,
	D3DFMT_X4R4G4B4 = 30,
	D3DFMT_A8P8 = 40,
	D3DFMT_P8 = 41,
	D3DFMT_L8 = 50,
	D3DFMT_A8L8 = 51,
	D3DFMT_A4L4 = 52,
	D3DFMT_V8U8 = 60,
	D3DFMT_L6V5U5 = 61,
	D3DFMT_X8L8V8U8 = 62,
	D3DFMT_Q8W8V8U8 = 63,
	D3DFMT_V16U16 = 64,
	D3DFMT_W11V11U10 = 65,
	D3DFMT_UYVY = MAKEFOURCC('U', 'Y', 'V', 'Y'),
	D3DFMT_YUY2 = MAKEFOURCC('Y', 'U', 'Y', '2'),
	D3DFMT_DXT1 = MAKEFOURCC('D', 'X', 'T', '1'),
	D3DFMT_DXT2 = MAKEFOURCC('D', 'X', 'T', '2'),
	D3DFMT_DXT3 = MAKEFOURCC('D', 'X', 'T', '3'),
	D3DFMT_DXT4 = MAKEFOURCC('D', 'X', 'T', '4'),
	D3DFMT_DXT5 = MAKEFOURCC('D', 'X', 'T', '5'),
	D3DFMT_D16_LOCKABLE = 70,
	D3DFMT_D32 = 71,
	D3DFMT_D15S1 = 73,
	D3DFMT_D24S8 = 75,
	D3DFMT_D24X8 = 77,
	D3DFMT_D24X4S4 = 79,
	D3DFMT_D16 = 80,
	D3DFMT_INDEX16 = 101,
	D3DFMT_INDEX32 = 102,
	D3DFMT_FORCE_DWORD = 0x7fffffff
} D3DFORMAT;

typedef enum _D3DPOOL {
	D3DPOOL_DEFAULT = 0,
	D3DPOOL_MANAGED = 1,
	D3DPOOL_SYSTEMMEM = 2,
	D3DPOOL_SCRATCH = 3,
	D3DPOOL_FORCE_DWORD = 0x7fffffff
} D3DPOOL;

typedef enum _D3DUSAGE {
	D3DUSAGE_RENDERTARGET = 0x00000001,
	D3DUSAGE_DEPTHSTENCIL = 0x00000002,
	D3DUSAGE_WRITEONLY = 0x00000008,
	D3DUSAGE_SOFTWAREPROCESSING = 0x00000010,
	D3DUSAGE_DONOTCLIP = 0x00000020,
	D3DUSAGE_POINTS = 0x00000040,
	D3DUSAGE_RTPATCHES = 0x00000080,
	D3DUSAGE_NPATCHES = 0x00000100,
	D3DUSAGE_DYNAMIC = 0x00000200,
	D3DUSAGE_FORCE_DWORD = 0x7fffffff
} D3DUSAGE;

typedef enum _D3DLOCK {
	D3DLOCK_READONLY = 0x00000010,
	D3DLOCK_NOSYSLOCK = 0x00000800,
	D3DLOCK_NOOVERWRITE = 0x00001000,
	D3DLOCK_DISCARD = 0x00002000,
	D3DLOCK_FORCE_DWORD = 0x7fffffff
} D3DLOCK;

typedef enum _D3DRENDERSTATETYPE {
	D3DRS_ZENABLE = 7,
	D3DRS_FILLMODE = 8,
	D3DRS_SHADEMODE = 9,
	D3DRS_ZWRITEENABLE = 14,
	D3DRS_ALPHATESTENABLE = 15,
	D3DRS_SRCBLEND = 19,
	D3DRS_DESTBLEND = 20,
	D3DRS_CULLMODE = 22,
	D3DRS_ZFUNC = 23,
	D3DRS_ALPHAREF = 24,
	D3DRS_ALPHAFUNC = 25,
	D3DRS_DITHERENABLE = 26,
	D3DRS_ALPHABLENDENABLE = 27,
	D3DRS_FOGENABLE = 28,
	D3DRS_SPECULARENABLE = 29,
	D3DRS_FOGCOLOR = 34,
	D3DRS_FOGTABLEMODE = 35,
	D3DRS_FOGSTART = 36,
	D3DRS_FOGEND = 37,
	D3DRS_FOGDENSITY = 38,
	D3DRS_EDGEANTIALIAS = 40,
	D3DRS_ZBIAS = 47,
	D3DRS_RANGEFOGENABLE = 48,
	D3DRS_CLIPPING = 136,
	D3DRS_LIGHTING = 137,
	D3DRS_AMBIENT = 139,
	D3DRS_FOGVERTEXMODE = 140,
	D3DRS_COLORVERTEX = 141,
	D3DRS_LOCALVIEWER = 142,
	D3DRS_NORMALIZENORMALS = 143,
	D3DRS_DIFFUSEMATERIALSOURCE = 145,
	D3DRS_SPECULARMATERIALSOURCE = 146,
	D3DRS_AMBIENTMATERIALSOURCE = 147,
	D3DRS_EMISSIVEMATERIALSOURCE = 148,
	D3DRS_VERTEXBLEND = 151,
	D3DRS_CLIPPLANEENABLE = 152,
	D3DRS_SOFTWAREVERTEXPROCESSING = 153,
	D3DRS_TEXTUREFACTOR = 160,
	D3DRS_PATCHSEGMENTS = 164,
	D3DRS_COLORWRITEENABLE = 168,
	D3DRS_STENCILENABLE = 52,
	D3DRS_STENCILFAIL = 53,
	D3DRS_STENCILZFAIL = 54,
	D3DRS_STENCILPASS = 55,
	D3DRS_STENCILFUNC = 56,
	D3DRS_STENCILREF = 57,
	D3DRS_STENCILMASK = 58,
	D3DRS_STENCILWRITEMASK = 59,
	D3DRS_FORCE_DWORD = 0x7fffffff
} D3DRENDERSTATETYPE;

#define D3DCOLORWRITEENABLE_RED 1
#define D3DCOLORWRITEENABLE_GREEN 2
#define D3DCOLORWRITEENABLE_BLUE 4
#define D3DCOLORWRITEENABLE_ALPHA 8

typedef enum _D3DSTENCILOP {
	D3DSTENCILOP_KEEP = 1,
	D3DSTENCILOP_ZERO = 2,
	D3DSTENCILOP_REPLACE = 3,
	D3DSTENCILOP_INCRSAT = 4,
	D3DSTENCILOP_DECRSAT = 5,
	D3DSTENCILOP_INVERT = 6,
	D3DSTENCILOP_INCR = 7,
	D3DSTENCILOP_DECR = 8,
	D3DSTENCILOP_FORCE_DWORD = 0x7fffffff
} D3DSTENCILOP;

typedef enum _D3DTEXTURESTAGESTATETYPE {
	D3DTSS_COLOROP = 1,
	D3DTSS_COLORARG1 = 2,
	D3DTSS_COLORARG2 = 3,
	D3DTSS_ALPHAOP = 4,
	D3DTSS_ALPHAARG1 = 5,
	D3DTSS_ALPHAARG2 = 6,
	D3DTSS_BUMPENVMAT00 = 7,
	D3DTSS_BUMPENVMAT01 = 8,
	D3DTSS_BUMPENVMAT10 = 9,
	D3DTSS_BUMPENVMAT11 = 10,
	D3DTSS_TEXCOORDINDEX = 11,
	D3DTSS_ADDRESSU = 13,
	D3DTSS_ADDRESSV = 14,
	D3DTSS_BORDERCOLOR = 15,
	D3DTSS_MAGFILTER = 16,
	D3DTSS_MINFILTER = 17,
	D3DTSS_MIPFILTER = 18,
	D3DTSS_MIPMAPLODBIAS = 19,
	D3DTSS_MAXMIPLEVEL = 20,
	D3DTSS_MAXANISOTROPY = 21,
	D3DTSS_BUMPENVLSCALE = 22,
	D3DTSS_BUMPENVLOFFSET = 23,
	D3DTSS_TEXTURETRANSFORMFLAGS = 24,
	D3DTSS_COLORARG0 = 26,
	D3DTSS_ALPHAARG0 = 27,
	D3DTSS_RESULTARG = 28,
	D3DTSS_FORCE_DWORD = 0x7fffffff
} D3DTEXTURESTAGESTATETYPE;

typedef enum _D3DTEXTUREOP {
	D3DTOP_DISABLE = 1,
	D3DTOP_SELECTARG1 = 2,
	D3DTOP_SELECTARG2 = 3,
	D3DTOP_MODULATE = 4,
	D3DTOP_MODULATE2X = 5,
	D3DTOP_MODULATE4X = 6,
	D3DTOP_ADD = 7,
	D3DTOP_ADDSIGNED = 8,
	D3DTOP_ADDSIGNED2X = 9,
	D3DTOP_SUBTRACT = 10,
	D3DTOP_ADDSMOOTH = 11,
	D3DTOP_BLENDDIFFUSEALPHA = 12,
	D3DTOP_BLENDTEXTUREALPHA = 13,
	D3DTOP_BLENDFACTORALPHA = 14,
	D3DTOP_BLENDTEXTUREALPHAPM = 15,
	D3DTOP_BLENDCURRENTALPHA = 16,
	D3DTOP_PREMODULATE = 17,
	D3DTOP_MODULATEALPHA_ADDCOLOR = 18,
	D3DTOP_MODULATECOLOR_ADDALPHA = 19,
	D3DTOP_MODULATEINVALPHA_ADDCOLOR = 20,
	D3DTOP_MODULATEINVCOLOR_ADDALPHA = 21,
	D3DTOP_BUMPENVMAP = 22,
	D3DTOP_BUMPENVMAPLUMINANCE = 23,
	D3DTOP_DOTPRODUCT3 = 24,
	D3DTOP_MULTIPLYADD = 25,
	D3DTOP_LERP = 26,
	D3DTOP_FORCE_DWORD = 0x7fffffff
} D3DTEXTUREOP;

typedef enum _D3DBLEND {
	D3DBLEND_ZERO = 1,
	D3DBLEND_ONE = 2,
	D3DBLEND_SRCCOLOR = 3,
	D3DBLEND_INVSRCCOLOR = 4,
	D3DBLEND_SRCALPHA = 5,
	D3DBLEND_INVSRCALPHA = 6,
	D3DBLEND_DESTALPHA = 7,
	D3DBLEND_INVDESTALPHA = 8,
	D3DBLEND_DESTCOLOR = 9,
	D3DBLEND_INVDESTCOLOR = 10,
	D3DBLEND_SRCALPHASAT = 11,
	D3DBLEND_BOTHSRCALPHA = 12,
	D3DBLEND_BOTHINVSRCALPHA = 13,
	D3DBLEND_FORCE_DWORD = 0x7fffffff
} D3DBLEND;

typedef enum _D3DCMPFUNC {
	D3DCMP_NEVER = 1,
	D3DCMP_LESS = 2,
	D3DCMP_EQUAL = 3,
	D3DCMP_LESSEQUAL = 4,
	D3DCMP_GREATER = 5,
	D3DCMP_NOTEQUAL = 6,
	D3DCMP_GREATEREQUAL = 7,
	D3DCMP_ALWAYS = 8,
	D3DCMP_FORCE_DWORD = 0x7fffffff
} D3DCMPFUNC;

typedef enum _D3DCULL {
	D3DCULL_NONE = 1,
	D3DCULL_CW = 2,
	D3DCULL_CCW = 3,
	D3DCULL_FORCE_DWORD = 0x7fffffff
} D3DCULL;

typedef enum _D3DFILLMODE {
	D3DFILL_POINT = 1,
	D3DFILL_WIREFRAME = 2,
	D3DFILL_SOLID = 3,
	D3DFILL_FORCE_DWORD = 0x7fffffff
} D3DFILLMODE;

typedef enum _D3DFOGMODE {
	D3DFOG_NONE = 0,
	D3DFOG_EXP = 1,
	D3DFOG_EXP2 = 2,
	D3DFOG_LINEAR = 3,
	D3DFOG_FORCE_DWORD = 0x7fffffff
} D3DFOGMODE;

typedef enum _D3DTEXTUREFILTERTYPE {
	D3DTEXF_NONE = 0,
	D3DTEXF_POINT = 1,
	D3DTEXF_LINEAR = 2,
	D3DTEXF_ANISOTROPIC = 3,
	D3DTEXF_FLATCUBIC = 4,
	D3DTEXF_GAUSSIANCUBIC = 5,
	D3DTEXF_FORCE_DWORD = 0x7fffffff
} D3DTEXTUREFILTERTYPE;

typedef enum _D3DTEXTUREADDRESS {
	D3DTADDRESS_WRAP = 1,
	D3DTADDRESS_MIRROR = 2,
	D3DTADDRESS_CLAMP = 3,
	D3DTADDRESS_BORDER = 4,
	D3DTADDRESS_MIRRORONCE = 5,
	D3DTADDRESS_FORCE_DWORD = 0x7fffffff
} D3DTEXTUREADDRESS;

typedef enum _D3DMATERIALCOLORSOURCE {
	D3DMCS_MATERIAL = 0,
	D3DMCS_COLOR1 = 1,
	D3DMCS_COLOR2 = 2,
	D3DMCS_FORCE_DWORD = 0x7fffffff
} D3DMATERIALCOLORSOURCE;

typedef enum _D3DTRANSFORMSTATETYPE {
	D3DTS_VIEW = 2,
	D3DTS_PROJECTION = 3,
	D3DTS_TEXTURE0 = 16,
	D3DTS_TEXTURE1 = 17,
	D3DTS_TEXTURE2 = 18,
	D3DTS_TEXTURE3 = 19,
	D3DTS_TEXTURE4 = 20,
	D3DTS_TEXTURE5 = 21,
	D3DTS_TEXTURE6 = 22,
	D3DTS_TEXTURE7 = 23,
	D3DTS_WORLD = 256,
	D3DTS_WORLD1 = 257,
	D3DTS_WORLD2 = 258,
	D3DTS_WORLD3 = 259,
	D3DTS_FORCE_DWORD = 0x7fffffff
} D3DTRANSFORMSTATETYPE;

typedef enum _D3DPRIMITIVETYPE {
	D3DPT_POINTLIST = 1,
	D3DPT_LINELIST = 2,
	D3DPT_LINESTRIP = 3,
	D3DPT_TRIANGLELIST = 4,
	D3DPT_TRIANGLESTRIP = 5,
	D3DPT_TRIANGLEFAN = 6,
	D3DPT_FORCE_DWORD = 0x7fffffff
} D3DPRIMITIVETYPE;

#define D3DFVF_RESERVED0 0x001
#define D3DFVF_POSITION_MASK 0x00E
#define D3DFVF_XYZ 0x002
#define D3DFVF_XYZRHW 0x004
#define D3DFVF_XYZB1 0x006
#define D3DFVF_XYZB2 0x008
#define D3DFVF_XYZB3 0x00a
#define D3DFVF_XYZB4 0x00c
#define D3DFVF_XYZB5 0x00e
#define D3DFVF_NORMAL 0x010
#define D3DFVF_PSIZE 0x020
#define D3DFVF_DIFFUSE 0x040
#define D3DFVF_SPECULAR 0x080
#define D3DFVF_TEXCOUNT_MASK 0xf00
#define D3DFVF_TEXCOUNT_SHIFT 8
#define D3DFVF_TEX0 0x000
#define D3DFVF_TEX1 0x100
#define D3DFVF_TEX2 0x200
#define D3DFVF_TEX3 0x300
#define D3DFVF_TEX4 0x400
#define D3DFVF_TEX5 0x500
#define D3DFVF_TEX6 0x600
#define D3DFVF_TEX7 0x700
#define D3DFVF_TEX8 0x800
#define D3DFVF_LASTBETA_UBYTE4 0x1000
#define D3DFVF_LASTBETA_D3DCOLOR 0x8000
#define D3DFVF_RESERVED2 0x6000
#define D3DFVF_TEXTUREFORMAT2 0
#define D3DFVF_TEXTUREFORMAT1 3
#define D3DFVF_TEXTUREFORMAT3 1
#define D3DFVF_TEXTUREFORMAT4 2
#define D3DFVF_TEXCOORDSIZE3(CoordIndex) (D3DFVF_TEXTUREFORMAT3 << (CoordIndex * 2 + 16))
#define D3DFVF_TEXCOORDSIZE2(CoordIndex) (D3DFVF_TEXTUREFORMAT2)
#define D3DFVF_TEXCOORDSIZE4(CoordIndex) (D3DFVF_TEXTUREFORMAT4 << (CoordIndex * 2 + 16))
#define D3DFVF_TEXCOORDSIZE1(CoordIndex) (D3DFVF_TEXTUREFORMAT1 << (CoordIndex * 2 + 16))

#define D3DTA_SELECTMASK 0x0000000f
#define D3DTA_DIFFUSE 0x00000000
#define D3DTA_CURRENT 0x00000001
#define D3DTA_TEXTURE 0x00000002
#define D3DTA_TFACTOR 0x00000003
#define D3DTA_SPECULAR 0x00000004
#define D3DTA_TEMP 0x00000005
#define D3DTA_CONSTANT 0x00000006
#define D3DTA_COMPLEMENT 0x00000010
#define D3DTA_ALPHAREPLICATE 0x00000020

#define D3DTSS_TCI_PASSTHRU 0x00000000
#define D3DTSS_TCI_CAMERASPACENORMAL 0x00010000
#define D3DTSS_TCI_CAMERASPACEPOSITION 0x00010001
#define D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR 0x00010002

#define D3DTTFF_DISABLE 0
#define D3DTTFF_COUNT1 1
#define D3DTTFF_COUNT2 2
#define D3DTTFF_COUNT3 3
#define D3DTTFF_COUNT4 4
#define D3DTTFF_PROJECTED 256

typedef enum _D3DLIGHTTYPE {
	D3DLIGHT_POINT = 1,
	D3DLIGHT_SPOT = 2,
	D3DLIGHT_DIRECTIONAL = 3,
	D3DLIGHT_FORCE_DWORD = 0x7fffffff
} D3DLIGHTTYPE;

typedef enum _D3DRESOURCETYPE {
	D3DRTYPE_SURFACE = 1,
	D3DRTYPE_VOLUME = 2,
	D3DRTYPE_TEXTURE = 3,
	D3DRTYPE_VOLUMETEXTURE = 4,
	D3DRTYPE_CUBETEXTURE = 5,
	D3DRTYPE_VERTEXBUFFER = 6,
	D3DRTYPE_INDEXBUFFER = 7,
	D3DRTYPE_FORCE_DWORD = 0x7fffffff
} D3DRESOURCETYPE;

typedef enum _D3DMULTISAMPLE_TYPE {
	D3DMULTISAMPLE_NONE = 0,
	D3DMULTISAMPLE_FORCE_DWORD = 0x7fffffff
} D3DMULTISAMPLE_TYPE;

typedef enum _D3DSWAPEFFECT {
	D3DSWAPEFFECT_DISCARD = 1,
	D3DSWAPEFFECT_FLIP = 2,
	D3DSWAPEFFECT_COPY = 3,
	D3DSWAPEFFECT_COPY_VSYNC = 4,
	D3DSWAPEFFECT_FORCE_DWORD = 0x7fffffff
} D3DSWAPEFFECT;

typedef enum _D3DDEVTYPE {
	D3DDEVTYPE_HAL = 1,
	D3DDEVTYPE_REF = 2,
	D3DDEVTYPE_SW = 3,
	D3DDEVTYPE_FORCE_DWORD = 0x7fffffff
} D3DDEVTYPE;

#define D3DADAPTER_DEFAULT 0
#define D3D_SDK_VERSION 220
#define D3DENUM_NO_WHQL_LEVEL 2
#define D3DPRESENT_RATE_DEFAULT 0x00000000
#define D3DPRESENT_INTERVAL_DEFAULT 0x00000000
#define D3DPRESENT_INTERVAL_ONE 0x00000001
#define D3DPRESENT_INTERVAL_TWO 0x00000002
#define D3DPRESENT_INTERVAL_THREE 0x00000004
#define D3DPRESENT_INTERVAL_IMMEDIATE 0x80000000
#define D3DCREATE_FPU_PRESERVE 0x00000002
#define D3DCREATE_MULTITHREADED 0x00000004
#define D3DCREATE_SOFTWARE_VERTEXPROCESSING 0x00000020
#define D3DCREATE_MIXED_VERTEXPROCESSING 0x00000080
#define D3DCAPS2_FULLSCREENGAMMA 0x00020000
#define D3DDEVCAPS_HWTRANSFORMANDLIGHT 0x00000001
#define D3DDEVCAPS_NPATCHES 0x00000002
#define D3DPRASTERCAPS_FOGRANGE 0x00000001
#define D3DPMISCCAPS_COLORWRITEENABLE 0x00000002
#define D3DPTFILTERCAPS_MAGFPOINT 0x00000100
#define D3DPTFILTERCAPS_MAGFLINEAR 0x00000200
#define D3DPTFILTERCAPS_MAGFANISOTROPIC 0x00000400
#define D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC 0x00001000
#define D3DPTFILTERCAPS_MAGFFLATCUBIC 0x00002000
#define D3DPTFILTERCAPS_MINFPOINT 0x00010000
#define D3DPTFILTERCAPS_MINFLINEAR 0x00020000
#define D3DPTFILTERCAPS_MINFANISOTROPIC 0x00040000
#define D3DPTFILTERCAPS_MIPFPOINT 0x01000000
#define D3DPTFILTERCAPS_MIPFLINEAR 0x02000000
#define D3DTEXOPCAPS_ADD 0x00000040
#define D3DTEXOPCAPS_ADDSMOOTH 0x00000400
#define D3DTEXOPCAPS_BLENDCURRENTALPHA 0x00008000
#define D3DTEXOPCAPS_BLENDTEXTUREALPHA 0x00000200
#define D3DTEXOPCAPS_BUMPENVMAP 0x00200000
#define D3DTEXOPCAPS_BUMPENVMAPLUMINANCE 0x00400000
#define D3DTEXOPCAPS_DOTPRODUCT3 0x00800000
#define D3DTEXOPCAPS_MODULATE 0x00000008
#define D3DTEXOPCAPS_SUBTRACT 0x00002000
#define D3DSGR_NO_CALIBRATION 0x00000000
#define D3DSGR_CALIBRATE 0x00000001
#define D3DBACKBUFFER_TYPE_MONO 0
#define D3DDP_MAXTEXCOORD 8
#define D3DCLEAR_TARGET 0x00000001
#define D3DCLEAR_ZBUFFER 0x00000002
#define D3DCLEAR_STENCIL 0x00000004
#define D3D_OK S_OK
#define D3DERR_DEVICELOST MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, 2152)
#define D3DERR_DEVICENOTRESET MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, 2153)
#define D3DERR_NOTAVAILABLE MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, 2154)
#define D3DERR_OUTOFVIDEOMEMORY MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, 380)

#define D3DCOLOR_COLORVALUE(r, g, b, a) \
	((D3DCOLOR)(((((a) >= 1.0f) ? 255 : (((a) <= 0.0f) ? 0 : (DWORD)((a) * 255.0f + 0.5f))) << 24) | \
	((((r) >= 1.0f) ? 255 : (((r) <= 0.0f) ? 0 : (DWORD)((r) * 255.0f + 0.5f))) << 16) | \
	((((g) >= 1.0f) ? 255 : (((g) <= 0.0f) ? 0 : (DWORD)((g) * 255.0f + 0.5f))) << 8) | \
	((((b) >= 1.0f) ? 255 : (((b) <= 0.0f) ? 0 : (DWORD)((b) * 255.0f + 0.5f)))))

typedef struct _D3DCOLORVALUE {
	float r;
	float g;
	float b;
	float a;
} D3DCOLORVALUE;

typedef struct _D3DVECTOR {
	float x;
	float y;
	float z;
} D3DVECTOR;

typedef struct _D3DMATRIX {
	union {
		struct {
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
		float m[4][4];
	};
} D3DMATRIX;

typedef struct _D3DVIEWPORT8 {
	DWORD X;
	DWORD Y;
	DWORD Width;
	DWORD Height;
	float MinZ;
	float MaxZ;
} D3DVIEWPORT8;

typedef struct _D3DLIGHT8 {
	D3DLIGHTTYPE Type;
	D3DCOLORVALUE Diffuse;
	D3DCOLORVALUE Specular;
	D3DCOLORVALUE Ambient;
	D3DVECTOR Position;
	D3DVECTOR Direction;
	float Range;
	float Falloff;
	float Attenuation0;
	float Attenuation1;
	float Attenuation2;
	float Theta;
	float Phi;
} D3DLIGHT8;

typedef struct _D3DMATERIAL8 {
	D3DCOLORVALUE Diffuse;
	D3DCOLORVALUE Ambient;
	D3DCOLORVALUE Specular;
	D3DCOLORVALUE Emissive;
	float Power;
} D3DMATERIAL8;

typedef struct _D3DDISPLAYMODE {
	UINT Width;
	UINT Height;
	UINT RefreshRate;
	D3DFORMAT Format;
} D3DDISPLAYMODE;

typedef struct _D3DADAPTER_IDENTIFIER8 {
	char Driver[512];
	char Description[512];
	LARGE_INTEGER DriverVersion;
	DWORD VendorId;
	DWORD DeviceId;
	DWORD SubSysId;
	DWORD Revision;
	GUID DeviceIdentifier;
	DWORD WHQLLevel;
} D3DADAPTER_IDENTIFIER8;

typedef struct _D3DGAMMARAMP {
	WORD red[256];
	WORD green[256];
	WORD blue[256];
} D3DGAMMARAMP;

typedef struct _D3DSURFACE_DESC {
	D3DFORMAT Format;
	D3DRESOURCETYPE Type;
	DWORD Usage;
	D3DPOOL Pool;
	UINT Size;
	D3DMULTISAMPLE_TYPE MultiSampleType;
	UINT Width;
	UINT Height;
} D3DSURFACE_DESC;

typedef struct _D3DLOCKED_RECT {
	INT Pitch;
	void *pBits;
} D3DLOCKED_RECT;

typedef struct _D3DPRESENT_PARAMETERS {
	UINT BackBufferWidth;
	UINT BackBufferHeight;
	D3DFORMAT BackBufferFormat;
	UINT BackBufferCount;
	D3DMULTISAMPLE_TYPE MultiSampleType;
	D3DSWAPEFFECT SwapEffect;
	HWND hDeviceWindow;
	BOOL Windowed;
	BOOL EnableAutoDepthStencil;
	D3DFORMAT AutoDepthStencilFormat;
	DWORD Flags;
	UINT FullScreen_RefreshRateInHz;
	UINT FullScreen_PresentationInterval;
} D3DPRESENT_PARAMETERS;

typedef struct _D3DCAPS8 {
	D3DDEVTYPE DeviceType;
	UINT AdapterOrdinal;
	DWORD Caps;
	DWORD Caps2;
	DWORD Caps3;
	DWORD PresentationIntervals;
	DWORD CursorCaps;
	DWORD DevCaps;
	DWORD PrimitiveMiscCaps;
	DWORD RasterCaps;
	DWORD ZCmpCaps;
	DWORD SrcBlendCaps;
	DWORD DestBlendCaps;
	DWORD AlphaCmpCaps;
	DWORD ShadeCaps;
	DWORD TextureCaps;
	DWORD TextureFilterCaps;
	DWORD CubeTextureFilterCaps;
	DWORD VolumeTextureFilterCaps;
	DWORD TextureAddressCaps;
	DWORD VolumeTextureAddressCaps;
	DWORD LineCaps;
	DWORD MaxTextureWidth, MaxTextureHeight;
	DWORD MaxVolumeExtent;
	DWORD MaxTextureRepeat;
	DWORD MaxTextureAspectRatio;
	DWORD MaxAnisotropy;
	float MaxVertexW;
	float GuardBandLeft;
	float GuardBandTop;
	float GuardBandRight;
	float GuardBandBottom;
	float ExtentsAdjust;
	DWORD StencilCaps;
	DWORD FVFCaps;
	DWORD TextureOpCaps;
	DWORD MaxTextureBlendStages;
	DWORD MaxSimultaneousTextures;
	DWORD VertexProcessingCaps;
	DWORD MaxActiveLights;
	DWORD MaxUserClipPlanes;
	DWORD MaxVertexBlendMatrices;
	DWORD MaxVertexBlendMatrixIndex;
	float MaxPointSize;
	DWORD MaxPrimitiveCount;
	DWORD MaxVertexIndex;
	DWORD MaxStreams;
	DWORD MaxStreamStride;
	DWORD VertexShaderVersion;
	DWORD MaxVertexShaderConst;
	DWORD PixelShaderVersion;
	float MaxPixelShaderValue;
} D3DCAPS8;

typedef struct _D3DXVECTOR3 {
	float x;
	float y;
	float z;
} D3DXVECTOR3;

typedef struct _D3DXVECTOR4 {
	float x;
	float y;
	float z;
	float w;
	_D3DXVECTOR4() : x(0), y(0), z(0), w(0) {}
	_D3DXVECTOR4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
	_D3DXVECTOR4(int _x, int _y, float _z, float _w) : x((float)_x), y((float)_y), z(_z), w(_w) {}
} D3DXVECTOR4;

typedef D3DMATRIX D3DXMATRIX;

// D3DX math helpers used by legacy shadow code
inline D3DXVECTOR4* D3DXVec4Transform(D3DXVECTOR4* pOut, const D3DXVECTOR4* pV, const D3DMATRIX* pM) {
	if (pOut) { pOut->x = pOut->y = pOut->z = pOut->w = 0.0f; }
	(void)pV; (void)pM; return pOut;
}
// REAL 4x4 inverse. This was an identity stub, which broke the road/noise/lightmap
// idiom "texture matrix = inv(view) * scale + camera-space-position texgen": with the
// stub the matrix carried no view at all, so uv = scale*(view*world) slid with the
// camera (field symptom #4b). A true inverse satisfies M*M^-1 = M^-1*M = I for the
// stored array layout (Matrix4::Row[4] of Vector4(X,Y,Z,W) aliases the row-major
// _11.._44/m[4][4] union), so the cancellation works in whatever composition order
// the caller and the Vulkan shader use. Singular -> identity (D3DX returns NULL;
// call sites here treat a NULL-return path as "leave matrix as fetched", so
// identity preserves the previous, crash-free behaviour).
inline D3DMATRIX* D3DXMatrixInverse(D3DMATRIX* pOut, float* pDeterminant, const D3DMATRIX* pM) {
	if (!pM || !pOut) {
		if (pOut) { pOut->_11 = pOut->_22 = pOut->_33 = pOut->_44 = 0; }
		if (pDeterminant) *pDeterminant = 0.0f;
		return pOut;
	}
	float a[4][8];
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) a[i][j] = pM->m[i][j];
		for (int j = 4; j < 8; j++) a[i][j] = (i == j - 4) ? 1.0f : 0.0f;
	}
	float det = 1.0f;
	for (int col = 0; col < 4; col++) {
		int piv = col; float best = a[col][col]; if (best < 0.0f) best = -best;
		for (int r = col + 1; r < 4; r++) { float v = a[r][col]; if (v < 0.0f) v = -v; if (v > best) { best = v; piv = r; } }
		if (best < 1e-6f) {
			if (pDeterminant) *pDeterminant = 0.0f;
			for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) pOut->m[i][j] = (i == j) ? 1.0f : 0.0f;
			return pOut;
		}
		if (piv != col) { for (int j = 0; j < 8; j++) { float t = a[col][j]; a[col][j] = a[piv][j]; a[piv][j] = t; } det = -det; }
		{
			float d = a[col][col];
			det *= d;
			for (int j = 0; j < 8; j++) a[col][j] /= d;
			for (int r = 0; r < 4; r++) if (r != col) {
				float f = a[r][col];
				if (f != 0.0f) for (int j = 0; j < 8; j++) a[r][j] -= f * a[col][j];
			}
		}
	}
	for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) pOut->m[i][j] = a[i][j + 4];
	if (pDeterminant) *pDeterminant = det;
	return pOut;
}
inline D3DMATRIX* D3DXMatrixIdentity(D3DMATRIX* pOut) {
	if (pOut) { pOut->_11 = pOut->_22 = pOut->_33 = pOut->_44 = 1; }
	return pOut;
}

#define D3DX_PI ((float)3.141592654f)
#define D3DX_DEFAULT ((UINT)-1)
#define D3DX_FILTER_NONE 1
#define D3DX_FILTER_POINT 2
#define D3DX_FILTER_LINEAR 3
#define D3DX_FILTER_TRIANGLE 4
#define D3DX_FILTER_BOX 5

// Opaque COM interface stubs (Vulkan backend replaces real D3D8 COM objects).
// Incomplete structs match existing forward declarations (struct IDirect3DSurface8;).
struct IDirect3DSurface8 {
	void Release() {}
	HRESULT GetDesc(D3DSURFACE_DESC* d) {
		if (d) { d->Width = 0; d->Height = 0; d->Format = D3DFMT_UNKNOWN; d->Pool = D3DPOOL_DEFAULT; d->Usage = 0; }
		return S_OK;
	}
};
struct IDirect3DTexture8 {
	void Release() {}
	HRESULT GetSurfaceLevel(UINT, IDirect3DSurface8**) { return E_FAIL; }
	void AddRef() {}
};
struct IDirect3DBaseTexture8 { void Release() {} void AddRef() {} };
struct IDirect3DVolumeTexture8;
struct IDirect3DVolume8;
struct IDirect3DCubeTexture8;
struct IDirect3DIndexBuffer8;
struct IDirect3D8 {
	HRESULT GetAdapterIdentifier(DWORD, DWORD, void*) { return E_FAIL; }
};

// Complete-enough stubs for legacy callers that Lock/Unlock/Create* on these
// Legacy-shadow adapter: real CPU-side buffers so the old device-API shadow
// code can create/lock/fill them (previously Create* returned E_FAIL and Lock
// returned null -> the whole shadow subsystem was silently disabled).
struct IDirect3DVertexBuffer8 {
	BYTE *pData;
	UINT Length;
	IDirect3DVertexBuffer8() : pData(nullptr), Length(0) {}
	~IDirect3DVertexBuffer8() { delete[] pData; }
	HRESULT Lock(UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags) {
		(void)Flags;
		if (!pData) return E_FAIL;
		if (ppbData) *ppbData = pData + OffsetToLock;
		return S_OK;
	}
	HRESULT Unlock() { return S_OK; }
	void Release() { delete this; }
};

struct IDirect3DDevice8;
typedef IDirect3DVertexBuffer8* LPDIRECT3DVERTEXBUFFER8;

// C-style shims (defined in dx8wrapper.cpp) so the device struct below can
// forward state calls without including dx8wrapper.h here.
extern "C" void DX8Wrapper_SetRenderState(unsigned state, unsigned value);
extern "C" unsigned DX8Wrapper_GetRenderState(unsigned state);
extern "C" void DX8Wrapper_SetTextureStageState(unsigned stage, unsigned type, unsigned value);

// Adapter (defined in dx8wrapper.cpp): legacy device-API forwarding for the
// shadow subsystem.
namespace D3D8Adapter {
	HRESULT CreateVertexBuffer(UINT Length, IDirect3DVertexBuffer8** ppVB);
	HRESULT CreateIndexBuffer(UINT Length, IDirect3DIndexBuffer8** ppIB);
	void SetIndices(IDirect3DIndexBuffer8 *ib, UINT baseVertex);
	void SetStreamSource(IDirect3DVertexBuffer8 *vb, UINT stride);
	void SetFVF(DWORD fvf);
	void SetWorldTransform(const void *matrix);	// WW-convention Matrix4*
	void DrawIndexedLegacy(D3DPRIMITIVETYPE type, INT base, INT numVerts,
		UINT startIndex, UINT primCount);	// (type, base, numVerts, startIdx, polys)
	void DrawIndexedD3D9(D3DPRIMITIVETYPE type, INT base, UINT minV, UINT numV,
		UINT startIndex, UINT primCount);
	void DrawPrimitiveUP(D3DPRIMITIVETYPE type, UINT primCount,
		const void *data, UINT stride);
}

struct IDirect3DDevice8 {
	HRESULT CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8** ppVB) {
		(void)Usage; (void)FVF; (void)Pool;
		return D3D8Adapter::CreateVertexBuffer(Length, ppVB);
	}
	HRESULT CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8** ppIB) {
		(void)Usage; (void)Format; (void)Pool;
		return D3D8Adapter::CreateIndexBuffer(Length, ppIB);
	}
	HRESULT SetIndices(IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex) {
		D3D8Adapter::SetIndices(pIndexData, BaseVertexIndex); return S_OK;
	}
	HRESULT SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer8* pStreamData, UINT Stride) {
		(void)StreamNumber; D3D8Adapter::SetStreamSource(pStreamData, Stride); return S_OK;
	}
	HRESULT SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) {
		DX8Wrapper_SetRenderState(State, Value); return S_OK;
	}
	HRESULT GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) {
		if (pValue) *pValue = DX8Wrapper_GetRenderState(State); return S_OK;
	}
	HRESULT SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) {
		DX8Wrapper_SetTextureStageState(Stage, Type, Value); return S_OK;
	}
	HRESULT SetTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* Matrix) {
		D3D8Adapter::SetWorldTransform(Matrix); return S_OK;
	}
	HRESULT SetVertexShader(DWORD Handle) { D3D8Adapter::SetFVF(Handle); return S_OK; }
	HRESULT SetTexture(DWORD Stage, void* Texture) { (void)Stage; (void)Texture; return S_OK; }
	HRESULT DrawIndexedPrimitive(D3DPRIMITIVETYPE t, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount) {
		D3D8Adapter::DrawIndexedD3D9(t, BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, PrimitiveCount); return S_OK;
	}
	// Legacy 5-arg form used by shadow code: (type, base, numVerts, startIndex, numPolys)
	HRESULT DrawIndexedPrimitive(D3DPRIMITIVETYPE t, INT base, INT numVerts, INT startIndex, INT primCount) {
		D3D8Adapter::DrawIndexedLegacy(t, base, numVerts, (UINT)startIndex, (UINT)primCount); return S_OK;
	}
	HRESULT DrawPrimitiveUP(D3DPRIMITIVETYPE t, UINT primCount, const void* data, UINT stride) {
		D3D8Adapter::DrawPrimitiveUP(t, primCount, data, stride); return S_OK; }
	HRESULT TestCooperativeLevel() { return S_OK; }
	HRESULT SetPixelShader(DWORD) { return S_OK; }
	HRESULT DeletePixelShader(DWORD) { return S_OK; }
	HRESULT SetPixelShaderConstant(UINT, const void*, UINT) { return S_OK; }
	HRESULT SetPixelShaderConstant(UINT, const D3DXVECTOR4&, UINT) { return S_OK; }
	HRESULT SetVertexShaderConstant(UINT, const void*, UINT) { return S_OK; }
	HRESULT SetVertexShaderConstant(UINT, const D3DXVECTOR4&, UINT) { return S_OK; }
	HRESULT CreateVertexShader(const DWORD*, const DWORD*, void*, DWORD) { return E_FAIL; }
	HRESULT CreateVertexShader(const void*, const void*, void*, DWORD) { return E_FAIL; }
	HRESULT CreateVertexShader(const DWORD*, const DWORD*, DWORD*, DWORD) { return E_FAIL; }
	HRESULT DeleteVertexShader(DWORD) { return S_OK; }
	HRESULT CreatePixelShader(const DWORD*, void*, DWORD) { return E_FAIL; }
	HRESULT CreatePixelShader(const DWORD*, unsigned long long*, DWORD) { return E_FAIL; }
	HRESULT CreatePixelShader(const void*, void*, DWORD) { return E_FAIL; }
	HRESULT CreatePixelShader(const void*, void**) { return E_FAIL; }
	HRESULT CreatePixelShader(const DWORD*, DWORD*, DWORD) { return E_FAIL; }
	HRESULT SetRenderTarget(IDirect3DSurface8*, IDirect3DSurface8*) { return S_OK; }
	HRESULT GetRenderTarget(IDirect3DSurface8**) { return E_FAIL; }
	HRESULT GetDepthStencilSurface(IDirect3DSurface8**) { return E_FAIL; }
	HRESULT CreateTexture(UINT, UINT, UINT, DWORD, D3DFORMAT, IDirect3DTexture8**) { return E_FAIL; }
	HRESULT CreateTexture(UINT, UINT, DWORD, D3DPOOL, IDirect3DTexture8**) { return E_FAIL; }
	HRESULT CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8** pp) {
		(void)Width; (void)Height; (void)Levels; (void)Usage; (void)Format; (void)Pool;
		if (pp) *pp = 0; return E_FAIL;
	}
	HRESULT ShowCursor(BOOL) { return S_OK; }
	HRESULT SetCursorProperties(UINT, UINT, IDirect3DSurface8*) { return S_OK; }
	HRESULT SetCursorPosition(int, int, DWORD) { return S_OK; }
	void Release() {}
};
#ifndef D3DCURSOR_IMMEDIATE_UPDATE
#define D3DCURSOR_IMMEDIATE_UPDATE 0x00000002
#endif

typedef IDirect3DDevice8* LPDIRECT3DDEVICE8;

struct IDirect3DIndexBuffer8 {
	WORD *pData;
	UINT Length;	// in bytes
	IDirect3DIndexBuffer8() : pData(nullptr), Length(0) {}
	~IDirect3DIndexBuffer8() { delete[] pData; }
	HRESULT Lock(UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags) {
		(void)Flags;
		if (!pData) return E_FAIL;
		if (ppbData) *ppbData = (BYTE*)pData + OffsetToLock;
		return S_OK;
	}
	HRESULT Unlock() { return S_OK; }
	void Release() { delete this; }
};

typedef IDirect3DSurface8* LPDIRECT3DSURFACE8;
typedef IDirect3DTexture8* LPDIRECT3DTEXTURE8;
typedef IDirect3DBaseTexture8* LPDIRECT3DBASETEXTURE8;
typedef IDirect3DVolumeTexture8* LPDIRECT3DVOLUMETEXTURE8;
typedef IDirect3DVolume8* LPDIRECT3DVOLUME8;
typedef IDirect3DCubeTexture8* LPDIRECT3DCUBETEXTURE8;
typedef IDirect3DIndexBuffer8* LPDIRECT3DINDEXBUFFER8;
typedef IDirect3D8* LPDIRECT3D8;

#ifndef D3DSHADE_FLAT
#define D3DSHADE_FLAT 1
#define D3DSHADE_GOURAUD 2
#define D3DSHADE_PHONG 3
#endif

#ifndef D3DRS_BLENDOP
#define D3DRS_BLENDOP 171
#endif
#ifndef D3DBLENDOP_ADD
#define D3DBLENDOP_ADD 1
#define D3DBLENDOP_SUBTRACT 2
#define D3DBLENDOP_REVSUBTRACT 3
#define D3DBLENDOP_MIN 4
#define D3DBLENDOP_MAX 5
#endif
#ifndef D3DLOCK_NO_DIRTY_UPDATE
#define D3DLOCK_NO_DIRTY_UPDATE 0x00002000
#endif
#ifndef D3DLOCK_READONLY
#define D3DLOCK_READONLY 0x00000010
#endif

#ifndef D3DVSD_STREAM
#define D3DVSD_END 0xFFFFFFFF
#define D3DVSD_STREAM(src) (0x40000000 | (src))
#define D3DVSD_REG(reg, type) (0x80000000 | ((reg) << 16) | (type))
#define D3DVSDT_NONE 0
#define D3DVSDT_FLOAT1 1
#define D3DVSDT_FLOAT2 2
#define D3DVSDT_FLOAT3 3
#define D3DVSDT_FLOAT4 4
#define D3DVSDT_D3DCOLOR 5
#define D3DVSDT_UBYTE4 6
#define D3DVSDT_SHORT2 7
#define D3DVSDT_SHORT4 8
#define D3DVSDT_UBYTE4N 9
#define D3DVSDT_SHORT2N 10
#define D3DVSDT_SHORT4N 11
#define D3DVSDT_USHORT2N 12
#define D3DVSDT_USHORT4N 13
#define D3DVSDT_UDEC3 14
#define D3DVSDT_DEC3N 15
#define D3DVSDT_FLOAT16_2 16
#define D3DVSDT_FLOAT16_4 17
#define D3DVSDE_POSITION 0
#define D3DVSDE_BLENDWEIGHT 1
#define D3DVSDE_BLENDMATRIX 2
#define D3DVSDE_NORMAL 3
#define D3DVSDE_POINTSIZE 4
#define D3DVSDE_TEXTURE5 15
#define D3DVSDE_TEXTURE6 14
#define D3DVSDE_TEXTURE7 13
#define D3DVSDE_TEXTURE8 12
#define D3DVSDE_TEXTURE9 11
#define D3DVSDE_DIFFUSE 10
#define D3DVSDE_SPECULAR 9
#endif
#ifndef D3DTSS_ADDRESSW
#define D3DTSS_ADDRESSW 13
#endif

// REAL scaling (was half-written: only 4 fields set, other 12 UNINITIALIZED).
inline D3DMATRIX* D3DXMatrixScaling(D3DMATRIX* pOut, float sx, float sy, float sz) {
	if (pOut) {
		for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) pOut->m[i][j] = (i == j) ? 1.0f : 0.0f;
		pOut->_11 = sx; pOut->_22 = sy; pOut->_33 = sz; pOut->_44 = 1;
	}
	return pOut;
}
// REAL translation (was half-written: only 4 fields set, other 12 UNINITIALIZED).
inline D3DMATRIX* D3DXMatrixTranslation(D3DMATRIX* pOut, float x, float y, float z) {
	if (pOut) {
		for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) pOut->m[i][j] = (i == j) ? 1.0f : 0.0f;
		pOut->_41 = x; pOut->_42 = y; pOut->_43 = z; pOut->_44 = 1;
	}
	return pOut;
}
// REAL 4x4 product (the stub returned a and silently DROPPED b - every caller of
// this was getting a copy of the left operand). pOut may alias a or b, so both
// operands are read into locals first. m[i][j] over the D3DMATRIX union reads
// Matrix4::Row[4] (Vector4{X,Y,Z,W}) memory correctly - 16 row-major floats.
inline D3DMATRIX* D3DXMatrixMultiply(D3DMATRIX* pOut, const D3DMATRIX* a, const D3DMATRIX* b) {
	if (a && b && pOut) {
		float x[4][4], y[4][4];
		for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) { x[i][j] = a->m[i][j]; y[i][j] = b->m[i][j]; }
		for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) {
			float v = x[i][0]*y[0][j] + x[i][1]*y[1][j] + x[i][2]*y[2][j] + x[i][3]*y[3][j];
			pOut->m[i][j] = v;
		}
	}
	else if (pOut) { for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) pOut->m[i][j] = (i == j) ? 1.0f : 0.0f; }
	return pOut;
}

#ifndef D3DWRAP_V
#define D3DWRAP_V 0x00002000
#define D3DWRAP_U 0x00001000
#define D3DWRAPCOORD_0 1
#define D3DWRAPCOORD_1 2
#define D3DWRAPCOORD_2 4
#define D3DWRAPCOORD_3 8
#endif
inline D3DMATRIX* D3DXMatrixTranspose(D3DMATRIX* pOut, const D3DMATRIX* pM) {
	if (pOut && pM) { for (int i=0;i<4;i++) for (int j=0;j<4;j++) { float tmp = pM->m[i][j]; pOut->m[j][i] = tmp; } }
	return pOut;
}

inline D3DMATRIX operator*(const D3DMATRIX& a, const D3DMATRIX& b) {
	D3DMATRIX r;
	for (int i=0;i<4;i++) for (int j=0;j<4;j++) {
		r.m[i][j] = a.m[0][j]*b.m[i][0] + a.m[1][j]*b.m[i][1] + a.m[2][j]*b.m[i][2] + a.m[3][j]*b.m[i][3];
	}
	return r;
}
inline D3DMATRIX& operator*=(D3DMATRIX& a, const D3DMATRIX& b) {
	a = a * b;
	return a;
}

#endif // DX8COMPAT_D3D8TYPES_H
#define D3DRS_WRAP0 142
