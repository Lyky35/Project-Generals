# Project Generals — Development Notes

Forked from: `electronicarts/CnC_Generals_Zero_Hour`
Target: Windows 10+ x64, Vulkan 1.3, MinGW-w64 cross-compile from Linux
Rewrite of old notes (see `NOTES.OLD` for history). Status below was verified against the tree on 2026-10-12, including a real cross-compile build.

---

## Status Overview

| Phase | Claimed before | **Verified status** |
|-------|----------------|---------------------|
| 1. x64 port | Complete | **Complete** — remaining `__asm` is guarded (`_M_IX86`/`__ICL`) with C++ fallbacks |
| 2. Win10 windowing | Complete | **Complete** — DPI awareness, `RegisterClassEx`, `WM_DPICHANGED`, `-borderless` present in `WinMain.cpp` |
| 3a–3d. Vulkan core (device/buffers/shaders/descriptors) | Complete | **Complete** — `VulkanRenderer` builds clean |
| 3e. WW3D2 render-state translation | Complete | **Mostly done** — `dx8wrapper.cpp` heavily ported, but callers still reference removed D3D APIs (see Issues) |
| 3e-5/6. Texture & asset system | "Complete" (heading) / "Planned" (body) | **NOT complete** — `texture.cpp`, `surfaceclass.cpp`, `textureloader.cpp` fail to compile (~86 errors) |
| 3e-7. Shadow render-state (Generals tree) | Complete | **Complete** — code migrated to `DX8Wrapper` Vulkan calls |
| 3e-8. GeneralsMD mirror | "Complete" (stage text) / "Not Started" (status) | **NOT started** — `GeneralsMD/.../dx8wrapper.cpp` has **zero** `Vulkan`/`VkTexture` references |
| 4. SDK emulation (Miles/Bink/Granny/DSound) | All 4 ported | **Headers + comments only** — no real OpenAL/FFmpeg/SDL2/GLM calls; some files syntactically broken |
| 5. Integration & testing | Pending | **Pending** |
| Full RTS.exe | Placeholder only | **Does not build** — blocked on WW3D2 texture errors |

---

## What Actually Works (verified by build)

Cross-compile: `x86_64-w64-mingw32-g++` (GCC 16.2.0), CMake 4.4.4, `Unix Makefiles`.

Targets that **build clean**:
`VulkanShaders`, `WWDebug`, `Compression`, `VulkanRenderer`, `WPAudio`, `WWLib`, `WWSaveLoad`, `WWMath`, `WWUtil`, `WWAudio`, `GameEngine` (11/15 library targets).

- `VulkanRenderer`: VkDevice (1.3 instance, swapchain, render pass), VkGpuBuffer, VkTexture, VkShaderManager (SPIR-V + pipeline cache), VkDescriptors. Shaders (`mesh.vert`/`mesh.frag`) compile to SPIR-V at build time.
- `GameEngine` links. All core engine sources compile.
- WinMain.cpp: full Win10 windowing (DPI PMv2, borderless flag, monitor-native fullscreen).

---

## Issues Found (blocking or dangerous)

### 1. WW3D2 texture subsystem does not compile — **blocks RTS.exe**

86 errors across 6 files. The texture port was started (headers renamed) but `.cpp` callers were not updated:

- `texture.cpp` (33 errors): references `D3DTexture`, `Peek_DX8_Texture`, `IDirect3DTexture8*` ctor — all removed.
- `textureloader.cpp` (26 errors): calls `DX8Wrapper::_Create_DX8_Texture()` (gone), returns `VkTexture*` where `IDirect3DTexture8*` expected.
- `surfaceclass.cpp` (18 errors): references `Peek_D3D_Surface`, `D3DSurface`, incomplete `IDirect3DSurface8`.
- `texproject.cpp`, `prim_anim.h`: `IDirect3DSurface8` not declared.
- `textdraw.h`: `#include "../wwmath/rect.h"` — path broken.
- `w3d_dep.cpp`: `size_t` not declared (missing `<cstddef>`).

`GameEngineDevice` and `RTS` targets are blocked behind `WW3D2`.

### 2. SDK "ports" are stubs — will not produce working audio/video/animation

- `MilesAudioManager.cpp`: includes `<AL/al.h>` but **zero** `alGen*`/`alcOpenDevice` calls. Callbacks are comments only.
- `BinkVideoPlayer.cpp`: **syntactically broken** — 4 orphan `#endif` with no matching `#if`; `createStream()` parameter replaced by a comment (`createStream( // HBINK... )`); `BinkOpen` replaced by comment. Would not compile even if reached.
- `W3DGranny.cpp`: still calls `granny_*` APIs (`GrannyGetFileInfo`, `granny_model`). Only the blending buffer was swapped to `glm::vec4`. `#pragma comment(lib, "granny2")` remains.
- `AUD_DSoundDriver.cpp`: includes SDL2/OpenAL but no real `SDL_OpenAudio`/`alcOpenDevice` usage; compiles with warnings.

The old notes' "✅ Ported" table overstated all four. Reality: headers added, function bodies are placeholders.

### 3. GeneralsMD mirror is not done

`GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/dx8wrapper.cpp`: 0 matches for `Vulkan`/`VkTexture` (vs 161 in the Generals tree). No `VulkanRenderer` exists under GeneralsMD. Phase 3e-8 is genuinely not started; old "mirror complete" claims were wrong.

### 4. CMake link line is incomplete for the real SDKs

`Main/CMakeLists.txt` links `openal` and `SDL2` by raw name but **no FFmpeg libraries** (comment mentions them; link line omits them). No `find_package(OpenAL/SDL2/FFmpeg)` at top level — relies on system MinGW libs being present.

### 5. CI is a stub, not a real build

`.github/workflows/ci-crosscompile.yml` compiles a standalone `WinMain.cpp` with `MessageBox` — it does not build the game. `windows-build.yml` uses Linux paths (`/home/cyklon/...`) on a Windows runner. Neither validates the actual tree.

### 6. Release artifacts are placeholders

`release/RTS.exe` and the zips contain a stub binary ("RTS Build Placeholder"), not a functional game.

---

## Build Commands

```bash
# Configure (cross-compile from Linux)
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=/usr/bin/x86_64-w64-mingw32-g++ \
  -DCMAKE_C_COMPILER=/usr/bin/x86_64-w64-mingw32-gcc

# Build everything (currently fails on WW3D2)
cmake --build build --target RTS -j$(nproc) -- -k

# Build only what works today
cmake --build build --target GameEngine -j$(nproc)
```

Dependencies: `g++-mingw-w64-x86-64`, `libopenal-dev`, `libsdl2-dev`, `libavcodec-dev libavformat-dev libswscale-dev`, `libglm-dev` (for the eventual Granny path).

---

## Next Steps (priority order)

1. **Fix WW3D2 texture subsystem compile errors** (~86 errors, 6 files). This is the single blocker for linking RTS.exe. Finish the half-done rename: update all `D3DTexture`/`Peek_DX8_Texture`/`_Create_DX8_Texture` callers to the Vulkan equivalents already present in `DX8Wrapper` (`_Create_Vulkan_Texture`, `Peek_Vulkan_Texture`, etc.). Fix the two trivial errors in `textdraw.h` and `w3d_dep.cpp` first (they take 2 minutes).
2. **Link RTS.exe** — once WW3D2 builds, resolve any GameEngineDevice link errors and produce a real `RTS.exe` (not the placeholder).
3. **Repair `BinkVideoPlayer.cpp`** — restore valid preprocessor structure and function signatures; then decide whether to implement real FFmpeg decode or keep a no-op stub that compiles.
4. **Implement real SDK backends** (replace the comment-stubs):
   - Miles Audio → OpenAL Soft (device init, buffer queueing, 3D sources)
   - DirectSound/WPAudio → OpenAL + SDL2 (actual `alcOpenDevice`/`SDL_OpenAudio` calls)
   - Granny → keep the `glm::vec4` buffer but wire real TinyGLTF model loading (or stub out `granny_*` calls cleanly)
   - Bink → FFmpeg decode + Vulkan texture upload
5. **Add FFmpeg to the link line** in `Main/CMakeLists.txt`.
6. **Rewrite CI** to run the actual `cmake --build build --target GameEngine` (and eventually RTS) instead of the MessageBox stub.
7. **GeneralsMD port** — mirror the Generals-tree Vulkan changes. Do this only after the Generals tree links and renders, to avoid porting an unfinished design twice.
8. **Runtime testing on Windows 10** — bundle `OpenAL32.dll` + `SDL2.dll`, verify window creation, then audio, then first-frame render.

---

## Key Architectural Decisions (unchanged)

- Native Vulkan 1.3 backend (no DX8 translation layer, no DXVK).
- C++17, CMake, MinGW-w64 x86_64 cross-compile from Linux.
- Case-insensitive symlinks (`vkdevice.cpp` → `VkDevice.cpp`, etc.) preserve original Windows filenames for the three case variants.
- STLport replaced with MSVC STL / MinGW std.
- GameSpy, BrowserEngine, SafeDisc removed; Miles/Bink/Granny replaced with open-source equivalents.

---

**Verified**: 2026-10-12 against commit `cbcf667`
**Build state**: 11/15 library targets build; `RTS` blocked on WW3D2 texture errors
**Repo**: `Lyky35/Project-Generals`
