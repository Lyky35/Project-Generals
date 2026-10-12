# Project Generals — Development Notes

Forked from: `electronicarts/CnC_Generals_Zero_Hour`
Target: Windows 10+ x64, Vulkan 1.3, modern toolchain

---

## Phase 0: Audit & Infrastructure

**Status:** In Progress

### Steps Taken
- Forked EA source release to `Lyky35/Project-Generals`
- Source uses C++98, STLport 4.5.3, Visual Studio 6.0 `.dsp/.dsw` projects
- Identified ~10 files with x86 inline assembly
- Mapped full dependency tree (Miles Sound, Bink Video, GameSpy SDK, SafeDisc, BrowserEngine.DLL)

### Issues
| Issue | Impact | Solution |
|-------|--------|----------|
| VS6 `.dsp/.dsw` projects unusable | No modern build | Create CMake build system |
| STLport 4.5.3 requires patching | Won't compile on MSVC 2022 | Replace with MSVC STL (C++17) |
| GameSpy SDK unavailable | No multiplayer | Stub out; use CnCNet later |
| BrowserEngine.DLL is proprietary EA DLL | Missing dependency | Remove; not needed for gameplay |
| SafeDisc DRM | Blocks launch on modern Windows | Remove entirely |

---

## Phase 1: x64 Port

**Status:** Pending

### Steps Taken
- Pointer-truncation audit needed (`DWORD` for handles, `int` for pointer diffs)
- `GetWindowLong` → `GetWindowLongPtr` migration identified
- Assembly removal plan documented for 10 files

### Issues
| Issue | Impact | Solution |
|-------|--------|----------|
| 32-bit pointer assumptions throughout | Crashes on x64 | Systematic `DWORD_PTR`/`uintptr_t` migration |
| x86 inline assembly (RDTSC, CPUID, stack walk) | Won't compile on x64 | Port to intrinsics (`__rdtsc`, `__cpuid`, `RtlCaptureContext`) |
| `sizeof(pointer)` assumptions in serialization | Save-game corruption | Version save format, add migration path |

---

## Phase 2: Windows 10+ Compatibility

**Status:** Pending

### Steps Taken
- Fullscreen crash on modern systems documented
- Widescreen support absent (hardcoded 4:3)
- 4,096-unit cap hardcoded

### Issues
| Issue | Impact | Solution |
|-------|--------|----------|
| Fullscreen crash | Game unplayable | Implement borderless windowed mode |
| No DPI awareness | Blurry on high-DPI | Add `SetProcessDpiAwarenessContext` manifest |
| Hardcoded resolutions | Poor UX on modern displays | Implement resolution detection + scaling |
| WinXP-era window class registration | Issues on Win10+ | Update window class for modern Windows |

---

## Phase 3: Vulkan Renderer

**Status:** Pending (largest phase)

### Steps Taken
- D3D8 fixed-function pipeline must be fully replaced
- WW3D2 renderer has thousands of call sites on D3D8
- Shader system needs ground-up rewrite (terrain, object, particle, water, shadow)

### Issues
| Issue | Impact | Solution |
|-------|--------|----------|
| D3D8 not supported on modern Windows | No rendering | Full Vulkan 1.3 renderer |
| Fixed-function → shader translation not 1:1 | Visual differences | Write new shaders matching visual output |
| WW3D2 API surface is enormous | Long implementation time | Incremental: get basic rendering first, add features progressively |
| Texture format differences (D3D8 vs Vulkan) | Corrupted textures | Use BCn compressed formats, verify with RenderDoc |
| No hardware T&L in original | Shaders must handle transforms | New vertex shaders with proper matrix math |

### Architecture Decision
Native Vulkan backend (no DX8 translation layer). This avoids DXVK overhead and gives full control over the rendering pipeline.

---

## Phase 4: Audio, Input & Video

**Status:** Pending

### Steps Taken
- Miles Sound System replacement identified: XAudio2
- DirectInput replacement identified: Raw Input + XInput
- Bink Video replacement identified: FFmpeg

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
