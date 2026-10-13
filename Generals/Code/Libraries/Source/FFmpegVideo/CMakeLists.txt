add_library(FFmpegVideo INTERFACE)

target_include_directories(FFmpegVideo INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Host pkg-config FFmpeg is Linux-only; MinGW cross builds must not pull /usr/include
if(NOT CMAKE_CXX_COMPILER MATCHES "mingw")
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(FFMPEG QUIET libavcodec libavformat libavutil libswscale)
    endif()
endif()

if(FFMPEG_FOUND)
    target_compile_definitions(FFmpegVideo INTERFACE HAS_FFMPEG=1)
    target_include_directories(FFmpegVideo INTERFACE ${FFMPEG_INCLUDE_DIRS})
    target_link_libraries(FFmpegVideo INTERFACE ${FFMPEG_LIBRARIES})
else()
    target_compile_definitions(FFmpegVideo INTERFACE HAS_FFMPEG=0)
endif()
