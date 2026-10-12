file(GLOB WPAudio_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(WPAudio STATIC ${WPAudio_SOURCES})

target_include_directories(WPAudio PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Include
)
