//  Copyright (C) 2001-2006 Constantin Kaplinsky. All Rights Reserved.
//  Copyright (C) 2002 Vladimir Vologzhanin. All Rights Reserved.
//  Copyright (C) 2000 Tridia Corporation. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// TightVNC distribution homepage on the Web: http://www.tightvnc.com/
//
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.


// vncClient.cpp

// The per-client object.  This object takes care of all per-client stuff,
// such as socket input and buffering of updates.

// vncClient class handles the following functions:
// - Recieves requests from the connected client and
//   handles them
// - Handles incoming updates properly, using a vncBuffer
//   object to keep track of screen changes
// It uses a vncBuffer and is passed the vncDesktop and
// vncServer to communicate with.

// Includes
#include "stdhdrs.h"
#include <omnithread.h>
#include "resource.h"

// Custom
#include "vncClient.h"
#include "VSocket.h"
#include "vncDesktop.h"
#include "vncRegion.h"
#include "vncBuffer.h"
#include "vncService.h"
#include "vncPasswd.h"
#include "vncAcceptDialog.h"
#include "vncKeymap.h"
#include <windows.h>
extern "C" {
#include "d3des.h"
}

#include "FileTransferItemInfo.h"
#include "vncMenu.h"

//
// Normally, using macros is no good, but this macro saves us from
// writing constants twice -- it constructs signature names from codes.
// Note that "code_sym" argument should be a single symbol, not an expression.
//

#define SetCapInfo(cap_ptr, code_sym, vendor)			\
{														\
	rfbCapabilityInfo *pcap = (cap_ptr);				\
	pcap->code = Swap32IfLE(code_sym);					\
	memcpy(pcap->vendorSignature, (vendor),				\
	sz_rfbCapabilityInfoVendor);						\
	memcpy(pcap->nameSignature, sig_##code_sym,			\
	sz_rfbCapabilityInfoName);							\
}

// vncClient thread class

class vncClientThread : public omni_thread
{
public:
	char * ConvertPath(char *path);

	// Init
	virtual BOOL Init(vncClient *client,
					  vncServer *server,
					  VSocket *socket,
					  BOOL reverse,
					  BOOL shared);

	// Sub-Init routines
	virtual BOOL InitVersion();
	virtual BOOL InitAuthenticate();
	virtual int GetAuthenticationType();
	virtual void SendConnFailedMessage(const char *reasonString);
	virtual BOOL SendTextStringMessage(const char *str);
	virtual BOOL NegotiateTunneling();
	virtual BOOL NegotiateAuthentication(int authType);
	virtual BOOL AuthenticateNone();
	virtual BOOL AuthenticateVNC();
	virtual BOOL ReadClientInit();
	virtual BOOL SendInteractionCaps();

	// The main thread function
	virtual void run(void *arg);

protected:
	virtual ~vncClientThread();

	// Fields
protected:
	VSocket *m_socket;
	vncServer *m_server;
	vncClient *m_client;
	BOOL m_reverse;
	BOOL m_shared;
};

vncClientThread::~vncClientThread()
{
	// If we have a client object then delete it
	if (m_client != NULL)
		delete m_client;
}

BOOL
vncClientThread::Init(vncClient *client, vncServer *server, VSocket *socket, BOOL reverse, BOOL shared)
{
	// Save the server pointer and window handle
	m_server = server;
	m_socket = socket;
	m_client = client;
	m_reverse = reverse;
	m_shared = shared;

	// Start the thread
	start();

	return TRUE;
}

BOOL
vncClientThread::InitVersion()
{
	// Generate the server's protocol version
	rfbProtocolVersionMsg protocolMsg;
	sprintf((char *)protocolMsg, rfbProtocolVersionFormat, 3, 8);

	// Send the protocol message
	if (!m_socket->SendExact((char *)&protocolMsg, sz_rfbProtocolVersionMsg))
		return FALSE;

	// Now, get the client's protocol version
	rfbProtocolVersionMsg protocol_ver;
	protocol_ver[12] = 0;
	if (!m_socket->ReadExact((char *)&protocol_ver, sz_rfbProtocolVersionMsg))
		return FALSE;

	// Check the protocol version
	int major, minor;
	sscanf((char *)&protocol_ver, rfbProtocolVersionFormat, &major, &minor);
	if (major != 3) {
		vnclog.Print(LL_CONNERR, VNCLOG("unsupported protocol version %d.%d\n"),
					 major, minor);
		return FALSE;
	}
	int effective_minor = minor;
	if (minor > 8) {						// buggy client
		effective_minor = 8;
	} else if (minor > 3 && minor < 7) {	// non-standard client
		effective_minor = 3;
	} else if (minor < 3) {					// ancient client
		effective_minor = 3;
	}
	if (effective_minor != minor) {
		vnclog.Print(LL_CONNERR,
					 VNCLOG("non-standard protocol version 3.%d, using 3.%d instead\n"),
					 minor, effective_minor);
	}

	// Save the minor number of the protocol version
	m_client->m_protocol_minor_version = effective_minor;

	// TightVNC protocol extensions are not enabled yet
	m_client->m_protocol_tightvnc = FALSE;

	vnclog.Print(LL_INTINFO, VNCLOG("negotiated protocol version, RFB 3.%d\n"),
				 effective_minor);
	return TRUE;
}

BOOL
vncClientThread::InitAuthenticate()
{
	int secType = GetAuthenticationType();
	if (secType == rfbSecTypeInvalid)
		return FALSE;

	if (m_client->m_protocol_minor_version >= 7) {
 		CARD8 list[2];
 		list[0] = (CARD8)1;					// number of security types
 		list[1] = (CARD8)rfbSecTypeItalc;			// primary security type
		if (!m_socket->SendExact((char *)list, sizeof(list)))
			return FALSE;
		CARD8 type;
		if (!m_socket->ReadExact((char *)&type, sizeof(type)))
			return FALSE;
 		if( type == rfbSecTypeItalc )
 		{
 			return( isdServer::authSecTypeItalc( vsocketDispatcher,
 							m_socket,
 							ItalcAuthDSA ) );
 		}
 		return( FALSE );
#if 0
		if (type == (CARD8)rfbSecTypeTight) {
			vnclog.Print(LL_INTINFO, VNCLOG("enabling TightVNC protocol extensions\n"));
			m_client->m_protocol_tightvnc = TRUE;
			if (!NegotiateTunneling())
				return FALSE;
			if (!NegotiateAuthentication(secType))
				return FALSE;
		} else if (type != (CARD8)secType) {
			vnclog.Print(LL_CONNERR, VNCLOG("incorrect security type requested\n"));
			return FALSE;
		}
#endif
	} else {
		CARD32 authValue = Swap32IfLE(secType);
		if (!m_socket->SendExact((char *)&authValue, sizeof(authValue)))
			return FALSE;
	}

	switch (secType) {
	case rfbSecTypeNone:
		vnclog.Print(LL_CLIENTS, VNCLOG("no authentication necessary\n"));
		return AuthenticateNone();
	case rfbSecTypeVncAuth:
		vnclog.Print(LL_CLIENTS, VNCLOG("performing VNC authentication\n"));
		return AuthenticateVNC();
	}

	return FALSE;	// should not happen but just in case...
}

int
vncClientThread::GetAuthenticationType()
{
#if 0
	if (!m_reverse && !m_server->ValidPasswordsSet())
	{
		vnclog.Print(LL_CONNERR,
					 VNCLOG("no password specified for server - client rejected\n"));

		// Send an error message to the client
		SendConnFailedMessage("This server does not have a valid password enabled. "
							  "Until a password is set, incoming connections cannot "
							  "be accepted.");
		return rfbSecTypeInvalid;
	}

	// By default we filter out local loop connections, because they're pointless
	if (!m_server->LoopbackOk())
	{
		char *localname = strdup(m_socket->GetSockName());
		char *remotename = strdup(m_socket->GetPeerName());

		// Check that the local & remote names are different!
		if (localname != NULL && remotename != NULL) {
			BOOL ok = strcmp(localname, remotename) != 0;

// FIXME: conceivable memory leak
			free(localname);
			free(remotename);

			if (!ok) {
				vnclog.Print(LL_CONNERR,
							 VNCLOG("loopback connection attempted - client rejected\n"));

				// Send an error message to the client
				SendConnFailedMessage("Local loop-back connections are disabled.");
				return rfbSecTypeInvalid;
			}
		}
	}

	// Verify the peer host name against the AuthHosts string
	vncServer::AcceptQueryReject verified;
	if (m_reverse) {
		verified = vncServer::aqrAccept;
	} else {
		verified = m_server->VerifyHost(m_socket->GetPeerName());
	}

	// If necessary, query the connection with a timed dialog
	verified = vncServer::aqrAccept;
	BOOL skip_auth = FALSE;
	if (verified == vncServer::aqrQuery) {
		vncAcceptDialog *acceptDlg =
			new vncAcceptDialog(m_server->QueryTimeout(),
								m_server->QueryAccept(),
								m_server->QueryAllowNoPass(),
								m_socket->GetPeerName());
		if (acceptDlg == NULL) {
			if (m_server->QueryAccept()) {
				verified = vncServer::aqrAccept;
			} else {
				verified = vncServer::aqrReject;
			}
		} else {
			int action = acceptDlg->DoDialog();
			if (action > 0) {
				verified = vncServer::aqrAccept;
				if (action == 2)
					skip_auth = TRUE;	// accept without authentication
			} else {
				verified = vncServer::aqrReject;
			}
			delete acceptDlg;
		}
	}

	// The connection should be rejected, either due to AuthHosts settings,
	// or because of the "Reject" action performed in the query dialog
	if (verified == vncServer::aqrReject) {
		vnclog.Print(LL_CONNERR, VNCLOG("Client connection rejected\n"));
		SendConnFailedMessage("Your connection has been rejected.");
		return rfbSecTypeInvalid;
	}

	// Return preferred authentication type
	if (m_reverse || skip_auth || m_server->ValidPasswordsEmpty()) {
		return rfbSecTypeNone;
	} else {
		return rfbSecTypeVncAuth;
	}
#endif
	return rfbSecTypeItalc;
}

//
// Send a "connection failed" message.
//

void
vncClientThread::SendConnFailedMessage(const char *reasonString)
{
	if (m_client->m_protocol_minor_version >= 7) {
		CARD8 zeroCount = 0;
		if (!m_socket->SendExact((char *)&zeroCount, sizeof(zeroCount)))
			return;
	} else {
		CARD32 authValue = Swap32IfLE(rfbSecTypeInvalid);
		if (!m_socket->SendExact((char *)&authValue, sizeof(authValue)))
			return;
	}
	SendTextStringMessage(reasonString);
}

//
// Send a text message preceded with a length counter.
//

BOOL
vncClientThread::SendTextStringMessage(const char *str)
{
	CARD32 len = Swap32IfLE(strlen(str));
	if (!m_socket->SendExact((char *)&len, sizeof(len)))
		return FALSE;
	if (!m_socket->SendExact(str, strlen(str)))
		return FALSE;

	return TRUE;
}

//
// Negotiate tunneling type (protocol versions 3.7t, 3.8t).
//

BOOL
vncClientThread::NegotiateTunneling()
{
	int nTypes = 0;

	// Advertise our tunneling capabilities (currently, nothing to advertise).
	rfbTunnelingCapsMsg caps;
	caps.nTunnelTypes = Swap32IfLE(nTypes);
	return m_socket->SendExact((char *)&caps, sz_rfbTunnelingCapsMsg);

	// Read tunneling type requested by the client (currently, not necessary).
	if (nTypes) {
		CARD32 tunnelType;
		if (!m_socket->ReadExact((char *)&tunnelType, sizeof(tunnelType)))
			return FALSE;
		tunnelType = Swap32IfLE(tunnelType);
		// We cannot do tunneling yet.
		vnclog.Print(LL_CONNERR, VNCLOG("unsupported tunneling type requested\n"));
		return FALSE;
	}

	vnclog.Print(LL_INTINFO, VNCLOG("negotiated tunneling type\n"));
	return TRUE;
}

//
// Negotiate authentication scheme (protocol versions 3.7t, 3.8t).
// NOTE: Here we always send en empty list for "no authentication".
//

BOOL
vncClientThread::NegotiateAuthentication(int authType)
{
	int nTypes = 0;

	if (authType == rfbAuthVNC) {
		nTypes++;
	} else if (authType != rfbAuthNone) {
		vnclog.Print(LL_INTERR, VNCLOG("unknown authentication type\n"));
		return FALSE;
	}

	rfbAuthenticationCapsMsg caps;
	caps.nAuthTypes = Swap32IfLE(nTypes);
	if (!m_socket->SendExact((char *)&caps, sz_rfbAuthenticationCapsMsg))
		return FALSE;

	if (authType == rfbAuthVNC) {
		// Inform the client about supported authentication types.
		rfbCapabilityInfo cap;
		SetCapInfo(&cap, rfbAuthVNC, rfbStandardVendor);
		if (!m_socket->SendExact((char *)&cap, sz_rfbCapabilityInfo))
			return FALSE;

		CARD32 type;
		if (!m_socket->ReadExact((char *)&type, sizeof(type)))
			return FALSE;
		type = Swap32IfLE(type);
		if ((int) type != authType) {
			vnclog.Print(LL_CONNERR, VNCLOG("incorrect authentication type requested\n"));
			return FALSE;
		}
	}

	return TRUE;
}

//
// Handle security type for "no authentication".
//

BOOL
vncClientThread::AuthenticateNone()
{
	if (m_client->m_protocol_minor_version >= 8) {
		CARD32 secResult = Swap32IfLE(rfbAuthOK);
		if (!m_socket->SendExact((char *)&secResult, sizeof(secResult)))
			return FALSE;
	}
	return TRUE;
}

//
// Perform standard VNC authentication
//

BOOL
vncClientThread::AuthenticateVNC()
{
	BOOL auth_ok = FALSE;

	// Retrieve local passwords
	char password[MAXPWLEN];
	BOOL password_set = m_server->GetPassword(password);
	vncPasswd::ToText plain(password);
	BOOL password_viewonly_set = m_server->GetPasswordViewOnly(password);
	vncPasswd::ToText plain_viewonly(password);

	// Now create a 16-byte challenge
	char challenge[16];
	char challenge_viewonly[16];

	vncRandomBytes((BYTE *)&challenge);
	memcpy(challenge_viewonly, challenge, 16);

	// Send the challenge to the client
	if (!m_socket->SendExact(challenge, sizeof(challenge)))
		return FALSE;

	// Read the response
	char response[16];
	if (!m_socket->ReadExact(response, sizeof(response)))
		return FALSE;

	// Encrypt the challenge bytes
	vncEncryptBytes((BYTE *)&challenge, plain);

	// Compare them to the response
	if (password_set && memcmp(challenge, response, sizeof(response)) == 0) {
		auth_ok = TRUE;
	} else {
		// Check against the view-only password
		vncEncryptBytes((BYTE *)&challenge_viewonly, plain_viewonly);
		if (password_viewonly_set && memcmp(challenge_viewonly, response, sizeof(response)) == 0) {
			m_client->EnablePointer(FALSE);
			m_client->EnableKeyboard(FALSE);
			auth_ok = TRUE;
		}
	}

	// Did the authentication work?
	CARD32 secResult;
	if (!auth_ok) {
		vnclog.Print(LL_CONNERR, VNCLOG("authentication failed\n"));

		secResult = Swap32IfLE(rfbAuthFailed);
		m_socket->SendExact((char *)&secResult, sizeof(secResult));
		SendTextStringMessage("Authentication failed");
		return FALSE;
	} else {
		// Tell the client we're ok
		secResult = Swap32IfLE(rfbAuthOK);
		if (!m_socket->SendExact((char *)&secResult, sizeof(secResult)))
			return FALSE;
	}

	return TRUE;
}

//
// Read client initialisation message
//

BOOL
vncClientThread::ReadClientInit()
{
	// Read the client's initialisation message
	rfbClientInitMsg client_ini;
	if (!m_socket->ReadExact((char *)&client_ini, sz_rfbClientInitMsg))
		return FALSE;

	// If the client wishes to have exclusive access then remove other clients
	if (!client_ini.shared && !m_shared)
	{
		// Which client takes priority, existing or incoming?
		if (m_server->ConnectPriority() < 1) {
			// Incoming
			vnclog.Print(LL_INTINFO, VNCLOG("non-shared connection - disconnecting old clients\n"));
			m_server->KillAuthClients();
		} else if (m_server->ConnectPriority() > 1) {
			// Existing
			if (m_server->AuthClientCount() > 0) {
				vnclog.Print(LL_CLIENTS, VNCLOG("connections already exist - client rejected\n"));
				return FALSE;
			}
		}
	}

	// Tell the server that this client is ok
	return m_server->Authenticated(m_client->GetClientId());
}

//
// Advertise our messaging capabilities (protocol version 3.7+).
//

BOOL
vncClientThread::SendInteractionCaps()
{
	// Update these constants on changing capability lists!
	const int MAX_SMSG_CAPS = 4;
	const int MAX_CMSG_CAPS = 6;
	const int MAX_ENC_CAPS = 14;

	int i;

	// Supported server->client message types
	rfbCapabilityInfo smsg_list[MAX_SMSG_CAPS];
	i = 0;

	if (m_server->FileTransfersEnabled() && m_client->IsInputEnabled()) {
#if 0
		SetCapInfo(&smsg_list[i++], rfbFileListData,       rfbTightVncVendor);
		SetCapInfo(&smsg_list[i++], rfbFileDownloadData,   rfbTightVncVendor);
		SetCapInfo(&smsg_list[i++], rfbFileUploadCancel,   rfbTightVncVendor);
		SetCapInfo(&smsg_list[i++], rfbFileDownloadFailed, rfbTightVncVendor);
#endif
	}

	int nServerMsgs = i;
	if (nServerMsgs > MAX_SMSG_CAPS) {
		vnclog.Print(LL_INTERR,
					 VNCLOG("assertion failed, nServerMsgs > MAX_SMSG_CAPS\n"));
		return FALSE;
	}

	// Supported client->server message types
	rfbCapabilityInfo cmsg_list[MAX_CMSG_CAPS];
	i = 0;

	if (m_server->FileTransfersEnabled() && m_client->IsInputEnabled()) {
#if 0
		SetCapInfo(&cmsg_list[i++], rfbFileListRequest,    rfbTightVncVendor);
		SetCapInfo(&cmsg_list[i++], rfbFileDownloadRequest,rfbTightVncVendor);
		SetCapInfo(&cmsg_list[i++], rfbFileUploadRequest,  rfbTightVncVendor);
		SetCapInfo(&cmsg_list[i++], rfbFileUploadData,     rfbTightVncVendor);
		SetCapInfo(&cmsg_list[i++], rfbFileDownloadCancel, rfbTightVncVendor);
		SetCapInfo(&cmsg_list[i++], rfbFileUploadFailed,   rfbTightVncVendor);
#endif
	}

	int nClientMsgs = i;
	if (nClientMsgs > MAX_CMSG_CAPS) {
		vnclog.Print(LL_INTERR,
					 VNCLOG("assertion failed, nClientMsgs > MAX_CMSG_CAPS\n"));
		return FALSE;
	}

	// Encoding types
	rfbCapabilityInfo enc_list[MAX_ENC_CAPS];
	i = 0;
	SetCapInfo(&enc_list[i++],  rfbEncodingCopyRect,       rfbStandardVendor);
	SetCapInfo(&enc_list[i++],  rfbEncodingRRE,            rfbStandardVendor);
	SetCapInfo(&enc_list[i++],  rfbEncodingCoRRE,          rfbStandardVendor);
	SetCapInfo(&enc_list[i++],  rfbEncodingHextile,        rfbStandardVendor);
	SetCapInfo(&enc_list[i++],  rfbEncodingZlib,           rfbTridiaVncVendor);
	SetCapInfo(&enc_list[i++],  rfbEncodingZlibHex,        rfbTridiaVncVendor);
	SetCapInfo(&enc_list[i++],  rfbEncodingTight,          rfbTightVncVendor);
	SetCapInfo(&enc_list[i++],  rfbEncodingCompressLevel0, rfbTightVncVendor);
	SetCapInfo(&enc_list[i++],  rfbEncodingQualityLevel0,  rfbTightVncVendor);
	SetCapInfo(&enc_list[i++],  rfbEncodingXCursor,        rfbTightVncVendor);
	SetCapInfo(&enc_list[i++],  rfbEncodingRichCursor,     rfbTightVncVendor);
	SetCapInfo(&enc_list[i++],  rfbEncodingPointerPos,     rfbTightVncVendor);
	SetCapInfo(&enc_list[i++],  rfbEncodingLastRect,       rfbTightVncVendor);
	SetCapInfo(&enc_list[i++],  rfbEncodingNewFBSize,      rfbTightVncVendor);
	int nEncodings = i;
	if (nEncodings > MAX_ENC_CAPS) {
		vnclog.Print(LL_INTERR,
					 VNCLOG("assertion failed, nEncodings > MAX_ENC_CAPS\n"));
		return FALSE;
	}

	// Create and send the header structure
	rfbInteractionCapsMsg intr_caps;
	intr_caps.nServerMessageTypes = Swap16IfLE(nServerMsgs);
	intr_caps.nClientMessageTypes = Swap16IfLE(nClientMsgs);
	intr_caps.nEncodingTypes = Swap16IfLE(nEncodings);
	intr_caps.pad = 0;
	if (!m_socket->SendExact((char *)&intr_caps, sz_rfbInteractionCapsMsg))
		return FALSE;

	// Send the capability lists
	if (nServerMsgs &&
		!m_socket->SendExact((char *)&smsg_list[0],
		sz_rfbCapabilityInfo * nServerMsgs))
		return FALSE;
	if (nClientMsgs &&
		!m_socket->SendExact((char *)&cmsg_list[0],
		sz_rfbCapabilityInfo * nClientMsgs))
		return FALSE;
	if (nEncodings &&
		!m_socket->SendExact((char *)&enc_list[0],
		sz_rfbCapabilityInfo * nEncodings))
		return FALSE;

	return TRUE;
}

void
ClearKeyState(BYTE key)
{
	// This routine is used by the VNC client handler to clear the
	// CAPSLOCK, NUMLOCK and SCROLL-LOCK states.

	BYTE keyState[256];

	GetKeyboardState((LPBYTE)&keyState);

	if(keyState[key] & 1)
	{
		// Simulate the key being pressed
		keybd_event(key, 0, KEYEVENTF_EXTENDEDKEY, 0);

		// Simulate it being release
		keybd_event(key, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
	}
}

void
vncClientThread::run(void *arg)
{
	// All this thread does is go into a socket-recieve loop,
	// waiting for stuff on the given socket

	// IMPORTANT : ALWAYS call RemoveClient on the server before quitting
	// this thread.

	vnclog.Print(LL_CLIENTS, VNCLOG("client connected : %s (id %hd)\n"),
				 m_client->GetClientName(), m_client->GetClientId());

	// Save the handle to the thread's original desktop
	HDESK home_desktop = GetThreadDesktop(GetCurrentThreadId());

	// To avoid people connecting and then halting the connection, set a timeout
	if (!m_socket->SetTimeout(30000))
		vnclog.Print(LL_INTERR,
					 VNCLOG("failed to set socket timeout, error=%d\n"),
					 GetLastError());

	// Initially blacklist the client so that excess connections from it get dropped
	m_server->AddAuthHostsBlacklist(m_client->GetClientName());

	// LOCK INITIAL SETUP
	// All clients have the m_protocol_ready flag set to FALSE initially, to prevent
	// updates and suchlike interfering with the initial protocol negotiations.

	// GET PROTOCOL VERSION
	if (!InitVersion()) {
		m_server->RemoveClient(m_client->GetClientId());
		return;
	}

	// AUTHENTICATE LINK
	if (!InitAuthenticate()) {
		m_server->RemoveClient(m_client->GetClientId());
		return;
	}

	// READ CLIENT INITIALIZATION MESSAGE
	if (!ReadClientInit()) {
		m_server->RemoveClient(m_client->GetClientId());
		return;
	}

	// Authenticated OK - remove from blacklist and remove timeout
	m_server->RemAuthHostsBlacklist(m_client->GetClientName());
	m_socket->SetTimeout(m_server->AutoIdleDisconnectTimeout()*1000);
	vnclog.Print(LL_INTINFO, VNCLOG("authenticated connection\n"));

	// INIT PIXEL FORMAT

	// Get the screen format
	m_client->m_fullscreen = m_client->m_buffer->GetSize();

	// Get the name of this desktop
	char desktopname[MAX_COMPUTERNAME_LENGTH+1];
	DWORD desktopnamelen = MAX_COMPUTERNAME_LENGTH + 1;
	if (GetComputerName(desktopname, &desktopnamelen))
	{
		// Make the name lowercase
		for (size_t x=0; x<strlen(desktopname); x++)
		{
			desktopname[x] = tolower(desktopname[x]);
		}
	}
	else
	{
		strcpy(desktopname, "WinVNC");
	}

	// Send the server format message to the client
	rfbServerInitMsg server_ini;
	server_ini.format = m_client->m_buffer->GetLocalFormat();

	// Endian swaps
	RECT sharedRect;
	sharedRect = m_server->GetSharedRect();
	server_ini.framebufferWidth = Swap16IfLE(sharedRect.right- sharedRect.left);
	server_ini.framebufferHeight = Swap16IfLE(sharedRect.bottom - sharedRect.top);
	server_ini.format.redMax = Swap16IfLE(server_ini.format.redMax);
	server_ini.format.greenMax = Swap16IfLE(server_ini.format.greenMax);
	server_ini.format.blueMax = Swap16IfLE(server_ini.format.blueMax);
	server_ini.nameLength = Swap32IfLE(strlen(desktopname));

	if (!m_socket->SendExact((char *)&server_ini, sizeof(server_ini)))
	{
		m_server->RemoveClient(m_client->GetClientId());
		return;
	}
	if (!m_socket->SendExact(desktopname, strlen(desktopname)))
	{
		m_server->RemoveClient(m_client->GetClientId());
		return;
	}
	vnclog.Print(LL_INTINFO, VNCLOG("sent pixel format to client\n"));

	// Inform the client about our interaction capabilities (protocol 3.7t)
	if (m_client->m_protocol_tightvnc) {
		if (!SendInteractionCaps()) {
			m_server->RemoveClient(m_client->GetClientId());
			return;
		}
		vnclog.Print(LL_INTINFO, VNCLOG("list of interaction capabilities sent\n"));
	}

	// UNLOCK INITIAL SETUP
	// Initial negotiation is complete, so set the protocol ready flag
	{
		omni_mutex_lock l(m_client->m_regionLock);
		m_client->m_protocol_ready = TRUE;
	}

	// Clear the CapsLock and NumLock keys
	if (m_client->IsKeyboardEnabled())
	{
		ClearKeyState(VK_CAPITAL);
		// *** JNW - removed because people complain it's wrong
		//ClearKeyState(VK_NUMLOCK);
		ClearKeyState(VK_SCROLL);
	}

	// MAIN LOOP

	BOOL connected = TRUE;
	while (connected)
	{
		rfbClientToServerMsg msg;

		// Ensure that we're running in the correct desktop
		if (!vncService::InputDesktopSelected())
			if (!vncService::SelectDesktop(NULL))
				break;

		// Try to read a message ID
		if (!m_socket->ReadExact((char *)&msg.type, sizeof(msg.type)))
		{
			connected = FALSE;
			break;
		}

		// What to do is determined by the message id
		switch(msg.type)
		{

		case rfbSetPixelFormat:
			// Read the rest of the message:
			if (!m_socket->ReadExact(((char *) &msg)+1, sz_rfbSetPixelFormatMsg-1))
			{
				connected = FALSE;
				break;
			}

			vnclog.Print(LL_INTINFO, VNCLOG("SetPixelFormat message received\n"));

			// Swap the relevant bits.
			msg.spf.format.redMax = Swap16IfLE(msg.spf.format.redMax);
			msg.spf.format.greenMax = Swap16IfLE(msg.spf.format.greenMax);
			msg.spf.format.blueMax = Swap16IfLE(msg.spf.format.blueMax);

			{
				omni_mutex_lock l(m_client->m_regionLock);

				// Tell the buffer object of the change
				if (!m_client->m_buffer->SetClientFormat(msg.spf.format))
				{
					vnclog.Print(LL_CONNERR, VNCLOG("remote pixel format invalid\n"));

					connected = FALSE;
				}

				// Set the palette-changed flag, just in case...
				m_client->m_palettechanged = TRUE;
			}
			break;

		case rfbSetEncodings:
			// Read the rest of the message:
			if (!m_socket->ReadExact(((char *) &msg)+1, sz_rfbSetEncodingsMsg-1))
			{
				connected = FALSE;
				break;
			}

			vnclog.Print(LL_INTINFO, VNCLOG("SetEncodings message received\n"));

			m_client->m_buffer->SetQualityLevel(-1);
			m_client->m_buffer->SetCompressLevel(6);
			m_client->m_buffer->EnableXCursor(FALSE);
			m_client->m_buffer->EnableRichCursor(FALSE);
			m_client->m_buffer->EnableLastRect(FALSE);
			m_client->m_use_PointerPos = FALSE;
			m_client->m_use_NewFBSize = FALSE;

			m_client->m_cursor_update_pending = FALSE;
			m_client->m_cursor_update_sent = FALSE;
			m_client->m_cursor_pos_changed = FALSE;

			// Read in the preferred encodings
			msg.se.nEncodings = Swap16IfLE(msg.se.nEncodings);
			{
				int x;
				BOOL encoding_set = FALSE;
				BOOL shapeupdates_requested = FALSE;
				BOOL pointerpos_requested = FALSE;

				{
					omni_mutex_lock l(m_client->m_regionLock);
					// By default, don't use copyrect!
					m_client->m_copyrect_use = FALSE;
				}

				for (x = 0; x < msg.se.nEncodings; x++)
				{
					omni_mutex_lock l(m_client->m_regionLock);
					CARD32 encoding;

					// Read an encoding in
					if (!m_socket->ReadExact((char *)&encoding, sizeof(encoding)))
					{
						connected = FALSE;
						break;
					}

					// Is this the CopyRect encoding (a special case)?
					if (Swap32IfLE(encoding) == rfbEncodingCopyRect)
					{
						// Client wants us to use CopyRect
						m_client->m_copyrect_use = TRUE;
						continue;
					}

					// Is this an XCursor encoding request?
					if (Swap32IfLE(encoding) == rfbEncodingXCursor) {
						m_client->m_buffer->EnableXCursor(TRUE);
						shapeupdates_requested = TRUE;
						vnclog.Print(LL_INTINFO, VNCLOG("X-style cursor shape updates enabled\n"));
						continue;
					}

					// Is this a RichCursor encoding request?
					if (Swap32IfLE(encoding) == rfbEncodingRichCursor) {
						m_client->m_buffer->EnableRichCursor(TRUE);
						shapeupdates_requested = TRUE;
						vnclog.Print(LL_INTINFO, VNCLOG("Full-color cursor shape updates enabled\n"));
						continue;
					}

					// Is this a CompressLevel encoding?
					if ((Swap32IfLE(encoding) >= rfbEncodingCompressLevel0) &&
						(Swap32IfLE(encoding) <= rfbEncodingCompressLevel9))
					{
						// Client specified encoding-specific compression level
						int level = (int)(Swap32IfLE(encoding) - rfbEncodingCompressLevel0);
						m_client->m_buffer->SetCompressLevel(level);
						vnclog.Print(LL_INTINFO, VNCLOG("compression level requested: %d\n"), level);
						continue;
					}

					// Is this a QualityLevel encoding?
					if ((Swap32IfLE(encoding) >= rfbEncodingQualityLevel0) &&
						(Swap32IfLE(encoding) <= rfbEncodingQualityLevel9))
					{
						// Client specified image quality level used for JPEG compression
						int level = (int)(Swap32IfLE(encoding) - rfbEncodingQualityLevel0);
						m_client->m_buffer->SetQualityLevel(level);
						vnclog.Print(LL_INTINFO, VNCLOG("image quality level requested: %d\n"), level);
						continue;
					}

					// Is this a PointerPos encoding request?
					if (Swap32IfLE(encoding) == rfbEncodingPointerPos) {
						pointerpos_requested = TRUE;
						continue;
					}

					// Is this a LastRect encoding request?
					if (Swap32IfLE(encoding) == rfbEncodingLastRect) {
						m_client->m_buffer->EnableLastRect(TRUE);
						vnclog.Print(LL_INTINFO, VNCLOG("LastRect protocol extension enabled\n"));
						continue;
					}

					// Is this a NewFBSize encoding request?
					if (Swap32IfLE(encoding) == rfbEncodingNewFBSize) {
						m_client->m_use_NewFBSize = TRUE;
						vnclog.Print(LL_INTINFO, VNCLOG("NewFBSize protocol extension enabled\n"));
						continue;
					}

					// Have we already found a suitable encoding?
					if (!encoding_set)
					{
						// omni_mutex_lock l(m_client->m_regionLock);

						// No, so try the buffer to see if this encoding will work...
						if (m_client->m_buffer->SetEncoding(Swap32IfLE(encoding))) {
							encoding_set = TRUE;
						}

					}
				}

				// Enable CursorPos encoding only if cursor shape updates were
				// requested by the client.
				if (shapeupdates_requested && pointerpos_requested) {
					m_client->m_use_PointerPos = TRUE;
					m_client->SetCursorPosChanged();
					vnclog.Print(LL_INTINFO, VNCLOG("PointerPos protocol extension enabled\n"));
				}

				// If no encoding worked then default to RAW!
				// FIXME: Protocol extensions won't work in this case.
				if (!encoding_set)
				{
					omni_mutex_lock l(m_client->m_regionLock);

					vnclog.Print(LL_INTINFO, VNCLOG("defaulting to raw encoder\n"));

					if (!m_client->m_buffer->SetEncoding(Swap32IfLE(rfbEncodingRaw)))
					{
						vnclog.Print(LL_INTERR, VNCLOG("failed to select raw encoder!\n"));

						connected = FALSE;
					}
				}
			}

			break;

		case rfbFramebufferUpdateRequest:
			// Read the rest of the message:
			if (!m_socket->ReadExact(((char *) &msg)+1, sz_rfbFramebufferUpdateRequestMsg-1))
			{
				connected = FALSE;
				break;
			}

			if (msg.fur.incremental) {
				vnclog.Print(LL_INTINFO, VNCLOG("FramebufferUpdateRequest(incr) received\n"));
			} else {
				vnclog.Print(LL_INTINFO, VNCLOG("FramebufferUpdateRequest(full) received\n"));
			}

			{
				RECT update;
				RECT sharedRect;
				{
					omni_mutex_lock l(m_client->m_regionLock);

					sharedRect = m_server->GetSharedRect();
					// Get the specified rectangle as the region to send updates for.
					update.left = Swap16IfLE(msg.fur.x)+ sharedRect.left;
					update.top = Swap16IfLE(msg.fur.y)+ sharedRect.top;
					update.right = update.left + Swap16IfLE(msg.fur.w);

//					_ASSERTE(Swap16IfLE(msg.fur.x) >= 0);
//					_ASSERTE(Swap16IfLE(msg.fur.y) >= 0);

					//if (update.right > m_client->m_fullscreen.right)
					//	update.right = m_client->m_fullscreen.right;
					if (update.right > sharedRect.right)
						update.right = sharedRect.right;
					if (update.left < sharedRect.left)
						update.left = sharedRect.left;

					update.bottom = update.top + Swap16IfLE(msg.fur.h);
					//if (update.bottom > m_client->m_fullscreen.bottom)
					//	update.bottom = m_client->m_fullscreen.bottom;
					if (update.bottom > sharedRect.bottom)
						update.bottom = sharedRect.bottom;
					if (update.top < sharedRect.top)
						update.top = sharedRect.top;

					// Set the update-wanted flag to true
					m_client->m_updatewanted = TRUE;

					// Clip the rectangle to the screen
					if (IntersectRect(&update, &update, &sharedRect))
					{
						// Is this request for an incremental region?
						if (msg.fur.incremental)
						{
							// Yes, so add it to the incremental region
							m_client->m_incr_rgn.AddRect(update);
						}
						else
						{
							// No, so add it to the full update region
							m_client->m_full_rgn.AddRect(update);

							// Disable any pending CopyRect
							m_client->m_copyrect_set = FALSE;
						}
					}

					// Trigger an update
					m_server->RequestUpdate();
				}
			}
			break;

		case rfbKeyEvent:
			// Read the rest of the message:
			if (m_socket->ReadExact(((char *) &msg)+1, sz_rfbKeyEventMsg-1))
			{
				vnclog.Print(LL_INTINFO, VNCLOG("KeyEvent message received\n"));

				if (m_client->IsKeyboardEnabled() && !m_client->IsInputBlocked())
				{
					msg.ke.key = Swap32IfLE(msg.ke.key);
					// Get the keymapper to do the work
					vncKeymap::keyEvent(msg.ke.key, msg.ke.down != 0,
						m_client->m_server);
					m_client->m_remoteevent = TRUE;
				}
			}
			break;

		case rfbPointerEvent:
			// Read the rest of the message:
			if (m_socket->ReadExact(((char *) &msg)+1, sz_rfbPointerEventMsg-1))
			{
				vnclog.Print(LL_INTINFO, VNCLOG("PointerEvent message received\n"));

				if (m_client->IsPointerEnabled() && !m_client->IsInputBlocked())
				{
					// Convert the coords to Big Endian
					msg.pe.x = Swap16IfLE(msg.pe.x);
					msg.pe.y = Swap16IfLE(msg.pe.y);

					// Remember cursor position for this client
					m_client->m_cursor_pos.x = msg.pe.x;
					m_client->m_cursor_pos.y = msg.pe.y;

					// if we share only one window...

					RECT coord;
					{
						omni_mutex_lock l(m_client->m_regionLock);

						coord = m_server->GetSharedRect();
					}

					// to put position relative to screen
					msg.pe.x = (CARD16)(msg.pe.x + coord.left);
					msg.pe.y = (CARD16)(msg.pe.y + coord.top);

					// Work out the flags for this event
					DWORD flags = MOUSEEVENTF_ABSOLUTE;
					flags |= MOUSEEVENTF_MOVE;
					m_server->SetMouseCounter(1, m_client->m_cursor_pos, false );

					if ( (msg.pe.buttonMask & rfbButton1Mask) != 
						(m_client->m_ptrevent.buttonMask & rfbButton1Mask) )
					{
						if (GetSystemMetrics(SM_SWAPBUTTON))
							flags |= (msg.pe.buttonMask & rfbButton1Mask) 
							? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
						else
							flags |= (msg.pe.buttonMask & rfbButton1Mask) 
							? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
						m_server->SetMouseCounter(1, m_client->m_cursor_pos, false);
					}
					if ( (msg.pe.buttonMask & rfbButton2Mask) != 
						(m_client->m_ptrevent.buttonMask & rfbButton2Mask) )
					{
						flags |= (msg.pe.buttonMask & rfbButton2Mask) 
							? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
						m_server->SetMouseCounter(1, m_client->m_cursor_pos, false);
					}
					if ( (msg.pe.buttonMask & rfbButton3Mask) != 
						(m_client->m_ptrevent.buttonMask & rfbButton3Mask) )
					{
						if (GetSystemMetrics(SM_SWAPBUTTON))
							flags |= (msg.pe.buttonMask & rfbButton3Mask) 
							? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
						else
							flags |= (msg.pe.buttonMask & rfbButton3Mask) 
							? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
						m_server->SetMouseCounter(1, m_client->m_cursor_pos, false);
					}

					// Treat buttons 4 and 5 presses as mouse wheel events
					DWORD wheel_movement = 0;
					if ((msg.pe.buttonMask & rfbButton4Mask) != 0 &&
						(m_client->m_ptrevent.buttonMask & rfbButton4Mask) == 0)
					{
						flags |= MOUSEEVENTF_WHEEL;
						wheel_movement = (DWORD)+120;
					}
					else if ((msg.pe.buttonMask & rfbButton5Mask) != 0 &&
						(m_client->m_ptrevent.buttonMask & rfbButton5Mask) == 0)
					{
						flags |= MOUSEEVENTF_WHEEL;
						wheel_movement = (DWORD)-120;
					}

					// Generate coordinate values
// PRB: should it be really only primary rect?
					HWND temp = GetDesktopWindow();
					GetWindowRect(temp,&coord);

					unsigned long x = (msg.pe.x * 65535) / (coord.right - coord.left - 1);
					unsigned long y = (msg.pe.y * 65535) / (coord.bottom - coord.top - 1);

					// Do the pointer event
					::mouse_event(flags, (DWORD)x, (DWORD)y, wheel_movement, 0);
					// Save the old position
					m_client->m_ptrevent = msg.pe;

					// Flag that a remote event occurred
					m_client->m_remoteevent = TRUE;
					m_client->m_pointer_event_time = time(NULL);

					// Flag that the mouse moved
					// FIXME: It should not set m_cursor_pos_changed here.
					m_client->UpdateMouse();

					// Trigger an update
					m_server->RequestUpdate();
				}
			}
			break;

		case rfbClientCutText:
			// Read the rest of the message:
			if (m_socket->ReadExact(((char *) &msg)+1, sz_rfbClientCutTextMsg-1))
			{
				vnclog.Print(LL_INTINFO, VNCLOG("ClientCutText message received\n"));

				// Allocate storage for the text
				const UINT length = Swap32IfLE(msg.cct.length);
				char *text = new char [length+1];
				if (text == NULL)
					break;
				text[length] = 0;

				// Read in the text
				if (!m_socket->ReadExact(text, length)) {
					delete [] text;
					break;
				}

				// Get the server to update the local clipboard
				if (m_client->IsKeyboardEnabled() && m_client->IsPointerEnabled())
					m_server->UpdateLocalClipText(text);

				// Free the clip text we read
				delete [] text;
			}
			break;

#if 0
		case rfbFileListRequest:
			if (!m_server->FileTransfersEnabled() || !m_client->IsInputEnabled()) {
				connected = FALSE;
				break;
			}
			if (m_socket->ReadExact(((char *) &msg)+1, sz_rfbFileListRequestMsg-1))
			{
				vnclog.Print(LL_INTINFO, VNCLOG("FileListRequest message received\n"));

				msg.flr.dirNameSize = Swap16IfLE(msg.flr.dirNameSize);
				if (msg.flr.dirNameSize > 255) break;
				char path[255 + 1];
				m_socket->ReadExact(path, msg.flr.dirNameSize);
				path[msg.flr.dirNameSize] = '\0';
				ConvertPath(path);
				if (!vncService::tryImpersonate()) {
					omni_mutex_lock l(m_client->m_sendUpdateLock);
					rfbFileListDataMsg fld;
					fld.type = rfbFileListData;
					fld.numFiles = Swap16IfLE(0);
					fld.dataSize = Swap16IfLE(0);
					fld.compressedSize = Swap16IfLE(0);
					fld.flags = msg.flr.flags | 0x80;
					m_socket->SendExact((char *)&fld, sz_rfbFileListDataMsg);
					break;
				}
				FileTransferItemInfo ftii;
				if (strlen(path) == 0) {
					TCHAR szDrivesList[256];
					if (GetLogicalDriveStrings(255, szDrivesList) == 0) {
						omni_mutex_lock l(m_client->m_sendUpdateLock);
						rfbFileListDataMsg fld;
						fld.type = rfbFileListData;
						fld.numFiles = Swap16IfLE(0);
						fld.dataSize = Swap16IfLE(0);
						fld.compressedSize = Swap16IfLE(0);
						fld.flags = msg.flr.flags | 0x80;
						m_socket->SendExact((char *)&fld, sz_rfbFileListDataMsg);
						vncService::undoImpersonate();
						break;
					}
					int i = 0;
					while (szDrivesList[i] != '\0') {
						char *drive = strdup(&szDrivesList[i]);
						char *backslash = strrchr(drive, '\\');
						if (backslash != NULL)
							*backslash = '\0';
						ftii.Add(drive, -1, 0);
						free(drive);
						i += strcspn(&szDrivesList[i], "\0") + 1;
					}
				} else {
					strcat(path, "\\*");
					HANDLE FLRhandle;
					WIN32_FIND_DATA FindFileData;
					UINT savedErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
					FLRhandle = FindFirstFile(path, &FindFileData);
					DWORD LastError = GetLastError();
					SetErrorMode(savedErrorMode);
					if (FLRhandle != INVALID_HANDLE_VALUE) {
						do {
							if (strcmp(FindFileData.cFileName, ".") != 0 &&
								strcmp(FindFileData.cFileName, "..") != 0) {
								LARGE_INTEGER li;
								li.LowPart = FindFileData.ftLastWriteTime.dwLowDateTime;
								li.HighPart = FindFileData.ftLastWriteTime.dwHighDateTime;							
								li.QuadPart = (li.QuadPart - 1164444736000000000) / 10000000;
								if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {	
									ftii.Add(FindFileData.cFileName, -1, 0);
								} else {
									if (!(msg.flr.flags & 0x10))
										ftii.Add(FindFileData.cFileName, FindFileData.nFileSizeLow, li.HighPart);
								}
							}

						} while (FindNextFile(FLRhandle, &FindFileData));
						FindClose(FLRhandle);
					} else {
						if (LastError != ERROR_SUCCESS && LastError != ERROR_FILE_NOT_FOUND) {
							omni_mutex_lock l(m_client->m_sendUpdateLock);

							rfbFileListDataMsg fld;
							fld.type = rfbFileListData;
							fld.numFiles = Swap16IfLE(0);
							fld.dataSize = Swap16IfLE(0);
							fld.compressedSize = Swap16IfLE(0);
							fld.flags = msg.flr.flags | 0x80;
							m_socket->SendExact((char *)&fld, sz_rfbFileListDataMsg);
							vncService::undoImpersonate();
							break;
						}
					}
				}
				int dsSize = ftii.GetNumEntries() * 8;
				int msgLen = sz_rfbFileListDataMsg + dsSize + ftii.GetSummaryNamesLength() + ftii.GetNumEntries();
				char *pAllMessage = new char [msgLen];
				rfbFileListDataMsg *pFLD = (rfbFileListDataMsg *) pAllMessage;
				FTSIZEDATA *pftsd = (FTSIZEDATA *) &pAllMessage[sz_rfbFileListDataMsg];
				char *pFilenames = &pAllMessage[sz_rfbFileListDataMsg + dsSize];
				pFLD->type = rfbFileListData;
				pFLD->flags = msg.flr.flags&0xF0;
				pFLD->numFiles = Swap16IfLE(ftii.GetNumEntries());
				pFLD->dataSize = Swap16IfLE(ftii.GetSummaryNamesLength() + ftii.GetNumEntries());
				pFLD->compressedSize = pFLD->dataSize;
				for (int i = 0; i < ftii.GetNumEntries(); i++) {
					pftsd[i].size = Swap32IfLE(ftii.GetSizeAt(i));
					pftsd[i].data = Swap32IfLE(ftii.GetDataAt(i));
					strcpy(pFilenames, ftii.GetNameAt(i));
					pFilenames = pFilenames + strlen(pFilenames) + 1;
				}
				omni_mutex_lock l(m_client->m_sendUpdateLock);
				m_socket->SendExact(pAllMessage, msgLen);
				vncService::undoImpersonate();
			}
			break;

		case rfbFileDownloadRequest:
			if (!m_server->FileTransfersEnabled() || !m_client->IsInputEnabled()) {
				connected = FALSE;
				break;
			}
			if (m_socket->ReadExact(((char *) &msg)+1, sz_rfbFileDownloadRequestMsg-1))
			{
				vnclog.Print(LL_INTINFO, VNCLOG("FileDownloadRequest message received\n"));

				msg.fdr.fNameSize = Swap16IfLE(msg.fdr.fNameSize);
				msg.fdr.position = Swap32IfLE(msg.fdr.position);

				if (!vncService::tryImpersonate()) {
					m_socket->ReadExact(NULL, msg.fdr.fNameSize);
					char reason[] = "Cannot impersonate logged on user";
					int reasonLen = strlen(reason);
					m_client->SendFileDownloadFailed(reasonLen, reason);
					break;
				}
				if (msg.fdr.fNameSize > 255) {
					m_socket->ReadExact(NULL, msg.fdr.fNameSize);
					char reason[] = "Path length exceeds 255 bytes";
					int reasonLen = strlen(reason);
					m_client->SendFileDownloadFailed(reasonLen, reason);
					vncService::undoImpersonate();
					break;
				}
				char path_file[255 + 1];
				m_socket->ReadExact(path_file, msg.fdr.fNameSize);
				path_file[msg.fdr.fNameSize] = '\0';
				ConvertPath(path_file);
				strcpy(m_client->m_DownloadFilename, path_file);

				HANDLE hFile;
				DWORD sz_rfbFileSize;
				DWORD sz_rfbBlockSize = 8192;
				DWORD dwNumberOfBytesRead = 0;
				DWORD dwNumberOfAllBytesRead = 0;
				WIN32_FIND_DATA FindFileData;
				UINT savedErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
				hFile = FindFirstFile(path_file, &FindFileData);
				DWORD LastError = GetLastError();
				SetErrorMode(savedErrorMode);

				vnclog.Print(LL_CLIENTS, VNCLOG("file download requested: %s\n"),
							 path_file);

				if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || 
					(hFile == INVALID_HANDLE_VALUE) || (path_file[0] == '\0')) {
					FindClose(hFile);
					char reason[] = "Cannot open file, perhaps it is absent or is a directory";
					int reasonLen = strlen(reason);
					m_client->SendFileDownloadFailed(reasonLen, reason);
					vncService::undoImpersonate();
					break;
				}
				sz_rfbFileSize = FindFileData.nFileSizeLow;
				FindClose(hFile);
				m_client->m_modTime = m_client->FiletimeToTime70(FindFileData.ftLastWriteTime);
				if (sz_rfbFileSize == 0) {
					m_client->SendFileDownloadData(m_client->m_modTime);
				} else {
					if (sz_rfbFileSize <= sz_rfbBlockSize) sz_rfbBlockSize = sz_rfbFileSize;
					UINT savedErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
					m_client->m_hFileToRead = CreateFile(path_file, GENERIC_READ, FILE_SHARE_READ, NULL,	OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
					SetErrorMode(savedErrorMode);
					if (m_client->m_hFileToRead != INVALID_HANDLE_VALUE) {
						m_client->m_bDownloadStarted = TRUE;
						m_client->SendFileDownloadPortion();
					}
				}
				vncService::undoImpersonate();
			}
			break;

		case rfbFileUploadRequest:
			if (!m_server->FileTransfersEnabled() || !m_client->IsInputEnabled()) {
				connected = FALSE;
				break;
			}
			if (m_socket->ReadExact(((char *) &msg)+1, sz_rfbFileUploadRequestMsg-1))
			{
				vnclog.Print(LL_INTINFO, VNCLOG("FileUploadRequest message received\n"));

				msg.fupr.fNameSize = Swap16IfLE(msg.fupr.fNameSize);
				msg.fupr.position = Swap32IfLE(msg.fupr.position);

				if (!vncService::tryImpersonate()) {
					m_socket->ReadExact(NULL, msg.fupr.fNameSize);
					char reason[] = "Cannot impersonate logged on user";
					int reasonLen = strlen(reason);
					m_client->SendFileUploadCancel(reasonLen, reason);
					break;
				}
				if (msg.fupr.fNameSize > MAX_PATH) {
					m_socket->ReadExact(NULL, msg.fupr.fNameSize);
					char reason[] = "Path length exceeds MAX_PATH value";
					int reasonLen = strlen(reason);
					m_client->SendFileUploadCancel(reasonLen, reason);
					vncService::undoImpersonate();
					break;
				}
				m_socket->ReadExact(m_client->m_UploadFilename, msg.fupr.fNameSize);
				m_client->m_UploadFilename[msg.fupr.fNameSize] = '\0';
				ConvertPath(m_client->m_UploadFilename);
				vnclog.Print(LL_CLIENTS, VNCLOG("file upload requested: %s\n"),
							 m_client->m_UploadFilename);
				m_client->m_hFileToWrite = CreateFile(m_client->m_UploadFilename, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
				m_client->m_bUploadStarted = TRUE;
				if (m_client->m_hFileToWrite == INVALID_HANDLE_VALUE) {
					char reason[] = "Could not create file";
					int reasonLen = strlen(reason);
					m_client->SendFileUploadCancel(reasonLen, reason);
					vncService::undoImpersonate();
					break;
				}
				/*
				DWORD dwError = GetLastError();
				SYSTEMTIME systime;
				FILETIME filetime;
				GetSystemTime(&systime);
				SystemTimeToFileTime(&systime, &filetime);
				m_client->beginUploadTime = m_client->FiletimeToTime70(filetime);
				*/        
				/*
				DWORD dwFilePtr;
				if (msg.fupr.position > 0) {
					dwFilePtr = SetFilePointer(m_hFiletoWrite, msg.fupr.position, NULL, FILE_BEGIN);
					if ((dwFilePtr == INVALID_SET_FILE_POINTER) && (dwError != NO_ERROR)) {
						char reason[] = "Invalid file pointer position";
						int reasonLen = strlen(reason);
						m_client->SendFileUploadCancel(reasonLen, reason);
						CloseHandle(m_hFiletoWrite);
						break;
					}
				}
				*/
				vncService::undoImpersonate();
			}				
			break;

		case rfbFileUploadData:
			if (!m_server->FileTransfersEnabled() || !m_client->IsInputEnabled()) {
				connected = FALSE;
				break;
			}
			if (m_socket->ReadExact(((char *) &msg)+1, sz_rfbFileUploadDataMsg-1))
			{
				vnclog.Print(LL_INTINFO, VNCLOG("FileUploadData message received\n"));

				msg.fud.realSize = Swap16IfLE(msg.fud.realSize);
				msg.fud.compressedSize = Swap16IfLE(msg.fud.compressedSize);

				if (!vncService::tryImpersonate()) {
					if (msg.fud.realSize == 0 && msg.fud.compressedSize == 0) {
						m_socket->ReadExact(NULL, sizeof(CARD32));
					} else {
						m_socket->ReadExact(NULL, msg.fud.compressedSize);
					}
					char reason[] = "Cannot impersonate logged on user";
					int reasonLen = strlen(reason);
					m_client->SendFileUploadCancel(reasonLen, reason);
					m_client->CloseUndoneFileTransfer();
					break;
				}
				if ((msg.fud.realSize == 0) && (msg.fud.compressedSize == 0)) {
					CARD32 mTime;
					m_socket->ReadExact((char *) &mTime, sizeof(CARD32));
					mTime = Swap32IfLE(mTime);
					FILETIME Filetime;
					m_client->Time70ToFiletime(mTime, &Filetime);
					if (!SetFileTime(m_client->m_hFileToWrite, &Filetime, &Filetime, &Filetime)) {
						vnclog.Print(LL_INTINFO, VNCLOG("SetFileTime() failed\n"));
					}
//					DWORD dwFileSize = GetFileSize(m_client->m_hFileToWrite, NULL);
					CloseHandle(m_client->m_hFileToWrite);
					m_client->m_bUploadStarted = FALSE;
//					SYSTEMTIME systime;
//					FILETIME filetime;
//					GetSystemTime(&systime);
//					SystemTimeToFileTime(&systime, &filetime);
//					m_client->endUploadTime = m_client->FiletimeToTime70(filetime);
//					unsigned int uploadTime = m_client->endUploadTime - m_client->beginUploadTime + 1;
//					DWORD dwBytePerSecond = dwFileSize / uploadTime;
//					vnclog.Print(LL_CLIENTS, VNCLOG("file upload complete: %s; Speed (B/s) = %d; FileSize = %d, UploadTime = %d\n"),
//								 m_client->m_UploadFilename, dwBytePerSecond, dwFileSize, uploadTime);
					vnclog.Print(LL_CLIENTS, VNCLOG("file upload complete: %s;\n"),
								 m_client->m_UploadFilename);
					vncService::undoImpersonate();
					break;
				}
				DWORD dwNumberOfBytesWritten;
				char *pBuff = new char [msg.fud.compressedSize];
				m_socket->ReadExact(pBuff, msg.fud.compressedSize);
				if (msg.fud.compressedLevel != 0) {
					delete[] pBuff;
					char reason[] = "Server does not support data compression on upload";
					int reasonLen = strlen(reason);
					m_client->SendFileUploadCancel(reasonLen, reason);
					m_client->CloseUndoneFileTransfer();
					vncService::undoImpersonate();
					break;
				}
				BOOL bResult = WriteFile(m_client->m_hFileToWrite, pBuff, msg.fud.compressedSize, &dwNumberOfBytesWritten, NULL);
				delete[] pBuff;
				if ((dwNumberOfBytesWritten != msg.fud.compressedSize) || !bResult) {
					char reason[] = "Error writing file data";
					int reasonLen = strlen(reason);
					m_client->SendFileUploadCancel(reasonLen, reason);
					m_client->CloseUndoneFileTransfer();
					vncService::undoImpersonate();
					break;
				}
			}
			break;

		case rfbFileDownloadCancel:
			if (!m_server->FileTransfersEnabled() || !m_client->IsInputEnabled()) {
				connected = FALSE;
				break;
			}
			if (m_socket->ReadExact(((char *) &msg)+1, sz_rfbFileDownloadCancelMsg-1))
			{
				vnclog.Print(LL_INTINFO, VNCLOG("FileDownloadCancel message received\n"));

				vncService::tryImpersonate();
				msg.fdc.reasonLen = Swap16IfLE(msg.fdc.reasonLen);
				char *reason = new char[msg.fdc.reasonLen + 1];
				m_socket->ReadExact(reason, msg.fdc.reasonLen);
				reason[msg.fdc.reasonLen] = '\0';
				m_client->CloseUndoneFileTransfer();
				delete [] reason;
				vncService::undoImpersonate();
			}
			break;

		case rfbFileUploadFailed:
			if (!m_server->FileTransfersEnabled() || !m_client->IsInputEnabled()) {
				connected = FALSE;
				break;
			}
			if (m_socket->ReadExact(((char *) &msg)+1, sz_rfbFileUploadFailedMsg-1))
			{
				vnclog.Print(LL_INTINFO, VNCLOG("FileUploadFailed message received\n"));

				vncService::tryImpersonate();
				msg.fuf.reasonLen = Swap16IfLE(msg.fuf.reasonLen);
				char *reason = new char[msg.fuf.reasonLen + 1];
				m_socket->ReadExact(reason, msg.fuf.reasonLen);
				reason[msg.fuf.reasonLen] = '\0';
				m_client->CloseUndoneFileTransfer();
				delete [] reason;
				vncService::undoImpersonate();
			}
			break;

		case rfbFileCreateDirRequest:
			if (!m_server->FileTransfersEnabled() || !m_client->IsInputEnabled()) {
				connected = FALSE;
				break;
			}
			if (m_socket->ReadExact(((char *) &msg)+1, sz_rfbFileCreateDirRequestMsg-1))
			{
				vnclog.Print(LL_INTINFO, VNCLOG("FileCreateDirRequest message received\n"));

				vncService::tryImpersonate();
				msg.fcdr.dNameLen = Swap16IfLE(msg.fcdr.dNameLen);
				char *dirName = new char[msg.fcdr.dNameLen + 1];
				m_socket->ReadExact(dirName, msg.fcdr.dNameLen);
				dirName[msg.fcdr.dNameLen] = '\0';
				dirName = ConvertPath(dirName);
				CreateDirectory((LPCTSTR) dirName, NULL);
				delete [] dirName;
				vncService::undoImpersonate();
			}

			break;
#endif
		case rfbItalcServiceRequest:
			if( !processItalcClient( vsocketDispatcher, m_socket ) )
			{
				connected = FALSE;
			}
			break;

		default:
			// Unknown message, so fail!
			vnclog.Print(LL_CLIENTS, VNCLOG("invalid message received : %d\n"),
						 (int)msg.type);
			connected = FALSE;
		}
	}

	// Move into the thread's original desktop
	vncService::SelectHDESK(home_desktop);

	// Quit this thread.  This will automatically delete the thread and the
	// associated client.
	vnclog.Print(LL_CLIENTS, VNCLOG("client disconnected : %s (id %hd)\n"),
				 m_client->GetClientName(), m_client->GetClientId());

	// Remove the client from the server, just in case!
	m_server->RemoveClient(m_client->GetClientId());
}

// The vncClient itself

vncClient::vncClient()
{
	vnclog.Print(LL_INTINFO, VNCLOG("vncClient() executing...\n"));

	m_socket = NULL;
	m_client_name = 0;
	m_server_name = 0;
	m_buffer = NULL;

	m_keyboardenabled = FALSE;
	m_pointerenabled = FALSE;
	m_inputblocked = FALSE;

	m_copyrect_use = FALSE;

	m_mousemoved = FALSE;
	m_ptrevent.buttonMask = 0;
	m_ptrevent.x = 0;
	m_ptrevent.y = 0;

	m_cursor_update_pending = FALSE;
	m_cursor_update_sent = FALSE;
	m_cursor_pos_changed = FALSE;
	m_pointer_event_time = (time_t)0;
	m_cursor_pos.x = -1;
	m_cursor_pos.y = -1;

	m_thread = NULL;
	m_updatewanted = FALSE;

	m_palettechanged = FALSE;

	m_copyrect_set = FALSE;

	m_remoteevent = FALSE;

	m_bDownloadStarted = FALSE;
	m_bUploadStarted = FALSE;

	// IMPORTANT: Initially, client is not protocol-ready.
	m_protocol_ready = FALSE;
	m_fb_size_changed = FALSE;

	m_use_NewFBSize = FALSE;

}

vncClient::~vncClient()
{
	vnclog.Print(LL_INTINFO, VNCLOG("~vncClient() executing...\n"));

	// We now know the thread is dead, so we can clean up
	if (m_client_name != 0) {
		free(m_client_name);
		m_client_name = 0;
	}
	if (m_server_name != 0) {
		free(m_server_name);
		m_server_name = 0;
	}

	// If we have a socket then kill it
	if (m_socket != NULL)
	{
		vnclog.Print(LL_INTINFO, VNCLOG("deleting socket\n"));

		delete m_socket;
		m_socket = NULL;
	}

	// Kill the screen buffer
	if (m_buffer != NULL)
	{
		vnclog.Print(LL_INTINFO, VNCLOG("deleting buffer\n"));

		delete m_buffer;
		m_buffer = NULL;
	}
}

// Init
BOOL
vncClient::Init(vncServer *server,
				VSocket *socket,
				BOOL reverse,
				BOOL shared,
				vncClientId newid)
{
	// Save the server id;
	m_server = server;

	// Save the socket
	m_socket = socket;

	// Save the name/ip of the connecting client
	char *name = m_socket->GetPeerName();
	if (name != 0)
		m_client_name = strdup(name);
	else
		m_client_name = strdup("<unknown>");

	// Save the server name/ip
	name = m_socket->GetSockName();
	if (name != 0)
		m_server_name = strdup(name);
	else
		m_server_name = strdup("<unknown>");

	// Save the client id
	m_id = newid;

	// Spawn the child thread here
	m_thread = new vncClientThread;
	if (m_thread == NULL)
		return FALSE;
	return ((vncClientThread *)m_thread)->Init(this, m_server, m_socket, reverse, shared);

	return FALSE;
}

void
vncClient::Kill()
{
	// Close file transfer
	CloseUndoneFileTransfer();

	// Close the socket
	if (m_socket != NULL)
		m_socket->Close();
}

// Client manipulation functions for use by the server
void
vncClient::SetBuffer(vncBuffer *buffer)
{
	// Until authenticated, the client object has no access
	// to the screen buffer.  This means that there only need
	// be a buffer when there's at least one authenticated client.
	m_buffer = buffer;
}


void
vncClient::TriggerUpdate()
{
	// Lock the updates stored so far
	omni_mutex_lock l(m_regionLock);
	if (!m_protocol_ready)
		return;

	if (m_updatewanted)
	{
		// Check if cursor shape update has to be sent
		m_cursor_update_pending = m_buffer->IsCursorUpdatePending();

		// Send an update if one is waiting
		if (!m_changed_rgn.IsEmpty() ||
			!m_full_rgn.IsEmpty() ||
			m_copyrect_set ||
			m_cursor_update_pending ||
			m_cursor_pos_changed ||
			(m_mousemoved && !m_use_PointerPos))
		{
			// Has the palette changed?
			if (m_palettechanged)
			{
				m_palettechanged = FALSE;
				if (!SendPalette())
					return;
			}

			// Now send the update
			m_updatewanted = !SendUpdate();
		}
	}
}

void
vncClient::UpdateMouse()
{
	if (!m_mousemoved && !m_cursor_update_sent)	{
		omni_mutex_lock l(m_regionLock);
		const RECT r = m_server->GetSharedRect();
		if (IntersectRect(&m_oldmousepos, &m_oldmousepos, &r))
			m_changed_rgn.AddRect(m_oldmousepos);

		m_mousemoved = TRUE;
	} else if (m_use_PointerPos) {
		omni_mutex_lock l(m_regionLock);

		SetCursorPosChanged();
	}
}

void
vncClient::UpdateRect(RECT &rect)
{
	// Add the rectangle to the update region
	if (IsRectEmpty(&rect))
		return;

	omni_mutex_lock l(m_regionLock);

	const RECT r = m_server->GetSharedRect();
	if (IntersectRect(&rect, &rect, &r))
		m_changed_rgn.AddRect(rect);
}

void
vncClient::UpdateRegion(vncRegion &region)
{
	// Merge our current update region with the supplied one
	if (region.IsEmpty())
		return;

	{
		omni_mutex_lock l(m_regionLock);

		// Merge the two
		vncRegion dummy;
		dummy.AddRect(m_server->GetSharedRect());
		region.Intersect(dummy);

		m_changed_rgn.Combine(region);
	}
}

void
vncClient::CopyRect(RECT &dest, POINT &source)
{
	// If CopyRect encoding is disabled or we already have a CopyRect pending,
	// then just redraw the region.
	if (!m_copyrect_use || m_copyrect_set) {
		UpdateRect(dest);
		return;
	}

	{
		omni_mutex_lock l(m_regionLock);

		// Clip the destination to the screen
		RECT destrect;
		const RECT r = m_server->GetSharedRect();
		if (!IntersectRect(&destrect, &dest, &r))
			return;

		// Adjust the source correspondingly
		source.x = source.x + (destrect.left - dest.left);
		source.y = source.y + (destrect.top - dest.top);

		// Work out the source rectangle
		RECT srcrect;
		srcrect.left = source.x;
		srcrect.top = source.y;

		// And fill out the right & bottom using the dest rect
		srcrect.right = destrect.right-destrect.left + srcrect.left;
		srcrect.bottom = destrect.bottom-destrect.top + srcrect.top;

		// Clip the source to the screen
		RECT srcrect2;
		if (!IntersectRect(&srcrect2, &srcrect, &r))
			return;

		// Correct the destination rectangle
		destrect.left += (srcrect2.left - srcrect.left);
		destrect.top += (srcrect2.top - srcrect.top);
		destrect.right = srcrect2.right-srcrect2.left + destrect.left;
		destrect.bottom = srcrect2.bottom-srcrect2.top + destrect.top;

		// Set the copyrect...
		m_copyrect_rect = destrect;
		m_copyrect_src.x = srcrect2.left;
		m_copyrect_src.y = srcrect2.top;

		m_copyrect_set = TRUE;
	}
}

void
vncClient::UpdateClipText(LPSTR text)
{
	if (!m_protocol_ready) return;

	// Don't send the clipboard contents to a view-only client
	if (!IsKeyboardEnabled() || !IsPointerEnabled())
		return;

	// Lock out any update sends and send clip text to the client
	omni_mutex_lock l(m_sendUpdateLock);

	rfbServerCutTextMsg message;
	message.length = Swap32IfLE(strlen(text));
	if (!SendRFBMsg(rfbServerCutText, (BYTE *) &message, sizeof(message)))
	{
		Kill();
		return;
	}
	if (!m_socket->SendQueued(text, strlen(text)))
	{
		Kill();
		return;
	}
}

void
vncClient::UpdatePalette()
{
	omni_mutex_lock l(m_regionLock);

	m_palettechanged = TRUE;
}

// Functions used to set and retrieve the client settings
const char*
vncClient::GetClientName()
{
	return (m_client_name != NULL) ? m_client_name : "[unknown]";
}

const char*
vncClient::GetServerName()
{
	return (m_server_name != NULL) ? m_server_name : "[unknown]";
}

// Internal methods
BOOL
vncClient::SendRFBMsg(CARD8 type, BYTE *buffer, int buflen)
{
	// Set the message type
	((rfbServerToClientMsg *)buffer)->type = type;

	// Send the message
	if (!m_socket->SendQueued((char *) buffer, buflen))
	{
		vnclog.Print(LL_CONNERR, VNCLOG("failed to send RFB message to client\n"));

		Kill();
		return FALSE;
	}
	return TRUE;
}


BOOL vncClient::SendUpdate()
{
#ifndef _DEBUG
	try
	{
#endif
		// First, check if we need to send pending NewFBSize message
		if (m_use_NewFBSize && m_fb_size_changed) {
			SetNewFBSize(TRUE);
			return TRUE;
		}

		vncRegion toBeSent;			// Region to actually be sent
		rectlist toBeSentList;		// List of rectangles to actually send
		vncRegion toBeDone;			// Region to check

		// Prepare to send cursor position update if necessary
		if (m_cursor_pos_changed) {
			POINT cursor_pos;
			if (!GetCursorPos(&cursor_pos)) {
				cursor_pos.x = 0;
				cursor_pos.y = 0;
			}
			RECT shared_rect = m_server->GetSharedRect();
			cursor_pos.x -= shared_rect.left;
			cursor_pos.y -= shared_rect.top;
			if (cursor_pos.x < 0) {
				cursor_pos.x = 0;
			} else if (cursor_pos.x >= shared_rect.right - shared_rect.left) {
				cursor_pos.x = shared_rect.right - shared_rect.left - 1;
			}
			if (cursor_pos.y < 0) {
				cursor_pos.y = 0;
			} else if (cursor_pos.y >= shared_rect.bottom - shared_rect.top) {
				cursor_pos.y = shared_rect.bottom - shared_rect.top - 1;
			}
			if (cursor_pos.x == m_cursor_pos.x && cursor_pos.y == m_cursor_pos.y) {
				m_cursor_pos_changed = FALSE;
			} else {
				m_cursor_pos.x = cursor_pos.x;
				m_cursor_pos.y = cursor_pos.y;
			}
		}

		toBeSent.Clear();
		if (!m_full_rgn.IsEmpty()) {
			m_incr_rgn.Clear();
			m_copyrect_set = false;
			toBeSent.Combine(m_full_rgn);
			m_changed_rgn.Clear();
			m_full_rgn.Clear();
		} else {
			if (!m_incr_rgn.IsEmpty()) {
				// Get region to send from vncDesktop
				toBeSent.Combine(m_changed_rgn);

				// Mouse stuff for the case when cursor shape updates are off
				if (!m_cursor_update_sent && !m_cursor_update_pending) {
					// If the mouse hasn't moved, see if its position is in the rect
					// we're sending. If so, make sure the full mouse rect is sent.
					if (!m_mousemoved) {
						vncRegion tmpMouseRgn;
						tmpMouseRgn.AddRect(m_oldmousepos);
						tmpMouseRgn.Intersect(toBeSent);
						if (!tmpMouseRgn.IsEmpty()) 
							m_mousemoved = TRUE;
					}
					// If the mouse has moved (or otherwise needs an update):
					if (m_mousemoved) {
						// Include an update for its previous position
						const RECT r = m_server->GetSharedRect();
						if (IntersectRect(&m_oldmousepos, &m_oldmousepos, &r)) 
							toBeSent.AddRect(m_oldmousepos);
						// Update the cached mouse position
						m_oldmousepos = m_buffer->GrabMouse();
						// Include an update for its current position
						if (IntersectRect(&m_oldmousepos, &m_oldmousepos, &r)) 
							toBeSent.AddRect(m_oldmousepos);
						// Indicate the move has been handled
						m_mousemoved = FALSE;
					}
				}
				m_changed_rgn.Clear();
			}
		}

		// Get the list of changed rectangles!
		int numrects = 0;
		if (toBeSent.Rectangles(toBeSentList))
		{
			// Find out how many rectangles this update will contain
			rectlist::iterator i;
			int numsubrects;
			for (i=toBeSentList.begin(); i != toBeSentList.end(); i++)
			{
				numsubrects = m_buffer->GetNumCodedRects(*i);

				// Skip remaining rectangles if an encoder will use LastRect extension.
				if (numsubrects == 0) {
					numrects = 0xFFFF;
					break;
				}
				numrects += numsubrects;
			}
		}

		if (numrects != 0xFFFF) {
			// Count cursor shape and cursor position updates.
			if (m_cursor_update_pending)
				numrects++;
			if (m_cursor_pos_changed)
				numrects++;
			// Handle the copyrect region
			if (m_copyrect_set)
				numrects++;
			// If there are no rectangles then return
			if (numrects != 0)
				m_incr_rgn.Clear();
			else
				return FALSE;
		}

		omni_mutex_lock l(m_sendUpdateLock);

		// Otherwise, send <number of rectangles> header
		rfbFramebufferUpdateMsg header;
		header.nRects = Swap16IfLE(numrects);
		if (!SendRFBMsg(rfbFramebufferUpdate, (BYTE *) &header, sz_rfbFramebufferUpdateMsg))
			return TRUE;

		// Send mouse cursor shape update
		if (m_cursor_update_pending) {
			if (!SendCursorShapeUpdate())
				return TRUE;
		}

		// Send cursor position update
		if (m_cursor_pos_changed) {
			if (!SendCursorPosUpdate())
				return TRUE;
		}

		// Encode & send the copyrect
		if (m_copyrect_set) {
			m_copyrect_set = FALSE;
			if(!SendCopyRect(m_copyrect_rect, m_copyrect_src))
				return TRUE;
		}

		// Encode & send the actual rectangles
		if (!SendRectangles(toBeSentList))
			return TRUE;

		// Send LastRect marker if needed.
		if (numrects == 0xFFFF) {
			if (!SendLastRect())
				return TRUE;
		}

		// Both lists should be empty when we exit
		_ASSERT(toBeSentList.empty());
#ifndef _DEBUG
	}
	catch (...)
	{
		vnclog.Print(LL_INTERR, VNCLOG("vncClient::SendUpdate caught an exception.\n"));
		throw;
	}
#endif

	return TRUE;
}

// Send a set of rectangles
BOOL
vncClient::SendRectangles(rectlist &rects)
{
	RECT rect;

	// Work through the list of rectangles, sending each one
	while(!rects.empty())
	{
		rect = rects.front();
		if (!SendRectangle(rect))
			return FALSE;

		rects.pop_front();
	}
	rects.clear();

	return TRUE;
}

// Tell the encoder to send a single rectangle
BOOL vncClient::SendRectangle(RECT &rect)
{
	RECT sharedRect;
	{
		omni_mutex_lock l(m_regionLock);
		sharedRect = m_server->GetSharedRect();
	}

	IntersectRect(&rect, &rect, &sharedRect);
	// Get the buffer to encode the rectangle
	UINT bytes = m_buffer->TranslateRect(
		rect,
		m_socket,
		sharedRect.left,
		sharedRect.top);

    // Send the encoded data
    return m_socket->SendQueued((char *)(m_buffer->GetClientBuffer()), bytes);
}

// Send a single CopyRect message
BOOL vncClient::SendCopyRect(RECT &dest, POINT &source)
{
	RECT rc_shr = m_server->GetSharedRect();

	// Create the message header
	rfbFramebufferUpdateRectHeader copyrecthdr;
	copyrecthdr.r.x = Swap16IfLE(dest.left - rc_shr.left);
	copyrecthdr.r.y = Swap16IfLE(dest.top - rc_shr.top);

	copyrecthdr.r.w = Swap16IfLE(dest.right-dest.left);
	copyrecthdr.r.h = Swap16IfLE(dest.bottom-dest.top);
	copyrecthdr.encoding = Swap32IfLE(rfbEncodingCopyRect);

	// Create the CopyRect-specific section
	rfbCopyRect copyrectbody;
	copyrectbody.srcX = Swap16IfLE(source.x - rc_shr.left);
	copyrectbody.srcY = Swap16IfLE(source.y - rc_shr.top);

	// Now send the message;
	if (!m_socket->SendQueued((char *)&copyrecthdr, sizeof(copyrecthdr)))
		return FALSE;
	if (!m_socket->SendQueued((char *)&copyrectbody, sizeof(copyrectbody)))
		return FALSE;

	return TRUE;
}

// Send LastRect marker indicating that there are no more rectangles to send
BOOL
vncClient::SendLastRect()
{
	// Create the message header
	rfbFramebufferUpdateRectHeader hdr;
	hdr.r.x = 0;
	hdr.r.y = 0;
	hdr.r.w = 0;
	hdr.r.h = 0;
	hdr.encoding = Swap32IfLE(rfbEncodingLastRect);

	// Now send the message;
	if (!m_socket->SendQueued((char *)&hdr, sizeof(hdr)))
		return FALSE;

	return TRUE;
}

// Send the encoder-generated palette to the client
// This function only returns FALSE if the SendQueued fails - any other
// error is coped with internally...
BOOL
vncClient::SendPalette()
{
	rfbSetColourMapEntriesMsg setcmap;
	RGBQUAD *rgbquad;
	UINT ncolours = 256;

	// Reserve space for the colour data
	rgbquad = new RGBQUAD[ncolours];
	if (rgbquad == NULL)
		return TRUE;

	// Get the data
	if (!m_buffer->GetRemotePalette(rgbquad, ncolours))
	{
		delete [] rgbquad;
		return TRUE;
	}

	// Compose the message
	omni_mutex_lock l(m_sendUpdateLock);

	setcmap.type = rfbSetColourMapEntries;
	setcmap.firstColour = Swap16IfLE(0);
	setcmap.nColours = Swap16IfLE(ncolours);

	if (!m_socket->SendQueued((char *) &setcmap, sz_rfbSetColourMapEntriesMsg))
	{
		delete [] rgbquad;
		return FALSE;
	}

	// Now send the actual colour data...
	for (UINT i=0; i<ncolours; i++)
	{
		struct _PIXELDATA {
			CARD16 r, g, b;
		} pixeldata;

		pixeldata.r = Swap16IfLE(((CARD16)rgbquad[i].rgbRed) << 8);
		pixeldata.g = Swap16IfLE(((CARD16)rgbquad[i].rgbGreen) << 8);
		pixeldata.b = Swap16IfLE(((CARD16)rgbquad[i].rgbBlue) << 8);

		if (!m_socket->SendQueued((char *) &pixeldata, sizeof(pixeldata)))
		{
			delete [] rgbquad;
			return FALSE;
		}
	}

	// Delete the rgbquad data
	delete [] rgbquad;

	return TRUE;
}

BOOL
vncClient::SendCursorShapeUpdate()
{
	m_cursor_update_pending = FALSE;

	if (!m_buffer->SendCursorShape(m_socket)) {
		m_cursor_update_sent = FALSE;

		return m_buffer->SendEmptyCursorShape(m_socket);
	}

	m_cursor_update_sent = TRUE;
	return TRUE;
}

BOOL
vncClient::SendCursorPosUpdate()
{
	m_cursor_pos_changed = FALSE;

	rfbFramebufferUpdateRectHeader hdr;
	hdr.encoding = Swap32IfLE(rfbEncodingPointerPos);
	hdr.r.x = Swap16IfLE(m_cursor_pos.x);
	hdr.r.y = Swap16IfLE(m_cursor_pos.y);
	hdr.r.w = Swap16IfLE(0);
	hdr.r.h = Swap16IfLE(0);

	return m_socket->SendQueued((char *)&hdr, sizeof(hdr));
}

// Send NewFBSize pseudo-rectangle to notify the client about
// framebuffer size change
BOOL
vncClient::SetNewFBSize(BOOL sendnewfb)
{
	rfbFramebufferUpdateRectHeader hdr;
	RECT sharedRect;

	sharedRect = m_server->GetSharedRect();

	m_full_rgn.Clear();
	m_incr_rgn.Clear();
	m_full_rgn.AddRect(sharedRect);

	if (!m_use_NewFBSize) {
		// We cannot send NewFBSize message right now, maybe later
		m_fb_size_changed = TRUE;

	} else if (sendnewfb) {
		hdr.r.x = 0;
		hdr.r.y = 0;
		hdr.r.w = Swap16IfLE(sharedRect.right - sharedRect.left);
		hdr.r.h = Swap16IfLE(sharedRect.bottom - sharedRect.top);
		hdr.encoding = Swap32IfLE(rfbEncodingNewFBSize);

		rfbFramebufferUpdateMsg header;
		header.nRects = Swap16IfLE(1);
		if (!SendRFBMsg(rfbFramebufferUpdate, (BYTE *)&header,
			sz_rfbFramebufferUpdateMsg))
            return FALSE;

		// Now send the message
		if (!m_socket->SendQueued((char *)&hdr, sizeof(hdr)))
			return FALSE;

		// No pending NewFBSize anymore
		m_fb_size_changed = FALSE;
	}

	return TRUE;
}

void
vncClient::UpdateLocalFormat()
{
	m_buffer->UpdateLocalFormat();
}

char * 
vncClientThread::ConvertPath(char *path)
{
	int len = strlen(path);
	if(len >= 255) return path;
	if((path[0] == '/') && (len == 1)) {path[0] = '\0'; return path;}
	for(int i = 0; i < (len - 1); i++) {
		if(path[i+1] == '/') path[i+1] = '\\';
		path[i] = path[i+1];
	}
	path[len-1] = '\0';
	return path;
}

void 
vncClient::SendFileUploadCancel(unsigned short reasonLen, char *reason)
{
#if 0
	omni_mutex_lock l(m_sendUpdateLock);

	int msgLen = sz_rfbFileUploadCancelMsg + reasonLen;
	char *pAllFUCMessage = new char[msgLen];
	rfbFileUploadCancelMsg *pFUC = (rfbFileUploadCancelMsg *) pAllFUCMessage;
	char *pFollow = &pAllFUCMessage[sz_rfbFileUploadCancelMsg];
	pFUC->type = rfbFileUploadCancel;
	pFUC->reasonLen = Swap16IfLE(reasonLen);
	memcpy(pFollow, reason, reasonLen);
	m_socket->SendExact(pAllFUCMessage, msgLen);
	delete [] pAllFUCMessage;
#endif
}

void 
vncClient::Time70ToFiletime(unsigned int mTime, FILETIME *pFiletime)
{
#if 0
	LONGLONG ll = Int32x32To64(mTime, 10000000) + 116444736000000000;
	pFiletime->dwLowDateTime = (DWORD) ll;
	pFiletime->dwHighDateTime = (DWORD)(ll >> 32);
#endif
}

void 
vncClient::SendFileDownloadFailed(unsigned short reasonLen, char *reason)
{
#if 0
	omni_mutex_lock l(m_sendUpdateLock);

	int msgLen = sz_rfbFileDownloadFailedMsg + reasonLen;
	char *pAllFDFMessage = new char[msgLen];
	rfbFileDownloadFailedMsg *pFDF = (rfbFileDownloadFailedMsg *) pAllFDFMessage;
	char *pFollow = &pAllFDFMessage[sz_rfbFileDownloadFailedMsg];
	pFDF->type = rfbFileDownloadFailed;
	pFDF->reasonLen = Swap16IfLE(reasonLen);
	memcpy(pFollow, reason, reasonLen);
	m_socket->SendExact(pAllFDFMessage, msgLen);
	delete [] pAllFDFMessage;
#endif
}

void 
vncClient::SendFileDownloadData(unsigned int mTime)
{
#if 0
	omni_mutex_lock l(m_sendUpdateLock);

	int msgLen = sz_rfbFileDownloadDataMsg + sizeof(unsigned int);
	char *pAllFDDMessage = new char[msgLen];
	rfbFileDownloadDataMsg *pFDD = (rfbFileDownloadDataMsg *) pAllFDDMessage;
	unsigned int *pFollow = (unsigned int *) &pAllFDDMessage[sz_rfbFileDownloadDataMsg];
	pFDD->type = rfbFileDownloadData;
	pFDD->compressLevel = 0;
	pFDD->compressedSize = Swap16IfLE(0);
	pFDD->realSize = Swap16IfLE(0);
	memcpy(pFollow, &mTime, sizeof(unsigned int));
	m_socket->SendExact(pAllFDDMessage, msgLen);
	delete [] pAllFDDMessage;
#endif
}

void
vncClient::SendFileDownloadPortion()
{
#if 0
	if (!m_bDownloadStarted) return;
	DWORD dwNumberOfBytesRead = 0;
	m_rfbBlockSize = 8192;
	char *pBuff = new char[m_rfbBlockSize];
	BOOL bResult = ReadFile(m_hFileToRead, pBuff, m_rfbBlockSize, &dwNumberOfBytesRead, NULL);
	if ((bResult) && (dwNumberOfBytesRead == 0)) {
		/* This is the end of the file. */
		SendFileDownloadData(m_modTime);
		vnclog.Print(LL_CLIENTS, VNCLOG("file download complete: %s\n"), m_DownloadFilename);
		CloseHandle(m_hFileToRead);
		m_bDownloadStarted = FALSE;
		return;
	}
	SendFileDownloadData((unsigned short)dwNumberOfBytesRead, pBuff);
	delete [] pBuff;
	PostToWinVNC(fileTransferDownloadMessage, (WPARAM) this, (LPARAM) 0);
#endif
}

void 
vncClient::SendFileDownloadData(unsigned short sizeFile, char *pFile)
{
#if 0
	omni_mutex_lock l(m_sendUpdateLock);

	int msgLen = sz_rfbFileDownloadDataMsg + sizeFile;
	char *pAllFDDMessage = new char[msgLen];
	rfbFileDownloadDataMsg *pFDD = (rfbFileDownloadDataMsg *) pAllFDDMessage;
	char *pFollow = &pAllFDDMessage[sz_rfbFileDownloadDataMsg];
	pFDD->type = rfbFileDownloadData;
	pFDD->compressLevel = 0;
	pFDD->compressedSize = Swap16IfLE(sizeFile);
	pFDD->realSize = Swap16IfLE(sizeFile);
	memcpy(pFollow, pFile, sizeFile);
	m_socket->SendExact(pAllFDDMessage, msgLen);
	delete [] pAllFDDMessage;
#endif

}

#if 0
unsigned int 
vncClient::FiletimeToTime70(FILETIME filetime)
{
	LARGE_INTEGER uli;
	uli.LowPart = filetime.dwLowDateTime;
	uli.HighPart = filetime.dwHighDateTime;
	uli.QuadPart = (uli.QuadPart - 116444736000000000) / 10000000;
	return uli.LowPart;
}
#endif

void
vncClient::CloseUndoneFileTransfer()
{
	if (m_bUploadStarted) {
		m_bUploadStarted = FALSE;
		CloseHandle(m_hFileToWrite);
		DeleteFile(m_UploadFilename);
	}
	if (m_bDownloadStarted) {
		m_bDownloadStarted = FALSE;
		CloseHandle(m_hFileToRead);
	}
}
