file(GLOB WWLib_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_library(WWLib STATIC ${WWLib_SOURCES})

target_include_directories(WWLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWDebug
)

target_link_libraries(WWLib PUBLIC WWDebug)
