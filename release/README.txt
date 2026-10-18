Project Generals - RTS.exe milestone build (release 1.2)
========================================================

What this is
------------
Cross-compiled Windows x64 build of the C&C Generals / Zero Hour source
port to native Vulkan + MinGW (see NOTES.md). This build reaches actual
skirmish gameplay: boot -> shell map -> in-game with terrain, buildings
and units rendering (Vulkan).

Run
---
Extract the whole folder and run RTS.exe on Windows 10+ x64, ideally next
to an installed copy of Generals / Zero Hour (the game data .big archives
and Data\ folder are required - the build reads them from the install
location / current directory).

Bundled DLLs (required, keep next to RTS.exe):
  libwinpthread-1.dll       (MinGW runtime; libgcc/libstdc++ are static)
  OpenAL32.dll              (OpenAL Soft - Miles audio backend)
  vulkan-1.dll              (Khronos Vulkan loader - real, linked)

shaders/                    (SPIR-V for the mesh pipeline - must sit next
                             to RTS.exe)

Audio: Miles SDK calls route to OpenAL Soft (Source/MSS/mss.cpp).
Graphics: RTS.exe links the real vulkan-1.dll; a Vulkan-capable GPU/driver
(ICD) is required to actually render.

Video: Bink (.bik) cutscenes decode via static FFmpeg (libavformat/libavcodec).
No extra video DLLs - FFmpeg is linked in. bcrypt.dll is a Windows system DLL.

Windows provides: ADVAPI32, DINPUT8, GDI32, KERNEL32, SHELL32, USER32,
WINMM, WS2_32, AVIFIL32, IMM32, and the api-ms-win-crt-* UCRT set.

Known state (see PROGRESS.md / NOTES.md)
----------------------------------------
- Full boot under wine with game data: static init -> WinMain -> GameMain
  -> all engine subsystems + INIs (1128+ object templates) -> GUI ->
  shell map -> skirmish load -> in-game render loop.
- Terrain, buildings, units (skinned meshes) render via the Vulkan
  pipeline. Teams spawn (Spawn 1.1, Comanche Patrol, etc.), AI/pathing
  runs, reinforcement scripts fire.
- Fixed in this build: the frame-74 hang on first skinned-unit render
  (shadowed 'cnt' in the skin deform loops made GCC spin forever); the
  team-registry wipe (addTeam shadowed-i bug that silently killed every
  reinforcement); texture churn (896+ GPU images during boot); Win10
  pre-WinMain crash (os_info.Code); Technical-Difficulties startup box.
- Real Vulkan loader linked (no vk* stubs). Miles audio = OpenAL Soft.
- GeneralsMD tree not yet mirrored.

Repo: https://github.com/Lyky35/Project-Generals


Troubleshooting
---------------
RTS_START.LOG  - written next to RTS.exe. Boot markers run first
                 (static-init ... late-static-done), then WinMain
                 breadcrumbs (WinMain enter, initializeAppWindows ok,
                 initMemoryManager done, entering GameMain). If RTS.exe
                 dies with an unhandled exception, CRASH lines follow with
                 the faulting module and a full FR0..FRn stack walk.
                 In-game breadcrumbs (hb frame=, Mesh::Render, flush,
                 SORTLISTS...) trail the boot section - the last line
                 before a stall names the phase that stopped.
RTS_DIAG.exe   - tiny helper: loads OpenAL32.dll + vulkan-1.dll via LoadLibrary.
                 Run it next to the DLLs. If it shows a message box, those DLLs
                 are fine and any RTS.exe crash is in our static init / code.
                 Check RTS_DIAG.LOG for which DLL failed.
