file(GLOB WWDebug_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(WWDebug STATIC ${WWDebug_SOURCES})

target_include_directories(WWDebug PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWLib
)

target_link_libraries(WWDebug PUBLIC WWLib)
