# Project Generals — Development Notes

Forked from: `electronicarts/CnC_Generals_Zero_Hour`
Target: Windows 10+ x64, Vulkan 1.3, modern toolchain

---

## Phase 0: Audit & Infrastructure

**Status:** Complete

### Steps Taken
- Forked EA source release to `Lyky35/Project-Generals`
- Source uses C++98, STLport 4.5.3, Visual Studio 6.0 `.dsp/.dsw` projects
- Created CMake build system with x64 target, MSVC 2022 support
- Created CMakePresets.json (windows-x64-msvc, windows-x64-msvc-debug, linux-x64-gcc, linux-x64-clang)
- Created stub libraries: GameSpyStub, DX8Compat, FFmpegVideo, VulkanRenderer, XAudio2Engine
- Audited all x86 assembly locations (~30 critical, ~20 medium)
- Audited pointer-truncation risks (~50 locations)
- Mapped full dependency tree (15 projects, complete dependency graph)

### Audit: x86 Assembly Locations

| Category | Count | Key Files | Severity |
|----------|-------|-----------|----------|
| RDTSC (timing) | 8 | PerfTimer.h, cpudetect.cpp, mpu.cpp, wwprofile.cpp | Critical |
| CPUID | 4 | cpudetect.cpp | Critical |
| x87 FPU math | 10 | wwmath.h, quat.cpp, matrix3d.cpp | Medium |
| SSE vector ops | 5 | vp.cpp (hot path) | Critical |
| RLE blit | 6 | rlerle.h (hot path) | Critical |
| LCW compression | 1 | lcw.cpp (~200 lines asm) | Critical |
| Spinlock | 3 | mutex.h, wwmemlog.cpp | Critical |
| Commented out | 3 | keyboard.cpp, matrix3d.cpp | Low |

### Audit: Pointer-Truncation Risks

| Category | Count | Severity | Hot Path |
|----------|-------|----------|----------|
| `(DWORD)` cast on pointer | 8 | Critical | No (tools) |
| `(int)` cast on pointer arithmetic | 8 | Medium | No |
| `DWORD` for pointer-sized handles | ~20 | Medium | Yes (shaders) |
| `GetWindowLong` with `GWL_USERDATA` | 12 | Critical | No (tools) |
| `sizeof` on dereferenced pointers | ~10 | Low | — |

### Dependency Graph (15 projects)

```
RTS.exe
├── GameEngineDevice.lib
│   ├── GameEngine.lib
│   │   ├── Compression.lib
│   │   └── WWLib.lib → WWDebug.lib
│   ├── WW3D2.lib
│   │   ├── WWLib, WWMath, WWDebug, WWSaveLoad, WWAudio
│   │   └── VulkanRenderer, DX8Compat, GameSpyStub
│   ├── WWLib, WWDebug, WWMath, WWSaveLoad
│   └── XAudio2Engine, FFmpegVideo
├── WW3D2.lib (same as above)
└── WWDebug.lib → WWLib.lib
```

### Issues & Solutions

| Issue | Impact | Solution |
|-------|--------|----------|
| VS6 `.dsp/.dsw` projects unusable | No modern build | CMake build system created |
| STLport 4.5.3 requires patching | Won't compile on MSVC 2022 | MSVC STL (C++17) |
| GameSpy SDK unavailable (50+ functions) | No multiplayer | GameSpyStub with no-op stubs |
| BrowserEngine.DLL proprietary | Missing dependency | Removed; not needed for gameplay |
| SafeDisc DRM | Blocks launch on modern Windows | Remove entirely |
| Miles Sound System unavailable | No audio | XAudio2Engine stub (Phase 4) |
| Bink Video unavailable | No cutscenes | FFmpegVideo stub (Phase 4) |

---

## Phase 1: x64 Port

**Status:** Complete

### Steps Taken
- Removed all x86 inline assembly from game + library code (or guarded to `_M_IX86` with C++ fallbacks)
- Replaced spinlocks with `_interlockedbittestandset` loops
- Ported `LCW_Comp` (~200 lines asm) to portable C++; verified with 13-case round-trip test suite (all pass)
- Fixed `DWORD` shader-handle storage → `DWORD_PTR` in renderer
- Fixed `GetWindowLong`/`SetWindowLong` → `Ptr` variants in tools
- Fixed `(int)` pointer-arithmetic casts → `ptrdiff_t`-safe `(int)(p - q)`
- Ported crash-reporting (`Except.cpp`, `StackDump.cpp`, `debug_stack.cpp`) to x64 (`RtlCaptureContext`, `Rip/Rsp/Rbp`, `IMAGE_FILE_MACHINE_AMD64`, XMM dump)
- Fixed `LCW_Uncomp` pointer truncation (`(unsigned)dest_ptr` → `(uintptr_t)dest_ptr`)

### Assembly Disposition

| File(s) | Action |
|---------|--------|
| `PerfTimer.h`, `cpudetect.cpp`, `mpu.cpp`, `wwprofile.cpp`, `timingTest.cpp` | RDTSC/CPUID → `__rdtsc()`/`__cpuid()` |
| `wwmath.h`, `vp.cpp` | Already guarded (`_M_IX86` / `__ICL` with C++ fallbacks) — no change |
| `quat.cpp`, `matrix3d.cpp` | Removed dead `#if 0` asm blocks |
| `rlerle.h`, `blitblit.h` | Guard narrowed to `_M_IX86`; x64 uses generic C++ templates |
| `lcw.cpp` (`LCW_Comp`) | Full C++ port (see below) |
| `mutex.h`, `wwmemlog.cpp`, `profile/internal.h` | Spinlock → interlocked loop |
| `BaseType.h`, `dx8wrapper.h` (`Convert_Color`) | Guard + C++ fallback |
| `Except.cpp`, `StackDump.cpp`, `debug_stack.cpp` | `RtlCaptureContext` + x64 CONTEXT fields |
| `wwdebug.h`, `Except.cpp` (`_purecall`) | `_asm int 3` → `__debugbreak()` |
| `keyboard.cpp` | Removed commented-out asm |

### LCW_Comp Port Notes
Faithful transliteration of the x86 asm, plus hardening for pre-existing 16-bit format limitations found during testing:
- MED/LONG absolute offsets capped to `<= 0xFFFF` (format field is 16-bit); oversized matches fall back to short-run or literals
- RLE/match counts capped at `0xFFFF` (multi-chunk emit for huge runs)
- Clean termination when input consumed exactly (original emitted a garbage trailing byte)
- Empty-input early return
- Verified: 13-case round-trip suite (single byte, RLE 64/65/1000/70000, repeats, text, mixed, 100KB random) — all pass

### Pointer Fixes
| Location | Fix |
|----------|-----|
| `W3DShaderManager.h/.cpp`, `W3DWater.h`, `W3DTreeBuffer.h` (MD) | `DWORD` shader handles → `DWORD_PTR` |
| max2w3d tools (`GameMtlDlg`, `GameMtlPassDlg`, `gmtldlg`, `skin`) | `Get/SetWindowLong` → `Ptr` + `GWLP_*` + `(LONG_PTR)` casts |
| `huffencode.cpp`, `meshmdlio.cpp`, `geometryexporttask.cpp` | `(int)p - (int)q` → `(int)(p - q)` |
| `Properties.cpp` | `(DWORD)font` → `(LPARAM)font` |
| `autorun.cpp` | `(DWORD)hBrush` → `(LRESULT)hBrush` |

### Known Limitations (documented, not fixed)
- Crash-report address fields truncate above 4GB (`ULONG` log buffers) — cosmetic only
- `EDITSTREAM.dwCookie` pointer storage in Babylon tools (tools-only, not in build)
- `m_addr[256]` signature DB in `debug_stack.cpp` stores truncated 32-bit (cosmetic)
| SSE intrinsics in vp.cpp | May need AVX2 fallback | Use `_mm_*` intrinsics (available on x64) |

---

## Phase 2: Windows 10+ Compatibility

**Status:** Win32 layer complete; display-mode work deferred to Phase 3 (lives in DX8 renderer)

### Steps Taken (WinMain.cpp, both Generals + GeneralsMD)
- `RegisterClass` → `RegisterClassEx` with `hIconSm`
- Removed `WS_EX_TOPMOST` mixed into regular style bits (TOPMOST via `SetWindowPos` only)
- Added `WS_CLIPCHILDREN` to all window styles
- Fullscreen/borderless windows now sized to native monitor res (was hardcoded 800x600)
- Windowed mode centered via DPI-correct metrics
- `CreateWindow` args cleaned (`NULL` instead of `0L`)
- DPI awareness: `SetProcessDpiAwarenessContext(PMv2)` via `GetProcAddress` + `SetProcessDPIAware` fallback, called before window creation
- `WM_DPICHANGED` handler honors Windows' suggested rect
- New `-borderless` cmdline flag → `ApplicationIsBorderless` global (renderer honors in Phase 3); declared `extern` in `WinMain.h`

### Deferred to Phase 3 (renderer-owned)
| Issue | Reason |
|-------|--------|
| Exclusive→borderless display modes | `DX8Wrapper` sets D3D modes; changes with Vulkan swapchain |
| Resolution detection/scaling | Display enumeration moves with new renderer |
| `WM_DISPLAYCHANGE` auto-reset | Risky without live testing; revisit with Vulkan |
| 4,096-unit cap | Gameplay change, post-rendering milestone |

---

## Phase 3: Vulkan Renderer

**Status:** In progress (3a done)

### 3a. DX8Compat types header — DONE
`Libraries/Source/DX8Compat/d3d8types.h` replaces `d3d8.h`/`d3dx8.h`/`d3dx8math.h`
for all plain-data usage (273 identifiers inventoried across WW3D2):
- 15 files use COM interfaces → rewritten against Vulkan backend (3b–3e)
- 14 files use only enums/structs → build unchanged with this header
- D3DX *function* calls replaced at call sites (WWMath equivalents, Vulkan image ops)

### 3b. Vulkan device layer — DONE
`VulkanRenderer/VkDevice.*`: 1.3 instance + validation layers, Win32 surface,
discrete-GPU selection, graphics/present queues, swapchain (mailbox preferred,
FIFO fallback), depth buffer, render pass, framebuffers, 2-frame flight sync,
`BeginFrame`/`EndFrame`/present with swapchain recreation, one-time-submit
helper for uploads. Compile-checked with MinGW-w64 x64.

### 3c. Resource management — DONE
`VkGpuBuffer` (staging upload + host-visible mapping), `VkTexture` (D3D→Vulkan
format table incl. DXT1-5, mipmaps, GPU mipmap gen, sampler cache),
`VkDescriptorAllocator` (frame-reset pool, texture sets). Compile-checked.

### 3d. Shader/pipeline system — DONE
Uber-shaders (`mesh.vert`/`mesh.frag`, validated SPIR-V via glslangValidator):
transform, 2 texture stages (disable/select/modulate/add), 4-light vertex
lighting, linear fog, alpha test — all via 6 specialization constants.
`VkShaderManager`: SPIR-V loading, state-hashed pipeline cache, 3 canonical
FVF layouts (XYZ_N_UV1 / XYZ_N_D_UV2 / XYZ_D_UV1), D3D→Vulkan translators
(cull/fill/compare/blend/topology), fan→list converter, 128B push WVP.
CMake compiles GLSL→SPV at build time.

### 3e. WW3D2 file ports — IN PROGRESS

### Architecture Decision
Native Vulkan backend (no DX8 translation layer). This avoids DXVK overhead and gives full control over the rendering pipeline.

### Steps Taken
- VulkanRenderer stub library created
- DX8Compat interface layer defined

### Issues
| Issue | Impact | Solution |
|-------|--------|----------|
| D3D8 not supported on modern Windows | No rendering | Full Vulkan 1.3 renderer |
| Fixed-function → shader translation not 1:1 | Visual differences | Write new shaders matching visual output |
| WW3D2 API surface is enormous (144 files) | Long implementation time | Incremental: get basic rendering first |
| Texture format differences | Corrupted textures | Use BCn compressed formats, verify with RenderDoc |
| No hardware T&L in original | Shaders must handle transforms | New vertex shaders with proper matrix math |
| 20+ render object types | All need Vulkan paths | Prioritize: mesh, terrain, particles, water first |

---

## Phase 4: Audio, Input & Video

**Status:** Pending

### Steps Taken
- XAudio2Engine stub library created
- FFmpegVideo stub library created

### Issues
| Issue | Impact | Solution |
|-------|--------|----------|
| Miles SDK unavailable | No audio | XAudio2 (Windows-native, low latency) |
| DirectInput deprecated | Input lag/crashes | Raw Input API + XInput for gamepad |
| Bink SDK unavailable | No cutscenes | FFmpeg with H.264/H.265 support |
| Audio 3D positional differences | Gameplay feel changes | Extensive playtesting and tuning |

---

## Phase 5: Integration & Testing

**Status:** Pending

### Issues
| Issue | Impact | Solution |
|-------|--------|----------|
| Lockstep multiplayer determinism | Cross-platform desync | Fixed-point math or platform-specific FP control |
| Save-game compatibility | Old saves won't load | Versioned format with migration |
| Mod compatibility | Existing mods break | Maintain INI-driven data architecture |
| Zero Hour variant | Separate build needed | Apply all phases to `GeneralsMD/` |

---

## Build System

### Files Created
- `CMakeLists.txt` — top-level build
- `CMakePresets.json` — 4 build presets
- 15 subdirectory `CMakeLists.txt` files
- 5 stub libraries (GameSpy, DX8Compat, FFmpegVideo, VulkanRenderer, XAudio2)

### Build Commands
```powershell
cmake --preset windows-x64-msvc
cmake --build --preset windows-x64-msvc
```

---

## Reference: Community Approaches

| Project | Approach | Status |
|---------|----------|--------|
| OpenSAGE | Clean-room C# reimplementation | 9+ years, not playable |
| GeneralsX | Port with SDL3 + DXVK + FFmpeg | Linux/macOS working |
| Generals-Android | iOS port on GeneralsX | Playable on iOS |
| Generals64 | D3D8 → D3D11 shim layer | Active |
| LibreSAGE | SDL3 + DXVK on Linux | Early stage |
| CnCNet | Replacement multiplayer servers | Active for RA2/TS |

## Key Principle

Get a working x64 executable with existing D3D8 renderer on Windows 10 **before** attempting the Vulkan rewrite. This de-risks the port and provides a fallback.
