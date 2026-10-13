#pragma once
#include "Common/AsciiString.h"
#ifndef __WEBBROWSER_H__
#define __WEBBROWSER_H__

// WebBrowser stub - ATL not available in MinGW
// The web browser functionality is disabled

class WebBrowserURL {
public:
    AsciiString m_url;
};

class WebBrowser {
public:
    void createBrowserWindow(const char* url, void* window) {}
    void closeBrowserWindow(void* window = 0) {}
    WebBrowserURL* findURL(const char* tag) { return 0; }
    WebBrowserURL* makeNewURL(const char* tag) { return 0; }
};

extern WebBrowser* TheWebBrowser;

#endif // __WEBBROWSER_H__
