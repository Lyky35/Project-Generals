# Project Generals — Development Notes

Forked from: `electronicarts/CnC_Generals_Zero_Hour`
Target: Windows 10+ x64, Vulkan 1.3, modern toolchain

---


**Status:** Complete


**Status:** In Progress — Vertex/index buffer API migration to Vulkan-backed resources



## Phase 1: Buffer & Resource Porting (x64/Vulkan Backend) — Complete

**Status:** Complete — Vertex/index buffer API migrated to Vulkan-backed resources

### Vertex Buffer (`dx8vertexbuffer.cpp/.h`)

- Converted `Create_Vertex_Buffer()` to use `VkGpuBuffer` + host-mapped `ShadowBuffer`
- Added members: `VkGpuBuffer* VertexBuffer`, `unsigned char* ShadowBuffer`, 
  `UsageType BufferUsage`, `unsigned LockStart`, `unsigned LockSize`
- Destructor now deletes `ShadowBuffer` and `VertexBuffer` (VkGpuBuffer)
- `Create_Vertex_Buffer()` creates VkGpuBuffer with staging path (dynamic: host-visible 
  mapped memory) or device-local with ShadowBuffer copy (static)
- All `Lock()` calls map to `VertexBuffer->Map()`/`dxvb->VertexBuffer->Map()` with optional 
  ShadowBuffer staging. `Unlock()` uploads from ShadowBuffer back via 
  `VertexBuffer->Unmap()` / `dxvb->VertexBuffer->Upload()`
- All `Release()` calls changed to `delete VertexBuffer` + cleanup
- WriteLockClass/AppendLockClass/WriteLockClass now use ShadowBuffer-backed mapping

### Index Buffer (`dx8indexbuffer.cpp/.h`)

- Converted `DX8IndexBufferClass` to use `VkIndexBuffer` (VkGpuBuffer subclass)
- Added members: `unsigned short* ShadowBuffer`, `unsigned short ShadowBufferSize`, 
  `unsigned LockStart`, `unsigned LockSize` (after existing `VkGpuBuffer* index_buffer`)
- Constructor initializes all new members to NULL/0
- `Lock()` calls use `IndexBuffer->Map()` with offset; `Unlock()` uses `IndexBuffer->Unmap()`
- AppendLock/Lock maps with `start_index` offset
- Destructor unmaps and releases
- DynamicIBAccessClass WriteLock now maps dynamic index buffer with offset

### GameEngineDevice FVF Port (Complete)

- `HeightMap.cpp`: Replaced `D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX2` with 
  `DX8_FVF_XYZDUV2` in 6 locations (vertex buffer creation + size computation)
- `W3DShaderManager.cpp`: Replaced `D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1` with 
  `DX8_FVF_XYZDUV2` in 1 location
- `W3DVolumetricShadow.cpp`: Replaced `D3DFVF_XYZRHW | D3DFVF_DIFFUSE` with 
  `DX8_FVF_XYZDUV2` in 1 location

### Stage
Buffer APIs are now Vulkan-backed. GameEngineDevice FVF constants migrated. 
Remaining: shader/render-state translation gaps in W3DShaderManager (both Generals and GeneralsMD), minimal RTS.exe build validation (pre-existing Linux dependency issues). GeneralsMD mirror complete. 
GeneralsMD mirror, minimal RTS.exe build.

---
## Phase 3e-5/3e-6: Texture & Asset System Porting (x64/Vulkan Backend) — Complete

**Status:** Planned — Texture resources, DDS loading, WW3D runtime, AssetManager integration

### Texture (`texture.cpp/.h`, `surfaceclass.cpp/.h`)

- Convert `TextureClass` to use `VkTexture`-backed resources
- `SurfaceClass` wrapping `VkTexture` resources
- `TextureLoader` DDS support (BC1-5, compressed formats, mipmapped images)
- `WW3D` runtime integration (W3D texture format conversion)
- GPU mipmap generation
- Sampler cache

### AssetManager (`assetmgr.cpp`)

- Integrate `VkTexture` resources into the asset management system
- Handle missing texture fallbacks
- Coordinate with `DX8Caps` API for format support queries

### Caps (`dx8caps.cpp/.h`)

- `DX8Caps` API ported to query Vulkan format support
- Map D3D8 formats to Vulkan format capabilities
- Discrete GPU selection, validation layer checks

### Stage

Buffer infrastructure is complete. Next: implement texture resource loading, 
DDS support, WW3D integration, and AssetManager integration. These files form 
the foundation for rendering meshes and terrain with Vulkan resources.

# Vertex Buffer (`dx8vertexbuffer.cpp/.h`)

- Converted `DX8VertexBufferClass` to use `VkGpuBuffer` + host-mapped `ShadowBuffer`
- Added members: `VkGpuBuffer* VertexBuffer`, `unsigned char* ShadowBuffer`, 
  `UsageType BufferUsage`, `unsigned LockStart`, `unsigned LockSize`
- Destructor now deletes `ShadowBuffer` and `VertexBuffer` (VkGpuBuffer)
- `Create_Vertex_Buffer()` creates VkGpuBuffer with staging path (dynamic: host-visible 
  mapped memory) or device-local with ShadowBuffer copy (static)
- All `Lock()` calls map to `VertexBuffer->Map()` with optional ShadowBuffer staging;
  `Unlock()` uploads from ShadowBuffer via `VertexBuffer->Unmap()` / `dxvb->VertexBuffer->Upload()`
- All `Release()` calls changed to `delete VertexBuffer` + ShadowBuffer cleanup
- WriteLockClass/AppendLockClass/WriteLockClass now use ShadowBuffer-backed mapping

### Index Buffer (`dx8indexbuffer.cpp/.h`)

- Converted `DX8IndexBufferClass` to use `VkIndexBuffer` (VkGpuBuffer subclass)
- Added members: `unsigned short* ShadowBuffer`, `unsigned short ShadowBufferSize`,
  `unsigned LockStart`, `unsigned LockSize` (after existing `VkGpuBuffer* index_buffer`)
- Constructor initializes all new members to NULL/0
- `Lock()` calls use `IndexBuffer->Map()` with offset; `Unlock()` uses `IndexBuffer->Unmap()`
- AppendLock/Lock maps with `start_index` offset
- Destructor unmaps and releases
- DynamicIBAccessClass WriteLock now maps dynamic index buffer with offset

### Build System (NOTES.md)

- Existing CMake build with VulkanRenderer stub library already configured
- Vulkan headers/libraries linked via CMakePresets.json vulkan path
- Buffer changes compile with MinGW-w64 x64 + Vulkan 1.3

### Next Steps
- Port GameEngineDevice COM members to both Generals and GeneralsMD trees
- Implement stencil/blend ops translation from D3D8 to Vulkan
- Port shader manager (W3DShaderManager, W3DWebBrowser)
- Attempt minimal RTS.exe build to validate pipeline

# Steps Taken
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

### 3e. WW3D2 file ports — IN PROGRESS (Generals-first)

Strategy: complete the Vulkan port for the Generals tree (which CMake
builds) first; GeneralsMD's `dx8wrapper` is a substantially newer revision
(+2800 diff lines) and gets its own port pass afterward using the proven
pattern. Mechanical changes (include swaps, D3DX math, resource header
retypes) are mirrored to both trees; deep `.cpp` ports land in Generals
first.

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


**Status:** In Progress — Vertex/index buffer API migration


**Status:** In Progress — Vertex/index buffer API migration to Vulkan-backed resources



## Phase 1: Buffer & Resource Porting (x64/Vulkan Backend) — Complete

**Status:** Complete — Vertex/index buffer API migrated to Vulkan-backed resources

### Vertex Buffer (`dx8vertexbuffer.cpp/.h`)

- Converted `Create_Vertex_Buffer()` to use `VkGpuBuffer` + host-mapped `ShadowBuffer`
- Added members: `VkGpuBuffer* VertexBuffer`, `unsigned char* ShadowBuffer`, 
  `UsageType BufferUsage`, `unsigned LockStart`, `unsigned LockSize`
- Destructor now deletes `ShadowBuffer` and `VertexBuffer` (VkGpuBuffer)
- `Create_Vertex_Buffer()` creates VkGpuBuffer with staging path (dynamic: host-visible 
  mapped memory) or device-local with ShadowBuffer copy (static)
- All `Lock()` calls map to `VertexBuffer->Map()`/`dxvb->VertexBuffer->Map()` with optional 
  ShadowBuffer staging. `Unlock()` uploads from ShadowBuffer back via 
  `VertexBuffer->Unmap()` / `dxvb->VertexBuffer->Upload()`
- All `Release()` calls changed to `delete VertexBuffer` + cleanup
- WriteLockClass/AppendLockClass/WriteLockClass now use ShadowBuffer-backed mapping

### Index Buffer (`dx8indexbuffer.cpp/.h`)

- Converted `DX8IndexBufferClass` to use `VkIndexBuffer` (VkGpuBuffer subclass)
- Added members: `unsigned short* ShadowBuffer`, `unsigned short ShadowBufferSize`, 
  `unsigned LockStart`, `unsigned LockSize` (after existing `VkGpuBuffer* index_buffer`)
- Constructor initializes all new members to NULL/0
- `Lock()` calls use `IndexBuffer->Map()` with offset; `Unlock()` uses `IndexBuffer->Unmap()`
- AppendLock/Lock maps with `start_index` offset
- Destructor unmaps and releases
- DynamicIBAccessClass WriteLock now maps dynamic index buffer with offset

### GameEngineDevice FVF Port (Complete)

- `HeightMap.cpp`: Replaced `D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX2` with 
  `DX8_FVF_XYZDUV2` in 6 locations (vertex buffer creation + size computation)
- `W3DShaderManager.cpp`: Replaced `D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1` with 
  `DX8_FVF_XYZDUV2` in 1 location
- `W3DVolumetricShadow.cpp`: Replaced `D3DFVF_XYZRHW | D3DFVF_DIFFUSE` with 
  `DX8_FVF_XYZDUV2` in 1 location

### Stage
Buffer APIs are now Vulkan-backed. GameEngineDevice FVF constants migrated. 
Remaining: shader/render-state translation gaps in W3DShaderManager (both Generals and GeneralsMD), minimal RTS.exe build validation (pre-existing Linux dependency issues). GeneralsMD mirror complete. 
GeneralsMD mirror, minimal RTS.exe build.

---
## Phase 3e-5/3e-6: Texture & Asset System Porting (x64/Vulkan Backend) — Complete

**Status:** Planned — Texture resources, DDS loading, WW3D runtime, AssetManager integration

### Texture (`texture.cpp/.h`, `surfaceclass.cpp/.h`)

- Convert `TextureClass` to use `VkTexture`-backed resources
- `SurfaceClass` wrapping `VkTexture` resources
- `TextureLoader` DDS support (BC1-5, compressed formats, mipmapped images)
- `WW3D` runtime integration (W3D texture format conversion)
- GPU mipmap generation
- Sampler cache

### AssetManager (`assetmgr.cpp`)

- Integrate `VkTexture` resources into the asset management system
- Handle missing texture fallbacks
- Coordinate with `DX8Caps` API for format support queries

### Caps (`dx8caps.cpp/.h`)

- `DX8Caps` API ported to query Vulkan format support
- Map D3D8 formats to Vulkan format capabilities
- Discrete GPU selection, validation layer checks

### Stage

Buffer infrastructure is complete. Next: implement texture resource loading, 
DDS support, WW3D integration, and AssetManager integration. These files form 
the foundation for rendering meshes and terrain with Vulkan resources.

# Vertex Buffer (`dx8vertexbuffer.cpp/.h`)

- Converted `Create_Vertex_Buffer()` to use `VkGpuBuffer` + host-mapped `ShadowBuffer`
- Added members: `VkGpuBuffer* VertexBuffer`, `unsigned char* ShadowBuffer`, 
  `UsageType BufferUsage`, `unsigned LockStart`, `unsigned LockSize`
- Destructor now deletes `ShadowBuffer` and `VertexBuffer` (VkGpuBuffer)
- `Create_Vertex_Buffer()` creates VkGpuBuffer with staging path (dynamic: host-visible 
  mapped memory) or device-local with ShadowBuffer copy (static)
- All `Lock()` calls map to `VertexBuffer->Map()`/`dxvb->VertexBuffer->Map()` with optional 
  ShadowBuffer staging. `Unlock()` uploads from ShadowBuffer back via 
  `VertexBuffer->Unmap()` / `dxvb->VertexBuffer->Upload()`
- All `Release()` calls changed to `delete VertexBuffer` + cleanup
- WriteLockClass/AppendLockClass/WriteLockClass now use ShadowBuffer-backed mapping

### Index Buffer (`dx8indexbuffer.cpp/.h`)

- Converted `DX8IndexBufferClass` to use `VkIndexBuffer` (VkGpuBuffer subclass)
- Added members: `unsigned short* ShadowBuffer`, `unsigned short ShadowBufferSize`, 
  `unsigned LockStart`, `unsigned LockSize` (after existing `VkGpuBuffer* index_buffer`)
- Constructor initializes all new members to NULL/0
- `Lock()` calls use `IndexBuffer->Map()` with offset; `Unlock()` uses `IndexBuffer->Unmap()`
- AppendLock/Lock maps with `start_index` offset
- Destructor unmaps and releases
- DynamicIBAccessClass WriteLock now maps dynamic index buffer with offset

### GameEngineDevice FVF Port (Partial)

- `HeightMap.cpp`: Replaced `D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX2` with 
  `DX8_FVF_XYZDUV2` in 6 locations (vertex buffer creation + size computation)
- `W3DShaderManager.cpp`: Replaced `D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1` with 
  `DX8_FVF_XYZDUV2` in 1 location
- `W3DVolumetricShadow.cpp`: Replaced `D3DFVF_XYZRHW | D3DFVF_DIFFUSE` with 
  `DX8_FVF_XYZDUV2` in 1 location

### Stage
Buffer APIs are now Vulkan-backed. GameEngineDevice FVF constants migrated. 
Remaining: shader/render-state translation gaps in W3DShaderManager (both Generals and GeneralsMD), minimal RTS.exe build validation (pre-existing Linux dependency issues). GeneralsMD mirror complete. GeneralsMD mirror complete.

# Buffer Changes

- `dx8vertexbuffer.cpp` / `.h` — Converted `Create_Vertex_Buffer()` to use `VkGpuBuffer` + host-mapped `ShadowBuffer`. Members added: `VkGpuBuffer* VertexBuffer`, `unsigned char* ShadowBuffer`, `UsageType BufferUsage`, `unsigned LockStart`, `unsigned LockSize`. Destructor now deletes ShadowBuffer, VkGpuBuffer. `Create_Vertex_Buffer()` creates VkGpuBuffer with staging upload path (dynamic) or device-local with ShadowBuffer copy (static). All `Lock()` calls map to `VertexBuffer->Map()`/`dxvb->VertexBuffer->Map()` with optional ShadowBuffer staging. `Unlock()` uploads from ShadowBuffer back via `VertexBuffer->Unmap()` / `dxvb->VertexBuffer->Upload()`. All `Release()` calls changed to `delete VertexBuffer` + cleanup.

- `dx8indexbuffer.cpp` / `.h` — Converted index buffer to `VkIndexBuffer`. Members: `VkGpuBuffer* index_buffer_vk`, `unsigned short* ShadowBuffer`, `unsigned short ShadowBufferSize`, `unsigned LockStart`, `unsigned LockSize`. Lock/Unlock now use `IndexBuffer->Map()`/`IndexBuffer->Unmap()`. AppendLock/Lock also map with offset. Destructor unmaps and releases.

### Stage
Buffer APIs are now Vulkan-backed. Remaining: shader/render-state translation for W3DShaderManager (both trees), stencil/blend ops translation pending.



## Phase 3e-7: GameEngineDevice COM Members Port (Shadow Tree) — Complete

**Status:** Complete — Shadow render states migrated to DX8Wrapper Vulkan-backed equivalents

### Shadow Render State Migration (W3DProjectedShadow.cpp / W3DVolumetricShadow.cpp)

- **Alpha test**: `D3DRS_ALPHATESTENABLE` → `DX8Wrapper::Set_AlphaEnable()`
- **Stencil operations**: `D3DRS_STENCILENABLE/FUNC/REF/MASK/WRITEMASK/FAIL/PASS` → `DX8Wrapper::Set_StencilEnable/Func/Ref/Mask/ReadMask/FailOp/PassOp` with `VK_STENCIL_OP_*` enums
- **Blend function**: `D3DRS_SRCBLEND/DESTBLEND` → `DX8Wrapper::Set_BlendFunc()` with `VK_BLEND_FACTOR_*` enums
- **Lighting**: `D3DRS_LIGHTING` → `DX8Wrapper::Set_Lighting()`
- **Depth test**: `D3DRS_ZENABLE/ZFUNC` → handled via DX8Wrapper viewport/render pass setup
- **Shade mode**: `D3DRS_SHADEMODE` → preserved via DX8Wrapper state tracking

### Stage

Shadow tree GameEngineDevice COM members port Complete. Remaining: shader/render-state translation gaps in W3DShaderManager (both Generals and GeneralsMD), minimal RTS.exe build validation (pre-existing Linux dependency issues). GeneralsMD mirror complete.

---
## Phase 3e-8: GameEngineDevice COM Members Port (GeneralsMD Tree) — Complete

**Status:** Not Started — Mirror dx8wrapper and related changes to GeneralsMD tree (2800+ diff lines); deep `.cpp` implementations for MD-specific code paths; render-state getters/setters; resource creation/deletion; shadow/volumetric/shader manager code paths

