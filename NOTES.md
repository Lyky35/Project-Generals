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

**Status:** Pending

### Steps Taken
- Pointer-truncation audit complete (see Phase 0)
- `GetWindowLong` → `GetWindowLongPtr` migration identified (12 locations)
- Assembly removal plan documented for 30+ locations

### Issues
| Issue | Impact | Solution |
|-------|--------|----------|
| 32-bit pointer assumptions throughout | Crashes on x64 | Systematic `DWORD_PTR`/`uintptr_t` migration |
| x86 inline assembly (RDTSC, CPUID, x87, SSE, RLE, LCW, spinlock) | Won't compile on x64 | Port to intrinsics (`__rdtsc`, `__cpuid`, `RtlCaptureContext`) |
| `sizeof(pointer)` assumptions in serialization | Save-game corruption | Version save format, add migration path |
| x87 FPU math in hot path (wwmath.h) | Different results on x64 | Replace with standard C++ `std::sin`/`std::cos` |
| SSE intrinsics in vp.cpp | May need AVX2 fallback | Use `_mm_*` intrinsics (available on x64) |

---

## Phase 2: Windows 10+ Compatibility

**Status:** Pending

### Issues
| Issue | Impact | Solution |
|-------|--------|----------|
| Fullscreen crash | Game unplayable | Implement borderless windowed mode |
| No DPI awareness | Blurry on high-DPI | Add `SetProcessDpiAwarenessContext` manifest |
| Hardcoded resolutions | Poor UX on modern displays | Implement resolution detection + scaling |
| WinXP-era window class registration | Issues on Win10+ | Update window class for modern Windows |
| 4,096-unit cap | Gameplay limitation | Make configurable or remove |

---

## Phase 3: Vulkan Renderer

**Status:** Pending (largest phase)

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
