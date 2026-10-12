file(GLOB_RECURSE GameEngineDevice_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/W3DDevice/GameLogic/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/W3DDevice/Common/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/W3DDevice/GameClient/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Win32Device/Common/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Win32Device/GameClient/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/VideoDevice/Bink/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/MilesAudioDevice/*.cpp
)

add_library(GameEngineDevice STATIC ${GameEngineDevice_SOURCES})

target_include_directories(GameEngineDevice PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/W3DDevice/GameClient
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/W3DDevice/GameLogic
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/W3DDevice/Common
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/MilesAudioDevice
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/VideoDevice/Bink
    ${CMAKE_CURRENT_SOURCE_DIR}/../GameEngine/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Include/Granny
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Include/MSS
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Source/WWVegas
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Source/WWVegas/WW3D2
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Source/WWVegas/WWLib
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Source/WWVegas/WWDebug
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Source/WWVegas/WWMath
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Source/WWVegas/WWSaveLoad
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Include/DX8Compat
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Source/VulkanRenderer
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Source/XAudio2
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Source/FFmpegVideo
)

target_link_libraries(GameEngineDevice PUBLIC
    GameEngine
    WW3D2
    WWLib
    WWDebug
    WWMath
    WWSaveLoad
    VulkanRenderer
    XAudio2Engine
    FFmpegVideo
)
