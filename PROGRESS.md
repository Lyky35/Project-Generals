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

<!-- Append new entries below. Commit after each completed step. -->
