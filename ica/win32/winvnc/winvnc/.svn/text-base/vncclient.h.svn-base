//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
//  Copyright (C) 2000-2002 Const Kaplinsky. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
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
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.


// vncClient.h

// vncClient class handles the following functions:
// - Recieves requests from the connected client and
//   handles them
// - Handles incoming updates properly, using a vncBuffer
//   object to keep track of screen changes
// It uses a vncBuffer and is passed the vncDesktop and
// vncServer to communicate with.

class vncClient;
typedef SHORT vncClientId;

#if (!defined(_WINVNC_VNCCLIENT))
#define _WINVNC_VNCCLIENT
#pragma warning(disable : 4786)

#include <list>
#include <string>
#include <vector>

typedef std::list<vncClientId> vncClientList;

// Includes
#include "stdhdrs.h"
#include "VSocket.h"
#include <omnithread.h>

// Custom
#include "vncDesktop.h"
#include "rfbRegion.h"
#include "rfbUpdateTracker.h"
#include "vncBuffer.h"
#include "vncEncodeMgr.h"
#include "TextChat.h" // sf@2002 - TextChat
#include "ZipUnZip32/zipUnZip32.h"
//#include "timer.h"

// The vncClient class itself
typedef UINT (WINAPI *pSendinput)(UINT,LPINPUT,INT);
#define SPI_GETMOUSESPEED         0x0070
#define SPI_SETMOUSESPEED         0x0071
#define MOUSEEVENTF_VIRTUALDESK	  0x4000


#define FT_PROTO_VERSION_OLD 1  // <= RC18 server.. "fOldFTPRotocole" version
#define FT_PROTO_VERSION_2   2  // base ft protocol
#define FT_PROTO_VERSION_3   3  // new ft protocol session messages



extern int CheckUserGroupPasswordUni(char * userin,char *password,const char *machine);

using namespace rfb;

class vncClient
{
public:
	// Constructor/destructor
	vncClient();
	~vncClient();

	// Allow the client thread to see inside the client object
	friend class vncClientThread;
	friend class vncClientUpdateThread;

	// Init
	virtual BOOL Init(vncServer *server,
						VSocket *socket,
						BOOL auth,
						BOOL shared,
						vncClientId newid);

	// Kill
	// The server uses this to close the client socket, causing the
	// client thread to fail, which in turn deletes the client object
	virtual void Kill();

	// Client manipulation functions for use by the server
	virtual void SetBuffer(vncBuffer *buffer);

	// Update handling functions
	// These all lock the UpdateLock themselves
	virtual void UpdateMouse();
	virtual void UpdateClipText(const char* text);
	virtual void UpdatePalette();
	virtual void UpdateLocalFormat();

	// Is the client waiting on an update?
	// YES IFF there is an incremental update region,
	//     AND no changed or copied updates intersect it
	virtual BOOL UpdateWanted() {
		omni_mutex_lock l(GetUpdateLock());
		return  !m_incr_rgn.is_empty() &&
			m_incr_rgn.intersect(m_update_tracker.get_changed_region()).is_empty() &&
			m_incr_rgn.intersect(m_update_tracker.get_cached_region()).is_empty() &&
			m_incr_rgn.intersect(m_update_tracker.get_copied_region()).is_empty();

	};

	// Has the client sent an input event?
	virtual BOOL RemoteEventReceived() {
		BOOL result = m_remoteevent;
		m_remoteevent = FALSE;
		return result;
	};

	// The UpdateLock
	// This must be held for a number of routines to be successfully invoked...
	virtual omni_mutex& GetUpdateLock() {return m_encodemgr.GetUpdateLock();};

	// Functions for setting & getting the client settings
	virtual void EnableKeyboard(BOOL enable) {m_keyboardenabled = enable;};
	virtual void EnablePointer(BOOL enable) {m_pointerenabled = enable;};
	virtual void EnableJap(bool enable) {m_jap = enable;};
	virtual void SetCapability(int capability) {m_capability = capability;};

	virtual int GetCapability() {return m_capability;};
	virtual const char *GetClientName();
	virtual vncClientId GetClientId() {return m_id;};

	// Disable/enable protocol messages to the client
	virtual void DisableProtocol();
	virtual void EnableProtocol();
	// resize desktop
	virtual BOOL SetNewSWSize(long w,long h,BOOL desktop);
	virtual void SetSWOffset(int x,int y);
	virtual void SetScreenOffset(int x,int y,int type);

	virtual TextChat* GetTextChatPointer() { return m_pTextChat; }; // sf@2002
	virtual void SetUltraViewer(bool fTrue) { m_fUltraViewer = fTrue;};
	virtual bool IsUltraViewer() { return m_fUltraViewer;};

	virtual void EnableCache(BOOL enabled);


	// sf@2002
	virtual void SetConnectTime(long lTime) {m_lConnectTime = lTime;};
	virtual long GetConnectTime() {return m_lConnectTime;};
	virtual bool IsSlowEncoding() {return m_encodemgr.IsSlowEncoding();};
	virtual bool IsUltraEncoding() {return m_encodemgr.IsUltraEncoding();};
	virtual bool IsFileTransBusy(){return (m_fFileUploadRunning||m_fFileDownloadRunning || m_fFileSessionOpen);};
	void SetProtocolVersion(rfbProtocolVersionMsg *protocolMsg);
	void Clear_Update_Tracker();
	void TriggerUpdate();
	void UpdateCursorShape();

	// sf@2004 - Asynchronous FileTransfer - Delta Transfer
	int  GenerateFileChecksums(HANDLE hFile, char* lpCSBuffer, int nCSBufferSize);
	bool ReceiveDestinationFileChecksums(int nSize, int nLen);
	bool ReceiveFileChunk(int nLen, int nSize);
	void FinishFileReception();
	bool SendFileChunk();
	void FinishFileSending();
	bool GetSpecialFolderPath(int nId, char* szPath);
	int  ZipPossibleDirectory(LPSTR szSrcFileName);
	int  CheckAndZipDirectoryForChecksuming(LPSTR szSrcFileName);
	bool  UnzipPossibleDirectory(LPSTR szFileName);
	bool MyGetFileSize(char* szFilePath, ULARGE_INTEGER* n2FileSize);
	bool DoFTUserImpersonation();
	void UndoFTUserImpersonation();

    // jdp@2008 - File Transfer event hooks
    void FTUploadStartHook();
    void FTUploadCancelledHook();
    void FTUploadFailureHook();
    void FTUploadCompleteHook();

    void FTDownloadStartHook();
    void FTDownloadCancelledHook();
    void FTDownloadFailureHook();
    void FTDownloadCompleteHook();

    void FTNewFolderHook(std::string name);
    void FTDeleteHook(std::string name, bool isDir);
    void FTRenameHook(std::string oldName, std::string newname);
    void SendServerStateUpdate(CARD32 state, CARD32 value);
    void SendKeepAlive(bool bForce = false);
    void SendFTProtocolMsg();
	// sf@2002 
	// Update routines
protected:
	BOOL SendUpdate(rfb::SimpleUpdateTracker &update);
	BOOL SendRFBMsg(CARD8 type, BYTE *buffer, int buflen);
	BOOL SendRectangles(const rfb::RectVector &rects);
	BOOL SendRectangle(const rfb::Rect &rect);
	BOOL SendCopyRect(const rfb::Rect &dest, const rfb::Point &source);
	BOOL SendPalette();
	// CACHE
	BOOL SendCacheRectangles(const rfb::RectVector &rects);
	BOOL SendCacheRect(const rfb::Rect &dest);
	BOOL SendCacheZip(const rfb::RectVector &rects); // sf@2002

	// Tight - CURSOR HANDLING
	BOOL SendCursorShapeUpdate();
	// nyama/marscha - PointerPos
	BOOL SendCursorPosUpdate();
	BOOL SendLastRect(); // Tight

	void TriggerUpdateThread();

	void PollWindow(HWND hwnd);


	// Specialised client-side UpdateTracker
protected:

	// This update tracker stores updates it receives and
	// kicks the client update thread every time one is received

	class ClientUpdateTracker : public rfb::SimpleUpdateTracker {
	public:
		ClientUpdateTracker() : m_client(0) {};
		virtual ~ClientUpdateTracker() {};

		void init(vncClient *client) {m_client=client;}

		virtual void add_changed(const rfb::Region2D &region) {
			{
				// RealVNC 336 change - omni_mutex_lock l(m_client->GetUpdateLock());
				SimpleUpdateTracker::add_changed(region);
				m_client->TriggerUpdateThread();
			}
		}
		virtual void add_cached(const rfb::Region2D &region) {
			{
				// RealVNC 336 change - omni_mutex_lock l(m_client->GetUpdateLock());
				SimpleUpdateTracker::add_cached(region);
				m_client->TriggerUpdateThread();
			}
		}
		
		virtual void add_copied(const rfb::Region2D &dest, const rfb::Point &delta) {
			{
				// RealVNC 336 change - omni_mutex_lock l(m_client->GetUpdateLock());
				SimpleUpdateTracker::add_copied(dest, delta);
				m_client->TriggerUpdateThread();
			}
		}

		virtual void clear() {
			// RealVNC 336 change - omni_mutex_lock l(m_client->GetUpdateLock());
			SimpleUpdateTracker::clear();
		}

		virtual void flush_update(rfb::UpdateInfo &info, const rfb::Region2D &cliprgn) {;
			// RealVNC 336 change - omni_mutex_lock l(m_client->GetUpdateLock());
			SimpleUpdateTracker::flush_update(info, cliprgn);
		}
		virtual void flush_update(rfb::UpdateTracker &to, const rfb::Region2D &cliprgn) {;
			// RealVNC 336 change - omni_mutex_lock l(m_client->GetUpdateLock());
			SimpleUpdateTracker::flush_update(to, cliprgn);
		}

		virtual void get_update(rfb::UpdateInfo &info) const {;
			// RealVNC 336 change - omni_mutex_lock l(m_client->GetUpdateLock());
			SimpleUpdateTracker::get_update(info);
		}
		virtual void get_update(rfb::UpdateTracker &to) const {
			// RealVNC 336 change - omni_mutex_lock l(m_client->GetUpdateLock());
			SimpleUpdateTracker::get_update(to);
		}

		virtual bool is_empty() const{
			// RealVNC 336 change -  omni_mutex_lock l(m_client->GetUpdateLock());
			return SimpleUpdateTracker::is_empty();
		}
	protected:
		vncClient *m_client;
	};

	friend class ClientUpdateTracker;

	// Make the update tracker available externally
public:

	rfb::UpdateTracker &GetUpdateTracker() {return m_update_tracker;};
	int				m_SWOffsetx;
	int				m_SWOffsety;
	int				m_ScreenOffsetx;
	int				m_ScreenOffsety;
	int				m_display_type;
	BOOL			m_NewSWUpdateWaiting;
	rfbProtocolVersionMsg ProtocolVersionMsg;
	//Timer Sendtimer;
	//int roundrobin_counter;
	//int timearray[rfbEncodingZRLE+1][31];
	//int sizearray[rfbEncodingZRLE+1][31];
	//int Totalsend;
	BOOL client_settings_passed;

	// Internal stuffs
protected:
	// Per-client settings
	BOOL			m_IsLoopback;
	BOOL			m_keyboardenabled;
	BOOL			m_pointerenabled;
	bool			m_jap;
	int				m_capability;
	vncClientId		m_id;
	long			m_lConnectTime;

	// Pixel translation & encoding handler
	vncEncodeMgr	m_encodemgr;

	// The server
	vncServer		*m_server;

	// The socket
	VSocket			*m_socket;
	char			*m_client_name;

	// The client thread
	omni_thread		*m_thread;


	// Count to indicate whether updates, clipboards, etc can be sent
	// to the client.  If 0 then OK, otherwise not.
	ULONG			m_disable_protocol;

	// User input information
	rfb::Rect		m_oldmousepos;
	BOOL			m_mousemoved;
	rfbPointerEventMsg	m_ptrevent;
	// vncKeymap		m_keymap;

	// Update tracking structures
	ClientUpdateTracker	m_update_tracker;

	// Client update transmission thread
	vncClientUpdateThread *m_updatethread;

	// Requested update region & requested flag
	rfb::Region2D	m_incr_rgn;

	// Full screen rectangle
//	rfb::Rect		m_fullscreen;

	// When the local display is palettized, it sometimes changes...
	BOOL			m_palettechanged;

	// Information used in polling mode!
	BOOL			m_remoteevent;

	// Clipboard data
	char*			m_clipboard_text;

	//SINGLE WINDOW
	BOOL			m_use_NewSWSize;
	BOOL			m_NewSWDesktop;
	int				NewsizeW;
	int				NewsizeH;

	// CURSOR HANDLING
	BOOL			m_cursor_update_pending;
	BOOL			m_cursor_update_sent;
	// nyama/marscha - PointerPos
	BOOL			m_cursor_pos_changed;
	BOOL			m_use_PointerPos;
	POINT			m_cursor_pos;

	// Modif sf@2002 - FileTransfer 
	BOOL m_fFileTransferRunning;
	CZipUnZip32		*m_pZipUnZip;

	char  m_szFullDestName[MAX_PATH + 64];
	char  m_szFileTime[18];
	char* m_pBuff;
	char* m_pCompBuff;
	HANDLE m_hDestFile;
	DWORD m_dwFileSize; 
	DWORD m_dwNbPackets;
	DWORD m_dwNbReceivedPackets;
	DWORD m_dwNbBytesWritten;
	DWORD m_dwTotalNbBytesWritten;
	bool m_fFileDownloadError;
	bool m_fFileDownloadRunning;
    bool m_fFileSessionOpen;

    // 8 April 2008 jdp
    bool m_fUserAbortedFileTransfer;
	char m_szSrcFileName[MAX_PATH + 64]; // Path + timestring
	HANDLE m_hSrcFile;
	bool m_fEof;
	DWORD m_dwNbBytesRead;
	DWORD m_dwTotalNbBytesRead;
	int m_nPacketCount;
	bool m_fCompressionEnabled;
	bool m_fFileUploadError;
	bool m_fFileUploadRunning;

	// sf@2004 - Delta Transfer
	char*	m_lpCSBuffer;
	int		m_nCSOffset;
	int		m_nCSBufferSize;

	// Modif sf@2002 - Scaling
	rfb::Rect		m_ScaledScreen;
	UINT			m_nScale;
	bool			fNewScale;
	bool			m_fPalmVNCScaling;
	bool			fFTRequest;

	// sf@2002 - DSM Plugin Rects alignement buffer
	BYTE* m_pNetRectBuf;
	bool m_fReadFromNetRectBuf;  // 
	bool m_fWriteToNetRectBuf;
	int m_nNetRectBufOffset;
	int m_nReadSize;
	int m_nNetRectBufSize;

	// sf@2002 
	BYTE* m_pCacheZipBuf;
	unsigned long m_nCacheZipBufSize;
	BYTE* m_pRawCacheZipBuf;
	unsigned long m_nRawCacheZipBufSize;

	friend class TextChat; 
	TextChat *m_pTextChat;	// Modif sf@2002 - Text Chat

	bool m_fUltraViewer; // sf@2002 

	// sf@2005 - FTUserImpersonation
	bool m_fFTUserImpersonatedOk;
	DWORD m_lLastFTUserImpersonationTime;

	//stats
	int totalraw;

	pSendinput Sendinput;
	// Modif cs@2005
#ifdef DSHOW
	HANDLE m_hmtxEncodeAccess;
#endif

    std::string m_OrigSourceDirectoryName;
    bool        m_wants_ServerStateUpdates;
    bool        m_bClientHasBlockedInput;
    bool        m_wants_KeepAlive;
};

#endif
