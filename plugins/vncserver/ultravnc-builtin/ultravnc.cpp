#include "stdhdrs.h"

#define rfbConnFailed 0
#define rfbInvalidAuth 0
#define rfbNoAuth 1
#define rfbVncAuth 2
#define rfbUltraVNC 17

#define rfbVncAuthOK 0
#define rfbVncAuthFailed 1

// adzm 2010-09 - rfbUltraVNC or other auths may send this to restart authentication (perhaps over a now-secure channel)
#define rfbVncAuthContinue 0xFFFFFFFF



#include "vncclient.cpp"


#ifdef IPV6V4
const UINT MENU_ADD_CLIENT6_MSG_INIT = RegisterWindowMessage("WinVNC.AddClient6.Message.Init");
const UINT MENU_ADD_CLIENT6_MSG = RegisterWindowMessage("WinVNC.AddClient6.Message");
#endif

const UINT MENU_ADD_CLIENT_MSG_INIT = RegisterWindowMessage("WinVNC.AddClient.Message.Init");
const UINT MENU_ADD_CLIENT_MSG = RegisterWindowMessage("WinVNC.AddClient.Message");
const UINT MENU_AUTO_RECONNECT_MSG = RegisterWindowMessage("WinVNC.AddAutoClient.Message");
const UINT MENU_STOP_RECONNECT_MSG = RegisterWindowMessage("WinVNC.AddStopClient.Message");
const UINT MENU_STOP_ALL_RECONNECT_MSG = RegisterWindowMessage("WinVNC.AddStopAllClient.Message");
const UINT MENU_REPEATER_ID_MSG = RegisterWindowMessage("WinVNC.AddRepeaterID.Message");


const char *MENU_CLASS_NAME = "WinVNC Tray Icon";


HWND G_MENU_HWND = NULL;

bool G_USE_PIXEL = false;
