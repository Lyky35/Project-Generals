file(GLOB WWDownload_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(WWDownload STATIC ${WWDownload_SOURCES})

target_include_directories(WWDownload PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Include
)

target_link_libraries(WWDownload PUBLIC ws2_32)
