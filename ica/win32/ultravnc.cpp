#include <algorithm>

#include "stdhdrs.h"
#include "ItalcRfbExt.h"
#include "ItalcCoreServer.h"
#include "vsocket.cpp"

// adzm 2010-09
typedef enum {
	clientInitShared       = 0x01,
} rfbClientInitMsgFlags;

/*-----------------------------------------------------------------------------
 * Authentication
 *
 * Once the protocol version has been decided, the server then sends a 32-bit
 * word indicating whether any authentication is needed on the connection.
 * The value of this word determines the authentication scheme in use.  For
 * version 3.0 of the protocol this may have one of the following values:
 */

// adzm 2010-09
/*
 pre-RFB 3.8 -- rfbUltraVNC_SecureVNCPlugin extension

 If using SecureVNCPlugin (or any plugin that uses the integrated plugin architecture) the unofficial 1.0.8.2 version sends
 the auth type rfbUltraVNC_SecureVNCPlugin.

 The intention of this auth type is to act as a 'master' and once complete, allow other authentication types to occur
 over the now-encrypted communication channel.

 So, server sends 32-bit network order int for rfbUltraVNC_SecureVNCPlugin and the loop begins:
   server sends 16-bit little-endian challenge length, followed by the challenge
   viewer responds with 16-bit little-endian response length, followed by the response
   this continues until the plugin says to stop the loop. Currently the SecureVNC plugin only
     does one loop, but this functionality exists in order to implement more complicated handshakes.
 
 If there was a failure, and an error message is available, rfbVncAuthFailedEx (3) is sent followed by the length of the
   error string and then the error string
 If there was a failure, and no error message is available, simply send rfbVncAuthFailed (1)
 otherwise, if using mslogon, send rfbMsLogon, and if not using mslogon, send rfbVncAuthOK(0)

 at this point the handshake is 'complete' and all further communication is encrypted.

 if using mslogon, mslogon authentication will now occur (since the rfbMsLogon packet was sent)

 RFB 3.8 changes

 Now we send a byte for # of supported auth types, and then a byte for each auth type.

 rfbUltraVNC is not being used for anything, although rfbUltraVNC_SecureVNCPlugin has been established somewhat.
 Like a lot of these things, most of the values in the authentication range will end up going unused.
 
 Rather than complicate things further, I hereby declare this scheme: the top 4 bits will define the 'owner'
 of that set of values, and the bottom 4 bits will define the type. All of the values in the RFB 3.8 spec
 can then be covered by 0x0 and 0x1 for the top 4 bits.

                              mask
 RealVNC-approved values:     0x0F
 RealVNC-approved values:     0x1F
 reserved:                    0x2F
 reserved:                    0x3F
 reserved:                    0x4F
 reserved:                    0x5F
 reserved:                    0x6F
 UltraVNC:                    0x7F
 TightVNC:                    0x8F
 reserved:                    0x9F
 reserved:                    0xAF
 reserved:                    0xBF
 reserved:                    0xCF
 reserved:                    0xDF
 reserved:                    0xEF
 reserved:                    0xFF
*/

#define rfbConnFailed 0
#define rfbInvalidAuth 0
#define rfbNoAuth 1
#define rfbVncAuth 2
#define rfbUltraVNC 17
// adzm 2010-09 - After rfbUltraVNC, auth repeats via rfbVncAuthContinue

#define rfbUltraVNC_SCPrompt 0x68
#define rfbUltraVNC_SessionSelect 0x69
// adzm 2010-09 - Ultra subtypes
#define rfbUltraVNC_MsLogonIAuth 0x70

	// mslogonI never seems to be used anymore -- the old code would say if (m_ms_logon) AuthMsLogon (II) else AuthVnc
	// and within AuthVnc would be if (m_ms_logon) { /* mslogon code */ }. That could never be hit since the first case
	// would always match!

#define rfbUltraVNC_MsLogonIIAuth 0x71
#define rfbUltraVNC_SecureVNCPluginAuth 0x72

//adzm 2010-05-10 - for backwards compatibility with pre-3.8
#define rfbLegacy_SecureVNCPlugin 17
#define rfbLegacy_MsLogon 0xfffffffa // UltraVNC's MS-Logon with (hopefully) better security

// please see ABOVE these definitions for more discussion on authentication


/*
 * rfbConnFailed:	For some reason the connection failed (e.g. the server
 *			cannot support the desired protocol version).  This is
 *			followed by a string describing the reason (where a
 *			string is specified as a 32-bit length followed by that
 *			many ASCII characters).
 *
 * rfbNoAuth:		No authentication is needed.
 *
 * rfbVncAuth:		The VNC authentication scheme is to be used.  A 16-byte
 *			challenge follows, which the client encrypts as
 *			appropriate using the password and sends the resulting
 *			16-byte response.  If the response is correct, the
 *			server sends the 32-bit word rfbVncAuthOK.  If a simple
 *			failure happens, the server sends rfbVncAuthFailed and
 *			closes the connection. If the server decides that too
 *			many failures have occurred, it sends rfbVncAuthTooMany
 *			and closes the connection.  In the latter case, the
 *			server should not allow an immediate reconnection by
 *			the client.
 */

#define rfbVncAuthOK 0
#define rfbVncAuthFailed 1
// neither of these are used any longer in RFB 3.8
#define rfbVncAuthTooMany 2
#define rfbVncAuthFailedEx 3 //adzm 2010-05-11 - Send an explanatory message for the failure (if any)

// adzm 2010-09 - rfbUltraVNC or other auths may send this to restart authentication (perhaps over a now-secure channel)
#define rfbVncAuthContinue 0xFFFFFFFF



// UltraVNC specific extensions of RFB protocol
//
#define rfbServerState 0xAD // 26 March 2008 jdp

#define rfbKeepAlive 13 // 16 July 2008 jdp -- bidirectional
// adzm 2010-09 - Notify streaming DSM plugin support
#define rfbNotifyPluginStreaming 0x50

#define rfbRequestSession 20
#define rfbSetSession 21

  
// adzm 2010-09 - Notify streaming DSM plugin support
#define rfbEncodingPluginStreaming       0xC0A1E5CF

// viewer requests server state updates
#define rfbEncodingServerState              0xFFFF8000
#define rfbEncodingEnableKeepAlive          0xFFFF8001
#define rfbEncodingFTProtocolVersion           0xFFFF8002
#define rfbEncodingpseudoSession    		0xFFFF8003

// adzm 2010-09 - Notify streaming DSM plugin support
typedef struct {
    uint8_t type;			/* always rfbServerCutText */
    uint8_t pad1;
    uint16_t flags; // reserved - always 0
} rfbNotifyPluginStreamingMsg;

#define sz_rfbNotifyPluginStreamingMsg	4



#define rfbFileTransferSessionStart 15 // indicates a client has the FT gui open
#define rfbFileTransferSessionEnd   16 // indicates a client has closed the ft gui.
#define rfbFileTransferProtocolVersion 17 // indicates ft protocol version understood by sender. contentParam is version #


#define rfbPartialFilePrefix   "!UVNCPFT-\0" // Files are transferred with this prefix, until complete. Must end with "-", does not apply to directory transfers
#define sz_rfbPartialFilePrefix 9 

// new message for sending server state to client

#define rfbServerState_Disabled     0
#define rfbServerState_Enabled      1

#define rfbServerRemoteInputsState  1
#define rfbKeepAliveInterval        2

typedef struct {
    uint8_t   type;          /* always rfbServerState */
    uint8_t pad1;
    uint8_t pad2;
    uint8_t pad3;
    uint32_t  state;         /* state id*/
    uint32_t  value;         /* state value */ 
} rfbServerStateMsg;


#define sz_rfbServerStateMsg 12


typedef struct {
	uint8_t type;
} rfbKeepAliveMsg;

typedef struct {
	uint8_t type;
} rfbRequestSessionMsg;

typedef struct {
	uint8_t type;
	uint8_t number;
} rfbSetSessionMsg;

#define sz_rfbKeepAliveMsg 1
#define sz_rfbRequestSessionMsg 1
#define sz_rfbSetSessionMsg 2


qint64 vsocketDispatcher( char * _buf, const qint64 _len,
                                const SocketOpCodes _op_code, void * _user )
{
	VSocket * sock = static_cast<VSocket *>( _user );
	switch( _op_code )
	{
		case SocketRead: return( sock->ReadExact( _buf, _len ) ?
								_len : 0 );
		case SocketWrite: return( sock->SendExact( _buf, _len ) ?
								_len : 0 );
		case SocketGetPeerAddress:
			strncpy( _buf, sock->GetPeerName(), _len );
			return( 0 );
	}
	return( 0 );

}


#include "vncclient.cpp"
#include "vncdesktop.cpp"

