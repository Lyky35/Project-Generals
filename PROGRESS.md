# PROGRESS.md — Session Log

Purpose: resumable log of active work. Newest entries at the bottom of each session.
Project state lives in `NOTES.md`; this file only tracks in-flight work.

## Goal

Fix the WW3D2 texture subsystem compile errors (86 errors, 6 files) that block
`GameEngineDevice` and the `RTS.exe` link. Then continue down the NOTES.md
next-steps list. After each accomplished step: commit + push.

## Plan

1. Fix trivial errors: `textdraw.h` include path, `w3d_dep.cpp` missing `<cstddef>`
2. Fix `texture.cpp` — remove remaining `D3DTexture`/`Peek_DX8_Texture`/`IDirect3DTexture8` references
3. Fix `surfaceclass.cpp` — same class of errors (`Peek_D3D_Surface`, `D3DSurface`)
4. Fix `textureloader.cpp` — `_Create_DX8_Texture` → `_Create_Vulkan_Texture` and surface types
5. Fix downstream: `texproject.cpp`, `prim_anim.h`, `sphereobj.cpp`, `ringobj.cpp`
6. Build `WW3D2` clean
7. Build `GameEngineDevice` and link `RTS.exe`

## Log

### 2026-10-12 — Session start

- Verified repo at `eba76c7` (notes rewrite), tree clean.
- Baseline build state: 11/15 library targets OK; WW3D2 fails with 86 errors;
  GameEngineDevice/RTS not reached.
- Build config: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
  with `/usr/bin/x86_64-w64-mingw32-g++` / `-gcc`.
- Created this file. Starting step 1 (trivial fixes).

### 2026-10-12 — Step 1 done: trivial WW3D2 fixes

- `textdraw.h`: `#include "../wwmath/rect.h"` → `"../WWMath/rect.h"` (case-sensitive FS).
- `w3d_dep.cpp`: added `#include <cstddef>` before `STLSpecialAlloc` (uses `size_t` before any other include).
- Verified: both `textdraw.cpp` and `w3d_dep.cpp` now compile (warnings only).
- Remaining WW3D2 errors: texture.cpp, textureloader.cpp, surfaceclass.cpp, texproject.cpp, prim_anim.h, ww3d.cpp, sphereobj.cpp, ringobj.cpp.
- Next: texture.cpp (33 errors — D3DTexture/Peek_DX8_Texture/IDirect3DTexture8 remnants).

### 2026-10-12 — Step 2 done: texture.cpp ported to Vulkan

- Removed `D3DTexture` member usage; all paths use `VulkanTexture` (`VkTexture*`).
- `Calculate_Texture_Memory_Usage`: estimates from `VkTexture` dimensions (32bpp/mip).
- Ctors: `_Create_DX8_Texture` → `_Create_Vulkan_Texture`; D3D-pool ctor arg kept (compat shim).
- `TextureClass(IDirect3DTexture8*)` → `TextureClass(VkTexture*)` (matches header).
- Surface ctor: builds via `_Create_Vulkan_Texture(w,h,format,mips)` from `SurfaceDescription`.
- Dtor/`Invalidate`/`Load_Locked_Surface`: `Destroy()`+`delete` on `VulkanTexture`.
- `Is_Missing_Texture`: compare against `MissingTexture::_Get_Missing_Texture()`.
- `Get_Mip_Level_Count`/`Get_Level_Description`: `VkTexture::GetMipLevels/GetWidth/GetHeight`.
- `Get_Surface_Level`: wraps `VulkanTexture` in `SurfaceClass(VkTexture*)`.
- `Get/Set_Priority`: no-op (return 0) — Vulkan has no D3D priority.
- `Apply`/`Apply_Null`: `DX8Wrapper::Set_Vulkan_Texture(stage, this/NULL)`.
- `Apply_New_Surface`: takes ownership via `TextureLoadTask->Peek_Vulkan_Texture()` + `Set_Vulkan_Texture(NULL)`.
- `_Init_Filters`: fixed typo `D3DPTFILTERCAPS_MAGFAFLATCUBIC` → `MAGFFLATCUBIC`.
- `textureloader.h`: renamed task member `D3DTexture`→`VulkanTexture`, `Peek/Set_D3D_Texture`→`Peek/Set_Vulkan_Texture` (impl updates pending in step 4).
- Verified: texture.cpp compiles (reorder warnings only).
- Remaining WW3D2 errors: surfaceclass.cpp (~5), textureloader.cpp (~25), ww3d.cpp (~5).

### 2026-10-12 — Step 3 done: surfaceclass.cpp ported to Vulkan

- Added `ShadowMemory`/`ShadowPitch` members to `surfaceclass.h` for CPU-side Lock.
- Ctor `(w,h,format)`: proper `VkTexture::Create(w,h,fmt,1,false)` (was wrong byteSize/imageType args).
- Ctor `(filename)`: blank 64x64 VkTexture (TODO: real DDS load).
- Ctor `(IDirect3DSurface8*)` → `(VkTexture*)` matching header.
- `Lock`/`Unlock`: shadow buffer + `UploadLevel` on unlock (VkTexture has no public Map).
- `Clear`/`Copy`/`CreateCopy`/`FindBB`/`Is_Transparent_Column`/`Get_Pixel`/`DrawPixel`/`DrawHLine`/`Hue_Shift`: rewritten on shadow buffer.
- Surface-to-surface `Copy`: `_Copy_Vulkan_Rects` (same format) or CPU memcpy fallback.
- `Stretch_Copy`: nearest-sample CPU fallback (was D3DXLoadSurfaceFromSurface).
- `Attach`/`Detach`: take/manage `VkTexture*` (no COM refcounting).
- `Get_Description`: uses stored `SurfaceFormat` (not raw VkFormat).
- Verified: surfaceclass.cpp compiles; zero D3D references remain.
- Remaining WW3D2 errors: textureloader.cpp (~25), ww3d.cpp (~6), prim_anim.h (1 template).

### 2026-10-12 — Step 4 done: WW3D2 builds clean

Fixed remaining files in one pass:

**textureloader.cpp** (full Vulkan port):
- `Load_Thumbnail`: `_Create_Vulkan_Texture` + `UploadLevel` + `GenerateMipmaps` (was multi-level LockRect).
- `Load_Compressed_Texture`: returns `VkTexture*`; DDS pixel upload TODO (needs DDSFileClass CPU path).
- `Load_Surface_Immediate`: returns `void*` (VkTexture*); TGA → `CreateFromData`.
- `Generate_Bumpmap`: stub returns missing texture (needs CPU-readable source).
- Load task: `D3DTexture`→`VulkanTexture`; mip locks are CPU shadow buffers uploaded in `End_Load` via `UploadLevel`; `Apply` transfers ownership (no COM Release).
- Header: `Load_Thumbnail` → `VkTexture*`; removed `IDirect3DTexture8` forward decl.

**ww3d.cpp**:
- Removed `Is_Vulkan_Lost` checks (no such API; swapchain recreation is internal).
- Screenshot + movie capture: `_Get_DX8_Front_Buffer`/LockRect → `_Read_Front_Buffer` (RGBA readback).

**Other**:
- `textdraw.cpp`: `Peek_Texture(ch)`/`Peek_Texture('A')` → `Peek_Texture()` (no per-char overload).
- `surfaceclass.cpp`: `min()` → ternary (NOMINMAX).
- `prim_anim.h`: dependent-base access via `this->` + `typedef typename ...::KeyClass` (GCC two-phase lookup).
- `texproject.cpp`: `Set_Render_Target((IDirect3DSurface8*)NULL)` → `(TextureClass*)NULL`.
- `texture.cpp`: Bumpmap ctor uses stubbed `Generate_Bumpmap` (returns VkTexture*).
- `textureloader.h`: `Generate_Bumpmap` → `VkTexture*`.

**Verified: `cmake --build build --target WW3D2` → 100% Built target WW3D2.**

Next: GameEngineDevice + link RTS.exe.

### 2026-10-12 — Committed pending WIP; GameEngineDevice baseline measured

- Committed leftover working-tree changes as `c0ed877` (was uncommitted since after step 4):
  - Vendored `third_party/include/{AL,SDL2,vulkan,vk_video}` — replaced symlinks-to-`/usr/include` with real header copies.
  - ZLib: `zlib.h`/`zconf.h` typechange symlink → vendored real files.
  - WW3D2 case-variant header symlinks (`AnimObj.h`→`animobj.h`, `TextureLoader.h`→`textureloader.h`, etc.) per NOTES.md arch decision.
  - `bink.h` stub (Bink types for the FFmpeg path).
  - `UnicodeString.h`: added `#include <wchar.h>`.
- Pushed to `origin/main`.

**GameEngineDevice build baseline** (`cmake --build build --target GameEngineDevice`):

- Fails with **~544 errors**; `RTS.exe` not reached. Error breakdown by file:
  - `Source/MilesAudioDevice/MilesAudioManager.cpp` — **~424 errors** (dominant). Miles "port" is a stub: missing `AIL_*` decls (`AIL_set_sample_volume_pan`, `AIL_register_EOS_callback`, `AIL_decompress_ADPCM`, `AIL_mem_free_lock`, …), missing `H3DSAMPLE`/`HAUDIO`/`AILSOUNDINFO`, missing `U32`, `WAVE_FORMAT_PCM`/`WAVE_FORMAT_IMA_ADPCM`, `m_mutex`/`CreateMutex`/`CloseHandle`, `struct PlayingAudio` lacks `m_3DSample`, `struct OpenAudioFile` lacks `m_soundInfo`, `ProviderInfo` lacks `id`, `m_listener`/`m_delayFilter` missing, `m_available3DSamples` non-class.
  - `Include/MilesAudioDevice/MilesAudioManager.h` — ~30 errors (upstream of the .cpp).
  - `Source/VideoDevice/Bink/binkvideoplayer.cpp` — 4 orphan `#endif` (lines 239/254/265/336; preprocessor corruption, same class NOTES.md #2 flagged).
  - `Include/Common/UnicodeString.h` — `_wcsicmp` undeclared under MinGW (×14; `<wchar.h>` alone doesn't provide it here).
  - `Include/Common/ScopedMutex.h` — 8 errors (likely Windows mutex API missing under this config).
  - `Include/W3DDevice/GameClient/WorldHeightMap.h:36` — **fatal**: `#include "WWLib/RefCount.h"` not found (file exists as `WWLib/refcount.h` + case-variant symlinks; include-path/case issue).
- W3DDevice and other GameEngineDevice sources not fully diagnosed yet — make stops at first fatal/failed objs; Miles dominates the log.

Next: fix GameEngineDevice compile errors (Miles stub, Bink #endif, `_wcsicmp`, RefCount include), then link RTS.exe.

<!-- Append new entries below. Commit after each completed step. -->
