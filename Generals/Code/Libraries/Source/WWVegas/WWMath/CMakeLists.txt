file(GLOB WWMath_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(WWMath STATIC ${WWMath_SOURCES})

target_include_directories(WWMath PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWLib
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWDebug
    ${CMAKE_CURRENT_SOURCE_DIR}/../WWSaveLoad
)

target_link_libraries(WWMath PUBLIC WWLib WWDebug WWSaveLoad)
