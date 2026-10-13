#ifndef _DDRAW_H_
#define _DDRAW_H_

#include <windows.h>
#include <stdint.h>

// ============================================================
// DirectDraw types (stub - replaced by Vulkan/DX8Compat)
// ============================================================

typedef struct DDPIXELFORMAT {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwFourCC;
    DWORD dwRGBBitCount;
    DWORD dwRBitMask;
    DWORD dwGBitMask;
    DWORD dwBBitMask;
    DWORD dwRGBAlphaBitMask;
} DDPIXELFORMAT;

typedef struct DDSCAPS {
    DWORD dwCaps;
} DDSCAPS;

typedef struct DDSCAPS2 {
    DWORD dwCaps;
    DWORD dwCaps2;
    DWORD dwCaps3;
    union {
        DWORD dwCaps4;
        DWORD dwVolumeDepth;
    };
} DDSCAPS2;

typedef struct DDSCAPS2_orig {
    DWORD dwCaps;
    DWORD dwCaps2;
    DWORD dwCaps3;
    DWORD dwCaps4;
} DDSCAPS2_orig;

typedef struct DDCOLORKEY {
    DWORD dwColorSpaceLowValue;
    DWORD dwColorSpaceHighValue;
} DDCOLORKEY;

typedef struct DDSURFACEDESC {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwHeight;
    DWORD dwWidth;
    LONG lPitch;
    DWORD dwBackBufferCount;
    DWORD dwMipMapCount;
    DWORD dwAlphaBitDepth;
    DWORD dwReserved;
    LPVOID lpSurface;
    DDCOLORKEY ddckCKDestOverlay;
    DDCOLORKEY ddckCKDestBlt;
    DDCOLORKEY ddckCKSrcOverlay;
    DDCOLORKEY ddckCKSrcBlt;
    DDPIXELFORMAT ddpfPixelFormat;
    DWORD dwCaps;
    DWORD dwCaps2;
    DDSCAPS2 ddsCaps;
    DWORD dwTextureStage;
} DDSURFACEDESC;

typedef struct DDBLTFX {
    DWORD dwSize;
    DWORD dwDDFX;
    DWORD dwROP;
    DWORD dwRotationAngle;
    DWORD dwZBufferBaseDest;
    DWORD dwZBufferTop;
    DWORD dwZBufferBottom;
    DWORD dwZBufferBaseSrc;
    DWORD dwZBufferTopSrc;
    DWORD dwZBufferBottomSrc;
    DWORD dwAlphaEdgeBlendBitDepth;
    DWORD dwAlphaEdgeBlend;
    DWORD dwReserved;
    DWORD dwAlphaDestConstBitDepth;
    DWORD dwAlphaDestConst;
    DWORD dwAlphaSrcConstBitDepth;
    DWORD dwAlphaSrcConst;
    DWORD dwFillColor;
    DWORD dwFillColor2;
    DWORD dwReserved2;
} DDBLTFX;

typedef struct DDGAMMARAMP {
    WORD red[256];
    WORD green[256];
    WORD blue[256];
} DDGAMMARAMP;

typedef struct DDOVERLAYFX {
    DWORD dwSize;
    DWORD dwAlphaEdgeBlendBitDepth;
    DWORD dwAlphaEdgeBlend;
    DWORD dwReserved;
    DWORD dwAlphaDestConstBitDepth;
    DWORD dwAlphaDestConst;
    DWORD dwAlphaSrcConstBitDepth;
    DWORD dwAlphaSrcConst;
    DDCOLORKEY dckDestColorkey;
    DDCOLORKEY dckSrcColorkey;
    DWORD dwDDFX;
    DWORD dwFlags;
} DDOVERLAYFX;

typedef struct DDCLIPPERDESC {
    DWORD dwSize;
    DWORD dwFlags;
    HWND hWnd;
    DWORD dwCount;
    RECT *prc;
} DDCLIPPERDESC;

typedef struct DDCOLORCONTROL {
    DWORD dwSize;
    DWORD dwFlags;
    LONG lBrightness;
    LONG lContrast;
    LONG lHue;
    LONG lSaturation;
    LONG lSharpness;
    LONG lGamma;
    LONG lColorEnable;
    DWORD dwReserved1;
} DDCOLORCONTROL;

typedef struct DDOVERLAYCAPS {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwColorSpaceLowValue;
    DWORD dwColorSpaceHighValue;
} DDOVERLAYCAPS;

typedef struct DDGAMMARAMP DDGAMMARAMP2;

typedef struct DDDEVICEIDENTIFIER {
    char szDriver[MAX_PATH];
    char szDescription[MAX_PATH];
    char szDeviceName[MAX_PATH];
    GUID guidDeviceIdentifier;
    DWORD dwWHQLLevel;
} DDDEVICEIDENTIFIER;

// Forward declarations
struct IDirectDraw;
struct IDirectDrawSurface;
struct IDirectDrawPalette;
struct IDirectDrawClipper;
struct IDirectDraw2;
struct IDirectDrawSurface2;
struct IDirectDrawSurface7;
struct IDirectDrawColorControl;
struct IDirectDrawGammaControl;

typedef IDirectDraw* LPDIRECTDRAW;
typedef IDirectDrawSurface* LPDIRECTDRAWSURFACE;
typedef IDirectDrawPalette* LPDIRECTDRAWPALETTE;
typedef IDirectDrawClipper* LPDIRECTDRAWCLIPPER;
typedef IDirectDraw2* LPDIRECTDRAW2;
typedef IDirectDrawSurface2* LPDIRECTDRAWSURFACE2;
typedef IDirectDrawSurface7* LPDIRECTDRAWSURFACE7;
typedef IDirectDrawColorControl* LPDIRECTDRAWCOLORCONTROL;
typedef IDirectDrawGammaControl* LPDIRECTDRAWGAMMACONTROL;

// DirectDraw error codes
#define DD_OK 0
#define DD_FALSE ((HRESULT)S_FALSE)
#define DDERR_ALREADYINITIALIZED ((HRESULT)0x8876017CL)
#define DDERR_INVALIDPARAMS ((HRESULT)0x80070057L)
#define DDERR_INVALIDDIRECTDRAWGUID ((HRESULT)0x88760001L)
#define DDERR_NOCOOPERATIVELEVELSET ((HRESULT)0x88760182L)
#define DDERR_SURFACELOST ((HRESULT)0x887601A2L)
#define DDERR_GENERIC ((HRESULT)0x80004005L)
#define DDERR_OUTOFMEMORY ((HRESULT)0x8007000EL)
#define DDERR_UNSUPPORTED ((HRESULT)0x80004001L)
#define DDERR_NOTINITIALIZED ((HRESULT)0x88760186L)
#define DDERR_ALREADYCREATED ((HRESULT)0x8876017CL)
#define DDERR_DIRECTDRAWALREADYCREATED ((HRESULT)0x88760190L)
#define DDERR_PRIMARYSURFACEALREADYEXISTS ((HRESULT)0x88760194L)
#define DDERR_NOCOOPERATIVELEVELSET ((HRESULT)0x88760182L)
#define DDERR_NOEXCLUSIVEMODE ((HRESULT)0x887601A0L)
#define DDERR_HWNDALREADYSET ((HRESULT)0x88760196L)
#define DDERR_HWNDSUBCLASSED ((HRESULT)0x88760198L)
#define DDERR_HWNDNOTSET ((HRESULT)0x8876019AL)
#define DDERR_NOHWND ((HRESULT)0x8876019CL)
#define DDERR_CLIPPERISUSINGHWND ((HRESULT)0x887601A6L)
#define DDERR_NOCLIPPERATTACHED ((HRESULT)0x887601A8L)
#define DDERR_NOCLIPLIST ((HRESULT)0x887601AAL)
#define DDERR_INVALIDCLIPLIST ((HRESULT)0x887601ACL)
#define DDERR_UNSUPPORTEDFORMAT ((HRESULT)0x887601C0L)
#define DDERR_UNSUPPORTEDMASK ((HRESULT)0x887601C2L)
#define DDERR_INVALIDCLIPLIST ((HRESULT)0x887601ACL)
#define DDERR_CANNOTATTACHSURFACE ((HRESULT)0x887601C8L)
#define DDERR_CANNOTDETACHSURFACE ((HRESULT)0x887601CAL)
#define DDERR_SURFACEBUSY ((HRESULT)0x887601B4L)
#define DDERR_SURFACEISOBSCURED ((HRESULT)0x887601B8L)
#define DDERR_SURFACELOST ((HRESULT)0x887601A2L)
#define DDERR_SURFACENOTATTACHED ((HRESULT)0x887601B6L)
#define DDERR_TOOBIGHEIGHT ((HRESULT)0x887601B0L)
#define DDERR_TOOBIGSIZE ((HRESULT)0x887601B2L)
#define DDERR_TOOBIGWIDTH ((HRESULT)0x887601AEL)
#define DDERR_UNSUPPORTED ((HRESULT)0x80004001L)
#define DDERR_WASSTILLDRAWING ((HRESULT)0x887601BE)
#define DDERR_INVALIDRECT ((HRESULT)0x887601B6L)
#define DDERR_INVALIDPIXELFORMAT ((HRESULT)0x887601D8L)
#define DDERR_COLORKEYNOTSET ((HRESULT)0x88760216L)
#define DDERR_NOCOLORKEY ((HRESULT)0x88760210L)
#define DDERR_NOCOLORKEYHW ((HRESULT)0x88760212L)
#define DDERR_NOTLOCKED ((HRESULT)0x88760214L)
#define DDERR_IMPLICITLYCREATED ((HRESULT)0x88760194L)
#define DDERR_NOTPALETTIZED ((HRESULT)0x887601D0L)
#define DDERR_UNSUPPORTEDMODE ((HRESULT)0x887601D2L)
#define DDERR_NOMIPMAPHW ((HRESULT)0x887601D4L)
#define DDERR_INVALIDCAPS ((HRESULT)0x887601C6L)
#define DDERR_NOCLIPLIST ((HRESULT)0x887601AAL)
#define DDERR_NOBLTHW ((HRESULT)0x887601DE)
#define DDERR_NODDROPSHW ((HRESULT)0x887601E0)
#define DDERR_OVERLAYCANTCLIP ((HRESULT)0x887601E2)
#define DDERR_OVERLAYCOLORKEYONLYONEACTIVE ((HRESULT)0x887601E4)
#define DDERR_PALETTEBUSY ((HRESULT)0x887601E6)
#define DDERR_COLORKEYONLYONEACTIVE ((HRESULT)0x887601E8)
#define DDERR_CURRENTLYNOTAVAIL ((HRESULT)0x887601EA)
#define DDERR_HDCALREADYEXISTS ((HRESULT)0x887601EC)
#define DDERR_NOTDOWNSAMPLINGHW ((HRESULT)0x887601EE)
#define DDERR_NOFOCUSWINDOW ((HRESULT)0x887601F0)
#define DDERR_NOTONMIPMAPSUBLEVEL ((HRESULT)0x887601F2)
#define DDERR_NOOPTIMIZEHW ((HRESULT)0x887601F4)
#define DDERR_NOTPAGELOCKED ((HRESULT)0x887601F6)
#define DDERR_NOTPAGELOCKED ((HRESULT)0x887601F6)
#define DDERR_NOPALETTEATTACHED ((HRESULT)0x887601F8)
#define DDERR_NOPALETTEHW ((HRESULT)0x887601FA)
#define DDERR_NOZBUFFERHW ((HRESULT)0x887601FC)
#define DDERR_NOZOVERLAYHW ((HRESULT)0x887601FE)
#define DDERR_ROZFAST ((HRESULT)0x88760200)
#define DDERR_INVALIDSURFACETYPE ((HRESULT)0x88760202)
#define DDERR_NOOPTIMIZEHW ((HRESULT)0x887601F4)
#define DDERR_NOTLOADED ((HRESULT)0x88760204)
#define DDERR_NOFOCUSWINDOW ((HRESULT)0x887601F0)
#define DDERR_NOTONMIPMAPSUBLEVEL ((HRESULT)0x887601F2)
#define DDERR_DCALREADYCREATED ((HRESULT)0x88760206)
#define DDERR_NONONLOCALVIDMEM ((HRESULT)0x88760208)
#define DDERR_CANTPAGELOCK ((HRESULT)0x8876020A)
#define DDERR_CANTPAGEUNLOCK ((HRESULT)0x8876020C)
#define DDERR_NOTPAGELOCKED ((HRESULT)0x887601F6)
#define DDERR_MOREDATA ((HRESULT)0x8876020E)
#define DDERR_EXPIRED ((HRESULT)0x88760210)
#define DDERR_TESTFINISHED ((HRESULT)0x88760212)
#define DDERR_NEWMODE ((HRESULT)0x88760214)
#define DDERR_D3DNOTINITIALIZED ((HRESULT)0x88760216)
#define DDERR_VIDEONOTACTIVE ((HRESULT)0x88760218)
#define DDERR_NOMONITORINFORMATION ((HRESULT)0x8876021A)
#define DDERR_NODRIVERSUPPORT ((HRESULT)0x8876021C)
#define DDERR_DEVICEDOESNTOWNSURFACE ((HRESULT)0x8876021E)
#define DDERR_NOTINITIALIZED ((HRESULT)0x88760186)
#define DDERR_CURRENTLYNOTAVAIL ((HRESULT)0x887601EA)
#define DDERR_INVALIDDIRECTDRAWGUID ((HRESULT)0x88760001)
#define DDERR_DIRECTDRAWALREADYCREATED ((HRESULT)0x88760190)
#define DDERR_NODIRECTDRAWHW ((HRESULT)0x88760192)
#define DDERR_PRIMARYSURFACEALREADYEXISTS ((HRESULT)0x88760194)
#define DDERR_NOEMULATION ((HRESULT)0x88760196)
#define DDERR_REGIONTOOSMALL ((HRESULT)0x88760198)
#define DDERR_CLIPPERISUSINGHWND ((HRESULT)0x887601A6)
#define DDERR_NOCLIPLISTATTACHED ((HRESULT)0x887601A8)
#define DDERR_NOCLIPPERATTACHED ((HRESULT)0x887601AA)
#define DDERR_NOCOLORKEY ((HRESULT)0x88760210)
#define DDERR_INCOMPATIBLEPRIMARY ((HRESULT)0x88760212)
#define DDERR_INVALIDCAPS ((HRESULT)0x887601C6)
#define DDERR_NOCOOPERATIVELEVELSET ((HRESULT)0x88760182)
#define DDERR_NOTLOADED ((HRESULT)0x88760204)
#define DDERR_OUTOFMEMORY ((HRESULT)0x8007000E)
#define DDERR_INVALIDDIRECTDRAWGUID ((HRESULT)0x88760001)
#define DDERR_UNSUPPORTED ((HRESULT)0x80004001)
#define DDERR_GENERIC ((HRESULT)0x80004005)
#define DDERR_INVALIDPARAMS ((HRESULT)0x80070057)

// Simplified error codes - just use the ones actually needed
#undef DDERR_INVALIDDIRECTDRAWGUID
#define DDERR_INVALIDDIRECTDRAWGUID ((HRESULT)0x88760001)
#undef DDERR_NOCOOPERATIVELEVELSET
#define DDERR_NOCOOPERATIVELEVELSET ((HRESULT)0x88760182)

// Coop level flags
#define DDSCL_FULLSCREEN 0x1
#define DDSCL_NORMAL 0x2
#define DDSCL_EXCLUSIVE 0x10
#define DDSCL_ALLOWREBOOT 0x20
#define DDSCL_NOWINDOWCHANGES 0x40
#define DDSCL_ALLOWMODEX 0x40
#define DDSCL_SETFOCUSWINDOW 0x80
#define DDSCL_SETDEVICEWINDOW 0x100
#define DDSCL_CREATEDEVICEWINDOW 0x200
#define DDSCL_MULTITHREADED 0x400
#define DDSCL_FPUSETUP 0x800

// Surface caps
#define DDSCAPS_PRIMARYSURFACE 0x200
#define DDSCAPS_FLIP 0x10
#define DDSCAPS_BACKBUFFER 0x4
#define DDSCAPS_COMPLEX 0x8
#define DDSCAPS_OFFSCREENPLAIN 0x40
#define DDSCAPS_SYSTEMMEMORY 0x800
#define DDSCAPS_VIDEOMEMORY 0x4000
#define DDSCAPS_VISIBLE 0x10000
#define DDSCAPS_ZBUFFER 0x2000000
#define DDSCAPS_MIPMAP 0x400000
#define DDSCAPS_LOCALVIDMEM 0x10000000
#define DDSCAPS_NONLOCALVIDMEM 0x20000000
#define DDSCAPS_PRIMARYSURFACELEFT 0x20
#define DDSCAPS_OWNDC 0x40000000
#define DDSCAPS_OWNDC DDSCAPS_OWNDC
#define DDSCAPS_MODEX 0x80000000

// Surface desc flags
#define DDSD_CAPS 0x1
#define DDSD_HEIGHT 0x2
#define DDSD_WIDTH 0x4
#define DDSD_PITCH 0x8
#define DDSD_BACKBUFFERCOUNT 0x20
#define DDSD_ZBUFFERBITDEPTH 0x40
#define DDSD_ALPHABITDEPTH 0x80
#define DDSD_LPSURFACE 0x800
#define DDSD_PIXELFORMAT 0x1000
#define DDSD_CKDESTOVERLAY 0x2000
#define DDSD_CKDESTBLT 0x4000
#define DDSD_CKSRCOVERLAY 0x8000
#define DDSD_CKSRCBLT 0x10000
#define DDSD_MIPMAPCOUNT 0x20000
#define DDSD_REFRESHRATE 0x40000
#define DDSD_LINEARSIZE 0x80000
#define DDSD_TEXTURESTAGE 0x100000
#define DDSD_FVF 0x200000
#define DDSD_SRCVBHANDLE 0x400000
#define DDSD_ALL 0xFFF9EE

// Palette caps
#define DDPCAPS_8BIT 0x4
#define DDPCAPS_ALLOW256 0x80
#define DDPCAPS_INITIALIZE 0x1
#define DDPCAPS_1BIT 0x1
#define DDPCAPS_2BIT 0x2
#define DDPCAPS_4BIT 0x4
#define DDPCAPS_8BIT 0x8
#define DDPCAPS_PRIMARYSURFACE 0x10
#define DDPCAPS_PRIMARYSURFACELEFT 0x20
#define DDPCAPS_BACKBUFFER 0x40
#define DDPCAPS_ALLOW256 0x80
#define DDPCAPS_VSYNC 0x100
#define DDPCAPS_SRGBLINRAMP 0x200

// Blt flags
#define DDBLT_WAIT 0x1000000
#define DDBLT_COLORFILL 0x400
#define DDBLT_KEYSRC 0x8000
#define DDBLT_ASYNC 0x200
#define DDBLT_DONOTWAIT 0x8000000

// Flip flags
#define DDFLIP_WAIT 0x1
#define DDFLIP_DONOTWAIT 0x40000000
#define DDFLIP_EVEN 0x2
#define DDFLIP_ODD 0x4

// Overlay flags
#define DDOVER_SHOW 0x1
#define DDOVER_HIDE 0x2
#define DDOVER_KEYDEST 0x400
#define DDOVER_KEYSRC 0x800
#define DDOVER_WAIT 0x1000

// Lock flags
#define DDLOCK_SURFACEMEMORYPTR 0x0
#define DDLOCK_WAIT 0x1
#define DDLOCK_EVENT 0x2
#define DDLOCK_READONLY 0x10
#define DDLOCK_WRITEONLY 0x20
#define DDLOCK_NOSYSLOCK 0x800
#define DDLOCK_NOOVERWRITE 0x1000
#define DDLOCK_DISCARDCONTENTS 0x2000

// Caps bits
#define DDCAPS_BLT 0x10
#define DDCAPS_BLTQUEUE 0x20
#define DDCAPS_BLTFOURCC 0x40
#define DDCAPS_BLTSTRETCH 0x80
#define DDCAPS_GDI 0x100
#define DDCAPS_OVERLAY 0x200
#define DDCAPS_OVERLAYCANTCLIP 0x400
#define DDCAPS_OVERLAYFOURCC 0x800
#define DDCAPS_OVERLAYSTRETCH 0x1000
#define DDCAPS_PALETTE 0x2000
#define DDCAPS_PALETTEVSYNC 0x4000
#define DDCAPS_READSCANLINE 0x8000
#define DDCAPS_VBI 0x10000
#define DDCAPS_ZBLTS 0x20000
#define DDCAPS_ZOVERLAYS 0x40000
#define DDCAPS_COLORKEY 0x100000
#define DDCAPS_ALPHA 0x200000
#define DDCAPS_COLORKEYHWASSIST 0x400000
#define DDCAPS_NOHARDWARE 0x800000
#define DDCAPS_BLTCOLORFILL 0x1000000
#define DDCAPS_BANKSWITCHED 0x2000000
#define DDCAPS_BLTDEPTHFILL 0x4000000
#define DDCAPS_CANCLIP 0x8000000
#define DDCAPS_CANCLIPSTRETCHED 0x10000000
#define DDCAPS_CANBLTSYSMEM 0x20000000

// DDCAPS struct
typedef struct DDCAPS {
    DWORD dwSize;
    DWORD dwCaps;
    DWORD dwCaps2;
    DWORD dwCKeyCaps;
    DWORD dwFXCaps;
    DWORD dwFXAlphaCaps;
    DWORD dwPalCaps;
    DWORD dwSVCaps;
    DWORD dwAlphaBltConstBitDepths;
    DWORD dwAlphaBltPixelBitDepths;
    DWORD dwAlphaBltSurfaceBitDepths;
    DWORD dwAlphaOverlayConstBitDepths;
    DWORD dwAlphaOverlayPixelBitDepths;
    DWORD dwAlphaOverlaySurfaceBitDepths;
    DWORD dwZBufferBitDepths;
    DWORD dwVidMemTotal;
    DWORD dwVidMemFree;
    DWORD dwMaxVisibleOverlays;
    DWORD dwCurrVisibleOverlays;
    DWORD dwNumFourCCCodes;
    DWORD dwAlignBoundarySrc;
    DWORD dwAlignSizeSrc;
    DWORD dwAlignBoundaryDest;
    DWORD dwAlignSizeDest;
    DWORD dwAlignStrideAlign;
    DWORD dwRops[8];
    DWORD dwMinOverlayStretch;
    DWORD dwMaxOverlayStretch;
    DWORD dwMinLiveVideoStretch;
    DWORD dwMaxLiveVideoStretch;
    DWORD dwMinHwCodecStretch;
    DWORD dwMaxHwCodecStretch;
    DWORD dwReserved1;
    DWORD dwReserved2;
    DWORD dwReserved3;
    DWORD dwSVBCaps;
    DWORD dwSVBCKeyCaps;
    DWORD dwSVBFXCaps;
    DWORD dwSVBRops[8];
    DWORD dwVSBCaps;
    DWORD dwVSBCKeyCaps;
    DWORD dwVSBFXCaps;
    DWORD dwVSBRops[8];
    DWORD dwSSBCaps;
    DWORD dwSSBCKeyCaps;
    DWORD dwSSBFXCaps;
    DWORD dwSSBRops[8];
    DWORD dwMaxVideoPorts;
    DWORD dwCurrVideoPorts;
    DWORD dwSVBCaps2;
} DDCAPS;

// IDirectDraw stub interface
struct IDirectDraw {
    HRESULT SetCooperativeLevel(HWND hwnd, DWORD flags) { (void)hwnd; (void)flags; return S_OK; }
    HRESULT SetDisplayMode(DWORD w, DWORD h, DWORD bpp) { (void)w; (void)h; (void)bpp; return S_OK; }
    HRESULT CreatePalette(DWORD flags, LPPALETTEENTRY entries, LPDIRECTDRAWPALETTE *out, void *outer) { (void)flags; (void)entries; (void)out; (void)outer; return S_OK; }
    HRESULT GetCaps(DDCAPS *caps, DDCAPS *helcaps) { (void)caps; (void)helcaps; return S_OK; }
    HRESULT QueryInterface(REFIID riid, LPVOID *obj) { (void)riid; *obj = NULL; return E_NOINTERFACE; }
    HRESULT GetDisplayMode(void *desc) { (void)desc; return S_OK; }
    ULONG Release() { return 0; }
    HRESULT CreateSurface(void *desc, LPDIRECTDRAWSURFACE *out, void *outer) { (void)desc; (void)out; (void)outer; return S_OK; }
    HRESULT EnumModes(DWORD flags, void *cb, void *ctx) { (void)flags; (void)cb; (void)ctx; return S_OK; }
    HRESULT GetMonitorFrequency(void *freq) { (void)freq; return S_OK; }
    HRESULT WaitForVerticalBlank(DWORD flags, void *event) { (void)flags; (void)event; return S_OK; }
    HRESULT RestoreDisplayMode() { return S_OK; }
    HRESULT GetVerticalBlankStatus(BOOL *status) { *status = FALSE; return S_OK; }
    HRESULT GetSurfaceCount(DWORD *count) { *count = 0; return S_OK; }
    HRESULT CreateClipper(DWORD flags, LPDIRECTDRAWCLIPPER *out, void *outer) { (void)flags; (void)out; (void)outer; return S_OK; }
    HRESULT DuplicateSurface(LPDIRECTDRAWSURFACE src, LPDIRECTDRAWSURFACE *dst) { (void)src; (void)dst; return E_NOTIMPL; }
    HRESULT GetDisplayModeEx(void *mode) { (void)mode; return E_NOTIMPL; }
    HRESULT GetGDISurface(LPDIRECTDRAWSURFACE *surface) { (void)surface; return E_NOTIMPL; }
    HRESULT GetMonitorFrequency2(void *freq) { (void)freq; return E_NOTIMPL; }
    HRESULT GetVerticalBlankStatus2(BOOL *status) { (void)status; return E_NOTIMPL; }
    HRESULT WaitForVerticalBlank2(DWORD flags, void *event) { (void)flags; (void)event; return E_NOTIMPL; }
};

// IDirectDrawSurface stub interface
struct IDirectDrawSurface {
    HRESULT AddAttachedSurface(void *surf) { (void)surf; return E_NOTIMPL; }
    HRESULT Blt(RECT *dst, IDirectDrawSurface *src, RECT *srcRect, DWORD flags, void *fx) { (void)dst; (void)src; (void)srcRect; (void)flags; (void)fx; return S_OK; }
    HRESULT BltFast(DWORD x, DWORD y, IDirectDrawSurface *src, RECT *srcRect, DWORD flags) { (void)x; (void)y; (void)src; (void)srcRect; (void)flags; return S_OK; }
    HRESULT DeleteAttachedSurface(DWORD flags, void *surf) { (void)flags; (void)surf; return S_OK; }
    HRESULT EnumAttachedSurfaces(LPVOID ctx, void *cb) { (void)ctx; (void)cb; return S_OK; }
    HRESULT EnumOverlayZOrders(DWORD flags, LPVOID ctx, void *cb) { (void)flags; (void)ctx; (void)cb; return S_OK; }
    HRESULT Flip(LPDIRECTDRAWSURFACE dest, DWORD flags) { (void)dest; (void)flags; return S_OK; }
    HRESULT GetAttachedSurface(void *caps, LPDIRECTDRAWSURFACE *out) { (void)caps; *out = NULL; return S_OK; }
    HRESULT GetBltStatus(DWORD flags) { (void)flags; return S_OK; }
    HRESULT GetCaps(DDCAPS *caps) { (void)caps; return S_OK; }
    HRESULT GetClipper(LPDIRECTDRAWCLIPPER *out) { (void)out; return S_OK; }
    HRESULT GetColorKey(DWORD flags, DDCOLORKEY *key) { (void)flags; (void)key; return S_OK; }
    HRESULT GetDC(HDC *out) { (void)out; return S_OK; }
    HRESULT GetFlipStatus(DWORD flags) { (void)flags; return S_OK; }
    HRESULT GetOverlayPosition(LONG *x, LONG *y) { (void)x; (void)y; return S_OK; }
    HRESULT GetPalette(LPDIRECTDRAWPALETTE *out) { (void)out; return S_OK; }
    HRESULT GetPixelFormat(DDPIXELFORMAT *fmt) { (void)fmt; return S_OK; }
    HRESULT GetSurfaceDesc(DDSURFACEDESC *desc) { (void)desc; return S_OK; }
    HRESULT IsLost() { return S_OK; }
    HRESULT Lock(RECT *rect, DDSURFACEDESC *desc, DWORD flags, HANDLE event) { (void)rect; (void)desc; (void)flags; (void)event; return S_OK; }
    HRESULT ReleaseDC(HDC hdc) { (void)hdc; return S_OK; }
    HRESULT Restore() { return S_OK; }
    HRESULT SetClipper(LPDIRECTDRAWCLIPPER clipper) { (void)clipper; return S_OK; }
    HRESULT SetColorKey(DWORD flags, DDCOLORKEY *key) { (void)flags; (void)key; return S_OK; }
    HRESULT SetOverlayPosition(LONG x, LONG y) { (void)x; (void)y; return S_OK; }
    HRESULT SetPalette(LPDIRECTDRAWPALETTE palette) { (void)palette; return S_OK; }
    HRESULT Unlock(LPVOID rect) { (void)rect; return S_OK; }
    HRESULT UpdateOverlay(RECT *src, IDirectDrawSurface *dst, RECT *dstRect, DWORD flags, void *fx) { (void)src; (void)dst; (void)dstRect; (void)flags; (void)fx; return S_OK; }
    HRESULT UpdateOverlayDisplay(DWORD flags) { (void)flags; return S_OK; }
    HRESULT UpdateOverlayZOrder(DWORD flags, void *ref) { (void)flags; (void)ref; return S_OK; }
    HRESULT Release() { return 0; }
    HRESULT QueryInterface(REFIID riid, LPVOID *obj) { (void)riid; *obj = NULL; return E_NOINTERFACE; }
    HRESULT SetSurfaceDesc(DDSURFACEDESC *desc, DWORD flags) { (void)desc; (void)flags; return S_OK; }
    HRESULT SetPriority(DWORD priority) { (void)priority; return S_OK; }
    HRESULT GetPriority(DWORD *priority) { (void)priority; return S_OK; }
    HRESULT SetLOD(DWORD lod) { (void)lod; return S_OK; }
    HRESULT GetLOD(DWORD *lod) { (void)lod; return S_OK; }
    HRESULT AddOverlayDirtyRect(RECT *rect) { (void)rect; return S_OK; }
    HRESULT GetDC2(HDC *out) { (void)out; return S_OK; }
    HRESULT PageLock(DWORD flags) { (void)flags; return S_OK; }
    HRESULT PageUnlock(DWORD flags) { (void)flags; return S_OK; }
};

// IDirectDrawPalette stub interface
struct IDirectDrawPalette {
    HRESULT GetCaps(DWORD *caps) { (void)caps; return S_OK; }
    HRESULT GetEntries(DWORD flags, DWORD base, DWORD num, LPPALETTEENTRY entries) { (void)flags; (void)base; (void)num; (void)entries; return S_OK; }
    HRESULT Initialize(void *dd, DWORD flags, LPPALETTEENTRY entries) { (void)dd; (void)flags; (void)entries; return S_OK; }
    HRESULT SetEntries(DWORD flags, DWORD base, DWORD num, LPPALETTEENTRY entries) { (void)flags; (void)base; (void)num; (void)entries; return S_OK; }
    HRESULT Release() { return 0; }
    HRESULT QueryInterface(REFIID riid, LPVOID *obj) { (void)riid; *obj = NULL; return E_NOINTERFACE; }
};

// IDirectDrawClipper stub interface
struct IDirectDrawClipper {
    HRESULT GetClipList(RECT *rect, LPRGNDATA list, DWORD *size) { (void)rect; (void)list; (void)size; return S_OK; }
    HRESULT GetHWnd(HWND *out) { (void)out; return S_OK; }
    HRESULT Initialize(void *dd, DWORD flags) { (void)dd; (void)flags; return S_OK; }
    HRESULT IsClipListChanged(BOOL *changed) { (void)changed; return S_OK; }
    HRESULT SetClipList(LPRGNDATA list, DWORD flags) { (void)list; (void)flags; return S_OK; }
    HRESULT SetHWnd(DWORD flags, HWND hwnd) { (void)flags; (void)hwnd; return S_OK; }
    HRESULT Release() { return 0; }
    HRESULT QueryInterface(REFIID riid, LPVOID *obj) { (void)riid; *obj = NULL; return E_NOINTERFACE; }
};

// IID constants
static const GUID IID_IDirectDraw = {0x6C14DB80,0xA733,0x11CE,{0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60}};
static const GUID IID_IDirectDraw2 = {0xB3A6F3E0,0x2B43,0x11CF,{0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56}};
static const GUID IID_IDirectDrawSurface = {0x6C14DB81,0xA733,0x11CE,{0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60}};
static const GUID IID_IDirectDrawSurface2 = {0x57805885,0x6EEC,0x11CF,{0x94,0x41,0xA8,0x23,0x03,0xC0,0xAA,0x65}};
static const GUID IID_IDirectDrawPalette = {0x6C14DB84,0xA733,0x11CE,{0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60}};
static const GUID IID_IDirectDrawClipper = {0x6C14DB85,0xA733,0x11CE,{0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60}};

// Wait flags
#define DDWAITVB_BLOCKBEGIN 0x1
#define DDWAITVB_BLOCKBEGINEVENT 0x2
#define DDWAITVB_BLOCKEND 0x4

// GetBltStatus flags
#define DDGBS_CANBLT 0x1
#define DDGBS_ISBLTDONE 0x2

// GetFlipStatus flags
#define DDGFS_CANFLIP 0x1
#define DDGFS_ISFLIPDONE 0x2

// Functions (stubs - replaced by Vulkan)
extern "C" {
    HRESULT WINAPI DirectDrawCreate(GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, void FAR *pUnkOuter);
}

#endif // _DDRAW_H_
