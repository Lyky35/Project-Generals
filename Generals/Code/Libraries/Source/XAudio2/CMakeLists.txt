add_library(XAudio2Engine INTERFACE)

target_include_directories(XAudio2Engine INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(XAudio2Engine INTERFACE
    HAS_XAUDIO2=1
)

file(GLOB XAUDIO2_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
if(XAUDIO2_SOURCES)
    target_sources(XAudio2Engine INTERFACE ${XAUDIO2_SOURCES})
endif()
