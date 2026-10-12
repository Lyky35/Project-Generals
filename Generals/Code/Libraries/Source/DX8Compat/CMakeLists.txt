add_library(DX8Compat INTERFACE)

target_include_directories(DX8Compat INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

file(GLOB DX8_COMPAT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

if(DX8_COMPAT_SOURCES)
    target_sources(DX8Compat INTERFACE ${DX8_COMPAT_SOURCES})
endif()
