add_executable(RTS WIN32
    ${CMAKE_CURRENT_SOURCE_DIR}/WinMain.cpp
)

target_include_directories(RTS PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../GameEngine/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../GameEngineDevice/Include
)

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
    GameSpyStub
    OpenAL::OpenAL
    FFmpeg::avcodec FFmpeg::avformat FFmpeg::avutil FFmpeg::swscale
    SDL2::SDL2
)

# Remove DirectX 8 SDK libraries (replaced by OpenAL + SDL2 + FFmpeg)
# d3d8.lib d3dx8.lib dxguid.lib dinput8.lib dsound.lib winmm.lib vfw32.lib
# wsock32.lib wininet.lib imm32.lib comctl32.lib advapi32.lib shell32.lib
# ole32.lib oleaut32.lib uuid.lib

# Note: Keep only the libraries that are still needed for window creation + input
# OpenAL handles audio, SDL2 handles input/window, FFmpeg handles video
