file(GLOB WWUtil_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(WWUtil STATIC ${WWUtil_SOURCES})

target_include_directories(WWUtil PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWLib
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWDebug
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWMath
)

target_link_libraries(WWUtil PUBLIC WWLib WWDebug WWMath)
