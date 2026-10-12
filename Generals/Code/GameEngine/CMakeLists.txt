file(GLOB_RECURSE GameEngine_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Common/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/GameLogic/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/GameClient/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/GameNetwork/*.cpp
)

add_library(GameEngine STATIC ${GameEngine_SOURCES})

target_include_directories(GameEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/Common
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/GameClient
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/GameLogic
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/GameNetwork
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/Precompiled
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Source/Compression
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Source/WWVegas
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Source/WWVegas/WWLib
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Source/GameSpy/Stub
)

target_link_libraries(GameEngine PUBLIC Compression WWLib GameSpyStub)
