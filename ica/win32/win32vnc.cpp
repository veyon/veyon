extern "C" char * strdup( const char * );
#include <assert.h>

#define _ASSERT assert
#define _ASSERTE assert

#include <rfb/rfbproto.h>
#include <rfb.h>

#define XMD_H
#define _WIN32_WINNT 0x0500

#define rfbAuthNone rfbNoAuth
#define rfbAuthVNC rfbVncAuth

#define rfbAuthOK rfbVncAuthOK
#define rfbAuthFailed rfbVncAuthFailed
#define rfbSecTypeTight rfbTight
#define rfbAuthUnixLogin 129
#define rfbAuthExternal 130


#define sig_rfbAuthNone "NOAUTH__"
#define sig_rfbAuthVNC "VNCAUTH_"
#define sig_rfbAuthUnixLogin "ULGNAUTH"
#define sig_rfbAuthExternal "XTRNAUTH"

#define rfbProtocolFallbackMinorVersion 3


/*-----------------------------------------------------------------------------
 * Structure used to describe protocol options such as tunneling methods,
 * authentication schemes and message types (protocol version 3.7t).
 */

typedef struct _rfbCapabilityInfo {

    CARD32 code;		/* numeric identifier */
    CARD8 vendorSignature[4];	/* vendor identification */
    CARD8 nameSignature[8];	/* abbreviated option name */

} rfbCapabilityInfo;

#define sz_rfbCapabilityInfoVendor 4
#define sz_rfbCapabilityInfoName 8
#define sz_rfbCapabilityInfo 16

/*
 * Vendors known by TightVNC: standard VNC/RealVNC, TridiaVNC, and TightVNC.
 */

#define rfbStandardVendor "STDV"
#define rfbTridiaVncVendor "TRDV"
#define rfbTightVncVendor "TGHT"


/*-----------------------------------------------------------------------------
 * Negotiation of Tunneling Capabilities (protocol version 3.7t)
 *
 * If the chosen security type is rfbSecTypeTight, the server sends a list of
 * supported tunneling methods ("tunneling" refers to any additional layer of
 * data transformation, such as encryption or external compression.)
 *
 * nTunnelTypes specifies the number of following rfbCapabilityInfo structures
 * that list all supported tunneling methods in the order of preference.
 *
 * NOTE: If nTunnelTypes is 0, that tells the client that no tunneling can be
 * used, and the client should not send a response requesting a tunneling
 * method.
 */

typedef struct _rfbTunnelingCapsMsg {
    CARD32 nTunnelTypes;
    /* followed by nTunnelTypes * rfbCapabilityInfo structures */
} rfbTunnelingCapsMsg;

#define sz_rfbTunnelingCapsMsg 4

/*-----------------------------------------------------------------------------
 * Tunneling Method Request (protocol version 3.7t)
 *
 * If the list of tunneling capabilities sent by the server was not empty, the
 * client should reply with a 32-bit code specifying a particular tunneling
 * method.  The following code should be used for no tunneling.
 */

#define rfbNoTunneling 0
#define sig_rfbNoTunneling "NOTUNNEL"


/*-----------------------------------------------------------------------------
 * Negotiation of Authentication Capabilities (protocol version 3.7t)
 *
 * After setting up tunneling, the server sends a list of supported
 * authentication schemes.
 *
 * nAuthTypes specifies the number of following rfbCapabilityInfo structures
 * that list all supported authentication schemes in the order of preference.
 *
 * NOTE: If nAuthTypes is 0, that tells the client that no authentication is
 * necessary, and the client should not send a response requesting an
 * authentication scheme.
 */

typedef struct _rfbAuthenticationCapsMsg {
    CARD32 nAuthTypes;
    /* followed by nAuthTypes * rfbCapabilityInfo structures */
} rfbAuthenticationCapsMsg;

#define sz_rfbAuthenticationCapsMsg 4

/* signatures for basic encoding types */
#define sig_rfbEncodingRaw       "RAW_____"
#define sig_rfbEncodingCopyRect  "COPYRECT"
#define sig_rfbEncodingRRE       "RRE_____"
#define sig_rfbEncodingCoRRE     "CORRE___"
#define sig_rfbEncodingHextile   "HEXTILE_"
#define sig_rfbEncodingZlib      "ZLIB____"
#define sig_rfbEncodingTight     "TIGHT___"
#define sig_rfbEncodingZlibHex   "ZLIBHEX_"

/* signatures for "fake" encoding types */
#define sig_rfbEncodingCompressLevel0  "COMPRLVL"
#define sig_rfbEncodingXCursor         "X11CURSR"
#define sig_rfbEncodingRichCursor      "RCHCURSR"
#define sig_rfbEncodingPointerPos      "POINTPOS"
#define sig_rfbEncodingLastRect        "LASTRECT"
#define sig_rfbEncodingNewFBSize       "NEWFBSIZ"
#define sig_rfbEncodingQualityLevel0   "JPEGQLVL"


/*-----------------------------------------------------------------------------
 * Server Interaction Capabilities Message (protocol version 3.7t)
 *
 * In the protocol version 3.7t, the server informs the client what message
 * types it supports in addition to ones defined in the protocol version 3.7.
 * Also, the server sends the list of all supported encodings (note that it's
 * not necessary to advertise the "raw" encoding sinse it MUST be supported in
 * RFB 3.x protocols).
 *
 * This data immediately follows the server initialisation message.
 */

typedef struct _rfbInteractionCapsMsg {
    CARD16 nServerMessageTypes;
    CARD16 nClientMessageTypes;
    CARD16 nEncodingTypes;
    CARD16 pad;			/* reserved, must be 0 */
    /* followed by nServerMessageTypes * rfbCapabilityInfo structures */
    /* followed by nClientMessageTypes * rfbCapabilityInfo structures */
} rfbInteractionCapsMsg;

#define sz_rfbInteractionCapsMsg 8


#include "isd_server.h"
#include "ica_main.h"
#include "src/VSocket.cpp"


qint64 vsocketDispatcher( char * _buf, const qint64 _len,
				const socketOpCodes _op_code, void * _user )
{
	VSocket * sock = static_cast<VSocket *>( _user );
	switch( _op_code )
	{
		case SocketRead: return( sock->ReadExact( _buf, _len ) ?
								_len : 0 );
		case SocketWrite: return( sock->SendExact( _buf, _len ) ?
						  		_len : 0 );
		case SocketGetIPBoundTo:
			strncpy( _buf, sock->GetPeerName(), _len );
			return( 0 );
	}
	return( 0 );

}



#include "src/ParseHost.c"
#include "src/vncKeymap.cpp"
#include "src/WinVNC.cpp"
#include "src/vncService.cpp"
#include "src/vncInstHandler.cpp"
#include "src/vncServer.cpp"
#include "src/vncClient.cpp"
#include "src/stdhdrs.cpp"
#include "src/omnithread/nt.cpp"
#include "src/RectList.cpp"
#include "src/translate.cpp"
#include "src/Log.cpp"
#include "src/vncEncoder.cpp"
#include "src/vncBuffer.cpp"
#include "src/vncRegion.cpp"
#include "src/vncSockConnect.cpp"
#include "src/VideoDriver.cpp"
#include "src/TsSessions.cpp"
#include "src/DynamicFn.cpp"

#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif

//#include "src/VNCHooks/VNCHooks.cpp"
//#include "src/VNCHooks/SharedData.cpp"
#include "src/VNCHooks/SharedData.h"
#include "src/vncDesktop.cpp"

