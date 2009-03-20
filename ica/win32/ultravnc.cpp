#include <algorithm>

#include "stdhdrs.h"
#include "italc_core_server.h"
#include "ica_main.h"
#include "vsocket.cpp"

// UltraVNC specific extensions of RFB protocol
//
/* rfbMsLogon indicates UltraVNC's MS-Logon with (hopefully) better security */
#define rfbMsLogon 0xfffffffa

#define rfbServerState 0xAD // 26 March 2008 jdp

#define rfbKeepAlive 13 // 16 July 2008 jdp -- bidirectional

// viewer requests server state updates
#define rfbEncodingServerState              0xFFFF8000
#define rfbEncodingEnableKeepAlive          0xFFFF8001
#define rfbEncodingFTProtocolVersion           0xFFFF8002


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

#define sz_rfbKeepAliveMsg 1


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

