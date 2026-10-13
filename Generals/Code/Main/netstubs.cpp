// netstubs.cpp - GameSpy/Firewall/WebBrowser/CD/registry stubs for RTS.exe link milestone
#include "Lib/BaseType.h"
#include "Common/AsciiString.h"
#include "Common/UnicodeString.h"
#include "GameNetwork/FirewallHelper.h"
#include "GameNetwork/GameSpyOverlay.h"
#include "GameNetwork/GameSpy/PersistentStorageThread.h"
#include "GameNetwork/GameSpy/PingThread.h"
#include "GameNetwork/GameSpy/GameResultsThread.h"
#include "GameNetwork/GameSpy/BuddyThread.h"
#include "GameNetwork/GameSpy/PeerThread.h"
#include "GameClient/GUICallbacks.h"
#include "WW3D2/dx8webbrowser.h"
#include "Common/INI.h"
#include <windows.h>
#include <string>

// --- FirewallHelperClass ---
FirewallHelperClass::FirewallHelperClass(void) {}
FirewallHelperClass::~FirewallHelperClass(void) {}
Bool FirewallHelperClass::detectFirewall(void) { return FALSE; }
Short FirewallHelperClass::getSourcePortAllocationDelta(void) { return 0; }
Int FirewallHelperClass::getFirewallHardness(FirewallBehaviorType) { return 0; }
Int FirewallHelperClass::getFirewallRetries(FirewallBehaviorType) { return 0; }
void FirewallHelperClass::readFirewallBehavior(void) {}
void FirewallHelperClass::reset(void) {}
Bool FirewallHelperClass::behaviorDetectionUpdate(void) { return FALSE; }
FirewallHelperClass::FirewallBehaviorType FirewallHelperClass::getFirewallBehavior(void) { return FIREWALL_TYPE_UNKNOWN; }
void FirewallHelperClass::writeFirewallBehavior(void) {}
void FirewallHelperClass::closeSpareSocket(UnsignedShort) {}
Bool FirewallHelperClass::openSpareSocket(UnsignedShort) { return FALSE; }
UnsignedShort FirewallHelperClass::getNextTemporarySourcePort(Int) { return 0; }
void FirewallHelperClass::getManglerName(Int, Char*) {}
UnsignedShort FirewallHelperClass::getManglerResponse(UnsignedShort, Int) { return 0; }
Bool FirewallHelperClass::sendToManglerFromPort(UnsignedInt, UnsignedShort, UnsignedShort, Bool) { return FALSE; }
void FirewallHelperClass::flagNeedToRefresh(Bool) {}
FirewallHelperClass* createFirewallHelper() { return new FirewallHelperClass; }
FirewallHelperClass* TheFirewallHelper = 0;

// --- DX8WebBrowser ---
bool DX8WebBrowser::Initialize(const char*, const char*, const char*, const char*) { return false; }
void DX8WebBrowser::Shutdown(void) {}
void DX8WebBrowser::Update(void) {}
void DX8WebBrowser::Render(int) {}
void DX8WebBrowser::CreateBrowser(const char*, const char*, int, int, int, int, int, LONG, LPDISPATCH) {}
void DX8WebBrowser::DestroyBrowser(const char*) {}
bool DX8WebBrowser::Is_Browser_Open(const char*) { return false; }
void DX8WebBrowser::Navigate(const char*, const char*) {}

// --- GameSpy overlays / chat ---
void GameSpyUpdateOverlays() {}
void GameSpyCloseAllOverlays() {}
void GameSpyOpenOverlay(GSOverlayType) {}
void GameSpyCloseOverlay(GSOverlayType) {}
void GameSpyToggleOverlay(GSOverlayType) {}
Bool GameSpyIsOverlayOpen(GSOverlayType) { return FALSE; }
void GSMessageBoxOk(UnicodeString, UnicodeString, void(*)()) {}
void GSMessageBoxOkCancel(UnicodeString, UnicodeString, void(*)(), void(*)()) {}
void RaiseGSMessageBox() {}
extern "C" {
int getQR2HostingStatus() { return 0; }
void ghttpStartup() {}
void ghttpCleanup() {}
void ghttpSetProxy(const char*) {}
void* ghttpGet(const char*, const char*, void*, int, void*, void*, int) { return 0; }
void* ghttpGetHeaders(const char*, const char*, void*, int, void*, void*, int) { return 0; }
void* ghttpHead(const char*, const char*, void*, int, void*, void*, int) { return 0; }
void ghttpThink() {}
}
int isThreadHosting = 0;

// Queue / interface singletons (declared in headers)
GameSpyPSMessageQueueInterface* TheGameSpyPSMessageQueue = 0;
GameSpyPeerMessageQueueInterface* TheGameSpyPeerMessageQueue = 0;
GameSpyBuddyMessageQueueInterface* TheGameSpyBuddyMessageQueue = 0;
GameResultsInterface* TheGameResultsQueue = 0;
PingerInterface* ThePinger = 0;

GameResultsInterface* GameResultsInterface::createNewGameResultsInterface() { return 0; }
GameSpyPSMessageQueueInterface* GameSpyPSMessageQueueInterface::createNewMessageQueue() { return 0; }
GameSpyPeerMessageQueueInterface* GameSpyPeerMessageQueueInterface::createNewMessageQueue() { return 0; }
GameSpyBuddyMessageQueueInterface* GameSpyBuddyMessageQueueInterface::createNewMessageQueue() { return 0; }
PingerInterface* PingerInterface::createNewPingerInterface() { return 0; }
std::string GameSpyPSMessageQueueInterface::formatPlayerKVPairs(PSPlayerStats) { return {}; }
PSPlayerStats GameSpyPSMessageQueueInterface::parsePlayerKVPairs(std::string) { return PSPlayerStats(); }

// PSRequest / PSPlayerStats ctors from PersistentStorageThread.h
PSRequest::PSRequest() {}
PSPlayerStats::PSPlayerStats() {}
PSPlayerStats::PSPlayerStats(const PSPlayerStats&) {}
void PSPlayerStats::reset() {}

// INI parsers
// GameSpy/web-page INI blocks no longer have real parsers, but the block's
// lines MUST still be consumed (up to the END token). An empty stub returns
// immediately and the next indented key is misread as a new top-level block -
// which threw during boot and produced the "Technical Difficulties" box.
void INI::parseOnlineChatColorDefinition(INI* ini)
{
	while (!ini->isEOF())
	{
		ini->readLine();
		const char *tok = strtok(ini->m_buffer, ini->getSeps());
		if (tok && stricmp(tok, ini->m_blockEndToken) == 0)
			break;
	}
}
void INI::parseWebpageURLDefinition(INI* ini)
{
	while (!ini->isEOF())
	{
		ini->readLine();
		const char *tok = strtok(ini->m_buffer, ini->getSeps());
		if (tok && stricmp(tok, ini->m_blockEndToken) == 0)
			break;
	}
}

// CD manager / misc
// the Win32CDManager device class isn't built (no CD/SafeDisc in this port);
// wire the generic CDManager instead - returning NULL here crashed
// initSubsystem(TheCDManager, ...) with this=0x8 on the first boot.
#include "Common/CDManager.h"
CDManagerInterface* CreateCDManager(void) { return new CDManager; }
void CheckReOpenPlayerInfo() {}
void ReOpenPlayerInfo() {}
void ReloadAllTextures() {}
void FillStackAddresses(void**, unsigned, unsigned) {}
void StackDumpFromAddresses(void**, unsigned, void(*)(const char*)) {}
void OSDisplayWarningBox(AsciiString, AsciiString, unsigned, unsigned) {}
void GetFunctionDetails(void*, char*, char*, unsigned*, unsigned*) {}
void DumpExceptionInfo(unsigned code, EXCEPTION_POINTERS* info) { (void)code; (void)info; }
int DX8Wrapper_IsWindowed = 0;
int DX8Wrapper_PreserveFPU = 0;
void* g_LastErrorDump = 0;
std::string MultiByteToWideCharSingleLine(const char*) { return {}; }
std::string WideCharStringToMultiByte(const wchar_t*) { return {}; }

// Registry
Bool GetStringFromRegistry(std::string, std::string, std::string&) { return FALSE; }
Bool GetUnsignedIntFromRegistry(std::string, std::string, unsigned&) { return FALSE; }
Bool SetStringInRegistry(std::string, std::string, std::string) { return FALSE; }
Bool SetUnsignedIntInRegistry(std::string, std::string, unsigned) { return FALSE; }
Bool FormatURLFromRegistry(std::string&, std::string&, std::string&, std::string&) { return FALSE; }

// Cftp

// MOTD
WindowMsgHandledType MOTDSystem(GameWindow*, UnsignedInt, WindowMsgData, WindowMsgData) { return MSG_HANDLED; }

// --- GameSpyInfo virtual methods (PeerDefsImplementation) ---
#include "GameNetwork/GameSpy/PeerDefsImplementation.h"
void GameSpyInfo::registerTextWindow(GameWindow*) {}
void GameSpyInfo::unregisterTextWindow(GameWindow*) {}
Int GameSpyInfo::addText(UnicodeString, Color, GameWindow*) { return 0; }
void GameSpyInfo::addChat(PlayerInfo, UnicodeString, Bool, Bool, GameWindow*) {}
void GameSpyInfo::addChat(AsciiString, Int, UnicodeString, Bool, Bool, GameWindow*) {}
Bool GameSpyInfo::sendChat(UnicodeString, Bool, GameWindow*) { return FALSE; }

// --- _set_se_translator (MSVC SEH; no-op on MinGW) ---
extern "C" void _set_se_translator(void(*)(unsigned, void*)) {}
// Import thunk referenced by WinMain (MSVC SEH hook)
extern "C" {
void (*__imp__Z18_set_se_translatorPFvjP19_EXCEPTION_POINTERSE)(void(*)(unsigned, void*)) = _set_se_translator;
}

Color GameSpyColor[GSCOLOR_MAX];
