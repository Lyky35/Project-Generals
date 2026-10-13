# Exclude web browser file (requires Windows DLL)
set(WW3D2_EXCLUDE dx8webbrowser.cpp)
file(GLOB WW3D2_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
# Exclude web browser file (requires Windows DLL)
list(FILTER WW3D2_SOURCES EXCLUDE REGEX "dx8webbrowser\.cpp$")

add_library(WW3D2 STATIC ${WW3D2_SOURCES})

target_include_directories(WW3D2 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWLib
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWMath
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWDebug
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWSaveLoad
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWAudio
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Include/Granny
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Include/MSS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Include/DX8Compat
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/GameSpy/Stub
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source/VulkanRenderer
)

target_link_libraries(WW3D2 PUBLIC WWLib WWMath WWDebug WWSaveLoad WWAudio GameSpyStub DX8Compat VulkanRenderer)
