file(GLOB WWSaveLoad_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(WWSaveLoad STATIC ${WWSaveLoad_SOURCES})

target_include_directories(WWSaveLoad PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWLib
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWDebug
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWMath
)

target_link_libraries(WWSaveLoad PUBLIC WWLib WWDebug WWMath)
