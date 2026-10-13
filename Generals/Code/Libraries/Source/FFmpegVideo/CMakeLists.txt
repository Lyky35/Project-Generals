add_library(FFmpegVideo INTERFACE)

target_include_directories(FFmpegVideo INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(CMAKE_CXX_COMPILER MATCHES "mingw")
    # Vendored MinGW static FFmpeg (Bink demuxer + decoder + swscale) — see third_party/ffmpeg
    set(FFMPEG_VENDORED ${CMAKE_SOURCE_DIR}/third_party/ffmpeg)
    target_compile_definitions(FFmpegVideo INTERFACE HAS_FFMPEG=1)
    target_include_directories(FFmpegVideo INTERFACE ${FFMPEG_VENDORED}/include)
    # Order matters for GNU ld: avformat -> avcodec -> swscale -> avutil
    target_link_libraries(FFmpegVideo INTERFACE
        ${FFMPEG_VENDORED}/lib/libavformat.a
        ${FFMPEG_VENDORED}/lib/libavcodec.a
        ${FFMPEG_VENDORED}/lib/libswscale.a
        ${FFMPEG_VENDORED}/lib/libavutil.a
        # avutil random_seed uses BCryptGenRandom (Windows)
        bcrypt
    )
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(FFMPEG QUIET libavcodec libavformat libavutil libswscale)
    endif()
    if(FFMPEG_FOUND)
        target_compile_definitions(FFmpegVideo INTERFACE HAS_FFMPEG=1)
        target_include_directories(FFmpegVideo INTERFACE ${FFMPEG_INCLUDE_DIRS})
        target_link_libraries(FFmpegVideo INTERFACE ${FFMPEG_LIBRARIES})
    else()
        target_compile_definitions(FFmpegVideo INTERFACE HAS_FFMPEG=0)
    endif()
endif()
