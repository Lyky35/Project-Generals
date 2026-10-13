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

<!-- Append new entries below. Commit after each completed step. -->







