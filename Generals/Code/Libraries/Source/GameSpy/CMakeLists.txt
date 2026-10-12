add_library(GameSpyStub INTERFACE)

target_include_directories(GameSpyStub INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Stub
)

target_compile_definitions(GameSpyStub INTERFACE
    HAS_GAMESPY=0
)
