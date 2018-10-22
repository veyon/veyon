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

extern bool G_USE_PIXEL;

bool G_USE_PIXEL = false;
