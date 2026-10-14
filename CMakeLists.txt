cmake_minimum_required(VERSION 3.20)

project(Project-Generals
    VERSION 1.0.0
    DESCRIPTION "Command & Conquer: Generals - Zero Hour (Modern Remaster)"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Workaround: CMake ABI detection fails for MinGW cross-compilation from Linux
# because the .loc file lacks the .exe extension, causing COPY_FILE to fail.
# Set the pointer size manually (we know it's 64-bit x86_64).
if(NOT CMAKE_SIZEOF_VOID_P)
    set(CMAKE_SIZEOF_VOID_P 8)
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "This project requires x64 (64-bit) build. Use -A x64 with MSVC.")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_compile_definitions(
    WIN32
    _WINDOWS
    _MBCS
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_WARNINGS
    NOMINMAX
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_DEBUG DEBUG _INTERNAL)
else()
    add_compile_definitions(NDEBUG _RELEASE)
endif()

if(MSVC)
    add_compile_options(/W3 /permissive- /Zc:__cplusplus /Zc:externConstexpr /Zc:throwingNew)
else()
    # -fno-delete-null-pointer-checks: this codebase relies on `if (this)`
    # guards in MemoryPoolObject::deleteInstance (e.g. ScriptConditions::reset
    # calls deleteInstance on a NULL list head on first init). MSVC honored
    # them; GCC deletes the check by default and the null deref crashes.
    # fix_bittest.h: MinGW winnt.h redefines BitTest as the _bittest intrinsic
    # (pointer+index); restore the value-mask semantics project-wide.
    add_compile_options(-Wall -Wextra -fpermissive -Wno-narrowing -fno-delete-null-pointer-checks
        -include ${CMAKE_CURRENT_SOURCE_DIR}/Generals/Code/Libraries/Include/fix_bittest.h)
    if(MINGW)
        add_compile_options(-fno-sized-deallocation)
    endif()
endif()

# Whole-program function-entry tracing: configure with -DRTS_FUNCTION_TRACE=ON,
# then run RTS.exe with the environment variable RTS_FUNC_TRACE=1 - every
# instrumented function entry is appended to RTS_FUNC.LOG as "E <hex addr>"
# (map back to names offline with `nm -C -n RTS.exe` / addr2line).
option(RTS_FUNCTION_TRACE "Instrument every function entry (enable at runtime with env RTS_FUNC_TRACE=1)" OFF)
if(RTS_FUNCTION_TRACE)
    add_compile_definitions(RTS_FUNCTION_TRACE_BUILD)
    add_compile_options(-finstrument-functions
        -finstrument-functions-exclude-file-list=/usr/x86_64-w64-mingw32/include,/usr/lib/gcc)
endif()

set(GAME_CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Generals/Code)
set(LIB_INCLUDE_DIR ${GAME_CODE_DIR}/Libraries/Include)
set(COMMON_DIR ${GAME_CODE_DIR}/Common)
set(WWVegas_DIR ${GAME_CODE_DIR}/Libraries/Source/WWVegas)

include_directories(
    ${LIB_INCLUDE_DIR}
    ${LIB_INCLUDE_DIR}/Lib
    ${COMMON_DIR}
    ${COMMON_DIR}/wpaudio
    ${GAME_CODE_DIR}/GameEngine/Include
    ${GAME_CODE_DIR}/GameEngineDevice/Include
    ${WWVegas_DIR}
    ${WWVegas_DIR}/WWLib
    ${WWVegas_DIR}/WWMath
    ${WWVegas_DIR}/WWDebug
    ${WWVegas_DIR}/WWSaveLoad
    ${WWVegas_DIR}/WW3D2
    ${WWVegas_DIR}/WWAudio
    ${WWVegas_DIR}/WWDownload
    ${WWVegas_DIR}/Wwutil
    ${GAME_CODE_DIR}/Libraries/Include/Granny
    ${GAME_CODE_DIR}/Libraries/Include/MSS
    ${GAME_CODE_DIR}/Libraries/Source/GameSpy/Stub
    ${GAME_CODE_DIR}/Libraries/Source
    ${GAME_CODE_DIR}/Libraries/Source/DX8Compat
    ${GAME_CODE_DIR}/Libraries/Source/VulkanRenderer
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/include
    ${LIB_INCLUDE_DIR}/ZLib
)

add_subdirectory(Generals/Code/Libraries/Source/Compression)
add_subdirectory(Generals/Code/Libraries/Source/DX8Compat)
add_subdirectory(Generals/Code/Libraries/Source/FFmpegVideo)
add_subdirectory(Generals/Code/Libraries/Source/GameSpy)
add_subdirectory(Generals/Code/Libraries/Source/VulkanRenderer)
add_subdirectory(Generals/Code/Libraries/Source/WPAudio)
add_subdirectory(Generals/Code/Libraries/Source/WWVegas/WWMath)
add_subdirectory(Generals/Code/Libraries/Source/WWVegas/WWLib)
add_subdirectory(Generals/Code/Libraries/Source/WWVegas/WWDebug)
add_subdirectory(Generals/Code/Libraries/Source/WWVegas/WWSaveLoad)
add_subdirectory(Generals/Code/Libraries/Source/WWVegas/Wwutil)
add_subdirectory(Generals/Code/Libraries/Source/WWVegas/WW3D2)
add_subdirectory(Generals/Code/Libraries/Source/WWVegas/WWAudio)
add_subdirectory(Generals/Code/Libraries/Source/WWVegas/WWDownload)
add_subdirectory(Generals/Code/Libraries/Source/XAudio2)
add_subdirectory(Generals/Code/Libraries/Source/Benchmark)
add_subdirectory(Generals/Code/GameEngine)
add_subdirectory(Generals/Code/GameEngineDevice)
add_subdirectory(Generals/Code/Main)
