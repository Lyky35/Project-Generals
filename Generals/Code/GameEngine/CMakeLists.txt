file(GLOB_RECURSE GameEngine_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Common/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/GameLogic/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/GameClient/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/GameNetwork/*.cpp
)

# Exclude files that use old/missing APIs (WPAudio streaming, DirectX simpleplayer, etc.)
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "[Gg]ame[Ss]peech\\.cpp$")
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "[Ss]imple[Pp]layer\\.cpp$")
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "[Uu]rl[Ll]aunch\\.cpp$")
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "[Ww][Oo][Ll][Bb][Rr][Oo][Ww][Ss][Ee][Rr]/")
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "INIWebpageURL\\.cpp$")
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "WOLLadderScreen\\.cpp$")
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "WOLLoginMenu\\.cpp$")
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "WOLWelcomeMenu\\.cpp$")
# Exclude GameSpy networking (extensive missing API stubs)
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "GameSpy\\.cpp$")
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "GameSpyChat\\.cpp$")
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "GameSpyGP\\.cpp$")
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "GameSpyGameInfo\\.cpp$")
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "GameSpyOverlay\\.cpp$")
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "GameSpyPersistentStorage\\.cpp$")
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "GameSpy/Chat\\.cpp$")
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "GameSpy/BuddyThread\\.cpp$")
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "GameSpy/PeerThread\\.cpp$")
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "GameSpy/PersistentStorageThread\\.cpp$")
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "GameSpy/GameResultsThread\\.cpp$")
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "GameSpy/Thread/")
list(FILTER GameEngine_SOURCES EXCLUDE REGEX "FirewallHelper\\.cpp$")

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
    ${CMAKE_CURRENT_SOURCE_DIR}/../Libraries/Source/WWVegas/WWDownload
)

target_link_libraries(GameEngine PUBLIC Compression WWLib GameSpyStub WWDownload)
