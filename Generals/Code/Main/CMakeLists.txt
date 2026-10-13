add_executable(RTS WIN32
    ${CMAKE_CURRENT_SOURCE_DIR}/WinMain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/guidstubs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/linkstubs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/netstubs.cpp
)

target_include_directories(RTS PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../GameEngine/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../GameEngineDevice/Include
)

# First link milestone: ignore unresolved (GameSpy/Firewall/Vulkan-loader/etc stubs)
# Remaining symbols get real impls as SDK backends land.
target_link_options(RTS PRIVATE -Wl,--unresolved-symbols=ignore-all)

target_link_libraries(RTS PRIVATE
    GameEngineDevice
    GameEngine
    WW3D2
    WWDebug
    WWLib
    WWUtil
    WWSaveLoad
    Compression
    VulkanRenderer
    XAudio2Engine
    FFmpegVideo
    WWAudio
    WPAudio
    DX8Compat
    winmm
    vfw32
    dinput8
    dsound
    ole32
    oleaut32
    uuid
    shell32
    imm32
    comctl32
    wininet
    advapi32
    ws2_32
    mingw32
    GameSpyStub
)

# Remove DirectX 8 SDK libraries (replaced by OpenAL + SDL2 + FFmpeg)
# d3d8.lib d3dx8.lib dxguid.lib dinput8.lib dsound.lib winmm.lib vfw32.lib
# wsock32.lib wininet.lib imm32.lib comctl32.lib advapi32.lib shell32.lib
# ole32.lib oleaut32.lib uuid.lib

# Note: Libraries linked for cross-compilation from Linux
# openal: Open Audio Library (audio emulation replacement for Miles SDK)
# FFmpeg::avcodec/avformat/avutil/swscale: Bink Video SDK - FFmpeg emulation
# SDL2: Simple DirectMedia Layer 2 (input/emulation replacement for DirectInput)
# VulkanRenderer: Native Vulkan rendering backend (replaces D3D8)
# WPAudio: Ported to OpenAL emulation
# FFmpegVideo: FFmpeg video decoding library (subdirectory)
