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

### 2026-10-12 — Trivial fixes + Miles/Bink/WWAudio stub pass; GameEngineDevice down to ~22 errors

**Trivial / infrastructure:**
- `UnicodeString.h`: `_wcsicmp` → MinGW-safe inline `towlower` loop (g++ `-std=gnu++17` hides MSVC `_wcsicmp`).
- `ScopedMutex.h`: added `#include <windows.h>` (HANDLE/WaitForSingleObject).
- `WWLib/RefCount.h` symlink → `refcount.h` (case-sensitive FS).
- `WorldHeightMap.h`: relative `../../gameengine/...` include → `"Common/MapObject.h"`; forward-declared `TerrainTextureClass`/`AlphaTerrainTextureClass`.
- `W3DDynamicLight.h`: `lib/baseType.h` → `Lib/BaseType.h`.
- `W3DShroud.h`: `IDirect3DSurface8*` member kept (opaque incomplete struct from d3d8types).
- `W3DModelDraw.h`: removed bogus `enum GameLODLevel;` forward decl (unused; type is `StaticGameLODLevel`).
- `DX8Compat/d3d8types.h`: added incomplete-struct opaque decls for `IDirect3DSurface8`/`IDirect3DTexture8`/`IDirect3DDevice8` etc. (matches existing `struct IDirect3DSurface8;` forwards).
- `FFmpegVideo/CMakeLists.txt`: **stop host pkg-config on MinGW** — was injecting `-I/usr/include` and mixing Linux glibc headers into MinGW TUs (source of `div_t`/`time_t`/`putenv` conflicts). `HAS_FFMPEG` now 0 for cross builds.
- `WWAudio.h`/`WWAudio.H`: consolidated to single real file + case symlinks; stripped duplicate Miles stubs; includes `Mss.H`.
- `SoundBuffer.cpp`: removed local `AILSOUNDINFO`/`AIL_WAV_info`; includes `Mss.H`.

**Miles (MSS) stub expansion — `Libraries/Include/MSS/mss.h`:**
- Types: `HSAMPLE`/`HSTREAM`/`HDIGDRIVER`/`HAUDIO`/`HPROVIDER`/`H3DSAMPLE`/`H3DPOBJECT`/`HPROENUM`/`U32`/`S32`/`AILSOUNDINFO`.
- Constants: `AIL_3D_*`, `DSSPEAKER_*`, `DP_FILTER`, `HPROENUM_FIRST`.
- ~50 `AIL_*` no-op stubs (register/start/stop/3D/stream/quick/WAV/ADPCM/file-callbacks).
- `MilesAudioManager.h`: `<windows.h>` before `MSS/MSS.h`; `HANDLE m_mutex` works.
- `MilesAudioManager.cpp`: forward-declared streaming/completion callbacks; stubbed DirectSound `GetSpeakerConfig` block; fixed `BitTest` clash with MinGW `ddk/wdm.h`; `AIL_quick_startup(int,int,...)` signature; `WAVE_FORMAT_IMA_ADPCM` fallback.

**Bink (`binkvideoplayer.cpp` + `bink.h`):**
- Restored `createStream(HBINK)` signature; removed 4 orphan `#endif`s; real `BinkOpen`/`BinkClose` calls; `BINK*` casts for FrameNum/Frames/Height/Width; `uint32_t` for surface flags.
- `bink.h`: added `BinkSetSoundTrack`/`BinkGoto`/`BinkSoundUseDirectSound`, `BINKSURFACE*` flags.

**Verified build state after this pass:**
- `WW3D2` still clean. `WWAudio` **100% built**.
- `GameEngineDevice`: **~22 errors** remaining (was ~544). Remaining clusters:
  - `W3DScene.h`: `enum CustomScenePassModes` opaque-enum forward decl needs underlying type or real enum.
  - `W3DPoliceCarDraw.cpp`: `common/RandomValue.h` case (want `Common/RandomValue.h`).
  - `W3DModelDraw.cpp:1067`: `c_it` undeclared (likely loop-var rename leftover).
  - Cascading W3DDevice draw-module errors behind those.

Next: fix remaining ~22 GameEngineDevice errors → link RTS.exe → continue NOTES.md list.

### 2026-10-12 — Shadow/W3D D3D stubs + case symlinks; TerrainTex remaining

- Committed `a75c22f` then `4c1a10d`.
- Fixed opaque-enum fwd decls: `CustomScenePassModes : int`, `ChipsetType : int`, `CpuType : int`.
- Case fixes/symlinks: `BitType.h`, `Matrix3d.h`, `Heightmap.h`, `Common/GlobalData.h`, `Common/AsciiString.h`, `Common/DrawModule.h`, `WWMath/Matrix3d.h`.
- `d3d8types.h`: complete stubs for `IDirect3DVertexBuffer8`/`IndexBuffer8`/`Device8` (Lock/Create*/Draw*/Set*); `D3DXVECTOR4` ctors; `D3DXMatrixInverse`/`Identity`; `D3DSHADE_*`.
- `DX8Wrapper`: no-op `_Get_D3D_Device8`, `Set_StreamSource`, `Set_Stencil*`, `Set_Lighting`, `Set_BlendFunc`, `Set_ZEnable`, `Set_ShadeMode`.
- `dx8vertex/indexbuffer.h`: `Get_DX8_*` aliases over Vulkan accessors.
- Shadow cpps: VK typo fixes, casts for VkGpuBuffer* vs IDirect3D*, `Vector3` rvalue fix, 5-arg `DrawIndexedPrimitive` overload.
- **TerrainTex.cpp** still has D3D remnants (~37 errors): `D3DTexture`/`Peek_DX8_Texture`/`GetSurfaceLevel`/`LockRect`/`Set_DX8_Texture`, corrupted Apply matrix math (`inv` undeclared after commented-out D3DX code). Port to `VulkanTexture`/`SurfaceClass::Lock`/`Set_Vulkan_Texture` next.

Next: finish TerrainTex Vulkan port → any remaining GameEngineDevice files → link RTS.exe.

### 2026-10-12 — TerrainTex/Granny/Shroud/Display/ShaderManager port; ~544→~23 errors

- Committed `bbdab1a`, symlink restore `d765dbe`/`927329e` (sed -i had flattened case-variant links).
- **TerrainTex**: `SurfaceClass::Lock` shadow path, `VulkanTexture`/`Peek_Vulkan_Texture`, `Release_Ref`, restored Apply UV matrices (identity+stretch+offset).
- **W3DGranny**: removed orphan `}`/`#endif` (INCLUDE_GRANNY_IN_BUILD), declared `MeshCount`, dropped `#pragma comment(lib,granny2)`.
- **W3DShroud**: `m_pSrcTexture` → `SurfaceClass*`, `Release_Ref`, `_Create_DX8_Surface` → `new SurfaceClass`, rect-copy → stub.
- **W3DDisplay**: screen-capture early-return (legacy front-buffer path `#if 0`); `const int numEntriesPerLine`; debug callback 3-arg; `WinMain.h` relative include.
- **W3DStatusCircle**: `D3DRS_BLENDOP`/`D3DBLENDOP_*` via d3d8types.
- **W3DShaderManager**: bulk `Peek_DX8_Texture`→`Peek_Vulkan_Texture`, `Set_DX8_Texture`→`Set_Vulkan_Texture`, `float4`→`D3DXVECTOR4`; `updateNoise1/2` take `Matrix4*`; scale/offset locals; `Create*Shader`/`SetPixelShader*` stubs; `_Get_D3D8`.
- **d3d8types**: complete `IDirect3DSurface8`/`Texture8`/`Device8` (Release/GetDesc/SetRenderTarget/CreateTexture/Create*Shader/cursor); `D3DVSD_*`; `D3DXMatrixScaling/Translation/Multiply/Inverse`; include-guard moved to EOF.
- **Benchmark** stub static lib + CMake for `W3DShaderManager` `RunBenchmark`.
- Case symlinks: `WWMath.h`, `Registry.h`, `W3DVideoBuffer.h`, `BitType.h`, `Heightmap.h`.

**Verified:** GameEngineDevice down to **~23 errors**, all in `W3DShaderManager.cpp` (D3DXMATRIX*↔Matrix4* at noise-update call sites; `CreatePixelShader` DWORD_PTR*; a few scale/offset locals).

Next: finish W3DShaderManager matrix/handle stubs → GameEngineDevice green → link RTS.exe.


### 2026-10-12 — GameEngineDevice green; RTS.exe links (milestone binary)

**GameEngineDevice: 100% Built target** (from ~544 errors at session start).

Remaining error clusters fixed this stretch:
- W3DWaterTracks: `enum waveType : int`, `const Vector2&` init, VkGpuBuffer Lock stubs
- W3DWater: pData for-init redecl, bump-map `#if 0`, D3DX assemble `#if 0`, `_Create_DX8_Texture` path disabled
- W3DWebBrowser: ActiveX stub; ShellExecute not used (create returns FALSE)
- W3DView/Win32*: relative `WinMain.h` includes; `Win32DEvice` case typo
- Win32GameEngine: `NEW W3DWebBrowser` instead of `CComObject<>`
- W3DShaderManager: remaining float4/Set_Vulkan_Texture/Create*Shader
- d3d8types: include-guard EOF, more D3D8 stub methods, D3DX matrix ops
- WWAudio/etc case symlinks restored after accidental sed flatten

**RTS.exe links** — `build/bin/RTS.exe` (17,866,290 bytes PE32+ x86-64).

Link support:
- `Main/linkstubs.cpp`: LZHL/BTREE/HUFF/REF (exact mangled names), zlib compress stubs, AutoPool/Int&lt;64&gt; statics, tchar import thunks
- `Main/netstubs.cpp`: FirewallHelper, DX8WebBrowser, GameSpy overlays/chat/queues, registry, Cftp/CDownload, CD manager, SEH `_set_se_translator` no-op
- `Main/guidstubs.cpp`: GUID/IID via initguid+dinput
- `VulkanRenderer/vkstubs.cpp`: empty `vk*` loader stubs for cross-link
- RTS link line: `-Wl,--unresolved-symbols=ignore-all` + winmm/vfw32/dinput8/dsound/ole32/etc.; openal/SDL2 dropped until MinGW import libs exist
- `dx8wrapper.h`: portable `Clamp_Color` for non-MSVC (was MSVC-only CMOV asm)

**Verified:** `cmake --build build --target RTS` → Built target RTS; `file build/bin/RTS.exe` → PE32+ executable.

Next: bundle OpenAL32.dll/SDL2.dll/vulkan-1.dll on Windows; replace ignore-all/stubs with real backends per NOTES.md (OpenAL Miles, FFmpeg Bink, GameSpy stub quality); then runtime test; GeneralsMD port after Generals tree renders.


### 2026-10-13 — Release 1.1 published

- Replaced placeholder `release/RTS.exe` with the real linked binary (`build/bin/RTS.exe`, PE32+ x86-64, ~17.9MB).
- Rebuilt release zips (previous v1.1 zips contained the 150KB MessageBox stub):
  - `release/Project-Generals-RTS-Full-V1.1.zip` — RTS.exe + MinGW runtime DLLs + README.txt + NOTES.md + PROGRESS.md
  - `release/Project-Generals-RTS-v1.1.zip` — slim: RTS.exe + DLLs + README.txt
- Bundled DLLs (the only non-system imports RTS.exe needs): `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`.
  Not yet needed: OpenAL32.dll / SDL2.dll / vulkan-1.dll (not linked in this milestone; vk* are in-tree stubs).
- Created GitHub release tag `v1.1` with both zips attached.

Next: replace link stubs with real backends (OpenAL Miles, FFmpeg Bink, GameSpy), wire real Vulkan loader, runtime test on Windows 10; GeneralsMD after Generals tree renders.


### 2026-10-13 — v1.1 first boot crash diagnosed: x64 pointer truncation in StringClass

Reported: Windows 10, `0xc0000005` at fault offset `0x58d513`.

- **Not an expected stub crash** — disassembly of that RVA in the shipped exe landed in
  `StringClass::Get_String(int,bool)` at `movq $0xf8,-8(%rax)`.
- **Root cause:** `wwstring.cpp` temp-pool math used
  `reinterpret_cast<unsigned>(m_TempStrings)` — truncates the 64-bit `.bss` address to 32 bits
  (`m_TempStrings` lives at `0x1408e3ce0`). First temporary `StringClass` alloc then wrote a
  header at a bogus low address → AV during early init, before any Vulkan/GameSpy code runs.
  Classic 32-bit-era code ported to x64 without updating pointer arithmetic.
- **Fix:** all three casts in `Get_String`/`Free_String` → `uintptr_t` (+ `<stdint.h>`).
  Verified in new binary: temp-pool base loaded with 64-bit `lea ..., %r8` and
  `and $0xfffffffffffff800,%r8` — no 32-bit truncation on the header store.
- Repo-wide scan: no other `reinterpret_cast<unsigned>(ptr)` / `(DWORD)ptr` truncations found.
- Rebuilt `RTS.exe`, refreshed both v1.1 zips, updated release assets.

Still expected to fail later (not yet fixed, by design for this milestone):
stubbed `vk*` loader returning garbage into renderer init, `ignore-all` leftovers if any,
GameSpy/Firewall no-ops. Those are NOTES.md next-steps, not this bug.


### 2026-10-13 — Next step: real OpenAL backend for Miles (MSS shim)

Replaced the header no-op `AIL_*` stubs with a real OpenAL Soft implementation:

- `Libraries/Include/MSS/mss.h` — types/constants kept; functions now extern declarations (+ new `AIL_service()`).
- `Libraries/Source/MSS/mss.cpp` (new, built into **WWAudio**) — voice pool shared by 2D/3D/stream/quick handles;
  WAV PCM load (`parseWav`) → `alBufferData`; EOS pump `AIL_service()` polls `AL_SOURCE_STATE`
  and fires registered callbacks; fake 3D providers enumerated with Miles names
  (`"Miles Fast 2D Positional Audio"`, `"Dolby Surround"`) so `selectProvider` succeeds;
  listener handle sentinel vs voice handles; ADPCM path returns silence buffer (no decoder yet).
- **`MilesAudioManager.cpp`**:
  - `setSampleCompleted` / `set3DSampleCompleted` / `setStreamCompleted` now call
    `TheAudio->notifyOfAudioCompletion(handle, PAT_*)` (were empty — no completion ever).
  - `update()` calls `AIL_service()` before processing lists.
  - `openDevice()` falls back to provider index 0 when preferred provider is missing/empty
    (was early-returning on `PROVIDER_ERROR == PROVIDER_ERROR`, leaving *all* sample pools empty).
- **Link**: vendored `third_party/openal/{OpenAL32.dll, OpenAL32.def, libOpenAL32.dll.a}`
  (x64 OpenAL Soft, import lib via dlltool; `.gitignore` exception for `*.a`);
  RTS links the import lib — exe now imports **`OpenAL32.dll`**.
- `WWAudio/CMakeLists.txt`: adds `Source/MSS/mss.cpp` + `third_party/include` (AL headers).
- Release zips refreshed with `OpenAL32.dll` + rebuilt exe; GH assets re-uploaded.

Known gaps in this backend: IMA-ADPCM decode is silence; pan is relative-X only;
distance scale untuned; streams are fully-loaded WAV only (no streaming/mp3);
`vk*` loader still stubbed (separate next step).


### 2026-10-13 — Next steps: real Vulkan loader + real CI; GeneralsMD deferred

**Vulkan loader (NOTES wire-loader):**
- Cross-compiled Khronos **Vulkan-Loader** 1.4.365 from source with the project MinGW toolchain
  (`/tmp/mingw-w64-x86_64.cmake` + Vulkan-Headers install prefix).
- Vendored into `third_party/vulkan/`: `vulkan-1.dll` (863KB) + `libvulkan-1.dll.a` (dlltool-style import lib from the build).
- Deleted `vkstubs.cpp` (empty `vk*` fns that made renderer init undefined-behavior).
- RTS links `third_party/vulkan/libvulkan-1.dll.a` — `objdump` now shows **`vulkan-1.dll`** in the import table
  alongside `OpenAL32.dll`. Renderer will call a real loader; still needs an ICD/GPU driver on the target machine.

**CI (NOTES rewrite CI):**
- Replaced the MessageBox-stub `ci-crosscompile.yml` with a real job: apt MinGW + CMake configure +
  build GameEngine/GameEngineDevice/WW3D2/VulkanRenderer/WWAudio/… + link RTS + verify PE imports
  + upload `RTS.exe`/`OpenAL32.dll`/`vulkan-1.dll` artifacts.
- Deleted `windows-build.yml` (Linux paths on a Windows runner; never validated the tree).

**Release:** both v1.1 zips rebuilt with `vulkan-1.dll` and refreshed README; GH assets re-uploaded.

**GeneralsMD:** deliberately **not started** — NOTES says only after the Generals tree links *and*
renders. No Windows runtime test yet; porting the unfinished Vulkan design twice would be wasteful.

Next: Windows 10 smoke test (do we get a window / first frame?), then FFmpeg Bink, then GeneralsMD
once rendering is proven.


### 2026-10-13 — FFmpeg Bink video backend (static MinGW build)

Unblocked FFmpeg for MinGW by cross-compiling a **minimal static FFmpeg 7.1.1**
(`--disable-everything` + bink demuxer/decoder/parser, file protocol, swscale, no x86asm):

- Vendored into `third_party/ffmpeg/{include,lib/*.a}` (~4.2MB headers+libs; `.gitignore` allows `*.a` there).
- `FFmpegVideo/CMakeLists.txt`: on MinGW links the static libs + `bcrypt` (avutil `BCryptGenRandom`);
  host pkg-config path kept for non-cross builds.

**BinkVideoPlayer rewrite** (`BinkVideoPlayer.h/.cpp`):
- Dropped Radiant Bink SDK (`bink.h` stubs). `BinkVideoStream` now owns an opaque `BinkFFmpegCtx`
  (`AVFormatContext`/`AVCodecContext`/`AVFrame`/`SwsContext`).
- `open()` resolves the movie path (mod dir → language → Data\\Movies) and demuxes via
  `avformat_open_input`; `load()` == `open()` (streamed from disk).
- Frame pump matches engine callers (Display / LoadScreen / WindowVideoManager):
  `isFrameReady` = wall-clock vs stream fps; `frameDecompress` reads/decodes next packet;
  `frameRender` sws_scale → VideoBuffer (X8R8G8B8 / R8G8B8 / 16-bit); `frameNext` loops via seek;
  `frameGoto` seeks (LoadScreen `frameGoto(frameCount())` clamps to last frame).
- Video-only for now — Bink audio still not wired (Miles path separate).

**Link:** RTS.exe grows to ~19.3MB; imports now include `bcrypt.dll` (system on Win10+).
No new redistributable DLLs — FFmpeg is fully static.

Next: Windows smoke test; audio-in-bik optional; GeneralsMD after render proof.


### 2026-10-13 — Fix ucrtbase 0xc0000005: real AutoPool/Int objects, drop ignore-all

Reported: faulting module **ucrtbase.dll**, offset `0x26cbc` (heap/allocator class of crash,
not a null call in RTS.exe).

Root causes addressed:
1. **`DEFINE_AUTO_POOL` emitted no symbol under MinGW/GCC** when used as
   `template<> ... Allocator;` (no initializer). `nm` showed `U` even in the defining TU.
   Callers then resolved to our `char[4096] = {0}` link stubs (zero-filled, no ctor).
   - `mempool.h`: macro now uses `= ObjectPoolClass<T,BLOCKSIZE>()` (emits `B` + static ctors).
   - `Allocator` made public; dedicated `autopools.cpp` per WWLib/WW3D2/WWAudio/WWMath
     define the pools; DEFINE lines removed from the original TUs.
   - Verified in exe: `B AutoPoolClass<...>::Allocator` + `_GLOBAL__sub_I_...Allocator`.
2. **`Int<64>::Remainder`** was a `char[256]` stub. int.cpp now uses
   `Remainder = Int<MAX_UNIT_PRECISION>()` so the static runs `XMP_Init`.
   Exe has `B Int<64>::Remainder`.
3. **Removed `Cftp`/`CDownload` empty stubs** from netstubs (Cftp dtor `fclose`s `m_pfLocalFile`;
   empty ctor left it uninitialized). Linked real **WWDownload** (`ftp.cpp` ZeroStuff ctor).
4. **Removed `-Wl,--unresolved-symbols=ignore-all`** — link is now strict (0 undefined).
5. **WinMain breadcrumbs**: writes `RTS_START.LOG` beside the exe
   (`WinMain enter` → `after checkProtection` → `initializeAppWindows ok` →
   `initMemoryManager done` → `entering GameMain` → `GameMain returned`).

Next: retest on Windows 10; if it still dies, the log file pinpoints the phase.


### 2026-10-13 — ucrtbase crash with NO log: static-init / DLL-load path

Same ucrtbase 0xc0000005 @ 0x26cbc and **no RTS_START.LOG** ⇒ crash is **before WinMain**
(DLL static imports load first, then CRT static ctors).

Mitigations in this build:
- **`-static-libgcc -static-libstdc++`** — no more `libgcc_s_seh-1.dll` / `libstdc++-6.dll`.
- **`std::mutex` in mss.cpp → Win32 `CRITICAL_SECTION`** (avoids pthread from our code).
- **FFmpeg rebuilt `--disable-pthreads`** (was still pulling `clock_gettime64`/`libwinpthread`).
- Time shims (`clock_gettime64`/`nanosleep64`/`clock`) in linkstubs.
- **`RTS_START.LOG` static-init breadcrumb** via `__attribute__((init_priority(101)))`
  (kernel32 CreateFile/WriteFile only — runs in CRT static ctor phase).
- **`RTS_DIAG.exe`** in the zip: WinMain + `LoadLibrary` OpenAL32.dll / vulkan-1.dll + log.
  Run it first — if a message box appears, those DLLs load OK and the crash is in our static init.
  If RTS_DIAG.LOG says a DLL failed, that DLL's DllMain is the crash source.

Still imported by RTS.exe: OpenAL32, vulkan-1, libwinpthread, bcrypt + system DLLs.

Next: user runs RTS_DIAG.exe + RTS.exe; RTS_START.LOG first line tells us the phase.


### 2026-10-13 — Field logs: static-init confirmed; DLLs OK

User logs from `log/log/`:
- **RTS_DIAG.LOG**: `WinMain` / `OpenAL32 loaded` / `vulkan-1 loaded` → those DLLs are fine.
- **RTS_START.LOG**: **only** `static-init` → our priority-101 static ran; **WinMain never reached**.
  Crash is in a **later C++ static constructor** (ucrtbase 0x26cbc, deterministic).

Added **unhandled-exception filter** in the early static path: on crash it appends
`CRASH code=… addr=… ip=…` to RTS_START.LOG (kernel32/`snprintf` only).
Next user run should give the faulting IP so we can map it to a static ctor.


### 2026-10-13 — Lazy CriticalSectionClass/MutexClass to stop static-init new[]

User log: `static-init` then CRASH in ucrtbase (no `late-static-done` on that run).
Dozens of **file-scope `CriticalSectionClass` objects** each ran
`new char[sizeof(CRITICAL_SECTION)]` + `InitializeCriticalSection` during CRT static init.

- `CriticalSectionClass` / `MutexClass`: **defer CreateMutex/InitializeCriticalSection + heap to first Lock()**.
  Static ctors now only zero the handle (no new, no Win32).
- Still have early/late static markers + crash filter in WinMain for the next run.


### 2026-10-13 — Boot bisect markers (init_priority 101–65535)

New `rts_bootlog.h` + markers with distinct `init_priority` so **one run** names the
static-init window that dies in ucrtbase:

| Priority | Marker line |
|----------|-------------|
| 101 | `static-init` |
| 200 | `boot:after-wwstring-mutex` |
| 210 | `boot:after-systimer` |
| 220 | `boot:after-cpudetect` |
| 230 | `boot:after-ffactory` |
| 240 | `boot:after-frame-timer` |
| 250 | `boot:after-mss-statics` |
| 65535 | `late-static-done` |

Plus `CRASH … module=…` from the unhandled-exception filter.

### 2026-10-13 — Pre-WinMain ucrtbase crash ROOT-CAUSED + FIXED; boot reaches GameMain

**Phase 1 — localizing the crashing static (no more field-trip guesses):**
- Mapped the real `.ctors` layout of `RTS.exe`: `__do_global_ctors` walks entries
  **high→low address**: `register_frame_ctor` → priority markers (101…250) → 111
  default-priority statics descending → `late-static-done` runs LAST (why it never
  logged: the crash always preceded it). Dumped all 119 entries → exact execution order.
- Enhanced `earlyCrashFilter` (WinMain.cpp): logs exception read/write + fault address
  and a **full stack walk** via `RtlLookupFunctionEntry`/`RtlVirtualUnwind` (ntdll via
  GetProcAddress — no new imports). This is the tool that nailed it.
- Installed **wine 11.17** and reproduced the crash locally (`/tmp/opencode/rts_test`,
  WINEPREFIX=/tmp/opencode/wineprefix): same ucrtbase AV, fault address varies (0x8/0x21)
  = garbage pointer. Stack: `__do_global_ctors → CPUDetectClass::Init_Compact_Log →
  StringClass::Format → _vsnprintf → ucrtbase strlen → AV`.
- winedbg at the Format call site: the `%s` argument (r8) = **0x8** (uninitialized).

**Root cause — original EA bug, exposed by running on modern Windows:**
`Get_OS_Info` (cpudetect.cpp) VER_PLATFORM_WIN32_NT case only handles major==4/==5.
Win10 reports major 6 (unmanifested GetVersionEx) or 10 → falls out of the switch
**without ever setting `os_info.Code`**; `OSInfoStruct os_info;` is an uninitialized
local → `COMPACTLOG(("%s\t", os_info.Code))` → `vsnprintf` → `strlen(stack garbage)`
→ deterministic AV during CRT static init, pre-WinMain. On 2000/XP (major 5) the
path never executed.

**Fixes in this pass:**
- `cpudetect.cpp`: NT major 6 (Vista..8.1) + ≥10 cases; `os_info.Code="WIN10"`
  fallback (never uninitialized); Win9x fall-through sets SubCode/Code;
  `os_info = {}` zero-init + null-guard on the COMPACTLOG format arg.
- WinMain.cpp breadcrumbs: WinMain-entry `fopen(RTS_START.LOG,"w")` → `"a"` (the "w"
  was **truncating all boot markers** the moment WinMain ran); `earlyStaticLog` now
  truncates the log once at priority-101 so each run is clean.
- Verified under wine: **full static init completes** — `late-static-done` logs for
  the first time — WinMain, window created.

**Phase 2 — next blocker found while testing: initMemoryManager always exit(-1)**
- GCC -O2/-O3 constant-folds the 2003 link test (`theLinkTester` expected +6 from
  global operator new/delete): compiled `initMemoryManager` into
  `theLinkTester=0; exit(-1)` after pool init, unconditionally. (GCC's replaceable-
  operator assumptions: `-fassume-sane-operators-new-delete` + sized deallocation
  emitting external `_ZdlPvm` whose ++ it can't attribute.)
- Fix: (1) sized `operator delete(void*, size_t)`/`delete[]` overloads in
  GameMemory.cpp forwarding to the unsized ones (single ++ per delete, visible to
  the optimizer in-TU); (2) link test moved into `memoryManagerLinkTest()` marked
  `__attribute__((noinline, noipa, optimize("O0")))` so the six operator calls
  actually run at runtime. Verified in the binary: real `call memoryManagerLinkTest;
  cmp $6; jne exit`.

**Current boot state (wine, bare test dir):**
`static-init → … → late-static-done → WinMain enter → initializeAppWindows ok ×2 →
initMemoryManager done → entering GameMain` → engine init throws
"Uncaught Exception during initialization" (ReleaseCrashInfo.txt) — expected without
the real game data (.big archives / Data dir). That is the next gate, on a machine
with Generals installed (Windows 10).

Next: run on Windows 10 with real game data; keep the stack-walk filter (it's now the
standard diagnostic); wine harness at /tmp/opencode/rts_test is reusable.

### 2026-10-13 — "Technical Difficulties" box ROOT-CAUSED (three bugs) + fixed; boots into GUI construction

User report: on Windows 10 with real game data, RTS.exe shows the infamous
"You have encountered a serious error... Technical Difficulties" box.
(GenPatcher lore applies to the ORIGINAL exe - SafeDisc/GameSpy are already
gone here; ours was three separate port bugs, found by running the user's
game install under wine at /tmp/opencode/rts_data with full .big data.)

**Bug 1 - stubbed GameSpy INI parser desynced the whole INI reader:**
- `GameSpy/Chat.cpp` is excluded from the build; `Main/netstubs.cpp` had
  `INI::parseOnlineChatColorDefinition(INI*) {}` - an EMPTY stub. When
  Multiplayer.ini's OnlineChatColors block hit it, the stub returned without
  consuming the block, the next indented key ("Default") was parsed as a
  top-level block, and INI::load threw.
- Same for `parseWebpageURLDefinition`.
- Deeper: the INI_* error constants were an UNNAMED ENUM in INI.h - thrown as
  an anonymous type (`._anon_700`, confirmed via __cxa_throw + typeinfo) that
  never matched `catch (ErrorCode)` in GameEngine::init, so every INI failure
  escaped to catch(...) = the generic box.
- Fixes: both stubs now consume the block up to the END token; INI.h constants
  are now `static const ErrorCode` so catch(ErrorCode) matches; the
  catch(ErrorCode) path RELEASE_CRASHes with the code value instead of
  silently swallowing; INI.cpp logs failing files/blocks to RTS_START.LOG.

**Bug 2 - CreateCDManager returned NULL:**
- netstubs `void* CreateCDManager(){return 0;}` -> initSubsystem(TheCDManager)
  dereferenced NULL (crash `this=0x8` in AsciiString::set via
  SubsystemInterfaceList::initSubsystem).
- Fix: CDManager (generic, compiled in GameEngine) is now concrete
  (added the createDrive override returning new CDDrive) and
  CreateCDManager returns `new CDManager`. createFirewallHelper likewise.

**Bug 3 - rts::hash<AsciiString> hashed POINTERS, not contents:**
- STLTypedefs.h specialized it as `std::hash<const char*>((const char*)ast.str())`
  - old MSVC hashed string CONTENTS, libstdc++ hashes the pointer. Every
  AsciiString-keyed hash_map (ThingFactory!) inserted under one address and
  looked up by another: `Object GenericBridgeTower` was registered and then
  NOT FOUND by the ObjectReskin two lines later ("ObjectReskin must come
  after the original Object"), which threw INI_INVALID_DATA = the box.
- Fix: FNV-1a content hash. Object loading went from 3 templates to 1128+.

**Boot state now (wine + user's game data):**
full static init -> WinMain -> window -> memory manager -> GameMain ->
entire GameEngine::init (all subsystems, ~all INIs, 1128 object templates)
-> GameClient::init -> IMEManager::init -> winCreateFromScript ->
parseWindow -> GadgetStaticTextSetText -> **UnicodeString copy-ctor AV**
(read of garbage 0xfe30dd08). Next session: window-script text data
(_TextData/WinInstanceData) x64 issue during GUI construction.

Next: diagnose the GadgetStaticText UnicodeString crash (likely another
x64 struct/pointer issue in the .win script parser); strip remaining
release-mode breadcrumbs if desired; then runtime smoke on Windows 10.

### 2026-10-13 — WindowMsgData x64 truncation fixed; boots through GUI into Vulkan init

Field log from Windows 10 (user run of the Technical-Difficulties build):
identical stack to wine - GadgetStaticTextSystem -> UnicodeString copy-ctor
AV, fault = 0x135fd858.

**Root cause:** `typedef UnsignedInt WindowMsgData` (GameWindow.h) - a 32-bit
type that doubles as the pointer carrier in winSendSystemMsg calls
(`(WindowMsgData)&text`). Fine on x86; on x64 every pointer passed through a
window message lost its high half. In GGM_SET_LABEL the saved mData1 came
back zero-extended (fault addr = stack pointer truncated to 0x135fd858 on
Windows, 0xfe30dd08 under wine - matches the disasm of
GadgetStaticTextSystem: `mov %r8d,0x70(%rsp)` / `mov 0x70(%rsp),%edx`).

**Fix:** `typedef UnsignedInt64 WindowMsgData;` - all integer uses still
compile; pointer round-trips are exact on x64.

**Boot state after fix (wine + full game data):** past ALL subsystems, INIs,
1128+ objects AND the complete GUI window-script construction
(IMEManager/winCreateFromScript/parseWindow/Gadgets) - now dies in
**Vulkan renderer init** (`vkCreateImage` under wine, whose vulkan stack has
no usable GPU ICD on this machine - `init_physical_devices` res -3). On
Windows 10 with real GPU drivers this is the next real test: either first
frames or a new logged failure (D3D-failure box or FR stack).

Next: Windows 10 run with GPU; if Vulkan device creation fails there too,
audit VulkanRenderer image creation; then audio/first-frame.

### 2026-10-13 — Root cause of the silent GameClient death: VulkanDevice::IsInitialized() lied

Field log + GC markers bracketed the silent death inside TheGameClient init
(no exception/CRT/atexit lines = driver-level or unix-side kill). Local
repro under wine (gdb caught unix-side SIGSEGV in ntdll's call dispatcher,
r10=NULL dispatch table, called from winevulkan): **a vk* call with a
NULL/garbage VkDevice**.

**Root cause chain:**
1. `VulkanDevice::IsInitialized()` was hardcoded `return true`.
2. `DX8Wrapper::Create_Device()` guards on it -> **always returned true
   without ever calling `device.Init(_Hwnd)`** - the Vulkan device was
   NEVER created, yet all guards thought it existed.
3. First real texture/buffer creation (W3DInGameUI phase) called
   vkCreateImage on an uninitialized VkDevice -> NULL device dispatch ->
   GPU driver / winevulkan hard-crash. Real drivers kill the process with
   their own filter = the silent Windows death; wine dies inside ntdll.so.

**Fixes:**
- `IsInitialized()` now returns `m_device != VK_NULL_HANDLE`.
- Create_Device logs `VK: device created` / `VK: device init FAILED` /
  shader-init failure to RTS_START.LOG.
- Release zips now include `shaders\mesh.vert.spv` + `mesh.frag.spv`
  (Create_Device needs them next to RTS.exe; they were never shipped!).

Local wine can't verify device init end-to-end (this machine's NVIDIA
kernel driver is not loaded - nvidia-smi fails - so there is no working
host Vulkan ICD). Windows 10 with working GPU drivers is the real test:
expect `VK: device created` + first-frame rendering, or a logged failure.

### 2026-10-13 — Vulkan init fixes for real GPUs (7900 XTX / RTX 4080)

User ran the IsInitialized fix on real Windows GPUs: log showed
`VK: device init FAILED` (then the legacy "DirectX 8.1" box - just the
game's old ERROR_INVALID_D3D message). Two more blockers found in
VulkanDevice.cpp:

1. `CreateInstance` enabled VK_KHR_surface TWICE and never enabled
   VK_KHR_win32_surface - a duplicate extension name makes
   vkCreateInstance fail with VK_ERROR_EXTENSION_NOT_PRESENT (-6) on real
   Windows loaders.
2. `CreateSurface()` was an UNIMPLEMENTED STUB (`return false`) - even a
   successful instance could never get a surface.

Fixes: proper VK_KHR_surface + VK_KHR_win32_surface list; real
vkCreateWin32SurfaceKHR via GetModuleHandle(NULL)+hwnd; per-step VK
result logging (`VK: vkCreateInstance -> N`, `VK: Init: Xxx FAILED`,
`VK: Init: Vulkan device READY`) so the next field run pinpoints any
remaining failure. Local wine still fails at instance creation (-3) -
this machine's NVIDIA kernel driver is not loaded, so no host ICD.

### 2026-10-13 — Fixed 0x0 DDS texture upload crashing the AMD driver

Field log after the upload hardening: Vulkan fully inits (`device READY`)
and ALL subsystems + GameClient complete; first terrain texture upload
logged `VK: upload w=0 h=0 fmt=37 size=0` then amdvlk64.dll AV.

Cause - the ported DDSFileClass ctor had two bugs:
1. It read `sizeof(LegacyDDSURFACEDESC2)` bytes into the header - on x64
   the legacy struct's `void*` member inflates it (128 vs the 124-byte
   on-disk DDS header) and shifts PixelFormat, so FourCC/format misparsed.
2. A port-placeholder `VkTexture::CreateFromData(..., Width, Height, ...,
   NULL)` was inserted BEFORE Width/Height are assigned from the header -
   so it created + uploaded a 0x0 texture with NULL pixels. AMD's driver
   tolerates the invalid creates then AVs on the copy.

Fixes: explicit 124-byte packed on-disk header parse with per-field
mapping; removed the premature placeholder texture creation (textures are
created later via the TextureLoader accessors, as in the original);
UploadLevel/Create now reject 0x0/NULL uploads.

<!-- Append new entries below. Commit after each completed step. -->

### 2026-10-14 — Invisible menu text: sentence surface -> texture copy was never implemented

Root cause of "fonts don't show on menus" (NOT driver/AMD-specific):
`Render2DSentenceClass::Build_Textures()` had the original
`DX8Wrapper::_Copy_DX8_Rects(surface, ..., texture_surface, ...)` replaced at
import time with `// Vulkan backend - surface copy not yet implemented`.
Glyphs are blitted by `FontCharsClass::Blit_Char` into the sentence
`SurfaceClass` shadow buffer, but they were never copied into the
`TextureClass` that `Render2DClass::Render()` binds (`DX8Wrapper::Set_Texture(0,
Texture)`), so every GUI string drew from a blank texture on any GPU.

Fix (render2dsentence.cpp, Build_Textures): lock source + destination surface,
memcpy `Width*4` bytes per row between the two shadow buffers (both are
A8R8G8B8/32bpp), `Unlock()` the texture surface - `SurfaceClass::Unlock()`
already does `VkTexture::UploadLevel`, so the glyphs reach the GPU. Logs
`VK: sentence texture copy w=.. h=.. pitch=../.. nonzero=..` for the first 8
copies (nonzero = glyph pixels actually in the buffer).

Also: the black menu background is expected, not a render bug - the shell map
is force-disabled at `GameEngine.cpp:329` (Bink/shell load was hanging the
boot), so nothing draws behind the menu UI.

**Verified (wine + lavapipe + full game data, run7):** all six main-menu labels
build and copy - `SOLO PLAY/LOAD/MULTIPLAYER/OPTIONS/CREDITS/EXIT GAME` ->
`VK: sentence texture copy w=64 h=64 pitch=256/256 nonzero=424/230/566/395/374/451`.
Glyph data (non-blank) now reaches the sampled texture; before the fix the
copy branch never executed at all (0 log lines). Capped breadcrumb logs added
along the way: `VK: Build_Sentence len/font/text` (24), `VK: sentence Render
pending/renderers/sentence` (12), `VK: DS draw` / `DS draw skipped (empty
text)` (16/6) - delete when no longer needed.

**New separate crash found during this test (pre-existing, unrelated to fonts):**
activating the OPTIONS button kills the game -
`CRASH code=0xc0000005 ip=...GadgetCheckBoxSetChecked fault=0x44`, stack
`GadgetPushButtonInput -> MainMenuSystem (MainMenu.cpp:1448 getOptionsLayout
-> runInit) -> OptionsMenuInit -> GadgetCheckBoxSetChecked(NULL)`.
`OptionsMenu.wnd` IS present in Window.big, so the layout loads, but the
name-key lookups (`winGetWindowFromId(NULL, "OptionsMenu.wnd:CheckLanguageFilter"`,
`...:CheckBox3DShadows`, ...) at OptionsMenu.cpp:1307+ return NULL, and
OptionsMenuInit:1608+ calls SetChecked on them unconditionally -> NULL
`winGetInstanceData()` deref. Next: log which lookups come back NULL (are ALL
missing = windows never entered `m_windowList`, or only specific gadget types
missing) and guard the SetChecked calls.

Status: font fix builds clean (warnings only) and is verified under
wine+lavapipe; Windows 10/AMD field run still the final check. Options-menu
crash open (blocks clicking OPTIONS).

### 2026-10-14 — OPTIONS click crash: missing windows in shipped data, now guarded

**Field result: fonts confirmed working on Windows 10/AMD** (user run). Moved on
to the Options-menu crash outlined above.

**Root cause (not a bad m_windowList - two windows genuinely do not exist in the
shipped `Window.big`):** a new breadcrumb in `winGetWindowFromId` (global
searches that find nothing, capped 40, logs `id + keyToName + caller address`)
named them on the first run:

- `OptionsMenu.wnd:CheckAlternateMouse` - looked up at OptionsMenu.cpp:1293 and
  **unconditionally dereferenced** by `GadgetCheckBoxSetChecked` at
  OptionsMenu.cpp:1697 (also 782 via setDefaults) = the AV at Options+0x14.
- `OptionsMenu.wnd:TextEntryFirewallPortOverride` - missing too, but both uses
  (OptionsMenu.cpp:1097, 1508) were already `if (handle)` guarded.

(`CheckAudioHardware`/`CheckAudioSurround`/`CheckLowTextureDetail` are also
absent from the data, but those call sites are commented out.)

**Fixes:**
- `GadgetCheckBoxSetChecked` / `GadgetCheckBoxIsChecked`: skip on NULL
  window/instance data (capped log `VK: GadgetCheckBoxSetChecked skipped`),
  `IsChecked` returns FALSE. Covers OptionsMenu 782/1126/1697 and any other
  menu whose data lacks a control.
- `winGetWindowFromId`: the MISS breadcrumb above (keep it - it turns every
  future data/name mismatch into a named log line instead of an AV).
- `TransitionWindow::init/update`: NULL-check `m_transition` (the
  `getTransitionForStyle` default returns NULL for an unregistered style ->
  instant deref) and log when the target window is missing. Found via the same
  breadcrumb: one transition entry has an **empty WinName** (key of ""),
  style=6 SCREENFADE - benign (ScreenFade never uses the window), but the
  unguarded `m_transition->getFrameLength()` in `update()` was one bad style
  away from the same AV.

**Also-observed misses (all call sites already guarded, left alone):**
`Diplomacy.wnd:ListboxSolo` (Diplomacy.cpp:172/236).

**Verified (wine + lavapipe + full data):** Options menu opened headlessly via a
temporary `RTS_TEST_OPTIONS` env hook in `Shell::showShell` (hook removed after
the test; local repro now needs a real click or re-adding it): OptionsMenuInit
runs to completion (`VK: ... skipped (NULL window)` instead of the AV), 45+
frames alive, no CRASH line, fonts still copy (6/6 labels). Final smoke run
without the hook: 130 frames, no crash, fonts OK.

Status: OPTIONS crash fixed and verified under wine; awaiting Windows 10 click
test. Next candidates: empty-WinName transition entry (data), and the remaining
named MISSes as they show up in field logs.

### 2026-10-15 — Field: fonts OK, Options OK; ~1fps + white-screen corruption ROOT-CAUSED (tchar shims compared WIDE over char data) + 1080p default + BMP capture removed

Field report after the previous two fixes: menus/fonts work, Options opens, but
(1) dropdown selections (resolution/quality) don't seem to register, (2) only a
few fps, (3) after a while the whole screen goes white (menus + background),
(4) please default to 1080p, (5) remove RTS_FRAME1.BMP. Wine tests are to stay
at 800x600 from now on (`wine RTS.exe -win -xres 800 -yres 600`).

**Root cause of (2)+(3) — one bug, found by instrumenting live GPU memory:**
`linkstubs.cpp` satisfied MinGW's `dllimport` decls of `_tcscmp/_tcsicmp/_tcsclen`
(header: `const char*`) with **wide** implementations (`wcscmp/_wcsicmp/wcslen`).
`StringClass::Compare()` therefore returned garbage for equal strings (verified:
`strcmp=0` but `operator==`=false in the same log line). `StringClass` is the key
type of the WW3D `TextureHash`, so **`Get_Texture()` never hit its cache**:
- every draw of the menu background constructed a NEW `TextureClass` for
  `scsmshelluserinterface512_001.tga` (new `texobj` each time) and re-decoded
  the TGA -> ~1 fps from CPU work alone;
- each duplicate held a `VkTexture` that was never released -> live texture
  memory climbed ~150MB/s (`VkTexture LIVE live=6599MB count=6342` after 45s);
- after ~2 min on the GPU: `vkAllocateMemory -> -2` (OUT_OF_DEVICE_MEMORY),
  `SurfaceClass: Create FAILED 64x64` x246 (no more font/label surfaces) and
  every texture fell back to white -> **the white screen**.

**Fix:** linkstubs tchar shims are now char-based (`strcmp`/`_stricmp`/`strlen`),
matching the tchar.h declarations.

**Also in this pass:**
- **1080p default**: `GlobalData` ctor 800x600 -> 1920x1080 (gamedata.ini's
  XResolution/YResolution lines are commented out so the ctor value applies) and
  `WinMain` `DEFAULT_X/YRESOLUTION` -> 1920x1080. New `-xres N -yres N`
  preparse in WinMain sets the *initial window size* before window creation
  (the CommandLine parser only updates GlobalData, after) - wine runs use
  `-xres 800 -yres 600`.
- **RTS_FRAME1.BMP removed**: the one-time first-present capture block (90 lines,
  incl. its vkQueueWaitIdle) is deleted from `VkDevice::EndFrame`.
- **VkPipelineCache created + passed to vkCreateGraphicsPipelines** (the handle
  existed but was never initialized/used; now ctor-inited, created in `Init()`,
  destroyed in `Shutdown()`), plus a bounded `VK: pipeline build #N ...` log.
- **Leak detector kept**: `VkTexture`/`VkGpuBuffer` live-byte accounting that
  logs only when live crosses 256MB steps (`VK: VkTexture LIVE live=..MB`) + every
  512 creates a `creates live=.. count=..` summary, and OOM logs
  (`VK: tex create OOM`, `depth create OOM`, `buffer create OOM`). Silent in a
  healthy run, a growth curve in a dying one.
- **Dropdown-bug breadcrumbs** (capped, for the next field click test):
  `VK: mouse DOWN/UP pos captor lone grab` in `winProcessMouseEvent`,
  `VK: listbox LEFT_UP mousey listY endPos selectPos oldPos ...` in
  `GadgetListBoxInput`, `VK: combo GLM_SELECTED sender listBox match pos ...`
  in `GadgetComboBoxSystem`. The Options menu chain itself is intact in code
  (`GadgetComboBoxSystem:GLM_SELECTED -> GCM_SELECTED -> OptionsMenuSystem:1894`),
  so the log will show where the click dies: routing (no listbox line),
  hit-test (selectPos=-1), or handler (`match=0`/`pos=-1`).

**Verified locally (wine + lavapipe,800x600,45s):**
- live texture memory **flat at 24-25MB** (was 6.6GB/45s), BIG texture creates
  **7 total** (was 5133), `Begin_Texture_Load` 40 (was 5000+), all
  `TextureHash` verify-gets now hit (`lost=0` on all 60 boot misses);
- **dt=32ms/frame (30fps limiter)** vs ~900-1500ms before;
- fonts still copy 6/6 labels with nonzero glyph pixels, extent=800x600,
  2 pipeline builds, no RTS_FRAME1.BMP, no CRASH.

**Still open / observed:**
- dropdown selection needs the field click test with the new logs;
- texture create/destroy churn remains (~125 small creates/s, live stays flat -
  something still rebuilds small surfaces each frame; not fatal, worth a pass);
- `W3DDisplay::draw freeze-loop guard hit` still spams (freezeTime stuck flag);
- exit=1 wine runs were the game quitting on real clicks on the visible window
  (mouse breadcrumb proved clicks were landing), not a crash.

Status: perf/white-screen root cause fixed + verified locally; awaiting
Windows 10 field run (expect 1080p window, stable fps, no white screen) and a
dropdown click log.

### 2026-10-15 — Background restored as far as data loading: RefPack (EAR) decode wired; shell map loads; next blocker is MeshClass::Render

User: "the background was temporarily set to black, can you bring it back" (the
shell map IS the menu background; we force-disabled it at boot). Also still
open: dropdown selections don't register - need a field RTS_START.LOG (the
click breadcrumbs from the previous entry are in the build; locally the whole
chain works: synthetic clicks at 800x600 produced `selectedPos=0` +
`editText='800 x 600'`).

**Chain found while re-enabling it (all verified locally):**
1. `GameLOD::applyLODLevel` disables the shell map when `!m_memPassed ||
   isReallyLowMHz()` - true under wine's misdetection, false on the user's box.
   Added `RTS_FORCE_SHELLMAP=1` override (GameLOD.cpp + GameEngine.cpp) so it
   can be tested locally; **default stays OFF** (see step 4).
2. With it forced on, the game threw `ErrorCode` during update. Diagnosed with
   a new `--wrap=__cxa_throw` tracer (linkstubs + CMake link option, logs
   `VK: cxa_throw type=... from=...`): `WorldHeightMap::WorldHeightMap(Chunk-
   InputStream*)` -> `throw ERROR_CORRUPT_FILE_FORMAT` because
   `DataChunkInput::parse` rejected the TOC.
3. Root cause: `Maps\ShellMap1\ShellMap1.map` starts with `EAR\0` (EA
   **RefPack**), `CompressionManager::decompressData` calls `REF_decode` - and
   the only REF_decode the linker saw was the **no-op stub in
   Main/linkstubs.cpp** (declared in EAC/refcodex.h with a signature that
   matched neither the stub nor the real EAC implementation). The compressed
   bytes were fed straight to the chunk parser.
   **Fix:** refcodex.h now declares the real EAC signatures
   (`REF_decode(void*, const void*, int*)` / 4-arg `REF_encode`), the matching
   `REF_encode(dest+8, src, srcLen, NULL)` call, and the REF stubs are gone
   from linkstubs. Validated host-side against the real dumped map:
   `REF_decode` returns exactly the EAR size field (415309) and the output
   begins with the chunk TOC (`WaypointsList`, `GlobalLighting`,
   `PolygonTriggers` - the registered parser labels).
4. With decode wired: map opens, decompresses, **loads** (15.6s, 32779 texture
   uploads) - then the first 3D frame of the shell map's objects crashes:
   `CRASH 0xc0000005 ip=MeshClass::Render fault=0x18` ->
   `Model->PolygonRendererList.Peek_Head()->Get_Texture_Category()` is **NULL**
   (the list is non-empty, so the head polygon renderer was built without its
   texture category) -> deref of `category->Get_Container()`.
   **Until that is fixed the shell map stays OFF by default** (GameEngine.cpp
   gates it behind `RTS_FORCE_SHELLMAP=1`), so the shipped build keeps the
   current stable black background instead of crashing at first render.
   Next session: why a shell-map polygon renderer has no texture category
   (DX8PolygonRenderer/TextureCategory registration path).

Also in this pass: `GameEngine::execute` now has `catch (ErrorCode)` (named
reason in ReleaseCrashInfo instead of "Uncaught Exception"), a bounded
`VK: CachedFile open 'name' size magic` breadcrumb in DataChunk.cpp, and the
mouse/dropdown breadcrumbs from before. Temp test hooks (combo click test,
map dump) removed after use.

Status: background unblocked at the data layer (RefPack fixed - this also
unblocks ANY compressed map/replay for later); rendering blocker documented.
Awaiting: dropdown field log + a pass at the MeshClass texture-category NULL.

### 2026-10-15 (2) — Field log #2: dropdown chain fires, mission crashes in amdvlk; HIGH quality default + shell map back ON for background testing

Field log analysed (`log/log/RTS_START.LOG`, 02:04 run):

**Dropdowns - the machinery works, so what is left is what the user SEE:**
- Real clicks: `VK: listbox LEFT_UP mousey=386 ... selectPos=2` and
  `selectPos=0` with `VK: combo GLM_SELECTED ... match=1 pos=2/0` - item
  hit-testing, selection and the combo handoff all fire; locally synthetic
  clicks also read back `editText='800 x 600'`.
- Several other clicks hit no window at all (`grab=0/0`).
- Added for the next log: `VK: combo label fetched pos/N len text=...`
  (what actually lands in the edit box) and `VK: Accept reads
  resolutionIdx=... detailIdx=...` (what ButtonAccept picks up - resolution
  only applies at Accept via DoResolutionDialog, which is easy to mistake for
  "didn't register").

**Mission start crash:** `Training01.map` opens and **decompresses** (RefPack
fix works for missions too, x3 CachedFile opens) -> a 2048x1024 upload ->
`CRASH 0xc0000005 ip=amdvlk64.dll fault=0x10`, stack: GameLogic::startNewGame
-> SinglePlayerLoadScreen::update -> WW3D::Begin_Render ->
VulkanBeginTargetPass -> driver. Note `graphics-hook64.dll` (OBS game
capture) sits in the stack - worth one run without OBS. Breadcrumbs added:
`VK: BeginTargetPass#1..40` (brackets the AV against pass begins) and a
sampled `VK: small tex create WxH fmt caller=...` (every 256th small create)
to name the texture churn source - live bytes stay flat while creates climb
(hundreds/sec), and a descriptor referencing a destroyed image still in flight
(2 frames in flight) is the leading suspect for BOTH driver crashes.

**Per user request - quality + background:**
- `GameLODManager::init` now forces `STATIC_GAME_LOD_HIGH` for BOTH the static
  level and the dynamic-LOD ideal (their Options.ini pinned
  `StaticGameLOD = Low`, i.e. texture reduction = halved textures). In-game
  Options can still change it for the session.
- Shell map (= animated menu background) is **ON by default** again
  (`RTS_NO_SHELLMAP=1` opts out; `RTS_FORCE_SHELLMAP=1` still bypasses the
  GameLOD low-mem/slow-CPU disable, which is what fires under wine).
- `MeshClass::Render`: the crash the field log showed (head polygon renderer
  with a destroyed/NULL `Texture_Category` -> deref of Get_Container()) no
  longer kills the process - orphaned polygon renderers are unlinked and
  deleted (capped `VK: dropped N orphan polygon renderer(s) from mesh ...`),
  and the mesh renders whatever still has a category.

**Local status:** default run stable (1200 frames, 30fps, no crash; wine's
GameLOD still disables the shell map locally unless forced). Forced-on shell
map run: map loads (15.6s, 32779 uploads) then dies in a lavapipe WORKER
THREAD (unix SIGSEGV, our SEH filter silent; orphan guard did NOT fire ->
not the field crash cause) at the first shell-map frame - same "first heavy
render after mass texture uploads" signature as the field amdvlk AV. Next
field log (with the new breadcrumbs) should name the churn source / pass.

### 2026-10-15 (3) — Black background root cause: signed Int overflow in the RAM test (>=2GB machines fail "has 256MB"); mission crash breadcrumbs

Field log #3 (02:41) showed `VK: shellMap check SKIPPED on=0` on the NEW build
(GameEngine no longer forces it off) - so something else killed it: the
GameLOD heuristic `if (!m_memPassed || isReallyLowMHz()) m_shellMapOn=false`
inside `applyStaticLODLevel`.

**Root cause (reproduced locally with raw numbers in the breadcrumb):**
`m_numRAM` is a **signed Int** (GameLOD.h:216) but is filled from
`GlobalMemoryStatus.dwTotalPhys` (unsigned). Any machine with >=2GB overflows:
`numRAM(signed)=-1852006400, rawPhysMem=2442960896` -> the test
`(Real)(m_numRAM)/(256MB) >= 0.94` evaluates NEGATIVE -> `m_memPassed` stayed
FALSE on every modern PC. Consequences on the user's machine:
- shell map (menu background) switched off -> black background;
- `if (!m_memPassed)` in GameLOD forces LOW-RES textures globally;
- `requestedTrees = m_memPassed` -> no trees.
**Fix:** compare as `(UnsignedInt)m_numRAM >= 256MB`; the breadcrumb now
prints the signed value so this can never hide again (`memPassed=1` verified
locally). The `applyStaticLODLevel` kill switch itself was also removed (2003
speed/mem heuristic): background now controlled only by the INI `ShellMapOn`
and `RTS_NO_SHELLMAP=1` (opt-out re-verified: on=0 + stable 300 frames).
`RTS_FORCE_SHELLMAP` is no longer needed for wine - the memPassed fix lets the
shell map run there too.

**Mission-start crash (still open):** amdvlk AV at `VulkanBeginTargetPass`
again after the 2048x1024 upload, with **`graphics-hook64.dll` loaded inside
RTS.exe and interleaved in the stack** - that is the OBS/Streamlabs game
capture library; even with no streaming app intentionally running, something
is injecting it (Task Manager -> Details for any *hook*/overlay process;
Steam/Discord/RTSS use different DLLs). Breadcrumbs added to pin the AV:
`BeginTargetPass` logs #1-40 AND every 100th (brackets late crashes),
`VK: tex create init WxH fmt from=<initiator> n=` samples who creates textures
(every 64th - names the churn source behind the descriptor-in-flight theory),
and uploads >=1024px are always logged (the crash follows one).

**Dropdowns:** log #3 had no dropdown interaction; the
`VK: combo label fetched ...` + `VK: Accept reads resolutionIdx/detailIdx`
lines will show what fails to stick on the next run.

Status: background unblocked on three counts (RefPack decode + kill-switch
removal + memPassed overflow); awaiting one field run of THIS build for
(a) background animation visible, (b) dropdown label/Accept lines,
(c) mission-crash breadcrumbs.

### 2026-10-15 (5) — Wine debug round 2: crash narrowed to wine-side; latent in-flight UAFs fixed; release cut

Kept re-running/debugging in wine (runs 50-67). Local shell-map crash is
deterministic at the FIRST rendered shell-map frame (after `pipeline build #16`
- the first pipeline variant the menu never uses): a PE-named thread's RIP
lands in a **freed wine arena executing zeros** (corrupted stack, no RTS.exe
frames on it). Ruled out by experiment: loader thread (not started -
`RTS_SYNC_TEXTURES=1` gates `TextureLoader::Init` too), synchronous texture
loads, audio (`-noaudio` AND a 4KB->256KB `AUD_service_thread` stack fix),
lavapipe thread count (`LP_NUM_THREADS=1`), texture churn (fixed, still
crashes), `Reset_Device` (never called - breadcrumb proves it), winedbg
doesn't catch it either (unix signal). Shape says wine/lavapipe internals or a
call sequence specific to that first lit pipeline - Win10/AMD did NOT die
here (it ran to mission load), so this one is treated as wine-side and
deferred; the field log with `BeginTargetPass#N` brackets decides if AMD has
its own version.

**Latent driver-UAF fixes made while hunting (real bugs regardless):**
- `DX8Wrapper::Reset_Device` / `Release_Device` now `vkDeviceWaitIdle` BEFORE
  destroying pipelines/textures/RT caches (they destroyed objects referenced
  by up to MAX_FRAMES_IN_FLIGHT in-flight command buffers; triggered by
  window-activate resets) + `VK: Reset_Device(reload=N)` breadcrumb.
- `AUD_service_thread` stack 4KB -> 256KB (one page, no guard, ran during the
  busiest load moments).
- `RTS_SYNC_TEXTURES=1` keeps the loader thread unstarted AND forces
  synchronous loads (escape hatch / diagnostic).

**Open items found by the live wine session (menus, 800x600, RTS_NO_SHELLMAP):**
- `VK: upload w=128 h=128 fmt=23 pitch=384` x 32771 in ~30s (~1000/sec):
  variable-width sentence textures still thrash the per-slot texture cache
  (widths alternate 64/128/256 -> cache slot invalidated -> create+upload
  storm). Perf sink; next pass should key the cache by width, not slot index.
- Game runs stable otherwise (30fps, no crash, fonts/menu fine).

Release: v1.1 zips rebuilt with everything from (1)-(5).

### 2026-10-15 (6) — Field log #4: mission crash fixed (NULL controlling player), upload storm root-caused (radar DrawPixel uploaded the whole surface per pixel), background still black with proof it never draws

**Training mission crash - OUR bug, fixed:** FR stack resolved to
`GameLogic::startNewGame -> ThingFactory::newObject -> Object::initObject ->
Object::updateUpgradeModules` with `fault=0x4` (NULL + 4): objects created
during startNewGame can have no controlling player yet, and
`getControllingPlayer()->getCompletedUpgradeMask()` dereferenced it. Fixed with
NULL guards (controlling player -> mask 0; `m_behaviors` NULL -> return) in
Object.cpp.

**The w=128 upload storm - root-caused and fixed:** your log had 65,540
`upload w=128 h=128 fmt=23` lines = EXACTLY 4 x 16,384 (=128x128 pixels).
Source: `W3DRadar` builds its terrain texture (`buildTerrainTexture`), shroud
rect and blips through `SurfaceClass::DrawPixel()`, which Lock->write->**Unlock
(= full-surface GPU upload) PER PIXEL** - ~786MB of redundant uploads per radar
rebuild (the EA comment even considered batching, meaningless in D3D8 managed
pool, catastrophic in Vulkan). Added `SurfaceClass::DrawPixelFast()` (no
Unlock) and a single Unlock per batch in all three radar loops. Verified
locally: 16,387 -> 2-5 uploads per run, stable 30fps, no crash.

**Black background - now proven to be a missing DRAW, not data:** their log
shows shellMapOn=1 (memPassed fix live: rawPhysMem=4.2GB), ShellMap1.map
opened, shell-map audio playing - but **only 16 pipelines ever build, all of
them the menu's** (`#1..#16`, same set as a no-shell-map run): the shell-map
3D scene never submits a single draw. Next: why W3DView/RTS3DScene render is
skipped while the game session runs.

Also: `VK: Begin_Texture_Load` name breadcrumb (storm attribution) and
`caller=` on upload lines.

Status: mission-crash + upload-storm fixes verified locally; awaiting field
run (mission start + background investigation log).

### 2026-10-15 (7) — Validation layers found the real root cause: R8G8B8/PACK16 formats are unsupported on modern AMD; fixed

User ran the game under VK_LAYER_KHRONOS_validation on the RX 7900 XTX
(`log/log/vkrts.txt`, 57MB full trace). The errors name the bug chain that
produced every symptom we chased (black background elements, shell-map AV,
mission AV, `upload w=128 fmt=23` lines):

- `vkCreateImage(VK_FORMAT_R8G8B8_UNORM, OPTIMAL, TRANSFER_DST|SAMPLED)`
  -> **VK_ERROR_FORMAT_NOT_SUPPORTED** (RDNA3 dropped optimal-tiling 24-bit
  RGB; only copy-dst features remain). Same for the PACK16 formats.
- `VkTexture::Create` returned false but LEFT m_device set with m_image=NULL
  -> every later UploadLevel/draw through that texture recorded commands
  against a dead image -> `VkImageView usage-02274`, `CopyBufferToImage
  dstImage-01997`, `commandBuffer-recording` invalidations -> amdvlk AV at a
  fixed address (the identical 0x...8c0 IP in all three field crashes) and
  the black menu/background elements that never drew.

**Fixes:**
- `VkTexture::TranslateFormat`: D3DFMT_R8G8B8 / R5G6B5 / X1R5G5B5 / A1R5G5B5 /
  A4R4G4B4 / X4R4G4B4 all promote to **VK_FORMAT_B8G8R8A8_UNORM** (every
  CPU-side source in this port is a 32-bit A8R8G8B8 shadow, so this also kills
  the 3-byte-pitch misreads the fmt=23 uploads were doing).
- `VkTexture::Create`: any creation failure now `Destroy()`s (m_device=NULL),
  so the existing NULL-device guard in UploadLevel turns later uses into a
  logged no-op instead of invalid commands.
- Dynamic VB grow path (`DynamicVBAccessClass::Allocate_DX8_Dynamic_Buffer`)
  now `vkDeviceWaitIdle`s before releasing the shared vertex buffer - the
  validation trace showed `VkBuffer ... was destroyed` invalidating a
  recording command buffer (the secondary error cluster).

Local wine still dies at the same first-shell-map-draw moment (lavapipe
accepts R8G8B8, so this is a separate wine-side issue as documented in (5));
the AMD side is what the validation log proved and what this fixes.

Status: field candidate - needs one Win10 run (menu background + mission start)
to confirm the AVs and the black elements are gone.

### 2026-10-15 (8) — The format fix missed the main path: SECOND translate table in dx8wrapper patched

Field run after (7) STILL showed `vkCreateImage(VK_FORMAT_R8G8B8_UNORM) ->
FORMAT_NOT_SUPPORTED` in the new validation trace: (7) only patched
`VkTexture::TranslateFormat` (used by SurfaceClass), but the texture pipeline
goes through `DX8Wrapper::_Create_Vulkan_Texture` -> **`VulkanTranslateWW3DFormat`
(dx8wrapper.cpp) - a second, separate translation table** that still emitted
R8G8B8/PACK16. Same promotion applied there now (R8G8B8/R5G6B5/X1R5G5B5/
A1R5G5B5/A4R4G4B4/X4R4G4B4 -> BGRA8). Local sanity: stable 314 frames,
0 create failures.

Also from that field run: shell-map load is slow (14.7s frame + 4s frame)
which should improve now that textures stop failing and retrying; crash still
"intermittent same-IP" - consistent with partially-broken texture set.

### 2026-10-15 (9) — Remaining validation errors fixed: shared samplers (the in-flight destroy AV), DemoteToHelperInvocation feature, dummy-VB usage

The 16:02 validation trace (with the complete format promotion in place -
0 R8G8B8 errors) still listed three real bugs, all fixed now:

1. `vkDestroySampler: sampler in use by a descriptor set in use by a command
   buffer` - samplers were per-texture and destroyed with the texture, while
   frames referencing them could still be in flight. This is the most likely
   cause of the fixed-address amdvlk AV. Samplers are now a process-wide
   shared cache (4 filtered/repeat combos, never destroyed at runtime).
2. `VkShaderModuleCreateInfo-pCode-08740` - mesh.frag declares
   DemoteToHelperInvocation (discard) but the feature was never enabled:
   the shader module was spec-invalid = undefined behavior at first use (first
   mesh draw = exactly where both platforms die). Now enabled via
   VkPhysicalDeviceVulkan13Features in vkCreateDevice.
3. `vkCmdCopyBuffer: dstBuffer VERTEX_BUFFER without TRANSFER_DST` -
   VulkanDummyVB was created VERTEX_BUFFER-only and then Upload()ed through a
   staging copy. TRANSFER_DST added.

Local wine: shell map still dies at the same first-draw instant (lavapipe
side, unchanged by these three - they are AMD/validation-driven fixes).
Also noted: a PRE-EXISTING shutdown crash in FontCharsClass::~FontCharsClass
(delete[] of a corrupted pointer) that only surfaces when the process exits
cleanly; not yet investigated.

### 2026-10-15 (10) - World-view update was dead since the port (the real black background gate) + dynamic-buffer retirement

Round-2 validation trace (formats clean) still showed the invalidation cluster
(`VkBuffer destroyed` -> command buffer invalidated) plus a new
`vkDestroyImageView in use`. Fixes:

1. **Dynamic VB/IB grow destroyed the old buffer MID-FRAME** - the command
   buffer being recorded may already bind it (the shell map's first big mesh is
   the first draw that exceeds the shared dynamic buffer and triggers a grow).
   Grow now RETIRES the old buffer into a list; `Flush_Retired_Dynamic_Buffers`
   (called from VulkanDevice::BeginFrame) destroys retired buffers only after
   `vkDeviceWaitIdle`. Covers the DX8 + sorting VB and the IB.
2. **Samplers destroyed while in flight** (vkDestroySampler-in-use): now a
   process-wide shared sampler cache, never destroyed at runtime.
3. **mesh.frag's DemoteToHelperInvocation capability** was never enabled ->
   the shader module was spec-invalid (pCode-08740) = UB at first mesh draw;
   enabled via VkPhysicalDeviceVulkan13Features.
4. VulkanDummyVB gets TRANSFER_DST (it is Upload()ed).

**And the black background's real gate:** `W3DDisplay::draw` updated the world
views only `if (DX8Wrapper::_Get_D3D_Device8() && TestCooperativeLevel()==D3D_OK)`
- but `_Get_D3D_Device8()` is a Vulkan-port STUB that returns NULL, so
**updateViews() never ran in the entire port** (the shell-map session ran,
audio played, but the view/camera/scene were never prepared). Gate replaced
with `VulkanDevice::Instance().IsInitialized()`.

**Local wine after all of the above:** the shell map now SURVIVES its first
draw (previously died at frame 11; now runs to the timeout), but pipelines are
still 16 (menu-only) and the background stays black - so `drawViews()` still
 submits nothing; next: what the view list needs for the shell-map world
(primaryW3DView attachment / view->draw path). Also logged: a pre-existing
shutdown crash in FontCharsClass::~FontCharsClass (only on clean exit).

### 2026-10-15 (11) - Mission crash now localised: garbage in Object::m_behaviors (heap corruption); background still not drawing

Field log (16:30): the updateUpgradeModules crash persisted after the NULL
controlling-player guard. Disassembly of the crash site: `mov (%rsi),%rcx`
then `mov (%rcx),%rax` with **rcx=4** - the first element of `m_behaviors`
literally contained the value 4 (the array itself is allocated correctly:
size = behaviorCount + NUM_SLEEP_HELPERS(5) + 1, helpers fill slots 1..5,
NULL terminator written; so the slot was CORRUPTED after init, or the object
read is stale) -> another x64/heap-corruption thread, needs its own session.
Defensive fix: updateUpgradeModules stops at a null-page/garbage module
(capped `VK: updateUpgradeModules stopped on garbage module` log).

### 2026-10-15 (12) - The world renders again: scene submits ~145k polys/frame locally (the user's 16:30 test predated the updateViews fix)

After the updateViews() gate fix (87be5f6) the local shell-map session now
renders: `VK: scene Render #N objects=194 polys=145647 skinPolys=0 ...` -
i.e. the terrain + objects are being submitted every frame (the counter is
recorded at Draw_Triangles, so these are real draws). The user's last field
run (16:30) was from a build BEFORE that fix, so the black background there
proves nothing about the current build.

Added for the next field run: `VK: scene Render objects/polys/skin/sort`
counters in RTS3DScene::Render, `VK: Mesh::Render calls/hidden/drawn`, and
`VK: drawViews #N viewCount=` - one look at the log will show whether the
world draws on the AMD box (polys > 0) or where it stops.

### 2026-10-15 (13) - World draws on the AMD box too (144k polys/frame) - black fragments traced to the lit-shader color path

Their log (new build) shows `scene Render objects=194 polys=144198` per frame
and 241k Mesh::Render draws: the shell map IS rendered on the RX 7900 XTX -
the draws just produce black fragments. Traced to mesh.frag: it folded the
material diffuse/opacity into the vertex color UNCONDITIONALLY, but D3D8 only
does that for LIT passes - unlit/prelit meshes (terrain: material diffuse is
typically black) must use the vertex color alone, so every terrain fragment
became 0*texture = black. Fixed: material diffuse/opacity only applied when
LIGHTING==1. Added `VK: FillUBO #N ambient/light0/material` breadcrumb (first
12 + every 600th) so the next field log shows the actual lighting state.

Local run after the fix: scene keeps drawing (polys ~1M cumulative counter),
ambient defaults to white (0x12345678 sentinel -> 0x00ffffff), light0 off.

### 2026-10-15 (15) - THE black background: double-transposed model-view-projection clipped every vertex

Matrix capture (VK: PROJ/VIEW/MVP-FULL in the draw path) proved the
projection and view matrices are individually correct (GL-style projection
with a valid perspective row; view with translation in column3) but the
COMPOSED MVP's clip-w row was `[-5.39, -5.83, -6.09, 0]`, i.e.
w = -5.39x -5.83y -6.09z for every vertex -> `|-w <= z <= w|` failed for all
of them -> zero rasterized fragments -> black world while every draw still
submitted (matches all field logs: polys counted, viewport correct, no
validation errors, black screen).

Cause: `mvp = mvp.Transpose()` (and the matching `world.Transpose()` for
normals) in VulkanDrawIndexed double-transposed - WW matrices are already
laid out so GLSL's column-major read yields the correct row-vector transform.
After removing both transposes the captured MVP's w-column is exactly
`[0, 0, -1, 0]` (w = -z, correct). Menus were unaffected (identity
projection is transpose-invariant), which is why UI worked while the world was
black.

Local verify: clean run, 317 frames, no crash, MVP w-column correct.

### 2026-10-15 (16) - Full 3D/world render pipe review: UBO ring stride bug fixed, rest of the path checked

Reviewed the whole draw pipeline (W3DDisplay::draw -> updateViews/drawViews ->
W3DView -> RTS3DScene::Render/Flush -> DX8Wrapper::VulkanDrawIndexed ->
BeginTargetPass/EndFrame/present) for logic errors:

**Found + fixed - UBO ring stride smaller than the struct:**
`VkFrameUniforms` is **352 bytes** but `VULKAN_UBO_ALIGN` (the ring stride)
was a flat **256** -> every UBO entry overlapped the previous one's tail
(worldViewProj/world stomped by the next draw's header) and the last entries
of each frame overran the buffer (descriptor range past the allocation).
Survivable only because the shaders happen to read the first 224 bytes.
Stride is now `round-up(sizeof(VkFrameUniforms), 256)` (512); ring buffer
size follows the same constant.

**Checked and correct:**
- depth clear = 1.0, D3D ZFUNC -> VkCompareOp table (LESS etc.), depth
  loadOp CLEAR, per-pass clears;
- viewport/scissor math for full-screen passes (flip + scissor offset both
  resolve to the full extent when vp.Y=0);
- UBO ring reset per frame + per-frame descriptor pools (2 frames in flight);
- index binding: shared dynamic IB bound at byte0 with firstIndex in ELEMENT
  units (matches DynamicIBAccessClass::IndexBufferOffset convention);
- descriptor allocation failure path (logs skips; none in field runs);
- swapchain pass layouts (UNDEFINED -> PRESENT_SRC, CLEAR ops).

**Still pending the magenta-probe field test** (pushed f850827): distinguishes
vertex/texture DATA from framebuffer targeting as the black-screen cause.

### 2026-10-15 (17) - PACKAGING BUG: shader fixes never shipped - the zips carried the original Oct-13 SPIR-V

The magenta probe (and every earlier shader fix - material diffuse gating)
"did nothing" in the field because **the release zips still contained
`shaders/mesh.frag.spv` compiled on 2026-10-13** - my release commits only
updated RTS.exe/PROGRESS, never the shader SPIR-V next to it (Create_Device
loads `shaders/mesh.*.spv` from beside the exe). Verified: zip spv md5 != build
spv md5 before the fix. Both zips now carry the rebuilt SPIR-V (mesh.frag.spv
4172B, matches the build).

Lesson recorded: **any change under Shaders/ must be re-zipped** - future
release commits must refresh shaders/ too (the shader compiler only runs in
the local build).

Next: the user's build now actually runs the magenta+depth-off+cull-off
probe - their answer (magenta everywhere vs world still black) decides
data-path vs geometry-path.

### 2026-10-15 (18) - Probe series concluded: geometry/material/vertex-colors all verified white; the black maker is FOG - fix shipped with the real shader

Probe ladder results (each verified to actually load via `VK: shader loaded
bytes=` breadcrumb - sizes: original6932, v2 4172, v3 4308, final 7116):
- **cull off + depth off + forced magenta: whole screen magenta** ->
  geometry, MVP, viewport, scissor, descriptors, blend all fine; the world
  DOES rasterize.
- **v3 (raw vertex diffuse): world solid white, menus colored squares** ->
  vertex color = white everywhere (inDiffuse fine), materialDiffuse=(1,1,1)
  (FillUBO log), white-fallback texture valid (whiteImg/view non-null),
  ambient=white sentinel, light0 off.

So every input to the real shader is WHITE - the only remaining way it turns
black is FOG: `fogParams` come from a bit-cast of the D3D states, and when
the game never sets them they stay the 0x12345678 sentinel -> denormal ~0 ->
fogEnd = ~1 -> factor = clamp(viewDist/1) = 1 for EVERY fragment ->
fogColor(black) over the whole world = the pitch-black background (and "as if
render was snubbed"). **Fix: sentinel fog params now mean fog OFF (1e30 range)
in VulkanFillFrameUniforms**; FillUBO now logs fogEnable/fogStart/fogEnd/
fogColor, and the pipeline-build log includes fog/lighting/alphaTest.

Probes removed, depth+cull restored to state-driven, real shader (7116B)
shipped WITH the zips (shaders always zipped together now).


### 2026-10-15 (20) - THE view-matrix bug: Matrix3D::Get_Inverse was a stub returning the matrix unchanged

After fixing the MVP order, the field log PROJ/VIEW/MVP-FULL chain matched the
matrices EXACTLY (MVP row3 = origin*view*proj) - and exposed that the view
matrix yields z_view=+319 for the shell-map origin. camera.cpp:608 states
"Forward is negative Z in our viewspace" and Get_D3D_Projection_Matrix maps
z_view in [-far,-near] to [0,1] with w=-z_view -> an in-front origin must be
-319. +319 => w=-319 < 0 => every vertex clipped => pitch black.

Root cause: `Matrix3D::Get_Inverse` (matrix3d.cpp:511) was a PORT STUB -
`{ /* simplified inverse */ mat4Inv = mat4; det = 1.0f; }` returned the
matrix UNCHANGED. CameraClass::Apply therefore stored the camera WORLD
transform as the VIEW matrix. The same stub silently broke
mesh.cpp:1234 `world.Get_Inverse(world_to_obj)` (picking/model-space work).

Fix: real affine inverse [R^-1, -R^-1*t; 0 1] via cofactors (singular ->
falls back to Get_Orthogonal_Inverse). MVP order fix (world*view*proj) and
this inverse fix together complete the transform chain end to end.


### 2026-10-15 (19) - Root cause of the black world: MVP composed in COLUMN-vector order (proj*view*world) for ROW-vector matrices

Field log `VK: PROJ/VIEW/MVP-FULL` (row-major) gave the smoking gun:
- PROJ is a sane Vulkan-style matrix (m22=-1.0008, m23=-1, m32=-10.0083);
- VIEW has translation in the LAST ROW (961.3, 727.5, 319.6) = row-vector
  convention (v*M, GLSL reads M^T*v == v*M, raw upload correct);
- but MVP = proj*view*world (column-vector order) -> diagonal showed
  (-1.575, -1.704, -320.16, 0): GPU computed w=-z_WORLD (never view-
  transformed), ndc_z=320 for every vertex -> clamped -> depth test ALWAYS
  FAILED vs clear 1.0 -> world rejected => black; negative x/y scales also
  flipped winding.
- Menus rendered correctly because for identity world/view
  proj*view*world == world*view*proj.

Fix: `Matrix4 mvp = world * view * proj;` (row-vector chain). Cull left
DISABLED this round as the bisect variable - re-enable next round to confirm
winding. All probes removed earlier; colors verified normal by the user
(frag spv 7064B).

### 2026-10-15 (14) - Draw-level breadcrumbs: viewport + fvf per 3D draw (next discriminator)

The unlit-shader fix is in and the world still draws black on both platforms
although the dummy vertex fallback supplies white diffuse for FVF-without-
diffuse formats (terrain 0x112). Note: locally (800x600) terrain draws use
vp=800x600 (full screen) - but that may be the MENU's leftover full-screen
viewport rather than the tactical view's: `View::init` leaves the view at
DEFAULT_VIEW 640x400 and NOTHING at startup resizes it to the real display
(only Display::setDisplayMode on a runtime resolution change, and the
ControlBar in-game). The3D camera viewport is normalised against
TheDisplay->getWidth() (W3DView.cpp:199-217), so a 640x400 view on a 1920x1080
window renders the world into a corner -> mostly black screen.

Added `VK: Draw3D #N fvf= vp=..@x,y idx=` (first60 + every2000th) so the next
field log shows the actual terrain viewport at 1080p. If vp != 1920x1080 ->
fix = size TheTacticalView to the display at startup (View::init /
GameClient init).

Background: pipelines still 16 after the updateViews() gate fix - the view
updates now but `drawViews()` still submits no 3D draws. Next thread: what
drawViews needs (view/world attachment for the shell-map session).

Also: `UploadLevel ... NULL device (w=128)` appears right before the crash -
a destroyed texture still referenced; related to the same corruption family.

### 2026-10-15 (4) — Wine re-test: sentence-texture churn eliminated (896->0 creates); shell-map render crash narrowed (not the loader thread)

Re-tested in wine+lavapipe with the memPassed fix (shell map now runs locally
without any force flags: `shellMap check ... shellMapOn=1`, map opens).

**Fixed this pass — the texture churn:** the `VK: tex create init ... from=`
sampler named the churn source: `Render2DSentenceClass::Build_Textures()` was
constructing a NEW procedural `TextureClass(64x64)` for every sentence build -
**896+ GPU images created during boot alone** (and ~400/sec in-game earlier),
each destroyed soon after: create/destroy of images that recent frames may
still reference (2 frames in flight) = the leading suspect for both driver
crashes. `Build_Textures` now reuses a per-slot cached texture
(`CachedTextures`/`CachedTextureWidths` members, created on first use or size
change, owned by the sentence object, released in the dtor). Verified:
`procedural TextureClass` creates **0** in a full boot (was 896+).

**Still crashing locally (unchanged spot):** first shell-map frame - map load
frame takes ~21s, next frame builds `pipeline build #16 ... cull=1` (a state
variant never used by the menu), then a SIGSEGV in a game-created thread with
a garbage backtrace (Thread 9, address in an unmapped/JIT region - lavapipe
shader-JIT shaped). Ruled out this pass: texture churn (fixed, still crashes)
and the loader thread (`RTS_SYNC_TEXTURES=1` forces all loads synchronous via
`Add_Load_Task` - crash unchanged). Next: the new pipeline's first use
(draw/descriptor state for lit/terrain materials) - try disabling the shader
JIT (LP_NATIVE_VECTOR_WIDTH/GALLIVM envs) to confirm, then audit the shell-map
material/descriptor setup for that variant.

**Also in this build (from the earlier passes of today):** LOD gate
breadcrumb prints the raw signed numRAM, `RTS_SYNC_TEXTURES=1` synchronous
texture loads, initiator/procedural texture samplers, BeginTargetPass
#1-40+every-100, Accept/combobox-label breadcrumbs.











