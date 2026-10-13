file(GLOB WWAudio_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
# WWAudio.cpp has too many Miles SDK dependencies - stub out the AIL functions instead
list(FILTER WWAudio_SOURCES EXCLUDE REGEX "[Ww][Ww][Aa][Uu][Dd][Ii][Oo]\.cpp$")

add_library(WWAudio STATIC ${WWAudio_SOURCES})

target_include_directories(WWAudio PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Include/MSS
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWLib
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWMath
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWDebug
    ${CMAKE_CURRENT_SOURCE_DIR}/../WW3D2
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWSaveLoad
)

target_link_libraries(WWAudio PUBLIC WWLib WWMath WWDebug WW3D2 WWSaveLoad)
