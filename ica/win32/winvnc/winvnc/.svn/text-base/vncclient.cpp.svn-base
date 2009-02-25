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
#include "vncServer.h"
#include "vncClient.h"
#include "VSocket.h"
#include "vncDesktop.h"
#include "rfbRegion.h"
#include "vncBuffer.h"
#include "vncService.h"
#include "vncPasswd.h"
#include "vncAcceptDialog.h"
#include "vncKeymap.h"
#include "vncMenu.h"

#include "rfb/dh.h"
#include "vncAuth.h"

#ifdef IPP
#include "ipp_zlib/zlib.h"
#else
#include "zlib/zlib.h"
#endif
#include "mmSystem.h" // sf@2002
#include "sys/types.h"
#include "sys/stat.h"

#include <string>
#include <shlobj.h>
#include "vncOSVersion.h"
#include "common/win32_helpers.h"

bool isDirectoryTransfer(const char *szFileName);

// take a full path & file name, split it, prepend prefix to filename, then merge it back
static std::string make_temp_filename(const char *szFullPath)
{
    // don't add prefix for directory transfers.
    if (isDirectoryTransfer(szFullPath))
        return szFullPath;
        
    std::string tmpName(szFullPath);
    std::string::size_type pos = tmpName.rfind('\\');
    if (pos != std::string::npos)
        tmpName.insert(pos + 1, rfbPartialFilePrefix);

    return tmpName;
}

std::string get_real_filename(std::string name)
{
    std::string::size_type pos;

    pos = name.find(rfbPartialFilePrefix);
    if (pos != std::string::npos)
        name.erase(pos, sz_rfbPartialFilePrefix);
 
    return name;
}


// #include "rfb.h"
bool DeleteFileOrDirectory(TCHAR *srcpath)
{
    TCHAR path[MAX_PATH + 1]; // room for extra null; SHFileOperation requires double null terminator
    memset(path, 0, sizeof path);
    
    _tcsncpy(path, srcpath, MAX_PATH);

    SHFILEOPSTRUCT op;
    memset(&op, 0, sizeof(SHFILEOPSTRUCT));
    op.wFunc = FO_DELETE;
    op.pFrom  = path;
    op.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
    
    int result = SHFileOperation(&op);
    // MSDN says to not look at the error code, just treat 0 as SUCCESS, nonzero is failure.
    // Do not use GetLastError with the return values of this function.

    return result == 0;
}

bool replaceFile(const char *src, const char *dst)
{
    DWORD dwFileAttribs;
    bool status;
    
    dwFileAttribs = GetFileAttributes(dst);
    // make the file read/write if it's read only.
    if (dwFileAttribs != INVALID_FILE_ATTRIBUTES && dwFileAttribs & FILE_ATTRIBUTE_READONLY)
        SetFileAttributes(dst, dwFileAttribs & ~FILE_ATTRIBUTE_READONLY);

    if (OSversion() == 3)
    {
        status = ::CopyFile(src, dst, false) ? true : false;
        if (status)
            ::DeleteFile(src);
    }
    else
        status = ::MoveFileEx(src, dst, MOVEFILE_REPLACE_EXISTING) ? true : false;

    // restore orginal file attributes, if we have them. We won't have them if
    // the destination file didn't exist prior to the copy/move.
    if (dwFileAttribs != INVALID_FILE_ATTRIBUTES)
        SetFileAttributes(dst, dwFileAttribs);

    return status;
}
#include "localization.h" // Act : add localization on messages
typedef BOOL (WINAPI *PGETDISKFREESPACEEX)(LPCSTR,PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);

DWORD GetExplorerLogonPid();
unsigned long updates_sent;

// vncClient update thread class

//	[v1.0.2-jp1 fix]
//	yak!'s File transfer patch
//	Simply forward strchr() and strrchr() to _mbschr() and _mbsrchr() to avoid 0x5c problem, respectively.
//	Probably, it is better to write forward functions internally.
#include <mbstring.h>
#define strchr(a, b) reinterpret_cast<char*>(_mbschr(reinterpret_cast<unsigned char*>(a), b))
#define strrchr(a, b) reinterpret_cast<char*>(_mbsrchr(reinterpret_cast<unsigned char*>(a), b))


// 31 January 2008 jdp
std::string AddDirPrefixAndSuffix(const char *name)
{
    std::string dirname(rfbDirPrefix);
    dirname += name;
    dirname += rfbDirSuffix;

    return dirname;
}

bool isDirectory(const char *name)
{
    struct _stat statbuf;

    _stat(name, &statbuf);
    return (statbuf.st_mode & _S_IFDIR) == _S_IFDIR;
}
bool isDirectoryTransfer(const char *szFileName)
{
    return (strncmp(strrchr(const_cast<char*>(szFileName), '\\') + 1, rfbZipDirectoryPrefix, strlen(rfbZipDirectoryPrefix)) == 0);
}

void GetZippedFolderPathName(const char *szZipFile, char *path)
{
    // input lookes like: "c:\temp\!UVNCDIR-folder.zip"
    // output should look like "c:\temp\folder"

    const char *zip_ext = ".zip";
    std::string folder_path(szZipFile);
    std::string::size_type pos;

    // remove "!UVNCDIR-"
    pos = folder_path.find(rfbZipDirectoryPrefix);
    if (pos != std::string::npos)
        folder_path.replace(pos, strlen(rfbZipDirectoryPrefix),"");

    // remove ".zip"
    pos = folder_path.rfind(zip_ext);
    if (pos != std::string::npos)
        folder_path.replace(pos, strlen(zip_ext), "");

    // remove "[ "
    pos = folder_path.find(rfbDirPrefix);
    if (pos != std::string::npos)
        folder_path.replace(pos, strlen(rfbDirPrefix),"");

    // remove " ]"
    pos = folder_path.rfind(rfbDirSuffix);
    if (pos != std::string::npos)
        folder_path.replace(pos, strlen(rfbDirSuffix),"");
    
    std::copy(folder_path.begin(), folder_path.end(), path);
    path[folder_path.size()] = 0; // terminate the string    
}

void SplitTransferredFileNameAndDate(char *szFileAndDate, char *filetime)
{
    char *p = strrchr(szFileAndDate, ',');
    if (p == NULL)
    {
        if (filetime)
            *filetime = '\0';
    }
    else
    {
        if (filetime)
            strcpy(filetime, p+1);
        *p = '\0';
    }
}


/*
 * File transfer event hooks
 *
 * The following functions are called from various points in the file transfer
 * process. They are notification hooks; the hook function cannot affect the
 * file operation. One possible use of these hooks is to implement auditing
 * of file operations on the server side.
 *
 * Note that uploads are transfers from the server to the viewer and a
 * download is a transfer from the viewer to the server.
 *
 * The hook will have to use the state stored in the vncClient object
 * to figure out what was done in most cases. For those cases where it
 * is not possible, the hook is passed enough information about the event.
 *  
 */

// called at the successful start of a file upload
void vncClient::FTUploadStartHook()
{
}

// called when an abort is received from the viewer. it is assumed that it was
// due to the user cancelling the transfer
void vncClient::FTUploadCancelledHook()
{
}

// Called when an upload fails for any reason: network failure, out of disk space, insufficient privileges etc.
void vncClient::FTUploadFailureHook()
{
}

// Called after the file is successfully uploaded. At this point the file transfer is complete.
void vncClient::FTUploadCompleteHook()
{
}

// called at the successful start of a file download
void vncClient::FTDownloadStartHook()
{
}

// called when an abort is received from the viewer. it is assumed that it was
// due to the user cancelling the transfer
void vncClient::FTDownloadCancelledHook()
{
}

// Called when an dowload fails for any reason: network failure, out of disk space, insufficient privileges etc.
void vncClient::FTDownloadFailureHook()
{
}

// Called after the file is successfully downloaded. At this point the file transfer is complete.
void vncClient::FTDownloadCompleteHook()
{
}

// Called when a new folder is created.
void vncClient::FTNewFolderHook(std::string name)
{
}

// called when a folder is created
void vncClient::FTDeleteHook(std::string name, bool isDir)
{
}

// called when a file or folder is renamed.
void vncClient::FTRenameHook(std::string oldName, std::string newname)
{
}



class vncClientUpdateThread : public omni_thread
{
public:

	// Init
	BOOL Init(vncClient *client);

	// Kick the thread to send an update
	void Trigger();

	// Kill the thread
	void Kill();

	// Disable/enable updates
	void EnableUpdates(BOOL enable);

	void get_time_now(unsigned long* abs_sec, unsigned long* abs_nsec);

	// The main thread function
    virtual void *run_undetached(void *arg);

protected:
	virtual ~vncClientUpdateThread();

	// Fields
protected:
	vncClient *m_client;
	omni_condition *m_signal;
	omni_condition *m_sync_sig;
	BOOL m_active;
	BOOL m_enable;
};

// Modif cs@2005
#ifdef DSHOW
class MutexAutoLock 
{
public:
	MutexAutoLock(HANDLE* phMutex) 
	{ 
		m_phMutex = phMutex;

		if(WAIT_OBJECT_0 != WaitForSingleObject(*phMutex, INFINITE))
		{
			vnclog.Print(LL_INTERR, VNCLOG("Could not get access to the mutex\n"));
		}
	}
	~MutexAutoLock() 
	{ 
		ReleaseMutex(*m_phMutex);
	}

	HANDLE* m_phMutex;
};
#endif

BOOL
vncClientUpdateThread::Init(vncClient *client)
{
	vnclog.Print(LL_INTINFO, VNCLOG("init update thread\n"));

	m_client = client;
	omni_mutex_lock l(m_client->GetUpdateLock());
	m_signal = new omni_condition(&m_client->GetUpdateLock());
	m_sync_sig = new omni_condition(&m_client->GetUpdateLock());
	m_active = TRUE;
	m_enable = m_client->m_disable_protocol == 0;
	if (m_signal && m_sync_sig) {
		start_undetached();
		return TRUE;
	}
	return FALSE;
}

vncClientUpdateThread::~vncClientUpdateThread()
{
	if (m_signal) delete m_signal;
	if (m_sync_sig) delete m_sync_sig;
	vnclog.Print(LL_INTINFO, VNCLOG("update thread gone\n"));
	m_client->m_updatethread=NULL;
}

void
vncClientUpdateThread::Trigger()
{
	// ALWAYS lock client UpdateLock before calling this!
	// Only trigger an update if protocol is enabled
	if (m_client->m_disable_protocol == 0) {
		m_signal->signal();
	}
}

void
vncClientUpdateThread::Kill()
{
	vnclog.Print(LL_INTINFO, VNCLOG("kill update thread\n"));

	omni_mutex_lock l(m_client->GetUpdateLock());
	m_active=FALSE;
	m_signal->signal();
}


void
vncClientUpdateThread::get_time_now(unsigned long* abs_sec, unsigned long* abs_nsec)
{
    static int days_in_preceding_months[12]
	= { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    static int days_in_preceding_months_leap[12]
	= { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };

    SYSTEMTIME st;

    GetSystemTime(&st);
    *abs_nsec = st.wMilliseconds * 1000000;

    // this formula should work until 1st March 2100

    DWORD days = ((st.wYear - 1970) * 365 + (st.wYear - 1969) / 4
		  + ((st.wYear % 4)
		     ? days_in_preceding_months[st.wMonth - 1]
		     : days_in_preceding_months_leap[st.wMonth - 1])
		  + st.wDay - 1);

    *abs_sec = st.wSecond + 60 * (st.wMinute + 60 * (st.wHour + 24 * days));
}
void
vncClientUpdateThread::EnableUpdates(BOOL enable)
{
	// ALWAYS call this with the UpdateLock held!
	if (enable) {
		vnclog.Print(LL_INTINFO, VNCLOG("enable update thread\n"));
	} else {
		vnclog.Print(LL_INTINFO, VNCLOG("disable update thread\n"));
	}

	m_enable = enable;
	m_signal->signal();
	unsigned long now_sec, now_nsec;
    get_time_now(&now_sec, &now_nsec);
	m_sync_sig->wait();
	/*if  (m_sync_sig->timedwait(now_sec+1,0)==0)
		{
//			m_signal->signal();
			vnclog.Print(LL_INTINFO, VNCLOG("thread timeout\n"));
		} */
	vnclog.Print(LL_INTINFO, VNCLOG("enable/disable synced\n"));
}

void*
vncClientUpdateThread::run_undetached(void *arg)
{
	rfb::SimpleUpdateTracker update;
	rfb::Region2D clipregion;
	char *clipboard_text = 0;
	update.enable_copyrect(true);
	BOOL send_palette = FALSE;

	updates_sent=0;

	vnclog.Print(LL_INTINFO, VNCLOG("starting update thread\n"));

	// Set client update threads to high priority
	// *** set_priority(omni_thread::PRIORITY_HIGH);

	while (1)
	{
		// Block waiting for an update to send
		{
			omni_mutex_lock l(m_client->GetUpdateLock());
			m_client->m_incr_rgn = m_client->m_incr_rgn.union_(clipregion);

			// We block as long as updates are disabled, or the client
			// isn't interested in them, unless this thread is killed.
			if (updates_sent < 1) 
			{
			while (m_active && (
						!m_enable || (
							m_client->m_update_tracker.get_changed_region().intersect(m_client->m_incr_rgn).is_empty() &&
							m_client->m_update_tracker.get_copied_region().intersect(m_client->m_incr_rgn).is_empty() &&
							m_client->m_update_tracker.get_cached_region().intersect(m_client->m_incr_rgn).is_empty() &&
							!m_client->m_clipboard_text &&
							!m_client->m_cursor_pos_changed // nyama/marscha - PointerPos
							)
						)
					) {
				// Issue the synchronisation signal, to tell other threads
				// where we have got to
				m_sync_sig->broadcast();

				// Wait to be kicked into action
				m_signal->wait();
			}
			}
			else
			{
				while (m_active && (
						!m_enable || (
							m_client->m_update_tracker.get_changed_region().intersect(m_client->m_incr_rgn).is_empty() &&
							m_client->m_update_tracker.get_copied_region().intersect(m_client->m_incr_rgn).is_empty() &&
							m_client->m_update_tracker.get_cached_region().intersect(m_client->m_incr_rgn).is_empty() &&
							!m_client->m_encodemgr.IsCursorUpdatePending() &&
							!m_client->m_clipboard_text &&
							!m_client->m_NewSWUpdateWaiting &&
							!m_client->m_cursor_pos_changed // nyama/marscha - PointerPos
							)
						)
					) {
				// Issue the synchronisation signal, to tell other threads
				// where we have got to
				m_sync_sig->broadcast();

				// Wait to be kicked into action
				m_signal->wait();
			}
			}
			// If the thread is being killed then quit
			if (!m_active) break;

			// SEND AN UPDATE!
			// The thread is active, updates are enabled, and the
			// client is expecting an update - let's see if there
			// is anything to send.

			// Modif sf@2002 - FileTransfer - Don't send anything if a file transfer is running !
			// if (m_client->m_fFileTransferRunning) return 0;
			// if (m_client->m_pTextChat->m_fTextChatRunning) return 0;

			// sf@2002
			// New scale requested, we do it before sending the next Update
			if (m_client->fNewScale)
			{
				// Send the new framebuffer size to the client
				rfb::Rect ViewerSize = m_client->m_encodemgr.m_buffer->GetViewerSize();
				
				// Copyright (C) 2001 - Harakan software
				if (m_client->m_fPalmVNCScaling)
				{
					rfb::Rect ScreenSize = m_client->m_encodemgr.m_buffer->GetSize();
					rfbPalmVNCReSizeFrameBufferMsg rsfb;
					rsfb.type = rfbPalmVNCReSizeFrameBuffer;
					rsfb.desktop_w = Swap16IfLE(ScreenSize.br.x);
					rsfb.desktop_h = Swap16IfLE(ScreenSize.br.y);
					rsfb.buffer_w = Swap16IfLE(ViewerSize.br.x);
					rsfb.buffer_h = Swap16IfLE(ViewerSize.br.y);
					m_client->m_socket->SendExact((char*)&rsfb,
													sz_rfbPalmVNCReSizeFrameBufferMsg,
													rfbPalmVNCReSizeFrameBuffer);
				}
				else // eSVNC-UltraVNC Scaling
				{
					rfbResizeFrameBufferMsg rsmsg;
					rsmsg.type = rfbResizeFrameBuffer;
					rsmsg.framebufferWidth  = Swap16IfLE(ViewerSize.br.x);
					rsmsg.framebufferHeigth = Swap16IfLE(ViewerSize.br.y);
					m_client->m_socket->SendExact((char*)&rsmsg,
													sz_rfbResizeFrameBufferMsg,
													rfbResizeFrameBuffer);
				}

				m_client->m_encodemgr.m_buffer->ClearCache();
				m_client->fNewScale = false;
				m_client->m_fPalmVNCScaling = false;

				// return 0;
			}

			// Has the palette changed?
			send_palette = m_client->m_palettechanged;
			m_client->m_palettechanged = FALSE;

			// Fetch the incremental region
			clipregion = m_client->m_incr_rgn;
			m_client->m_incr_rgn.clear();

			// Get the clipboard data, if any
			if (m_client->m_clipboard_text) {
				clipboard_text = m_client->m_clipboard_text;
				m_client->m_clipboard_text = 0;
			}
		
			// Get the update details from the update tracker
			m_client->m_update_tracker.flush_update(update, clipregion);

		//if (!m_client->m_encodemgr.m_buffer->m_desktop->IsVideoDriverEnabled())
		//TEST if (!m_client->m_encodemgr.m_buffer->m_desktop->m_hookdriver)
		{
			// Render the mouse if required

		if (updates_sent > 1 ) m_client->m_cursor_update_pending = m_client->m_encodemgr.WasCursorUpdatePending();
		if (updates_sent == 1 ) m_client->m_cursor_update_pending = true;

		if (!m_client->m_cursor_update_sent && !m_client->m_cursor_update_pending) 
			{
				if (m_client->m_mousemoved)
				{
					// Re-render its old location
					m_client->m_oldmousepos =
						m_client->m_oldmousepos.intersect(m_client->m_ScaledScreen); // sf@2002
					if (!m_client->m_oldmousepos.is_empty())
						update.add_changed(m_client->m_oldmousepos);

					// And render its new one
					m_client->m_encodemgr.m_buffer->GetMousePos(m_client->m_oldmousepos);
					m_client->m_oldmousepos =
						m_client->m_oldmousepos.intersect(m_client->m_ScaledScreen);
					if (!m_client->m_oldmousepos.is_empty())
						update.add_changed(m_client->m_oldmousepos);
	
					m_client->m_mousemoved = FALSE;
				}
			}
		}
		
		}

		// SEND THE CLIPBOARD
		// If there is clipboard text to be sent then send it
		// Also allow in loopbackmode
		// Loopback mode with winvncviewer will cause a loping
		// But ssh is back working
        if (clipboard_text && !m_client->m_fFileSessionOpen){
			rfbServerCutTextMsg message;

			message.length = Swap32IfLE(strlen(clipboard_text));
			if (!m_client->SendRFBMsg(rfbServerCutText,
				(BYTE *) &message, sizeof(message))) {
				m_client->m_socket->Close();
			}
			if (!m_client->m_socket->SendExact(clipboard_text, strlen(clipboard_text)))
			{
				m_client->m_socket->Close();
			}
			free(clipboard_text);
			clipboard_text = 0;
		}

		// SEND AN UPDATE
		// We do this without holding locks, to avoid network problems
		// stalling the server.

		// Update the client palette if necessary

		if (send_palette) {
			m_client->SendPalette();
		}

		// Send updates to the client - this implicitly clears
		// the supplied update tracker
		if (m_client->SendUpdate(update)) {
			updates_sent++;
			clipregion.clear();
		}

		yield();
	}

	vnclog.Print(LL_INTINFO, VNCLOG("stopping update thread\n"));
	
	vnclog.Print(LL_INTERR, "client sent %lu updates\n", updates_sent);
	return 0;
}

// vncClient thread class

class vncClientThread : public omni_thread
{
public:

	// Init
	virtual BOOL Init(vncClient *client,
		vncServer *server,
		VSocket *socket,
		BOOL auth,
		BOOL shared);

	// Sub-Init routines
	virtual BOOL InitVersion();
	virtual BOOL InitAuthenticate();
	virtual BOOL AuthMsLogon();

	// The main thread function
	virtual void run(void *arg);

protected:
	virtual ~vncClientThread();

	// Fields
protected:
	VSocket *m_socket;
	vncServer *m_server;
	vncClient *m_client;
	BOOL m_auth;
	BOOL m_shared;
	BOOL m_ms_logon;
};

vncClientThread::~vncClientThread()
{
	// If we have a client object then delete it
	if (m_client != NULL)
		delete m_client;
}

BOOL
vncClientThread::Init(vncClient *client, vncServer *server, VSocket *socket, BOOL auth, BOOL shared)
{
	// Save the server pointer and window handle
	m_server = server;
	m_socket = socket;
	m_client = client;
	m_auth = auth;
	m_shared = shared;

	// Start the thread
	start();

	return TRUE;
}

BOOL
vncClientThread::InitVersion()
{
	rfbProtocolVersionMsg protocol_ver;
	protocol_ver[12] = 0;
	if (strcmp(m_client->ProtocolVersionMsg,"0.0.0.0")==NULL)
	{
		// Generate the server's protocol version
		rfbProtocolVersionMsg protocolMsg;
		sprintf((char *)protocolMsg,
				rfbProtocolVersionFormat,
				rfbProtocolMajorVersion,
				rfbProtocolMinorVersion + (m_server->MSLogonRequired() ? 0 : 2)); // 4: mslogon+FT,
																			  // 6: VNClogon+FT
		// Send the protocol message
		//m_socket->SetTimeout(0); // sf@2006 - Trying to fix neverending authentication bug - Not sure it's a good idea...
		if (!m_socket->SendExact((char *)&protocolMsg, sz_rfbProtocolVersionMsg))
			return FALSE;

		// Now, get the client's protocol version
		if (!m_socket->ReadExact((char *)&protocol_ver, sz_rfbProtocolVersionMsg))
			return FALSE;
	}
	else 
		memcpy(protocol_ver,m_client->ProtocolVersionMsg, sz_rfbProtocolVersionMsg);

	// sf@2006 - Trying to fix neverending authentication bug - Check if this is RFB protocole
	if (strncmp(protocol_ver,"RFB", 3)!=NULL)
		return FALSE;

	// Check viewer's the protocol version
	int major, minor;
	sscanf((char *)&protocol_ver, rfbProtocolVersionFormat, &major, &minor);
	if (major != rfbProtocolMajorVersion)
		return FALSE;

	// TODO: Maybe change this UltraVNC specific minor value because
	// TightVNC viewer uses minor = 5 ...
	// For now:
	// UltraViewer always sends minor = 4 (sf@2005: or 6, as it returns the minor version received from the server)
	// UltraServer sends minor = 4 or minor = 6
	// m_ms_logon = false; // For all non-UltraVNC logon compatible viewers
	m_ms_logon = m_server->MSLogonRequired();
	vnclog.Print(LL_INTINFO, VNCLOG("m_ms_logon set to %s"), m_ms_logon ? "true" : "false");
	if (minor == 4 || minor == 6) m_client->SetUltraViewer(true); else m_client->SetUltraViewer(false); // sf@2005 - Fix Open TextChat from server bug 

	return TRUE;
}

BOOL
vncClientThread::InitAuthenticate()
{
	vnclog.Print(LL_INTINFO, "Entered InitAuthenticate");
	// Retrieve the local password
	char password[MAXPWLEN];
	char passwordMs[MAXMSPWLEN];
	m_server->GetPassword(password);
	vncPasswd::ToText plain(password);

	// Verify the peer host name against the AuthHosts string
	vncServer::AcceptQueryReject verified;
	if (m_auth) {
		verified = vncServer::aqrAccept;
	} else {
		verified = m_server->VerifyHost(m_socket->GetPeerName());
	}
	
	// If necessary, query the connection with a timed dialog
	char username[UNLEN+1];
	if (!vncService::CurrentUser(username, sizeof(username))) return false;
	if ((strcmp(username, "") != 0) || m_server->QueryIfNoLogon()) // marscha@2006 - Is AcceptDialog required even if no user is logged on
    {
		if (verified == vncServer::aqrQuery) {
            // 10 Dec 2008 jdp reject/accept all incoming connections if the workstation is locked
            if (vncService::IsWSLocked() && !m_server->QueryIfNoLogon()) {
                verified = m_server->QueryAccept() == 1 ? vncServer::aqrAccept : verified = vncServer::aqrReject;
            } else {

			vncAcceptDialog *acceptDlg = new vncAcceptDialog(m_server->QueryTimeout(),m_server->QueryAccept(), m_socket->GetPeerName());
	
			if (acceptDlg == NULL) 
				{
					if (m_server->QueryAccept()==1) 
					{
						verified = vncServer::aqrAccept;
					}
					else 
					{
						verified = vncServer::aqrReject;
					}
				}
			else 
				{
						HDESK desktop;
						desktop = OpenInputDesktop(0, FALSE,
													DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
													DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
													DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
													DESKTOP_SWITCHDESKTOP | GENERIC_WRITE
													);

						HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
						
					
						SetThreadDesktop(desktop);

						if ( !(acceptDlg->DoDialog()) ) verified = vncServer::aqrReject;

						CloseDesktop(desktop);
						SetThreadDesktop(old_desktop);
				}
            }
		}
    }

	if (verified == vncServer::aqrReject) {
		CARD32 auth_val = Swap32IfLE(rfbConnFailed);
		char *errmsg = "Your connection has been rejected.";
		CARD32 errlen = Swap32IfLE(strlen(errmsg));
		if (!m_socket->SendExact((char *)&auth_val, sizeof(auth_val)))
			return FALSE;
		if (!m_socket->SendExact((char *)&errlen, sizeof(errlen)))
			return FALSE;
		m_socket->SendExact(errmsg, strlen(errmsg));
		return FALSE;
	}

	// By default we disallow passwordless workstations!
	if ((strlen(plain) == 0) && m_server->AuthRequired())
	{
		vnclog.Print(LL_CONNERR, VNCLOG("no password specified for server - client rejected\n"));

		// Send an error message to the client
		CARD32 auth_val = Swap32IfLE(rfbConnFailed);
		char *errmsg =
			"This server does not have a valid password enabled.  "
			"Until a password is set, incoming connections cannot be accepted.";
		CARD32 errlen = Swap32IfLE(strlen(errmsg));

		if (!m_socket->SendExact((char *)&auth_val, sizeof(auth_val)))
			return FALSE;
		if (!m_socket->SendExact((char *)&errlen, sizeof(errlen)))
			return FALSE;
		m_socket->SendExact(errmsg, strlen(errmsg));

		return FALSE;
	}

	// By default we filter out local loop connections, because they're pointless
	if (!m_server->LoopbackOk())
	{
		char *localname = _strdup(m_socket->GetSockName());
		char *remotename = _strdup(m_socket->GetPeerName());

		// Check that the local & remote names are different!
		if ((localname != NULL) && (remotename != NULL))
		{
			BOOL ok = _stricmp(localname, remotename) != 0;

			if (localname != NULL)
				free(localname);

			if (remotename != NULL)
				free(remotename);

			if (!ok)
			{
				vnclog.Print(LL_CONNERR, VNCLOG("loopback connection attempted - client rejected\n"));
				
				// Send an error message to the client
				CARD32 auth_val = Swap32IfLE(rfbConnFailed);
				char *errmsg = "Local loop-back connections are disabled.";
				CARD32 errlen = Swap32IfLE(strlen(errmsg));

				if (!m_socket->SendExact((char *)&auth_val, sizeof(auth_val)))
					return FALSE;
				if (!m_socket->SendExact((char *)&errlen, sizeof(errlen)))
					return FALSE;
				m_socket->SendExact(errmsg, strlen(errmsg));

				return FALSE;
			}
		}
	}
	else
	{
		char *localname = _strdup(m_socket->GetSockName());
		char *remotename = _strdup(m_socket->GetPeerName());

		// Check that the local & remote names are different!
		if ((localname != NULL) && (remotename != NULL))
		{
			BOOL ok = _stricmp(localname, remotename) != 0;

			if (localname != NULL)
				free(localname);

			if (remotename != NULL)
				free(remotename);

			if (!ok)
			{
				vnclog.Print(LL_CONNERR, VNCLOG("loopback connection attempted - client accepted\n"));
				m_client->m_IsLoopback=true;
			}
		}

	}

	// Authenticate the connection, if required
	if (m_auth || (strlen(plain) == 0))
	{
		// Send no-auth-required message
		CARD32 auth_val = Swap32IfLE(rfbNoAuth);
		if (!m_socket->SendExact((char *)&auth_val, sizeof(auth_val)))
			return FALSE;
	}
	else if (m_ms_logon) /*(mslogon && !oldmslogon) TODO*/
	{
		if (!AuthMsLogon())
			return FALSE;
	}
	else
	{
		// Send auth-required message
		CARD32 auth_val = Swap32IfLE(rfbVncAuth);
		if (!m_socket->SendExact((char *)&auth_val, sizeof(auth_val)))
			return FALSE;

		BOOL auth_ok = TRUE;
		{
			/*
			// sf@2002 - DSMPlugin
			// Use Plugin only from this point for the moment
			if (m_server->GetDSMPluginPointer()->IsEnabled())
			{
				m_socket->EnableUsePlugin(true);
				// TODO: Make a more secured challenge (with time stamp)
			}
			*/

			// Now create a 16-byte challenge
			char challenge[16];
			char challengems[64];
			char response[16];
			char responsems[64];
			char *plainmsPasswd;
			char user[256];
			char domain[256];
			vncRandomBytes((BYTE *)&challenge);
			vncRandomBytesMs((BYTE *)&challengems);

			// Send the challenge to the client
			// m_ms_logon = false;
			if (m_ms_logon)
			{
				vnclog.Print(LL_INTINFO, "MS-Logon authentication");
				if (!m_socket->SendExact(challengems, sizeof(challengems)))
					return FALSE;
				if (!m_socket->SendExact(challenge, sizeof(challenge)))
						return FALSE;
				if (!m_socket->ReadExact(user, sizeof(char)*256))
					return FALSE;
				if (!m_socket->ReadExact(domain, sizeof(char)*256))
					return FALSE;
				// Read the response
				if (!m_socket->ReadExact(responsems, sizeof(responsems)))
					return FALSE;
				if (!m_socket->ReadExact(response, sizeof(response)))
						return FALSE;
				
				// TODO: Improve this... 
				for (int i = 0; i < 32 ;i++)
				{
					passwordMs[i] = challengems[i]^responsems[i];
				}

				// REM: Instead of the fixedkey, we could use VNC password
				// -> the user needs to enter the VNC password on viewer's side.
				plainmsPasswd = vncDecryptPasswdMs((char *)passwordMs);

				// We let it as is right now for testing purpose.
				if (strlen(user) == 0 && !m_server->MSLogonRequired())
				{
					vnclog.Print(LL_INTINFO, "No user specified, mslogon not required");
					vncEncryptBytes((BYTE *)&challenge, plain);
	
					// Compare them to the response
					for (int i=0; i<sizeof(challenge); i++)
					{
						if (challenge[i] != response[i])
						{
							auth_ok = FALSE;
							break;
						}
					}
				}
				else 
				{
					vnclog.Print(LL_INTINFO, "User specified or mslogon required");
					int result=CheckUserGroupPasswordUni(user,plainmsPasswd,m_client->GetClientName());
					vnclog.Print(LL_INTINFO, "CheckUserGroupPasswordUni result=%i", result);
					if (result==0) auth_ok = FALSE;
					if (result==2)
					{
						m_client->EnableKeyboard(false);
						m_client->EnablePointer(false);
					}
				}
				if (plainmsPasswd) free(plainmsPasswd);
				plainmsPasswd=NULL;
			}
			else
			{
				vnclog.Print(LL_INTINFO, "password authentication");
				if (!m_socket->SendExact(challenge, sizeof(challenge)))
                {
					vnclog.Print(LL_SOCKERR, VNCLOG("Failed to send challenge to client\n"));
					return FALSE;
                }


				// Read the response
				if (!m_socket->ReadExact(response, sizeof(response)))
                {
					vnclog.Print(LL_SOCKERR, VNCLOG("Failed to receive challenge response from client\n"));
					return FALSE;
                }
				// Encrypt the challenge bytes
				vncEncryptBytes((BYTE *)&challenge, plain);

				// Compare them to the response
				for (int i=0; i<sizeof(challenge); i++)
				{
					if (challenge[i] != response[i])
					{
						auth_ok = FALSE;
						break;
					}
				}
			}
		}

		// Did the authentication work?
		CARD32 authmsg;
		if (!auth_ok)
		{
			vnclog.Print(LL_CONNERR, VNCLOG("authentication failed\n"));
			//////////////////
			// LOG it also in the event
			//////////////////
			{
				typedef BOOL (*LogeventFn)(char *machine);
				LogeventFn Logevent = 0;
				char szCurrentDir[MAX_PATH];
				if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
					{
						char* p = strrchr(szCurrentDir, '\\');
						*p = '\0';
						strcat (szCurrentDir,"\\logging.dll");
					}
				HMODULE hModule = LoadLibrary(szCurrentDir);
				if (hModule)
					{
						BOOL result=false;
						Logevent = (LogeventFn) GetProcAddress( hModule, "LOGFAILED" );
						Logevent((char *)m_client->GetClientName());
						FreeLibrary(hModule);
					}
			}

			authmsg = Swap32IfLE(rfbVncAuthFailed);
			m_socket->SendExact((char *)&authmsg, sizeof(authmsg));
			return FALSE;
		}
		else
		{
			// Tell the client we're ok
			authmsg = Swap32IfLE(rfbVncAuthOK);
			//////////////////
			// LOG it also in the event
			//////////////////
			if (!m_ms_logon){
				typedef BOOL (*LogeventFn)(char *machine);
				LogeventFn Logevent = 0;
				char szCurrentDir[MAX_PATH];
				if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
					{
						char* p = strrchr(szCurrentDir, '\\');
						*p = '\0';
						strcat (szCurrentDir,"\\logging.dll");
					}
				HMODULE hModule = LoadLibrary(szCurrentDir);
				if (hModule)
					{
						BOOL result=false;
						Logevent = (LogeventFn) GetProcAddress( hModule, "LOGLOGON" );
						Logevent((char *)m_client->GetClientName());
						FreeLibrary(hModule);
					}
			}
			if (!m_socket->SendExact((char *)&authmsg, sizeof(authmsg)))
				return FALSE;
		}
	}

	// Read the client's initialisation message
	rfbClientInitMsg client_ini;
	if (!m_socket->ReadExact((char *)&client_ini, sz_rfbClientInitMsg))
		return FALSE;

	// If the client wishes to have exclusive access then remove other clients
	if (m_server->ConnectPriority()==3 && !m_shared )
	{
		// Existing
			if (m_server->AuthClientCount() > 0)
			{
				vnclog.Print(LL_CLIENTS, VNCLOG("connections already exist - client rejected\n"));
				return FALSE;
			}
	}
	if (!client_ini.shared && !m_shared)
	{
		// Which client takes priority, existing or incoming?
		if (m_server->ConnectPriority() < 1)
		{
			// Incoming
			vnclog.Print(LL_INTINFO, VNCLOG("non-shared connection - disconnecting old clients\n"));
			m_server->KillAuthClients();
		} else if (m_server->ConnectPriority() > 1)
		{
			// Existing
			if (m_server->AuthClientCount() > 0)
			{
				vnclog.Print(LL_CLIENTS, VNCLOG("connections already exist - client rejected\n"));
				return FALSE;
			}
		}
	}

	vnclog.Print(LL_CLIENTS, VNCLOG("Leaving InitAuthenticate\n"));
	// Tell the server that this client is ok
	return m_server->Authenticated(m_client->GetClientId());
}

// marscha@2006: Try to better hide the windows password.
// I know that this is no breakthrough in modern cryptography.
// It's just a patch/kludge/workaround.
BOOL 
vncClientThread::AuthMsLogon() {
	// Send MS-Logon-required message
	vnclog.Print(LL_INTINFO, "MS-Logon (DH) authentication");
	CARD32 auth_val = Swap32IfLE(rfbMsLogon);
	if (!m_socket->SendExact((char *)&auth_val, sizeof(auth_val)))
		return FALSE;

	DH dh;
	char gen[8], mod[8], pub[8], resp[8];
	char user[256], passwd[64];
	unsigned char key[8];
	
	dh.createKeys();
	int64ToBytes(dh.getValue(DH_GEN), gen);
	int64ToBytes(dh.getValue(DH_MOD), mod);
	int64ToBytes(dh.createInterKey(), pub);
		
	if (!m_socket->SendExact(gen, sizeof(gen))) return FALSE;
	if (!m_socket->SendExact(mod, sizeof(mod))) return FALSE;
	if (!m_socket->SendExact(pub, sizeof(pub))) return FALSE;
	if (!m_socket->ReadExact(resp, sizeof(resp))) return FALSE;
	if (!m_socket->ReadExact(user, sizeof(user))) return FALSE;
	if (!m_socket->ReadExact(passwd, sizeof(passwd))) return FALSE;

	int64ToBytes(dh.createEncryptionKey(bytesToInt64(resp)), (char*) key);
	vnclog.Print(0, "After DH: g=%I64u, m=%I64u, i=%I64u, key=%I64u\n", bytesToInt64(gen), bytesToInt64(mod), bytesToInt64(pub), bytesToInt64((char*) key));
	vncDecryptBytes((unsigned char*) user, sizeof(user), key); user[255] = '\0';
	vncDecryptBytes((unsigned char*) passwd, sizeof(passwd), key); passwd[63] = '\0';

	int result = CheckUserGroupPasswordUni(user, passwd, m_client->GetClientName());
	vnclog.Print(LL_INTINFO, "CheckUserGroupPasswordUni result=%i\n", result);
	if (result == 2) {
		m_client->EnableKeyboard(false);
		m_client->EnablePointer(false);
	}

	CARD32 authmsg = Swap32IfLE(result ? rfbVncAuthOK : rfbVncAuthFailed);
	if (!m_socket->SendExact((char *)&authmsg, sizeof(authmsg)))
		return FALSE;
		
	if (!result) {
		vnclog.Print(LL_CONNERR, VNCLOG("authentication failed\n"));
		return FALSE;
	}
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


// Modif sf@2002
// Get the local ip addresses as a human-readable string.
// If more than one, then with \n between them.
// If not available, then gets a message to that effect.
void GetIPString(char *buffer, int buflen)
{
    char namebuf[256];

    if (gethostname(namebuf, 256) != 0)
	{
		strncpy(buffer, "Host name unavailable", buflen);
		return;
    }

    HOSTENT *ph = gethostbyname(namebuf);
    if (!ph)
	{
		strncpy(buffer, "IP address unavailable", buflen);
		return;
    }

    *buffer = '\0';
    char digtxt[5];
    for (int i = 0; ph->h_addr_list[i]; i++)
	{
    	for (int j = 0; j < ph->h_length; j++)
		{
	    sprintf(digtxt, "%d.", (unsigned char) ph->h_addr_list[i][j]);
	    strncat(buffer, digtxt, (buflen-1)-strlen(buffer));
		}	
		buffer[strlen(buffer)-1] = '\0';
		if (ph->h_addr_list[i+1] != 0)
			strncat(buffer, ", ", (buflen-1)-strlen(buffer));
    }
}



void
vncClientThread::run(void *arg)
{
	// All this thread does is go into a socket-receive loop,
	// waiting for stuff on the given socket

	// IMPORTANT : ALWAYS call RemoveClient on the server before quitting
	// this thread.

	vnclog.Print(LL_CLIENTS, VNCLOG("client connected : %s (%hd)\n"),
								m_client->GetClientName(),
								m_client->GetClientId());
	// Save the handle to the thread's original desktop
	HDESK home_desktop = GetThreadDesktop(GetCurrentThreadId());
	
	// To avoid people connecting and then halting the connection, set a timeout
	if (!m_socket->SetTimeout(30000))
		vnclog.Print(LL_INTERR, VNCLOG("failed to set socket timeout(%d)\n"), GetLastError());

	// sf@2002 - DSM Plugin - Tell the client's socket where to find the DSMPlugin 
	if (m_server->GetDSMPluginPointer() != NULL)
	{
		m_socket->SetDSMPluginPointer(m_server->GetDSMPluginPointer());
		vnclog.Print(LL_INTINFO, VNCLOG("DSMPlugin Pointer to socket OK\n"));
	}
	else
	{
		vnclog.Print(LL_INTINFO, VNCLOG("Invalid DSMPlugin Pointer\n"));
		return;
	}

	// Initially blacklist the client so that excess connections from it get dropped
	m_server->AddAuthHostsBlacklist(m_client->GetClientName());

	// LOCK INITIAL SETUP
	// All clients have the m_protocol_ready flag set to FALSE initially, to prevent
	// updates and suchlike interfering with the initial protocol negotiations.

	// sf@2002 - DSMPlugin
	// Use Plugin only from this point (now BEFORE Protocole handshaking)
	if (m_server->GetDSMPluginPointer()->IsEnabled())
	{

		// sf@2007 - Current DSM code does not support multithreading
		// Data mix is causing server crash and viewer drops when more than one viewer is copnnected at a time
		// We must reject any new viewer connection BEFORE any data passes through the plugin
		// This is a dirty workaround. We ignore all Multi Viewer connection settings...
		if (m_server->AuthClientCount() > 0)
		{
			vnclog.Print(LL_CLIENTS, VNCLOG("A connection using DSM already exist - client rejected to avoid crash \n"));
			return;
		} 

		if (m_server->AuthClientCount() == 0)
			m_server->GetDSMPluginPointer()->ResetPlugin();	//SEC reset if needed
		m_socket->EnableUsePlugin(true);
		m_client->m_encodemgr.EnableQueuing(false);
		// TODO: Make a more secured challenge (with time stamp)
	}
	else
		m_client->m_encodemgr.EnableQueuing(true);

	// GET PROTOCOL VERSION
	if (!InitVersion())
	{
		m_server->RemoveClient(m_client->GetClientId());
		
		// wa@2005 - AutoReconnection attempt if required
		if (m_server->AutoReconnect())
			vncService::PostAddNewClient(1111, 1111);

		return;
	}
	vnclog.Print(LL_INTINFO, VNCLOG("negotiated version\n"));

	// AUTHENTICATE LINK
	if (!InitAuthenticate())
	{
		m_server->RemoveClient(m_client->GetClientId());
		return;
	}

	// Authenticated OK - remove from blacklist and remove timeout
	m_server->RemAuthHostsBlacklist(m_client->GetClientName());
	m_socket->SetTimeout(m_server->AutoIdleDisconnectTimeout()*1000);
	vnclog.Print(LL_INTINFO, VNCLOG("authenticated connection\n"));

	// Set Client Connect time
	m_client->SetConnectTime(timeGetTime());

	// INIT PIXEL FORMAT

	// Get the screen format
//	m_client->m_fullscreen = m_client->m_encodemgr.GetSize();

	// Modif sf@2002 - Scaling
	{
	omni_mutex_lock l(m_client->GetUpdateLock());
	m_client->m_encodemgr.m_buffer->SetScale(m_server->GetDefaultScale()); // v1.1.2
	}
	m_client->m_ScaledScreen = m_client->m_encodemgr.m_buffer->GetViewerSize();
	m_client->m_nScale = m_client->m_encodemgr.m_buffer->GetScale();


	// Get the name of this desktop
	// sf@2002 - v1.1.x - Complete the computer name with the IP address if necessary
	bool fIP = false;
    char desktopname[MAX_COMPUTERNAME_LENGTH + 1 + 256] = {0};
	DWORD desktopnamelen = MAX_COMPUTERNAME_LENGTH + 1 + 256;
	memset((char*)desktopname, 0, sizeof(desktopname));
	if (GetComputerName(desktopname, &desktopnamelen))
	{
		// Make the name lowercase
		for (size_t x=0; x<strlen(desktopname); x++)
		{
			desktopname[x] = tolower(desktopname[x]);
		}
		// Check for the presence of "." in the string (then it's presumably an IP adr)
		if (strchr(desktopname, '.') != NULL) fIP = true;
	}
	else
	{
		strcpy(desktopname, "WinVNC");
	}

	// We add the IP address(es) to the computer name, if possible and necessary
	if (!fIP)
	{
		char szIP[256];
		GetIPString(szIP, sizeof(szIP));
		if (strlen(szIP) > 3 && szIP[0] != 'I' && szIP[0] != 'H') 
		{
			strcat(desktopname, " ( ");
			strcat(desktopname, szIP);
			strcat(desktopname, " )");
		}
	}

	// Send the server format message to the client
	rfbServerInitMsg server_ini;
	server_ini.format = m_client->m_encodemgr.m_buffer->GetLocalFormat();

	// Endian swaps
	// Modif sf@2002 - Scaling
	server_ini.framebufferWidth = Swap16IfLE(m_client->m_ScaledScreen.br.x - m_client->m_ScaledScreen.tl.x);
	server_ini.framebufferHeight = Swap16IfLE(m_client->m_ScaledScreen.br.y - m_client->m_ScaledScreen.tl.y);
	// server_ini.framebufferWidth = Swap16IfLE(m_client->m_fullscreen.br.x-m_client->m_fullscreen.tl.x);
	// server_ini.framebufferHeight = Swap16IfLE(m_client->m_fullscreen.br.y-m_client->m_fullscreen.tl.y);

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

	// UNLOCK INITIAL SETUP
	// Initial negotiation is complete, so set the protocol ready flag
	m_client->EnableProtocol();
	
	// Add a fullscreen update to the client's update list
	// sf@2002 - Scaling
	// m_client->m_update_tracker.add_changed(m_client->m_fullscreen);
	{ // RealVNC 336
		omni_mutex_lock l(m_client->GetUpdateLock());
		m_client->m_update_tracker.add_changed(m_client->m_ScaledScreen);
	}

	// MAIN LOOP

	// Set the input thread to a high priority
	set_priority(omni_thread::PRIORITY_HIGH);

	BOOL connected = TRUE;
	// added jeff
	BOOL need_to_disable_input = m_server->LocalInputsDisabled();
    bool need_to_clear_keyboard = true;
    bool need_first_keepalive = false;
    bool need_ft_version_msg =  false;

	while (connected)
	{
		rfbClientToServerMsg msg;

		// Ensure that we're running in the correct desktop
		if (!m_client->IsFileTransBusy())
		if (!vncService::InputDesktopSelected()) 
			if (!vncService::SelectDesktop(NULL)) 
					break;
		// added jeff
        // 2 May 2008 jdp paquette@atnetsend.net moved so that we're on the right desktop  when we're a service
	    // Clear the CapsLock and NumLock keys
	    if (m_client->m_keyboardenabled && need_to_clear_keyboard)
	    {
		    ClearKeyState(VK_CAPITAL);
		    // *** JNW - removed because people complain it's wrong
		    //ClearKeyState(VK_NUMLOCK);
		    ClearKeyState(VK_SCROLL);
            need_to_clear_keyboard = false;
	    }
        //
        if (need_to_disable_input)
        {
            // have to do this here if we're a service so that we're on the correct desktop
            m_client->m_encodemgr.m_buffer->m_desktop->SetDisableInput(m_server->LocalInputsDisabled());
            need_to_disable_input = false;
        }

        // reclaim input block after local C+A+D if user currently has it blocked 
        m_client->m_encodemgr.m_buffer->m_desktop->block_input(m_client->m_encodemgr.m_buffer->m_desktop->GetBlockInputState());

        if (need_first_keepalive)
        {
            // send first keepalive to let the client know we accepted the encoding request
            m_client->SendServerStateUpdate(rfbKeepAliveInterval, m_server->GetKeepAliveInterval());
            m_client->SendKeepAlive();
            need_first_keepalive = false;
        }
        if (need_ft_version_msg)
        {
            // send a ft protocol message to client.
            m_client->SendFTProtocolMsg();
            need_ft_version_msg = false;
        }
		// sf@2002 - v1.1.2
		int nTO = 1; // Type offset
		// If DSM Plugin, we must read all the transformed incoming rfb messages (type included)
		if (m_socket->IsUsePluginEnabled() && m_server->GetDSMPluginPointer()->IsEnabled())
		{
			if (!m_socket->ReadExact((char *)&msg.type, sizeof(msg.type)))
			{
				connected = FALSE;
				break;
			}
		    nTO = 0;
		}
		else
		{
			// Try to read a message ID
			if (!m_socket->ReadExact((char *)&msg.type, sizeof(msg.type)))
			{
				connected = FALSE;
				break;
			}
		}

		// What to do is determined by the message id
		switch(msg.type)
		{

        case rfbKeepAlive:
            // nothing else to read.
            // NO-OP
#if defined(_DEBUG)
                    {
                        static time_t lastrecv = 0;
                        time_t now = time(&now);
                        time_t delta = now - lastrecv;
                        lastrecv = now;
                        char msg[255];
                        sprintf(msg, "keepalive received %u seconds since last one\n", delta);
                        OutputDebugString(msg);

                    }
#endif
            if (sz_rfbKeepAliveMsg > 1)
            {
			    if (!m_socket->ReadExact(((char *) &msg)+nTO, sz_rfbKeepAliveMsg-nTO))
			    {
				    connected = FALSE;
				    break;
			    }
            }
            break;

		case rfbSetPixelFormat:
			// Read the rest of the message:
			if (!m_socket->ReadExact(((char *) &msg)+nTO, sz_rfbSetPixelFormatMsg-nTO))
			{
				connected = FALSE;
				break;
			}

			// Swap the relevant bits.
			msg.spf.format.redMax = Swap16IfLE(msg.spf.format.redMax);
			msg.spf.format.greenMax = Swap16IfLE(msg.spf.format.greenMax);
			msg.spf.format.blueMax = Swap16IfLE(msg.spf.format.blueMax);

			// sf@2005 - Additional param for Grey Scale transformation
			m_client->m_encodemgr.EnableGreyPalette((msg.spf.format.pad1 == 1));
			
			// Prevent updates while the pixel format is changed
			m_client->DisableProtocol();
				
			// Tell the buffer object of the change			
			if (!m_client->m_encodemgr.SetClientFormat(msg.spf.format))
			{
				vnclog.Print(LL_CONNERR, VNCLOG("remote pixel format invalid\n"));

				connected = FALSE;
			}

			// Set the palette-changed flag, just in case...
			m_client->m_palettechanged = TRUE;

			// Re-enable updates
			m_client->EnableProtocol();
			
			break;

		case rfbSetEncodings:
			// Read the rest of the message:
			if (!m_socket->ReadExact(((char *) &msg)+nTO, sz_rfbSetEncodingsMsg-nTO))
			{
				connected = FALSE;
				break;
			}

			// RDV cache
			m_client->m_encodemgr.EnableCache(FALSE);

	        // RDV XOR and client detection
			m_client->m_encodemgr.AvailableXOR(FALSE);
			m_client->m_encodemgr.AvailableZRLE(FALSE);
			m_client->m_encodemgr.AvailableTight(FALSE);

			// sf@2002 - Tight
			m_client->m_encodemgr.SetQualityLevel(-1);
			m_client->m_encodemgr.SetCompressLevel(6);
			m_client->m_encodemgr.EnableLastRect(FALSE);

			// Tight - CURSOR HANDLING
			m_client->m_encodemgr.EnableXCursor(FALSE);
			m_client->m_encodemgr.EnableRichCursor(FALSE);
			m_server->EnableXRichCursor(FALSE);
			m_client->m_cursor_update_pending = FALSE;
			m_client->m_cursor_update_sent = FALSE;

			// Prevent updates while the encoder is changed
			m_client->DisableProtocol();

			// Read in the preferred encodings
			msg.se.nEncodings = Swap16IfLE(msg.se.nEncodings);
			{
				int x;
				BOOL encoding_set = FALSE;

				// By default, don't use copyrect!
				m_client->m_update_tracker.enable_copyrect(false);

				for (x=0; x<msg.se.nEncodings; x++)
				{
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
						m_client->m_update_tracker.enable_copyrect(true);
						continue;
					}

					// Is this a NewFBSize encoding request?
					if (Swap32IfLE(encoding) == rfbEncodingNewFBSize) {
						m_client->m_use_NewSWSize = TRUE;
						continue;
					}

					// CACHE RDV
					if (Swap32IfLE(encoding) == rfbEncodingCacheEnable)
					{
						m_client->m_encodemgr.EnableCache(TRUE);
						vnclog.Print(LL_INTINFO, VNCLOG("Cache protocol extension enabled\n"));
						continue;
					}


					// XOR zlib
					if (Swap32IfLE(encoding) == rfbEncodingXOREnable) {
						m_client->m_encodemgr.AvailableXOR(TRUE);
						vnclog.Print(LL_INTINFO, VNCLOG("XOR protocol extension enabled\n"));
						continue;
					}


					// Is this a CompressLevel encoding?
					if ((Swap32IfLE(encoding) >= rfbEncodingCompressLevel0) &&
						(Swap32IfLE(encoding) <= rfbEncodingCompressLevel9))
					{
						// Client specified encoding-specific compression level
						int level = (int)(Swap32IfLE(encoding) - rfbEncodingCompressLevel0);
						m_client->m_encodemgr.SetCompressLevel(level);
						vnclog.Print(LL_INTINFO, VNCLOG("compression level requested: %d\n"), level);
						continue;
					}

					// Is this a QualityLevel encoding?
					if ((Swap32IfLE(encoding) >= rfbEncodingQualityLevel0) &&
						(Swap32IfLE(encoding) <= rfbEncodingQualityLevel9))
					{
						// Client specified image quality level used for JPEG compression
						int level = (int)(Swap32IfLE(encoding) - rfbEncodingQualityLevel0);
						m_client->m_encodemgr.SetQualityLevel(level);
						vnclog.Print(LL_INTINFO, VNCLOG("image quality level requested: %d\n"), level);
						continue;
					}

					// Is this a LastRect encoding request?
					if (Swap32IfLE(encoding) == rfbEncodingLastRect) {
						m_client->m_encodemgr.EnableLastRect(TRUE); // We forbid Last Rect for now 
						vnclog.Print(LL_INTINFO, VNCLOG("LastRect protocol extension enabled\n"));
						continue;
					}

					// Is this an XCursor encoding request?
					if (Swap32IfLE(encoding) == rfbEncodingXCursor) {
						m_client->m_encodemgr.EnableXCursor(TRUE);
						m_server->EnableXRichCursor(TRUE);
						vnclog.Print(LL_INTINFO, VNCLOG("X-style cursor shape updates enabled\n"));
						continue;
					}

					// Is this a RichCursor encoding request?
					if (Swap32IfLE(encoding) == rfbEncodingRichCursor) {
						m_client->m_encodemgr.EnableRichCursor(TRUE);
						m_server->EnableXRichCursor(TRUE);
						vnclog.Print(LL_INTINFO, VNCLOG("Full-color cursor shape updates enabled\n"));
						continue;
					}

					// Is this a PointerPos encoding request? nyama/marscha - PointerPos
					if (Swap32IfLE(encoding) == rfbEncodingPointerPos) {
						m_client->m_use_PointerPos = TRUE;
						vnclog.Print(LL_INTINFO, VNCLOG("PointerPos protocol extension enabled\n"));
						continue;
					}
					// 21 March 2008 jdp - client wants server state updates
					if (Swap32IfLE(encoding) == rfbEncodingServerState) {
						m_client->m_wants_ServerStateUpdates = true;
                        m_server->EnableServerStateUpdates(true);
						vnclog.Print(LL_INTINFO, VNCLOG("ServerState protocol extension enabled\n"));
                        continue;
					}
					// 21 March 2008 jdp  - client wants keepalive messages
					if (Swap32IfLE(encoding) == rfbEncodingEnableKeepAlive) {
						m_client->m_wants_KeepAlive = true;
                        m_server->EnableKeepAlives(true);
                        need_first_keepalive = true;
						vnclog.Print(LL_INTINFO, VNCLOG("KeepAlive protocol extension enabled\n"));
                        continue;
					}
					if (Swap32IfLE(encoding) == rfbEncodingFTProtocolVersion) {
                        need_ft_version_msg = true;
						vnclog.Print(LL_INTINFO, VNCLOG("FTProtocolVersion protocol extension enabled\n"));
                        continue;
					}

					// RDV - We try to detect which type of viewer tries to connect
					if (Swap32IfLE(encoding) == rfbEncodingZRLE) {
						m_client->m_encodemgr.AvailableZRLE(TRUE);
						vnclog.Print(LL_INTINFO, VNCLOG("ZRLE found \n"));
						// continue;
					}

					if (Swap32IfLE(encoding) == rfbEncodingTight) {
						m_client->m_encodemgr.AvailableTight(TRUE);
						vnclog.Print(LL_INTINFO, VNCLOG("Tight found\n"));
						// continue;
					}

					// Have we already found a suitable encoding?
					if (!encoding_set)
					{
						// No, so try the buffer to see if this encoding will work...
						omni_mutex_lock l(m_client->GetUpdateLock());
						if (m_client->m_encodemgr.SetEncoding(Swap32IfLE(encoding),FALSE))
							encoding_set = TRUE;
					}
				}

				// If no encoding worked then default to RAW!
				if (!encoding_set)
				{
					vnclog.Print(LL_INTINFO, VNCLOG("defaulting to raw encoder\n"));
					omni_mutex_lock l(m_client->GetUpdateLock());
					if (!m_client->m_encodemgr.SetEncoding(Swap32IfLE(rfbEncodingRaw),FALSE))
					{
						vnclog.Print(LL_INTERR, VNCLOG("failed to select raw encoder!\n"));

						connected = FALSE;
					}
				}

				// sf@2002 - For now we disable cache protocole when more than one client are connected
				// (But the cache buffer (if exists) is kept intact (for XORZlib usage))
				if (m_server->AuthClientCount() > 1)
					m_server->DisableCacheForAllClients();

			}

			// Re-enable updates
			m_client->client_settings_passed=true;
			m_client->EnableProtocol();

            // 26 March 2008 jdp 
            // send notification of remote input state to new client
            m_client->SendServerStateUpdate(rfbServerRemoteInputsState, m_server->GetDesktopPointer()->GetBlockInputState() ? rfbServerState_Disabled : rfbServerState_Enabled);
			break;
			
		case rfbFramebufferUpdateRequest:
			// Read the rest of the message:
			if (!m_socket->ReadExact(((char *) &msg)+nTO, sz_rfbFramebufferUpdateRequestMsg-nTO))
			{
				connected = FALSE;
				break;
			}
			/*{
				m_client->Sendtimer.stop();
				int sendtime=m_client->Sendtimer.read()*1000;
				if (m_client->Totalsend>1500 && sendtime!=0) 
					{
						//vnclog.Print(LL_SOCKERR, VNCLOG("Send Size %i %i %i %i\n"),m_socket->Totalsend,sendtime,m_socket->Totalsend/sendtime,m_client->m_encodemgr.m_encoding);
						m_client->timearray[m_client->m_encodemgr.m_encoding][m_client->roundrobin_counter]=sendtime;
						m_client->sizearray[m_client->m_encodemgr.m_encoding][m_client->roundrobin_counter]=m_client->Totalsend;
						m_client->Sendtimer.reset();
						for (int j=0;j<17;j++)
						{
							int totsize=0;
							int tottime=0;
							for (int i=0;i<31;i++)
								{
								totsize+=m_client->sizearray[j][i];
								tottime+=m_client->timearray[j][i];
								}
							if (tottime!=0 && totsize>1500)
								vnclog.Print(LL_SOCKERR, VNCLOG("Send Size %i %i %i %i\n"),totsize,tottime,totsize/tottime,j);
						}
						m_client->roundrobin_counter++;
						if (m_client->roundrobin_counter>30) m_client->roundrobin_counter=0;
					}
				m_client->Sendtimer.reset();
				m_client->Totalsend=0;

			}*/

			{
				rfb::Rect update;

				// Get the specified rectangle as the region to send updates for
				// Modif sf@2002 - Scaling.
				update.tl.x = (Swap16IfLE(msg.fur.x) + m_client->m_SWOffsetx) * m_client->m_nScale;
				update.tl.y = (Swap16IfLE(msg.fur.y) + m_client->m_SWOffsety) * m_client->m_nScale;
			//	update.tl.x = 0;
			//	update.tl.y = 0;
				update.br.x = update.tl.x + Swap16IfLE(msg.fur.w) * m_client->m_nScale;
				update.br.y = update.tl.y + Swap16IfLE(msg.fur.h) * m_client->m_nScale;
			//	update.br.x = 2880;
			//	update.br.y = 1200;
				rfb::Region2D update_rgn = update;
//				vnclog.Print(LL_SOCKERR, VNCLOG("Update asked for region %i %i %i %i %i\n"),update.tl.x,update.tl.y,update.br.x,update.br.y,m_client->m_SWOffsetx);

				// RealVNC 336
				if (update_rgn.is_empty()) {
					vnclog.Print(LL_INTERR, VNCLOG("FATAL! client update region is empty!\n"));
					connected = FALSE;
					break;
				}

				{
					omni_mutex_lock l(m_client->GetUpdateLock());

					// Add the requested area to the incremental update cliprect
					m_client->m_incr_rgn = m_client->m_incr_rgn.union_(update_rgn);

					// Is this request for a full update?
					if (!msg.fur.incremental)
					{
						// Yes, so add the region to the update tracker
						m_client->m_update_tracker.add_changed(update_rgn);
						
						// Tell the desktop grabber to fetch the region's latest state
						m_client->m_encodemgr.m_buffer->m_desktop->QueueRect(update);
					}
					
					/* RealVNC 336 (removed)
					// Check that this client has an update thread
					// The update thread never dissappears, and this is the only
					// thread allowed to create it, so this can be done without locking.
					if (!m_client->m_updatethread)
					{
						m_client->m_updatethread = new vncClientUpdateThread;
						connected = (m_client->m_updatethread &&
							m_client->m_updatethread->Init(m_client));
					}
					*/

					 // Kick the update thread (and create it if not there already)
					m_client->m_encodemgr.m_buffer->m_desktop->TriggerUpdate();
					m_client->TriggerUpdateThread();
				}
			}
			break;

		case rfbKeyEvent:
			// Read the rest of the message:
			if (m_socket->ReadExact(((char *) &msg)+nTO, sz_rfbKeyEventMsg-nTO))
			{				
				if (m_client->m_keyboardenabled)
				{
					msg.ke.key = Swap32IfLE(msg.ke.key);

					// Get the keymapper to do the work
					// m_client->m_keymap.DoXkeysym(msg.ke.key, msg.ke.down);
					vncKeymap::keyEvent(msg.ke.key, (0 != msg.ke.down),m_client->m_jap);

					m_client->m_remoteevent = TRUE;
				}
			}
			break;

		case rfbPointerEvent:
			// Read the rest of the message:
			if (m_socket->ReadExact(((char *) &msg)+nTO, sz_rfbPointerEventMsg-nTO))
			{
				if (m_client->m_pointerenabled)
				{
					// Convert the coords to Big Endian
					// Modif sf@2002 - Scaling
					msg.pe.x = (Swap16IfLE(msg.pe.x) + m_client->m_SWOffsetx+m_client->m_ScreenOffsetx) * m_client->m_nScale;
					msg.pe.y = (Swap16IfLE(msg.pe.y) + m_client->m_SWOffsety+m_client->m_ScreenOffsety) * m_client->m_nScale;

					// Work out the flags for this event
					DWORD flags = MOUSEEVENTF_ABSOLUTE;

					if (msg.pe.x != m_client->m_ptrevent.x ||
						msg.pe.y != m_client->m_ptrevent.y)
						flags |= MOUSEEVENTF_MOVE;
					if ( (msg.pe.buttonMask & rfbButton1Mask) != 
						(m_client->m_ptrevent.buttonMask & rfbButton1Mask) )
					{
					    if (GetSystemMetrics(SM_SWAPBUTTON))
						flags |= (msg.pe.buttonMask & rfbButton1Mask) 
						    ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
					    else
						flags |= (msg.pe.buttonMask & rfbButton1Mask) 
						    ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
					}
					if ( (msg.pe.buttonMask & rfbButton2Mask) != 
						(m_client->m_ptrevent.buttonMask & rfbButton2Mask) )
					{
						flags |= (msg.pe.buttonMask & rfbButton2Mask) 
						    ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
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
					}


					// Treat buttons 4 and 5 presses as mouse wheel events
					DWORD wheel_movement = 0;
					if (m_client->m_encodemgr.IsMouseWheelTight())
					{
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
					}
					else
					{
						// RealVNC 335 Mouse wheel support
						if (msg.pe.buttonMask & rfbWheelUpMask) {
							flags |= MOUSEEVENTF_WHEEL;
							wheel_movement = WHEEL_DELTA;
						}
						if (msg.pe.buttonMask & rfbWheelDownMask) {
							flags |= MOUSEEVENTF_WHEEL;
							wheel_movement = -WHEEL_DELTA;
						}
					}

					// Generate coordinate values
					// bug fix John Latino
					// offset for multi display
					int screenX, screenY, screenDepth;
					m_server->GetScreenInfo(screenX, screenY, screenDepth);
//					vnclog.Print(LL_INTINFO, VNCLOG("########mouse :%i %i %i %i \n"),screenX, screenY,m_client->m_ScreenOffsetx,m_client->m_ScreenOffsety );
					if (m_client->m_display_type==1)
						{//primary display always have (0,0) as corner
							unsigned long x = (msg.pe.x *  65535) / (screenX-1);
							unsigned long y = (msg.pe.y * 65535) / (screenY-1);
							// Do the pointer event
							::mouse_event(flags, (DWORD) x, (DWORD) y, wheel_movement, 0);
//							vnclog.Print(LL_INTINFO, VNCLOG("########mouse_event :%i %i \n"),x,y);
						}
					else
						{//second or spanned
							if (m_client->Sendinput)
							{							
								INPUT evt;
								evt.type = INPUT_MOUSE;
								msg.pe.x=msg.pe.x-GetSystemMetrics(SM_XVIRTUALSCREEN);
								msg.pe.y=msg.pe.y-GetSystemMetrics(SM_YVIRTUALSCREEN);
								evt.mi.dx = (msg.pe.x * 65535) / (GetSystemMetrics(SM_CXVIRTUALSCREEN)-1);
								evt.mi.dy = (msg.pe.y* 65535) / (GetSystemMetrics(SM_CYVIRTUALSCREEN)-1);
								evt.mi.dwFlags = flags | MOUSEEVENTF_VIRTUALDESK;
								evt.mi.dwExtraInfo = 0;
								evt.mi.mouseData = wheel_movement;
								evt.mi.time = 0;
								m_client->Sendinput(1, &evt, sizeof(evt));
							}
							else
							{
								POINT cursorPos; GetCursorPos(&cursorPos);
								ULONG oldSpeed, newSpeed = 10;
								ULONG mouseInfo[3];
								if (flags & MOUSEEVENTF_MOVE) 
									{
										flags &= ~MOUSEEVENTF_ABSOLUTE;
										SystemParametersInfo(SPI_GETMOUSE, 0, &mouseInfo, 0);
										SystemParametersInfo(SPI_GETMOUSESPEED, 0, &oldSpeed, 0);
										ULONG idealMouseInfo[] = {10, 0, 0};
										SystemParametersInfo(SPI_SETMOUSESPEED, 0, &newSpeed, 0);
										SystemParametersInfo(SPI_SETMOUSE, 0, &idealMouseInfo, 0);
									}
								::mouse_event(flags, msg.pe.x-cursorPos.x, msg.pe.y-cursorPos.y, wheel_movement, 0);
								if (flags & MOUSEEVENTF_MOVE) 
									{
										SystemParametersInfo(SPI_SETMOUSE, 0, &mouseInfo, 0);
										SystemParametersInfo(SPI_SETMOUSESPEED, 0, &oldSpeed, 0);
									}
							}
					}
					// Save the old position
					m_client->m_ptrevent = msg.pe;

					// Flag that a remote event occurred
					m_client->m_remoteevent = TRUE;

					// Tell the desktop hook system to grab the screen...
					m_client->m_encodemgr.m_buffer->m_desktop->TriggerUpdate();
				}
			}	
			break;

		case rfbClientCutText:
			// Read the rest of the message:
			if (m_socket->ReadExact(((char *) &msg)+nTO, sz_rfbClientCutTextMsg-nTO))
			{
				// Allocate storage for the text
				const UINT length = Swap32IfLE(msg.cct.length);
				if (length > 104857600) // 100 MBytes max
					break;
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
				m_server->UpdateLocalClipText(text);

				// Free the clip text we read
				delete [] text;
			}
			break;


		// Modif sf@2002 - Scaling
		// Server Scaling Message received
		case rfbPalmVNCSetScaleFactor:
			m_client->m_fPalmVNCScaling = true;
		case rfbSetScale: // Specific PalmVNC SetScaleFactor
			{
			// m_client->m_fPalmVNCScaling = false;
			// Read the rest of the message 
			if (m_client->m_fPalmVNCScaling)
			{
				if (!m_socket->ReadExact(((char *) &msg) + nTO, sz_rfbPalmVNCSetScaleFactorMsg - nTO))
				{
					connected = FALSE;
					break;
				}
			}
			else
			{
				if (!m_socket->ReadExact(((char *) &msg) + nTO, sz_rfbSetScaleMsg - nTO))
				{
					connected = FALSE;
					break;
				}
			}

			// Only accept reasonable scales...
			if (msg.ssc.scale < 1 || msg.ssc.scale > 9) break;

			m_client->m_nScale = msg.ssc.scale;
			{
				omni_mutex_lock l(m_client->GetUpdateLock());
				if (!m_client->m_encodemgr.m_buffer->SetScale(msg.ssc.scale))
					{
						connected = FALSE;
						break;
					}
	
				m_client->fNewScale = true;
				InvalidateRect(NULL,NULL,TRUE);
				m_client->TriggerUpdateThread();
			}

			}
			break;


		// Set Server Input
		case rfbSetServerInput:
			if (!m_socket->ReadExact(((char *) &msg) + nTO, sz_rfbSetServerInputMsg - nTO))
			{
				connected = FALSE;
				break;
			}
			if (m_client->m_keyboardenabled)
				{
					//if (msg.sim.status==1) m_client->m_encodemgr.m_buffer->m_desktop->SetDisableInput(true);
					//if (msg.sim.status==0) m_client->m_encodemgr.m_buffer->m_desktop->SetDisableInput(false);
					// added jeff
                vnclog.Print(LL_INTINFO, VNCLOG("rfbSetServerInput: inputs %s\n"), (msg.sim.status==1) ? "disabled" : "enabled");

                // only allow change if this is the client that originally changed the input state
                if (m_server->GetDesktopPointer()->GetBlockInputState() && !m_client->m_bClientHasBlockedInput) 
                {
                    CARD32 state = m_server->GetDesktopPointer()->GetBlockInputState() ? rfbServerState_Disabled : rfbServerState_Enabled;
                    m_server->NotifyClients_StateChange(rfbServerRemoteInputsState, state);
                }
                else
                {
                    m_client->m_encodemgr.m_buffer->m_desktop->SetBlockInputState(msg.sim.status==1);
                    m_client->m_bClientHasBlockedInput = (msg.sim.status==1);
                }
				}
			break;


		// Set Single Window
		case rfbSetSW:
			if (!m_socket->ReadExact(((char *) &msg) + nTO, sz_rfbSetSWMsg - nTO))
			{
				connected = FALSE;
				break;
			}
			if (Swap16IfLE(msg.sw.x)<5 && Swap16IfLE(msg.sw.y)<5) 
			{
				m_client->m_encodemgr.m_buffer->m_desktop->SetSW(1,1);
				break;
			}
			m_client->m_encodemgr.m_buffer->m_desktop->SetSW(
				(Swap16IfLE(msg.sw.x) + m_client->m_SWOffsetx+m_client->m_ScreenOffsetx) * m_client->m_nScale,
				(Swap16IfLE(msg.sw.y) + m_client->m_SWOffsety+m_client->m_ScreenOffsety) * m_client->m_nScale);
			break;


		// Modif sf@2002 - TextChat
		case rfbTextChat:
			m_client->m_pTextChat->ProcessTextChatMsg(nTO);
			break;


		// Modif sf@2002 - FileTransfer
		// File Transfer Message
		case rfbFileTransfer:
			{
			// sf@2004 - An unlogged user can't access to FT
			bool fUserOk = true;
			if (m_server->FTUserImpersonation())
			{
				fUserOk = m_client->DoFTUserImpersonation();
			}

			omni_mutex_lock l(m_client->GetUpdateLock());

			// Read the rest of the message:
			m_client->m_fFileTransferRunning = TRUE; 
			if (m_socket->ReadExact(((char *) &msg) + nTO, sz_rfbFileTransferMsg - nTO))
			{
				switch (msg.ft.contentType)
				{
				// A new file is received from the client
					// case rfbFileHeader:
                    case rfbFileTransferSessionStart:
                        m_client->m_fFileSessionOpen = true;
                        break;
                    case rfbFileTransferSessionEnd:
                        m_client->m_fFileSessionOpen = false;
                        break;
					case rfbFileTransferOffer:
						{
						omni_mutex_lock l(m_client->GetUpdateLock());
						if (!m_server->FileTransferEnabled() || !fUserOk) break;
						// bool fError = false;
						const UINT length = Swap32IfLE(msg.ft.length);
						memset(m_client->m_szFullDestName, 0, sizeof(m_client->m_szFullDestName));
						if (length > sizeof(m_client->m_szFullDestName)) break;
						// Read in the Name of the file to create
						if (!m_socket->ReadExact(m_client->m_szFullDestName, length)) 
						{
							//MessageBox(NULL, "1. Abort !", "Ultra WinVNC", MB_OK);
							// vnclog.Print(LL_INTINFO, VNCLOG("*** FileTransfer: Failed to receive FileName from Viewer. Abort !\n"));
							break;
						}
						
						// sf@2004 - Improving huge files size handling
						CARD32 sizeL = Swap32IfLE(msg.ft.size);
						CARD32 sizeH = 0;
						CARD32 sizeHtmp = 0;
						if (!m_socket->ReadExact((char*)&sizeHtmp, sizeof(CARD32)))
						{
							//MessageBox(NULL, "2. Abort !", "Ultra WinVNC", MB_OK);
							//vnclog.Print(LL_INTINFO, VNCLOG("*** FileTransfer: Failed to receive SizeH from Viewer. Abort !\n"));
							break;
						}
						sizeH = Swap32IfLE(sizeHtmp);

						// Parse the FileTime
						char *p = strrchr(m_client->m_szFullDestName, ',');
						if (p == NULL)
							m_client->m_szFileTime[0] = '\0';
						else 
						{
							strcpy(m_client->m_szFileTime, p+1);
							*p = '\0';
						}


                        // make a temp file name
                        strcpy(m_client->m_szFullDestName, make_temp_filename(m_client->m_szFullDestName).c_str());
                        
						// sf@2004 - Directory Delta Transfer
						// If the offered file is a zipped directory, we test if it already exists here
						// and create the zip accordingly. This way we can generate the checksums for it.
						// m_client->CheckAndZipDirectoryForChecksuming(m_client->m_szFullDestName);

						// Create Local Dest file
						m_client->m_hDestFile = CreateFile(m_client->m_szFullDestName, 
															GENERIC_WRITE | GENERIC_READ,
															FILE_SHARE_READ | FILE_SHARE_WRITE, 
															NULL,
															OPEN_ALWAYS,
															FILE_FLAG_SEQUENTIAL_SCAN,
															NULL);

						// sf@2004 - Delta Transfer
						bool fAlreadyExists = (GetLastError() == ERROR_ALREADY_EXISTS);

						DWORD dwDstSize = (DWORD)0; // Dummy size, actually a return value
						if (m_client->m_hDestFile == INVALID_HANDLE_VALUE)
							dwDstSize = 0xFFFFFFFF;
						else
							dwDstSize = 0x00;

						// Also check the free space on destination drive
						ULARGE_INTEGER lpFreeBytesAvailable;
						ULARGE_INTEGER lpTotalBytes;		
						ULARGE_INTEGER lpTotalFreeBytes;
						unsigned long dwFreeKBytes;
						char *szDestPath = new char [length + 1 + 64];
						if (szDestPath == NULL) break;
						memset(szDestPath, 0, length + 1 + 64);
						strcpy(szDestPath, m_client->m_szFullDestName);
						*strrchr(szDestPath, '\\') = '\0'; // We don't handle UNCs for now

						// loadlibrary
						// needed for 95 non OSR2
						// Possible this will block filetransfer, but at least server will start
						PGETDISKFREESPACEEX pGetDiskFreeSpaceEx;
						pGetDiskFreeSpaceEx = (PGETDISKFREESPACEEX)GetProcAddress( GetModuleHandle("kernel32.dll"),"GetDiskFreeSpaceExA");

						if (pGetDiskFreeSpaceEx)
						{
							if (!pGetDiskFreeSpaceEx((LPCTSTR)szDestPath,
											&lpFreeBytesAvailable,
											&lpTotalBytes,
											&lpTotalFreeBytes)
								) 
								dwDstSize = 0xFFFFFFFF;
						}

						delete [] szDestPath;
						dwFreeKBytes  = (unsigned long) (Int64ShraMod32(lpFreeBytesAvailable.QuadPart, 10));
						__int64 nnFileSize = (((__int64)sizeH) << 32 ) + sizeL;
						if ((__int64)dwFreeKBytes < (__int64)(nnFileSize / 1000)) dwDstSize = 0xFFFFFFFF;

						// Allocate buffer for file packets
						m_client->m_pBuff = new char [sz_rfbBlockSize + 1024];
						if (m_client->m_pBuff == NULL)
							dwDstSize = 0xFFFFFFFF;

						// Allocate buffer for DeCompression
						m_client->m_pCompBuff = new char [sz_rfbBlockSize];
						if (m_client->m_pCompBuff == NULL)
							dwDstSize = 0xFFFFFFFF;

						rfbFileTransferMsg ft;
						ft.type = rfbFileTransfer;

						// sf@2004 - Delta Transfer
						if (fAlreadyExists && dwDstSize != 0xFFFFFFFF)
						{
							ULARGE_INTEGER n2SrcSize;
							bool bSize = m_client->MyGetFileSize(m_client->m_szFullDestName, &n2SrcSize); 
							//DWORD dwFileSize = GetFileSize(m_client->m_hDestFile, NULL); 
							//if (dwFileSize != 0xFFFFFFFF)
							if (bSize)
							{
								int nCSBufferSize = (4 * (int)(n2SrcSize.QuadPart / sz_rfbBlockSize)) + 1024;
								char* lpCSBuff = new char [nCSBufferSize];
								if (lpCSBuff != NULL)
								{
									int nCSBufferLen = m_client->GenerateFileChecksums(	m_client->m_hDestFile,
																						lpCSBuff,
																						nCSBufferSize);
									if (nCSBufferLen != -1)
									{
										ft.contentType = rfbFileChecksums;
										ft.size = Swap32IfLE(nCSBufferSize);
										ft.length = Swap32IfLE(nCSBufferLen);
										m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
										m_socket->SendExact((char *)lpCSBuff, nCSBufferLen);
									}
								}
							}
						}

						ft.contentType = rfbFileAcceptHeader;
						ft.size = Swap32IfLE(dwDstSize); // File Size in bytes, 0xFFFFFFFF (-1) means error
						ft.length = Swap32IfLE(strlen(m_client->m_szFullDestName));
						m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
						m_socket->SendExact((char *)m_client->m_szFullDestName, strlen(m_client->m_szFullDestName));

						if (dwDstSize == 0xFFFFFFFF)
						{
                            helper::close_handle(m_client->m_hDestFile);
							if (m_client->m_pCompBuff != NULL)
								delete m_client->m_pCompBuff;
							if (m_client->m_pBuff != NULL)
								delete m_client->m_pBuff;

							//MessageBox(NULL, "3. Abort !", "Ultra WinVNC", MB_OK);
							//vnclog.Print(LL_INTINFO, VNCLOG("*** FileTransfer: Wrong Dest File size. Abort !\n"));
                            m_client->FTDownloadFailureHook();
							break;
						}

						m_client->m_dwFileSize = Swap32IfLE(msg.ft.size);
						m_client->m_dwNbPackets = (DWORD)(m_client->m_dwFileSize / sz_rfbBlockSize);
						m_client->m_dwNbReceivedPackets = 0;
						m_client->m_dwNbBytesWritten = 0;
						m_client->m_dwTotalNbBytesWritten = 0;

						m_client->m_fUserAbortedFileTransfer = false;
						m_client->m_fFileDownloadError = false;
						m_client->m_fFileDownloadRunning = true;
                        m_client->FTDownloadStartHook();
                        m_socket->SetRecvTimeout(m_server->GetFTTimeout());

						}
						break;

					// The client requests a File
					case rfbFileTransferRequest:
						{
						omni_mutex_lock l(m_client->GetUpdateLock());
						m_client->m_fCompressionEnabled = (Swap32IfLE(msg.ft.size) == 1);
						const UINT length = Swap32IfLE(msg.ft.length);
						memset(m_client->m_szSrcFileName, 0, sizeof(m_client->m_szSrcFileName));
						if (length > sizeof(m_client->m_szSrcFileName)) break;
						// Read in the Name of the file to create
						if (!m_socket->ReadExact(m_client->m_szSrcFileName, length))
						{
							//MessageBox(NULL, "4. Abort !", "Ultra WinVNC", MB_OK);
							//vnclog.Print(LL_INTINFO, VNCLOG("*** FileTransfer: Cannot read requested filename. Abort !\n"));
							break;
						}
						
                        // moved jdp 8/5/08 -- have to read whole packet to keep protocol in sync
						if (!m_server->FileTransferEnabled() || !fUserOk) break;
						// sf@2003 - Directory Transfer trick
						// If the file is an Ultra Directory Zip Request we zip the directory here
						// and we give it the requested name for transfer
						int nDirZipRet = m_client->ZipPossibleDirectory(m_client->m_szSrcFileName);
						if (nDirZipRet == -1)
						{
							//MessageBox(NULL, "5. Abort !", "Ultra WinVNC", MB_OK);
							//vnclog.Print(LL_INTINFO, VNCLOG("*** FileTransfer: Failed to zip requested dir. Abort !\n"));

							//	[v1.0.2-jp1 fix] Empty directory receive problem
							rfbFileTransferMsg ft;
							ft.type = rfbFileTransfer;
							ft.contentType = rfbFileHeader;
							ft.size = Swap32IfLE(0xffffffffu); // File Size in bytes, 0xFFFFFFFF (-1) means error
							ft.length = Swap32IfLE(strlen(m_client->m_szSrcFileName));
							m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
							m_socket->SendExact((char *)m_client->m_szSrcFileName, strlen(m_client->m_szSrcFileName));
                            // 2 May 2008 jdp send the highpart too, else the client will hang
                            // sf@2004 - Improving huge file size handling
                            // TODO: what if we're speaking the old protocol, how can we tell? 
					        CARD32 sizeH = Swap32IfLE(0xffffffffu);
					        m_socket->SendExact((char *)&sizeH, sizeof(CARD32));

							m_client->m_fFileUploadError = true;
							m_client->m_fFileUploadRunning = false;
                            m_client->FTUploadFailureHook();

							break;
						}


						// Open source file
						m_client->m_hSrcFile = CreateFile(
															m_client->m_szSrcFileName,		
															GENERIC_READ,		
															FILE_SHARE_READ,	
															NULL,				
															OPEN_EXISTING,		
															FILE_FLAG_SEQUENTIAL_SCAN,	
															NULL
														);				
						
						// DWORD dwSrcSize = (DWORD)0;
						ULARGE_INTEGER n2SrcSize;
						if (m_client->m_hSrcFile == INVALID_HANDLE_VALUE)
						{
							DWORD TheError = GetLastError();
							// dwSrcSize = 0xFFFFFFFF;
							n2SrcSize.LowPart = 0xFFFFFFFF;
                            n2SrcSize.HighPart = 0xFFFFFFFF;
						}
						else
						{	
							// Source file size 
							bool bSize = m_client->MyGetFileSize(m_client->m_szSrcFileName, &n2SrcSize); 
							// dwSrcSize = GetFileSize(m_client->m_hSrcFile, NULL); 
							// if (dwSrcSize == 0xFFFFFFFF)
							if (!bSize)
							{
								helper::close_handle(m_client->m_hSrcFile);
								n2SrcSize.LowPart = 0xFFFFFFFF;
                                n2SrcSize.HighPart = 0xFFFFFFFF;
							}
							else
							{
								// Add the File Time Stamp to the filename
								FILETIME SrcFileModifTime; 
								BOOL fRes = GetFileTime(m_client->m_hSrcFile, NULL, NULL, &SrcFileModifTime);
								if (fRes)
								{
									char szSrcFileTime[18];
									// sf@2003 - Convert file time to local time
									// We've made the choice off displaying all the files 
									// off client AND server sides converted in clients local
									// time only. So we don't convert server's files times.
									/*
									FILETIME LocalFileTime;
									FileTimeToLocalFileTime(&SrcFileModifTime, &LocalFileTime);
									*/
									SYSTEMTIME FileTime;
									FileTimeToSystemTime(&SrcFileModifTime/*&LocalFileTime*/, &FileTime);
									wsprintf(szSrcFileTime,"%2.2d/%2.2d/%4.4d %2.2d:%2.2d",
											FileTime.wMonth,
											FileTime.wDay,
											FileTime.wYear,
											FileTime.wHour,
											FileTime.wMinute
											);
									strcat(m_client->m_szSrcFileName, ",");
									strcat(m_client->m_szSrcFileName, szSrcFileTime);
								}
							}
						}

						// sf@2004 - Delta Transfer
						if (m_client->m_lpCSBuffer != NULL) 
						{
							delete [] m_client->m_lpCSBuffer;
							m_client->m_lpCSBuffer = NULL;
						}
						m_client->m_nCSOffset = 0;
						m_client->m_nCSBufferSize = 0;

						// Send the FileTransferMsg with rfbFileHeader
						rfbFileTransferMsg ft;
						
						ft.type = rfbFileTransfer;
						ft.contentType = rfbFileHeader;
						ft.size = Swap32IfLE(n2SrcSize.LowPart); // File Size in bytes, 0xFFFFFFFF (-1) means error
						ft.length = Swap32IfLE(strlen(m_client->m_szSrcFileName));
						m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
						m_socket->SendExact((char *)m_client->m_szSrcFileName, strlen(m_client->m_szSrcFileName));
						
						// sf@2004 - Improving huge file size handling
						CARD32 sizeH = Swap32IfLE(n2SrcSize.HighPart);
						m_socket->SendExact((char *)&sizeH, sizeof(CARD32));

						// delete [] szSrcFileName;
						if (n2SrcSize.LowPart == 0xFFFFFFFF && n2SrcSize.HighPart == 0xFFFFFFFF)
						{
							//MessageBox(NULL, "6. Abort !", "Ultra WinVNC", MB_OK);
							//vnclog.Print(LL_INTINFO, VNCLOG("*** FileTransfer: Wrong Src File size. Abort !\n"));
                            m_client->FTUploadFailureHook();
							break; // If error, we don't send anything else
						}
                        m_client->FTUploadStartHook();
						}
						break;

					// sf@2004 - Delta Transfer
					// Destination file already exists - the viewer sends the checksums
					case rfbFileChecksums:
                        m_socket->SetSendTimeout(m_server->GetFTTimeout()*1000);
						connected = m_client->ReceiveDestinationFileChecksums(Swap32IfLE(msg.ft.size), Swap32IfLE(msg.ft.length));
						break;

					// Destination file (viewer side) is ready for reception (size > 0) or not (size = -1)
					case rfbFileHeader:
						{
						// Check if the file has been created on client side
						if (Swap32IfLE(msg.ft.size) == -1)
						{
							helper::close_handle(m_client->m_hSrcFile);
                            m_client->FTUploadFailureHook();
							// MessageBox(NULL, "7. Abort !", "Ultra WinVNC", MB_OK);
							//vnclog.Print(LL_INTINFO, VNCLOG("*** FileTransfer: File not created on client side. Abort !\n"));
							break;
						}

						// Allocate buffer for file packets
						m_client->m_pBuff = new char [sz_rfbBlockSize];
						if (m_client->m_pBuff == NULL)
						{
							helper::close_handle(m_client->m_hSrcFile);
							//MessageBox(NULL, "8. Abort !", "Ultra WinVNC", MB_OK);
							//vnclog.Print(LL_INTINFO, VNCLOG("*** FileTransfer: rfbFileHeader - Unable to allocate buffer. Abort !\n"));
                            m_client->FTUploadFailureHook();
							break;
						}

						// Allocate buffer for compression
						// Todo : make a global buffer with CheckBufferSize proc
						m_client->m_pCompBuff = new char [sz_rfbBlockSize + 1024]; // TODO: Improve this
						if (m_client->m_pCompBuff == NULL)
						{
							helper::close_handle(m_client->m_hSrcFile);
							if (m_client->m_pBuff != NULL)
								delete m_client->m_pBuff;
							//MessageBox(NULL, "9. Abort !", "Ultra WinVNC", MB_OK);
							//vnclog.Print(LL_INTINFO, VNCLOG("*** FileTransfer: rfbFileHeader - Unable to allocate comp. buffer. Abort !\n"));
                            m_client->FTUploadFailureHook();
							break;
						}

						m_client->m_fEof = false;
						m_client->m_dwNbBytesRead = 0;
						m_client->m_dwTotalNbBytesWritten = 0;
						m_client->m_nPacketCount = 0;
						m_client->m_fFileUploadError = false;
						m_client->m_fFileUploadRunning = true;
                        m_client->m_fUserAbortedFileTransfer = false;

						connected = m_client->SendFileChunk();
						}
						break;


					case rfbFilePacket:
						if (!m_server->FileTransferEnabled() || !fUserOk) break;
						connected = m_client->ReceiveFileChunk(Swap32IfLE(msg.ft.length), Swap32IfLE(msg.ft.size));
                        m_client->SendKeepAlive();
						break;


					case rfbEndOfFile:
						if (!m_server->FileTransferEnabled() || !fUserOk) break;

						if (m_client->m_fFileDownloadRunning)
						{
							m_client->FinishFileReception();
						}
						break;

					// We use this message for FileTransfer rights (<=RC18 versions)
					// The client asks for FileTransfer permission
					case rfbAbortFileTransfer:

						// For now...
						if (m_client->m_fFileDownloadRunning)
						{
							m_client->m_fFileDownloadError = true;
                            m_client->m_fUserAbortedFileTransfer = true;
							m_client->FinishFileReception();
						}
						else if (m_client->m_fFileUploadRunning)
						{
							m_client->m_fFileUploadError = true;
                            m_client->m_fUserAbortedFileTransfer = true;
							// m_client->FinishFileSending();
						}
						else // Old method for FileTransfer handshake perimssion (<=RC18)
						{
							// We reject any <=RC18 Viewer FT 
							m_client->fFTRequest = true;

							// sf@2002 - DO IT HERE FOR THE MOMENT 
							// FileTransfer permission requested by the client
							if (m_client->fFTRequest)
							{
								rfbFileTransferMsg ft;
								ft.type = rfbFileTransfer;
								ft.contentType = rfbAbortFileTransfer;

								bool bOldFTProtocole = (msg.ft.contentParam == 0);
								if (bOldFTProtocole)
									ft.contentType = rfbAbortFileTransfer; // Viewer with New V2 FT protocole
								else
									ft.contentType = rfbFileTransferAccess; // Viewer with old FT protocole

								if (!bOldFTProtocole && m_server->FileTransferEnabled() && m_client->m_server->RemoteInputsEnabled() && fUserOk)
								   ft.size = Swap32IfLE(1);
								else
								   ft.size = Swap32IfLE(-1); 
								m_client->m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
								m_client->fFTRequest = false;
							}
						}
						break;

					/* Not yet used because we want backward compatibility...
					// From RC19 versions, the viewer uses this new message to request FT persmission
					// It also transmits its FT versions
					case rfbFileTransferAccess:
						m_client->fFTRequest = true;

						// FileTransfer permission requested by the client
						if (m_client->fFTRequest)
						{
							rfbFileTransferMsg ft;
							ft.type = rfbFileTransfer;
							ft.contentType = rfbFileTransferAccess;

							if (m_server->FileTransferEnabled() && m_client->m_server->RemoteInputsEnabled())
							   ft.size = Swap32IfLE(1);
							else
							   ft.size = Swap32IfLE(-1); 
							m_client->m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
							m_client->fFTRequest = false;
						}

						break;
					*/

					// The client requests the content of a directory or Drives List
					case rfbDirContentRequest:
						switch (msg.ft.contentParam)
						{
							// Client requests the List of Local Drives
							case rfbRDrivesList:
								{
								TCHAR szDrivesList[256]; // Format when filled : "C:\<NULL>D:\<NULL>....Z:\<NULL><NULL>
								DWORD dwLen;
								DWORD nIndex = 0;
								int nType = 0;
								TCHAR szDrive[4];
								dwLen = GetLogicalDriveStrings(256, szDrivesList);
                                // moved jdp 8/5/08 -- have to read whole packet to keep protocol in sync
						        if (!m_server->FileTransferEnabled() || !fUserOk) break;

								// We add Drives types to this drive list...
								while (nIndex < dwLen - 3)
								{
									strcpy(szDrive, szDrivesList + nIndex);
									// We replace the "\" char following the drive letter and ":"
									// with a char corresponding to the type of drive
									// We obtain something like "C:l<NULL>D:c<NULL>....Z:n\<NULL><NULL>"
									// Isn't it ugly ?
									nType = GetDriveType(szDrive);
									switch (nType)
									{
									case DRIVE_FIXED:
										szDrivesList[nIndex + 2] = 'l';
										break;
									case DRIVE_REMOVABLE:
										szDrivesList[nIndex + 2] = 'f';
										break;
									case DRIVE_CDROM:
										szDrivesList[nIndex + 2] = 'c';
										break;
									case DRIVE_REMOTE:
										szDrivesList[nIndex + 2] = 'n';
										break;
									}
									nIndex += 4;
								}

								rfbFileTransferMsg ft;
								ft.type = rfbFileTransfer;
								ft.contentType = rfbDirPacket;
								ft.contentParam = rfbADrivesList;
								ft.length = Swap32IfLE((int)dwLen);
								m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
								m_socket->SendExact((char *)szDrivesList, (int)dwLen);
								}
								break;

							// Client requests the content of a directory
							case rfbRDirContent:
								{
									//omni_mutex_lock l(m_client->GetUpdateLock());

									const UINT length = Swap32IfLE(msg.ft.length);
									char szDir[MAX_PATH];
									if (length > sizeof(szDir)) break;

									// Read in the Name of Dir to explore
									if (!m_socket->ReadExact(szDir, length)) break;
									szDir[length] = 0;

                                    // moved jdp 8/5/08 -- have to read whole packet to keep protocol in sync
						            if (!m_server->FileTransferEnabled() || !fUserOk) break;
									// sf@2004 - Shortcuts Case
									// Todo: Cultures translation ?
									int nFolder = -1;
									char szP[MAX_PATH + 2];
									bool fShortError = false;
									if (!_strnicmp(szDir, "My Documents", 11))
										nFolder = CSIDL_PERSONAL;
									if (!_strnicmp(szDir, "Desktop", 7))
										nFolder = CSIDL_DESKTOP;
									if (!_strnicmp(szDir, "Network Favorites", 17))
										nFolder = CSIDL_NETHOOD;

									if (nFolder != -1)
										// if (SHGetSpecialFolderPath(NULL, szP, nFolder, FALSE))
										if (m_client->GetSpecialFolderPath(nFolder, szP))
										{
											if (szP[strlen(szP)-1] != '\\') strcat(szP,"\\");
											strcpy(szDir, szP);
										}
										else
											fShortError = true;

									strcat(szDir, "*");

									WIN32_FIND_DATA fd;
									HANDLE ff;
									BOOL fRet = TRUE;

									rfbFileTransferMsg ft;
									ft.type = rfbFileTransfer;
									ft.contentType = rfbDirPacket;
									ft.contentParam = rfbADirectory; // or rfbAFile...

									DWORD errmode = SetErrorMode(SEM_FAILCRITICALERRORS); // No popup please !
									ff = FindFirstFile(szDir, &fd);
									SetErrorMode( errmode );
									
									// Case of media not accessible
									if (ff == INVALID_HANDLE_VALUE || fShortError)
									{
										ft.length = Swap32IfLE(0);										
										m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
										break;
									}

									ft.length = Swap32IfLE(strlen(szDir)-1);										
									m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
									// sf@2004 - Also send back the full directory path to the viewer (necessary for Shorcuts)
									m_socket->SendExact((char *)szDir, strlen(szDir)-1);


									while ( fRet )
									{
										// sf@2003 - Convert file time to local time
										// We've made the choice off displaying all the files 
										// off client AND server sides converted in clients local
										// time only. So we don't convert server's files times.
										/* 
										FILETIME LocalFileTime;
										FileTimeToLocalFileTime(&fd.ftLastWriteTime, &LocalFileTime);
										fd.ftLastWriteTime.dwLowDateTime = LocalFileTime.dwLowDateTime;
										fd.ftLastWriteTime.dwHighDateTime = LocalFileTime.dwHighDateTime;
										*/

										if (((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY && strcmp(fd.cFileName, "."))
											||
											(!strcmp(fd.cFileName, "..")))
										{
											// Serialize the interesting part of WIN32_FIND_DATA
											char szFileSpec[sizeof(WIN32_FIND_DATA)];
											int nOptLen = sizeof(WIN32_FIND_DATA) - MAX_PATH - 14 + lstrlen(fd.cFileName);
											memcpy(szFileSpec, &fd, nOptLen);

											ft.length = Swap32IfLE(nOptLen);
											m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
											m_socket->SendExact((char *)szFileSpec, nOptLen);
										}
										else if (strcmp(fd.cFileName, "."))
										{
											// Serialize the interesting part of WIN32_FIND_DATA
											// Get rid of the trailing blanck chars. It makes a BIG
											// difference when there's a lot of files in the dir.
											char szFileSpec[sizeof(WIN32_FIND_DATA)];
											int nOptLen = sizeof(WIN32_FIND_DATA) - MAX_PATH - 14 + lstrlen(fd.cFileName);
											memcpy(szFileSpec, &fd, nOptLen);

											ft.length = Swap32IfLE(nOptLen);
											m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
											m_socket->SendExact((char *)szFileSpec, nOptLen);

										}
										fRet = FindNextFile(ff, &fd);
									}
									FindClose(ff);
									
									// End of the transfer
									ft.contentParam = 0;
									ft.length = Swap32IfLE(0);
									m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
								}
								break;
						}
						break;

					// The client sends a command
					case rfbCommand:
						switch (msg.ft.contentParam)
						{
							// Client requests the creation of a directory
							case rfbCDirCreate:
								{
									const UINT length = Swap32IfLE(msg.ft.length);
									char szDir[MAX_PATH];
									if (length > sizeof(szDir)) break;

									// Read in the Name of Dir to explore
									if (!m_socket->ReadExact(szDir, length))
									{
										// todo: manage error !
										break;
									}
									szDir[length] = 0;

                                    // moved jdp 8/5/08 -- have to read whole packet to keep protocol in sync
						            if (!m_server->FileTransferEnabled() || !fUserOk) break;
									// Create the Dir
									BOOL fRet = CreateDirectory(szDir, NULL);

									rfbFileTransferMsg ft;
									ft.type = rfbFileTransfer;
									ft.contentType = rfbCommandReturn;
									ft.contentParam = rfbADirCreate;
									ft.size = fRet ? 0 : -1;
									ft.length = msg.ft.length;

									m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
									m_socket->SendExact((char *)szDir, (int)length);
                                    if (fRet)
                                        m_client->FTNewFolderHook(szDir);
								}
								break;

							// Client requests the deletion of a file
							case rfbCFileDelete:
								{
									UINT length = Swap32IfLE(msg.ft.length);
									char szFile[MAX_PATH];
									if (length > sizeof(szFile)) break;

									// Read in the Name of the File 
									if (!m_socket->ReadExact(szFile, length))
									{
										// todo: manage error !
										break;
									}
									szFile[length] = 0;
                                    // moved jdp 8/5/08 -- have to read whole packet to keep protocol in sync
						            if (!m_server->FileTransferEnabled() || !fUserOk) break;

									// Delete the file
                                    // 13 February 2008 jdp
                                    bool isDir = isDirectory(szFile);
                                    std::string newname(szFile);
                                    // put the '[]' around the name if it's a folder, so that the client can display
                                    // the proper messages. Otherwise, the client assumes it's a file.
                                    if (isDir)
                                        newname = AddDirPrefixAndSuffix(szFile);

                                    length = newname.length()+1;
									BOOL fRet = DeleteFileOrDirectory(szFile);

									rfbFileTransferMsg ft;
									ft.type = rfbFileTransfer;
									ft.contentType = rfbCommandReturn;
									ft.contentParam = rfbAFileDelete;
									ft.size = fRet ? 0 : -1;
									ft.length = Swap32IfLE(length);

									m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
									m_socket->SendExact((char *)newname.c_str(), length);
                                    if (fRet)
                                        m_client->FTDeleteHook(szFile, isDir);
								}
								break;

							// Client requests the Renaming of a file/directory
							case rfbCFileRename:
								{
									const UINT length = Swap32IfLE(msg.ft.length);
									char szNames[(2 * MAX_PATH) + 1];
									if (length > sizeof(szNames)) break;

									// Read in the Names
									if (!m_socket->ReadExact(szNames, length))
									{
										// todo: manage error !
										break;
									}
									szNames[length] = 0;
                                    // moved jdp 8/5/08 -- have to read whole packet to keep protocol in sync
						            if (!m_server->FileTransferEnabled() || !fUserOk) break;

									char *p = strrchr(szNames, '*');
									if (p == NULL) break;
									char szCurrentName[ (2 * MAX_PATH) + 1];
									char szNewName[ (2 * MAX_PATH) + 1];

									strcpy(szNewName, p + 1); 
									*p = '\0';
									strcpy(szCurrentName, szNames);
									*p = '*';

									// Rename
									BOOL fRet = MoveFile(szCurrentName, szNewName);

									rfbFileTransferMsg ft;
									ft.type = rfbFileTransfer;
									ft.contentType = rfbCommandReturn;
									ft.contentParam = rfbAFileRename;
									ft.size = fRet ? 0 : -1;
									ft.length = msg.ft.length;

									m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
									m_socket->SendExact((char *)szNames, (int)length);
                                    if (fRet)
                                        m_client->FTRenameHook(szCurrentName, szNewName);
								}
								break;


						}  // End of swith
						break;

				} // End of switch

			} // End of if
			else // Fix from Jeremy C. 
			{
				if (m_client->m_fFileDownloadRunning)
				{
					m_client->m_fFileDownloadError = true;
					FlushFileBuffers(m_client->m_hDestFile);
                    helper::close_handle(m_client->m_hDestFile);
                    m_client->FTDownloadFailureHook();
                    m_client->m_fFileDownloadRunning = false;
				}
                if (m_client->m_fFileUploadRunning)
                {
					m_client->m_fFileUploadError = true;
                    FlushFileBuffers(m_client->m_hSrcFile);
                    helper::close_handle(m_client->m_hSrcFile);
                    m_client->FTUploadFailureHook();
                    m_client->m_fFileUploadRunning = false;
				}
				//vnclog.Print(LL_INTINFO, VNCLOG("*** FileTransfer: message content reading error\n"));
			}
			/*
			// sf@2005 - Cancel FT User impersonation if possible
			// Fix from byteboon (Jeremy C.)
			if (m_server->FTUserImpersonation())
			{
				m_client->UndoFTUserImpersonation();
			}
			*/

			m_client->m_fFileTransferRunning = FALSE;
			}
			break;

			// Modif cs@2005
#ifdef DSHOW
		case rfbKeyFrameRequest:
			{
				MutexAutoLock l_Lock(&m_client->m_hmtxEncodeAccess);

				omni_mutex_lock l(m_client->GetUpdateLock());

				if(m_client->m_encodemgr.ResetZRLEEncoding())
				{
					rfb::Rect update;

					rfb::Rect ViewerSize = m_client->m_encodemgr.m_buffer->GetViewerSize();

					update.tl.x = 0;
					update.tl.y = 0;
					update.br.x = ViewerSize.br.x;
					update.br.y = ViewerSize.br.y;
					rfb::Region2D update_rgn = update;

					// Add the requested area to the incremental update cliprect
					m_client->m_incr_rgn = m_client->m_incr_rgn.union_(update_rgn);

					// Yes, so add the region to the update tracker
					m_client->m_update_tracker.add_changed(update_rgn);
					
					// Tell the desktop grabber to fetch the region's latest state
					m_client->m_encodemgr.m_buffer->m_desktop->QueueRect(update);

					// Kick the update thread (and create it if not there already)
					m_client->TriggerUpdateThread();

					// Send a message back to the client to confirm that we have reset the zrle encoding			
					rfbKeyFrameUpdateMsg header;
					header.type = rfbKeyFrameUpdate;
					m_client->SendRFBMsg(rfbKeyFrameUpdate, (BYTE *)&header, sz_rfbKeyFrameUpdateMsg);
				}				
				else
				{
					vnclog.Print(LL_INTERR, VNCLOG("[rfbKeyFrameRequest] Unable to Reset ZRLE Encoding\n"));
				}
			}
			break;
#endif
		default:
			// Unknown message, so fail!
			connected = FALSE;
		}

		// sf@2005 - Cancel FT User impersonation if possible
		// We do it here to ensure impersonation is cancelled
		if (m_server->FTUserImpersonation())
		{
			m_client->UndoFTUserImpersonation();
		}

	}

    if (m_client->m_fFileDownloadRunning)
    {
        m_client->m_fFileDownloadError = true;
        FlushFileBuffers(m_client->m_hDestFile);
        m_client->FTDownloadFailureHook();
        m_client->m_fFileDownloadRunning = false;
        helper::close_handle(m_client->m_hDestFile);
    }
        
    if (m_client->m_fFileUploadRunning)
    {
        m_client->m_fFileUploadError = true;
        FlushFileBuffers(m_client->m_hSrcFile);
        m_client->FTUploadFailureHook();
        m_client->m_fFileUploadRunning = false;
        helper::close_handle(m_client->m_hSrcFile);
    }

  

	// Move into the thread's original desktop
	// TAG 14
	vncService::SelectHDESK(home_desktop);
    if (m_client->m_bClientHasBlockedInput)
    {
        m_client->m_encodemgr.m_buffer->m_desktop->SetBlockInputState(false);
        m_client->m_bClientHasBlockedInput = false;
    }

	// Quit this thread.  This will automatically delete the thread and the
	// associated client.
	vnclog.Print(LL_CLIENTS, VNCLOG("client disconnected : %s (%hd)\n"),
									m_client->GetClientName(),
									m_client->GetClientId());
	//////////////////
	// LOG it also in the event
	//////////////////
	{
		typedef BOOL (*LogeventFn)(char *machine);
		LogeventFn Logevent = 0;
		char szCurrentDir[MAX_PATH];
		if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
		{
			char* p = strrchr(szCurrentDir, '\\');
			*p = '\0';
			strcat (szCurrentDir,"\\logging.dll");
		}
		HMODULE hModule = LoadLibrary(szCurrentDir);
		if (hModule)
			{
				BOOL result=false;
				Logevent = (LogeventFn) GetProcAddress( hModule, "LOGEXIT" );
				Logevent((char *)m_client->GetClientName());
				FreeLibrary(hModule);
			}
	}


	// Disable the protocol to ensure that the update thread
	// is not accessing the desktop and buffer objects
	m_client->DisableProtocol();

	// Finally, it's safe to kill the update thread here
	if (m_client->m_updatethread) {
		m_client->m_updatethread->Kill();
		m_client->m_updatethread->join(NULL);
	}
	// Remove the client from the server
	// This may result in the desktop and buffer being destroyed
	// It also guarantees that the client will not be passed further
	// updates
	m_server->RemoveClient(m_client->GetClientId());

	// sf@2003 - AutoReconnection attempt if required
	//if (!fShutdownOrdered)
		if (m_server->AutoReconnect())
		vncService::PostAddNewClient(1111, 1111);
}

// The vncClient itself

vncClient::vncClient()
{
	vnclog.Print(LL_INTINFO, VNCLOG("vncClient() executing...\n"));

	m_socket = NULL;
	m_client_name = 0;

	// Initialise mouse fields
	m_mousemoved = FALSE;
	m_ptrevent.buttonMask = 0;
	m_ptrevent.x = 0;
	m_ptrevent.y=0;

	// Other misc flags
	m_thread = NULL;
	m_palettechanged = FALSE;

	// Initialise the two update stores
	m_updatethread = NULL;
	m_update_tracker.init(this);

	m_remoteevent = FALSE;

	m_clipboard_text = 0;

	// IMPORTANT: Initially, client is not protocol-ready.
	m_disable_protocol = 1;

	//SINGLE WINDOW
	m_use_NewSWSize = FALSE;
	m_SWOffsetx=0;
	vnclog.Print(LL_INTINFO, VNCLOG("TEST 4\n"));
	m_SWOffsety=0;
	m_ScreenOffsetx=0;
	m_ScreenOffsety=0;

	// sf@2002 
	fNewScale = false;
	m_fPalmVNCScaling = false;
	fFTRequest = false;

	// Modif sf@2002 - FileTransfer
	m_fFileTransferRunning = FALSE;
	m_pZipUnZip = new CZipUnZip32(); // Directory FileTransfer utils

	m_hDestFile = 0;
	//m_szFullDestName = NULL;
	m_dwFileSize = 0; 
	m_dwNbPackets = 0;
	m_dwNbReceivedPackets = 0;
	m_dwNbBytesWritten = 0;
	m_dwTotalNbBytesWritten = 0;
	m_fFileDownloadError = false;
    m_fUserAbortedFileTransfer  = false;
	m_fFileDownloadRunning = false;
	m_hSrcFile = INVALID_HANDLE_VALUE;
	//m_szSrcFileName = NULL;
	m_fEof = false;
	m_dwNbBytesRead = 0;
	m_dwTotalNbBytesRead = 0;
	m_nPacketCount = 0;
	m_fCompressionEnabled = false;
	m_fFileUploadError = false;
	m_fFileUploadRunning = false;

	// sf@2004 - Delta Transfer
	m_lpCSBuffer = NULL;
	m_nCSOffset = 0;
	m_nCSBufferSize = 0;

	// CURSOR HANDLING
	m_cursor_update_pending = FALSE;
	m_cursor_update_sent = FALSE;
	// nyama/marscha - PointerPos
	m_use_PointerPos = FALSE;
	m_cursor_pos_changed = FALSE;
	m_cursor_pos.x = 0;
	m_cursor_pos.y = 0;

	//cachestats
	totalraw=0;

	m_pRawCacheZipBuf = NULL;
	m_nRawCacheZipBufSize = 0;
	m_pCacheZipBuf = NULL;
	m_nCacheZipBufSize = 0;

	// sf@2005 - FTUserImpersonation
	m_fFTUserImpersonatedOk = false;
	m_lLastFTUserImpersonationTime = 0L;

	// Modif sf@2002 - Text Chat
	m_pTextChat = new TextChat(this); 	
	m_fUltraViewer = true;
	m_IsLoopback=false;
	m_NewSWUpdateWaiting=false;
	client_settings_passed=false;
/*	roundrobin_counter=0;
	for (int i=0;i<rfbEncodingZRLE+1;i++)
		for (int j=0;j<31;j++)
		{
		  timearray[i][j]=0;
		  sizearray[i][j]=0;
		}*/
	Sendinput=0;
	Sendinput=(pSendinput) GetProcAddress(LoadLibrary("user32.dll"),"SendInput");
// Modif cs@2005
#ifdef DSHOW
	m_hmtxEncodeAccess = CreateMutex(NULL, FALSE, NULL);
#endif
    m_wants_ServerStateUpdates =  false;
    m_bClientHasBlockedInput = false;
    m_wants_KeepAlive = false;
    m_fFileSessionOpen = false;
}

vncClient::~vncClient()
{
	vnclog.Print(LL_INTINFO, VNCLOG("~vncClient() executing...\n"));

	// Modif sf@2002 - Text Chat
	if (m_pTextChat) 
	{
        m_pTextChat->KillDialog();
		delete(m_pTextChat);
		m_pTextChat = NULL;
	}

	// Directory FileTransfer utils
	if (m_pZipUnZip) delete m_pZipUnZip;

	// We now know the thread is dead, so we can clean up
	if (m_client_name != 0) {
		free(m_client_name);
		m_client_name = 0;
	}

	// If we have a socket then kill it
	if (m_socket != NULL)
	{
		vnclog.Print(LL_INTINFO, VNCLOG("deleting socket\n"));

		delete m_socket;
		m_socket = NULL;
	}
	if (m_pRawCacheZipBuf != NULL)
		{
			delete [] m_pRawCacheZipBuf;
			m_pRawCacheZipBuf = NULL;
		}
	if (m_pCacheZipBuf != NULL)
		{
			delete [] m_pCacheZipBuf;
			m_pCacheZipBuf = NULL;
		}
	if (m_clipboard_text) {
		free(m_clipboard_text);
	}
// Modif cs@2005
#ifdef DSHOW
	CloseHandle(m_hmtxEncodeAccess);
#endif

	//thos give sometimes errors, hlogfile is already removed at this point
	//vnclog.Print(LL_INTINFO, VNCLOG("cached %d \n"),totalraw);
}

// Init
BOOL
vncClient::Init(vncServer *server,
				VSocket *socket,
				BOOL auth,
				BOOL shared,
				vncClientId newid)
{
	// Save the server id;
	m_server = server;

	// Save the socket
	m_socket = socket;

	// Save the name of the connecting client
	char *name = m_socket->GetPeerName();
	if (name != 0)
		m_client_name = _strdup(name);
	else
		m_client_name = _strdup("<unknown>");

	// Save the client id
	m_id = newid;

	// Spawn the child thread here
	m_thread = new vncClientThread;
	if (m_thread == NULL)
		return FALSE;
	return ((vncClientThread *)m_thread)->Init(this, m_server, m_socket, auth, shared);

	return FALSE;
}

void
vncClient::Kill()
{
	// Close the socket
	vnclog.Print(LL_INTERR, VNCLOG("client Kill() called"));
	if (m_pTextChat)
        m_pTextChat->KillDialog();
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
	m_encodemgr.SetBuffer(buffer);
}

void
vncClient::TriggerUpdateThread()
{
	// ALWAYS lock the client UpdateLock before calling this!
	// RealVNC 336	
	// Check that this client has an update thread
	// The update thread never dissappears, and this is the only
	// thread allowed to create it, so this can be done without locking.
	
	if (!m_updatethread)
	{
		m_updatethread = new vncClientUpdateThread;
		if (!m_updatethread || 
			!m_updatethread->Init(this)) {
			Kill();
		}
	}				
	if (m_updatethread)
		m_updatethread->Trigger();
}

void
vncClient::UpdateMouse()
{
	if (!m_mousemoved && !m_cursor_update_sent)
	{
	omni_mutex_lock l(GetUpdateLock());
    m_mousemoved=TRUE;
	}
	// nyama/marscha - PointerPos
	if (m_use_PointerPos && !m_cursor_pos_changed) {
		POINT cursorPos;
		GetCursorPos(&cursorPos);
		//vnclog.Print(LL_INTINFO, VNCLOG("UpdateMouse m_cursor_pos(%d, %d), new(%d, %d)\n"), 
		//  m_cursor_pos.x, m_cursor_pos.y, cursorPos.x, cursorPos.y);
		if (cursorPos.x != m_cursor_pos.x || cursorPos.y != m_cursor_pos.y) {
			// This movement isn't by this client, but generated locally or by other client.
			// Send it to this client.
			omni_mutex_lock l(GetUpdateLock());
			m_cursor_pos.x = cursorPos.x;
			m_cursor_pos.y = cursorPos.y;
			m_cursor_pos_changed = TRUE;
			TriggerUpdateThread();
		}
	}
}

void
vncClient::UpdateClipText(const char* text)
{
	omni_mutex_lock l(GetUpdateLock());
	if (m_clipboard_text) {
		free(m_clipboard_text);
		m_clipboard_text = 0;
	}
	m_clipboard_text = _strdup(text);
	TriggerUpdateThread();
}

void
vncClient::UpdateCursorShape()
{
	omni_mutex_lock l(GetUpdateLock());
	TriggerUpdateThread();
}

void
vncClient::UpdatePalette()
{
	omni_mutex_lock l(GetUpdateLock());
	m_palettechanged = TRUE;
}

void
vncClient::UpdateLocalFormat()
{
	DisableProtocol();
	vnclog.Print(LL_INTERR, VNCLOG("updating local pixel format\n"));
	m_encodemgr.SetServerFormat();
	EnableProtocol();
}

BOOL
vncClient::SetNewSWSize(long w,long h,BOOL Desktop)
{
	if (!m_use_NewSWSize) return FALSE;
	DisableProtocol();

	vnclog.Print(LL_INTERR, VNCLOG("updating local pixel format and buffer size\n"));
	m_encodemgr.SetServerFormat();
	m_palettechanged = TRUE;
	// no lock needed Called from desktopthread
	if (Desktop) m_encodemgr.SetEncoding(0,TRUE);//0=dummy
//	m_fullscreen = m_encodemgr.GetSize();
	m_NewSWUpdateWaiting=true;
	NewsizeW=w;
	NewsizeH=h;
	EnableProtocol();
//	TriggerUpdateThread();
	return TRUE;
}

// Functions used to set and retrieve the client settings
const char*
vncClient::GetClientName()
{
	return m_client_name;
}

// Enabling and disabling clipboard/GFX updates
void
vncClient::DisableProtocol()
{
	BOOL disable = FALSE;
	{	omni_mutex_lock l(GetUpdateLock());
		if (m_disable_protocol == 0)
			disable = TRUE;
		m_disable_protocol++;
		if (disable && m_updatethread)
			m_updatethread->EnableUpdates(FALSE);
	}
}

void
vncClient::EnableProtocol()
{
	{	omni_mutex_lock l(GetUpdateLock());
		if (m_disable_protocol == 0) {
			vnclog.Print(LL_INTERR, VNCLOG("protocol enabled too many times!\n"));
			m_socket->Close();
			return;
		}
		m_disable_protocol--;
		if ((m_disable_protocol == 0) && m_updatethread)
			m_updatethread->EnableUpdates(TRUE);
	}
}

// Internal methods
BOOL
vncClient::SendRFBMsg(CARD8 type, BYTE *buffer, int buflen)
{
	// Set the message type
	((rfbServerToClientMsg *)buffer)->type = type;

	// Send the message
	if (!m_socket->SendExact((char *) buffer, buflen, type))
	{
		vnclog.Print(LL_CONNERR, VNCLOG("failed to send RFB message to client\n"));

		Kill();
		return FALSE;
	}
	return TRUE;
}


BOOL
vncClient::SendUpdate(rfb::SimpleUpdateTracker &update)
{
	// If there is nothing to send then exit

	if (update.is_empty() && !m_cursor_update_pending && !m_NewSWUpdateWaiting && !m_cursor_pos_changed) return FALSE;
	
	// Get the update info from the tracker
	rfb::UpdateInfo update_info;
	update.get_update(update_info);
	update.clear();
	//Old update could be outsite the new bounding
	//We first make sure the new size is send to the client
	//The cleint ask a full update after screen_size change
	if (m_NewSWUpdateWaiting) 
		{
			rfbFramebufferUpdateRectHeader hdr;
			if (m_use_NewSWSize) {
				hdr.r.x = 0;
				hdr.r.y = 0;
				hdr.r.w = Swap16IfLE(NewsizeW);
				hdr.r.h = Swap16IfLE(NewsizeH);
				hdr.encoding = Swap32IfLE(rfbEncodingNewFBSize);
				rfbFramebufferUpdateMsg header;
				header.nRects = Swap16IfLE(1);
				SendRFBMsg(rfbFramebufferUpdate, (BYTE *)&header,sz_rfbFramebufferUpdateMsg);
				m_socket->SendExact((char *)&hdr, sizeof(hdr));
				m_NewSWUpdateWaiting=false;
				return TRUE;
			}
		}

	// Find out how many rectangles in total will be updated
	// This includes copyrects and changed rectangles split
	// up by codings such as CoRRE.

	int updates = 0;
	int numsubrects = 0;
	updates += update_info.copied.size();
	if (m_encodemgr.IsCacheEnabled())
	{
		if (update_info.cached.size() > 5)
		{
			updates++;
		}
		else 
		{
			updates += update_info.cached.size();
			//vnclog.Print(LL_INTERR, "cached %d\n", updates);
		}
	}
	
	rfb::RectVector::const_iterator i;
	if (updates!= 0xFFFF)
	{
		for ( i=update_info.changed.begin(); i != update_info.changed.end(); i++)
		{
			// Tight specific (lastrect)
			numsubrects = m_encodemgr.GetNumCodedRects(*i);
			
			// Skip rest rectangles if an encoder will use LastRect extension.
			if (numsubrects == 0) {
				updates = 0xFFFF;
				break;
			}
			updates += numsubrects;
			//vnclog.Print(LL_INTERR, "changed %d\n", updates);
		}
	}	
	
	// if no cache is supported by the other viewer
	// We need to send the cache as a normal update
	if (!m_encodemgr.IsCacheEnabled() && updates!= 0xFFFF) 
	{
		for (i=update_info.cached.begin(); i != update_info.cached.end(); i++)
		{
			// Tight specific (lastrect)
			numsubrects = m_encodemgr.GetNumCodedRects(*i);
			
			// Skip rest rectangles if an encoder will use LastRect extension.
			if (numsubrects == 0) {
				updates = 0xFFFF;
				break;
			}	
			updates += numsubrects;
			//vnclog.Print(LL_INTERR, "cached2 %d\n", updates);
		}
	}
	
	// 
	if (!m_encodemgr.IsXCursorSupported()) m_cursor_update_pending=false;
	// Tight specific (lastrect)
	if (updates != 0xFFFF)
	{
		// Tight - CURSOR HANDLING
		if (m_cursor_update_pending)
		{
			updates++;
		}
		// nyama/marscha - PointerPos
		if (m_cursor_pos_changed)
			updates++;
		if (updates == 0) return FALSE;
	}

//	Sendtimer.start();

	omni_mutex_lock l(GetUpdateLock());
	// Otherwise, send <number of rectangles> header
	rfbFramebufferUpdateMsg header;
	header.nRects = Swap16IfLE(updates);
	if (!SendRFBMsg(rfbFramebufferUpdate, (BYTE *) &header, sz_rfbFramebufferUpdateMsg))
		return TRUE;
	
	// CURSOR HANDLING
	if (m_cursor_update_pending) {
		if (!SendCursorShapeUpdate())
			return FALSE;
	}
	// nyama/marscha - PointerPos
	if (m_cursor_pos_changed)
		if (!SendCursorPosUpdate())
			return FALSE;
	
	// Send the copyrect rectangles
	if (!update_info.copied.empty()) {
		rfb::Point to_src_delta = update_info.copy_delta.negate();
		for (i=update_info.copied.begin(); i!=update_info.copied.end(); i++) {
			rfb::Point src = (*i).tl.translate(to_src_delta);
			if (!SendCopyRect(*i, src))
				return FALSE;
		}
	}
	
	if (m_encodemgr.IsCacheEnabled())
	{
		if (update_info.cached.size() > 5)
		{
			if (!SendCacheZip(update_info.cached))
				return FALSE;
		}
		else
		{
			if (!SendCacheRectangles(update_info.cached))
				return FALSE;
		}
	}
	else 
	{
		if (!SendRectangles(update_info.cached))
			return FALSE;
	}
	
	if (!SendRectangles(update_info.changed))
		return FALSE;

	// Tight specific - Send LastRect marker if needed.
	if (updates == 0xFFFF)
	{
		m_encodemgr.LastRect(m_socket);
		if (!SendLastRect())
			return FALSE;
	}
	m_socket->ClearQueue();
	// vnclog.Print(LL_INTINFO, VNCLOG("Update cycle\n"));
	return TRUE;
}

// Send a set of rectangles
BOOL
vncClient::SendRectangles(const rfb::RectVector &rects)
{
	// Modif cs@2005
#ifdef DSHOW
	MutexAutoLock l_Lock(&m_hmtxEncodeAccess);
#endif
//	rfb::Rect rect;
	rfb::RectVector::const_iterator i;

	// Work through the list of rectangles, sending each one
	for (i=rects.begin();i!=rects.end();i++) {
		if (!SendRectangle(*i))
			return FALSE;
	}

	return TRUE;
}

// Tell the encoder to send a single rectangle
BOOL
vncClient::SendRectangle(const rfb::Rect &rect)
{
	// Get the buffer to encode the rectangle
	// Modif sf@2002 - Scaling
	rfb::Rect ScaledRect;
	ScaledRect.tl.y = rect.tl.y / m_nScale;
	ScaledRect.br.y = rect.br.y / m_nScale;
	ScaledRect.tl.x = rect.tl.x / m_nScale;
	ScaledRect.br.x = rect.br.x / m_nScale;


	//	Totalsend+=(ScaledRect.br.x-ScaledRect.tl.x)*(ScaledRect.br.y-ScaledRect.tl.y);

	// sf@2002 - DSMPlugin
	// Some encoders (Hextile, ZRLE, Raw..) store all the data to send into 
	// m_clientbuffer and return the total size from EncodeRect()
	// Some Encoders (Tight, Zlib, ZlibHex..) send data on the fly and return
	// a partial size from EncodeRect(). 
	// On the viewer side, the data is read piece by piece or in one shot 
	// still depending on the encoding...
	// It is not compatible with DSM: we need to read/write data blocks of same 
	// size on both sides in one shot
	// We create a common method to send the data 
	if (m_socket->IsUsePluginEnabled() && m_server->GetDSMPluginPointer()->IsEnabled())
	{
		// Tell the SendExact() calls to write into the local NetRectBuffer memory buffer
		m_socket->SetWriteToNetRectBuffer(true);
		m_socket->SetNetRectBufOffset(0);
		// sf@2003 - we can't easely predict how many rects are going to be sent 
		// (for Tight encoding for instance)
		// Then we take the worse case (screen buffer size * 1.5) for the net rect buffer size.
		// m_socket->CheckNetRectBufferSize((int)(m_encodemgr.GetClientBuffSize() * 2));
		m_socket->CheckNetRectBufferSize((int)(m_encodemgr.m_buffer->m_desktop->ScreenBuffSize() * 3 / 2));
		UINT bytes = m_encodemgr.EncodeRect(ScaledRect, m_socket);
		m_socket->SetWriteToNetRectBuffer(false);

		BYTE* pDataBuffer = NULL;
		UINT TheSize = 0;

		// If SendExact() was called from inside the encoder
		if (m_socket->GetNetRectBufOffset() > 0)
		{
			TheSize = m_socket->GetNetRectBufOffset();
			m_socket->SetNetRectBufOffset(0);
			pDataBuffer = m_socket->GetNetRectBuf();
			// Add the rest to the data buffer if it exists
			if (bytes > 0)
			{
				memcpy(pDataBuffer + TheSize, m_encodemgr.GetClientBuffer(), bytes);
			}
		}
		else // If all data was stored in m_clientbuffer
		{
			TheSize = bytes;
			bytes = 0;
			pDataBuffer = m_encodemgr.GetClientBuffer();
		}

		// Send the header
		m_socket->SendExactQueue((char *)pDataBuffer, sz_rfbFramebufferUpdateRectHeader);

		// Send the size of the following rects data buffer
		CARD32 Size = (CARD32)(TheSize + bytes - sz_rfbFramebufferUpdateRectHeader);
		Size = Swap32IfLE(Size);
		m_socket->SendExactQueue((char*)&Size, sizeof(CARD32));
		// Send the data buffer
		m_socket->SendExactQueue(((char *)pDataBuffer + sz_rfbFramebufferUpdateRectHeader),
							TheSize + bytes - sz_rfbFramebufferUpdateRectHeader
							);
		
	}
	else // Normal case - No DSM - Symetry is not important
	{
		UINT bytes = m_encodemgr.EncodeRect(ScaledRect, m_socket);

		// if (bytes == 0) return false; // From realvnc337. No! Causes viewer disconnections/

		// Send the encoded data
		return m_socket->SendExactQueue((char *)(m_encodemgr.GetClientBuffer()), bytes);
	}

	return true;
}

// Send a single CopyRect message
BOOL
vncClient::SendCopyRect(const rfb::Rect &dest, const rfb::Point &source)
{
	// Create the message header
	// Modif sf@2002 - Scaling
	rfbFramebufferUpdateRectHeader copyrecthdr;
	copyrecthdr.r.x = Swap16IfLE((dest.tl.x - m_SWOffsetx) / m_nScale );
	copyrecthdr.r.y = Swap16IfLE((dest.tl.y - m_SWOffsety) / m_nScale);
	copyrecthdr.r.w = Swap16IfLE((dest.br.x - dest.tl.x) / m_nScale);
	copyrecthdr.r.h = Swap16IfLE((dest.br.y - dest.tl.y) / m_nScale);
	copyrecthdr.encoding = Swap32IfLE(rfbEncodingCopyRect);

	// Create the CopyRect-specific section
	rfbCopyRect copyrectbody;
	copyrectbody.srcX = Swap16IfLE((source.x- m_SWOffsetx) / m_nScale);
	copyrectbody.srcY = Swap16IfLE((source.y- m_SWOffsety) / m_nScale);

	// Now send the message;
	if (!m_socket->SendExactQueue((char *)&copyrecthdr, sizeof(copyrecthdr)))
		return FALSE;
	if (!m_socket->SendExactQueue((char *)&copyrectbody, sizeof(copyrectbody)))
		return FALSE;

	return TRUE;
}

// Send the encoder-generated palette to the client
// This function only returns FALSE if the SendExact fails - any other
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
	if (!m_encodemgr.GetPalette(rgbquad, ncolours))
	{
		delete [] rgbquad;
		return TRUE;
	}

	// Compose the message
	setcmap.type = rfbSetColourMapEntries;
	setcmap.firstColour = Swap16IfLE(0);
	setcmap.nColours = Swap16IfLE(ncolours);

	if (!m_socket->SendExact((char *) &setcmap, sz_rfbSetColourMapEntriesMsg, rfbSetColourMapEntries))
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

		if (!m_socket->SendExact((char *) &pixeldata, sizeof(pixeldata)))
		{
			delete [] rgbquad;
			return FALSE;
		}
	}

	// Delete the rgbquad data
	delete [] rgbquad;

	return TRUE;
}

void
vncClient::SetSWOffset(int x,int y)
{
	//if (m_SWOffsetx!=x || m_SWOffsety!=y) m_encodemgr.m_buffer->ClearCache();
	m_SWOffsetx=x;
	m_SWOffsety=y;
	m_encodemgr.SetSWOffset(x,y);
}

void
vncClient::SetScreenOffset(int x,int y,int type)
{
	m_ScreenOffsetx=x;
	m_ScreenOffsety=y;
	m_display_type=type;
}

// CACHE RDV
// Send a set of rectangles
BOOL
vncClient::SendCacheRectangles(const rfb::RectVector &rects)
{
//	rfb::Rect rect;
	rfb::RectVector::const_iterator i;

	if (rects.size() == 0) return TRUE;
	vnclog.Print(LL_INTINFO, VNCLOG("******** Sending %d Cache Rects \r\n"), rects.size());

	// Work through the list of rectangles, sending each one
	for (i= rects.begin();i != rects.end();i++)
	{
		if (!SendCacheRect(*i))
			return FALSE;
	}

	return TRUE;
}

// Tell the encoder to send a single rectangle
BOOL
vncClient::SendCacheRect(const rfb::Rect &dest)
{
	// Create the message header
	// Modif rdv@2002 - v1.1.x - Application Resize
	// Modif sf@2002 - Scaling
	rfbFramebufferUpdateRectHeader cacherecthdr;
	cacherecthdr.r.x = Swap16IfLE((dest.tl.x - m_SWOffsetx) / m_nScale );
	cacherecthdr.r.y = Swap16IfLE((dest.tl.y - m_SWOffsety) / m_nScale);
	cacherecthdr.r.w = Swap16IfLE((dest.br.x - dest.tl.x) / m_nScale);
	cacherecthdr.r.h = Swap16IfLE((dest.br.y - dest.tl.y) / m_nScale);
	cacherecthdr.encoding = Swap32IfLE(rfbEncodingCache);

	totalraw+=(dest.br.x - dest.tl.x)*(dest.br.y - dest.tl.y)*32 / 8; // 32bit test
	// Create the CopyRect-specific section
	rfbCacheRect cacherectbody;
	cacherectbody.special = Swap16IfLE(9999); //not used dummy

	// Now send the message;
	if (!m_socket->SendExactQueue((char *)&cacherecthdr, sizeof(cacherecthdr)))
		return FALSE;
	if (!m_socket->SendExactQueue((char *)&cacherectbody, sizeof(cacherectbody)))
		return FALSE;
	return TRUE;
}

BOOL
vncClient::SendCursorShapeUpdate()
{
	m_cursor_update_pending = FALSE;

	if (!m_encodemgr.SendCursorShape(m_socket)) {
		m_cursor_update_sent = FALSE;
		return m_encodemgr.SendEmptyCursorShape(m_socket);
	}

	m_cursor_update_sent = TRUE;
	return TRUE;
}

BOOL
vncClient::SendCursorPosUpdate()
{
	m_cursor_pos_changed = FALSE;

	rfbFramebufferUpdateRectHeader hdr;
	hdr.r.x = Swap16IfLE(m_cursor_pos.x);
	hdr.r.y = Swap16IfLE(m_cursor_pos.y);
	hdr.r.w = 0;
	hdr.r.h = 0;
	hdr.encoding = Swap32IfLE(rfbEncodingPointerPos);

	if (!m_socket->SendExactQueue((char *)&hdr, sizeof(hdr)))
		return FALSE;

	return TRUE;
}

// Tight specific - Send LastRect marker indicating that there are no more rectangles to send
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
	if (!m_socket->SendExactQueue((char *)&hdr, sizeof(hdr)))
		return FALSE;

	return TRUE;
}


//
// sf@2002 - New cache rects transport - Uses Zlib
//
// 
BOOL vncClient::SendCacheZip(const rfb::RectVector &rects)
{
	//int totalCompDataLen = 0;

	int nNbCacheRects = rects.size();
	if (!nNbCacheRects) return true;
	unsigned long rawDataSize = nNbCacheRects * sz_rfbRectangle;
	unsigned long maxCompSize = (rawDataSize + (rawDataSize/100) + 8);

	// Check RawCacheZipBuff
	// create a space big enough for the Zlib encoded cache rects list
	if (m_nRawCacheZipBufSize < rawDataSize)
	{
		if (m_pRawCacheZipBuf != NULL)
		{
			delete [] m_pRawCacheZipBuf;
			m_pRawCacheZipBuf = NULL;
		}
		m_pRawCacheZipBuf = new BYTE [rawDataSize+1];
		if (m_pRawCacheZipBuf == NULL) 
			return false;
		m_nRawCacheZipBufSize = rawDataSize;
	}

	// Copy all the cache rects coordinates into the RawCacheZip Buffer 
	rfbRectangle theRect;
	rfb::RectVector::const_iterator i;
	BYTE* p = m_pRawCacheZipBuf;
	for (i = rects.begin();i != rects.end();i++)
	{
		theRect.x = Swap16IfLE(((*i).tl.x - m_SWOffsetx) / m_nScale );
		theRect.y = Swap16IfLE(((*i).tl.y - m_SWOffsety) / m_nScale);
		theRect.w = Swap16IfLE(((*i).br.x - (*i).tl.x) / m_nScale);
		theRect.h = Swap16IfLE(((*i).br.y - (*i).tl.y) / m_nScale);
		memcpy(p, (BYTE*)&theRect, sz_rfbRectangle);
		p += sz_rfbRectangle;
	}

	// Create a space big enough for the Zlib encoded cache rects list
	if (m_nCacheZipBufSize < maxCompSize)
	{
		if (m_pCacheZipBuf != NULL)
		{
			delete [] m_pCacheZipBuf;
			m_pCacheZipBuf = NULL;
		}
		m_pCacheZipBuf = new BYTE [maxCompSize+1];
		if (m_pCacheZipBuf == NULL) return 0;
		m_nCacheZipBufSize = maxCompSize;
	}

	int nRet = compress((unsigned char*)(m_pCacheZipBuf),
						(unsigned long*)&maxCompSize,
						(unsigned char*)m_pRawCacheZipBuf,
						rawDataSize
						);

	if (nRet != 0)
	{
		return false;
	}

	vnclog.Print(LL_INTINFO, VNCLOG("*** Sending CacheZip Rects=%d Size=%d (%d)\r\n"), nNbCacheRects, maxCompSize, nNbCacheRects * 14);

	// Send the Update Rect header
	rfbFramebufferUpdateRectHeader CacheRectsHeader;
	CacheRectsHeader.r.x = Swap16IfLE(nNbCacheRects);
	CacheRectsHeader.r.y = 0;
	CacheRectsHeader.r.w = 0;
 	CacheRectsHeader.r.h = 0;
	CacheRectsHeader.encoding = Swap32IfLE(rfbEncodingCacheZip);

	// Format the ZlibHeader
	rfbZlibHeader CacheZipHeader;
	CacheZipHeader.nBytes = Swap32IfLE(maxCompSize);

	// Now send the message
	if (!m_socket->SendExactQueue((char *)&CacheRectsHeader, sizeof(CacheRectsHeader)))
		return FALSE;
	if (!m_socket->SendExactQueue((char *)&CacheZipHeader, sizeof(CacheZipHeader)))
		return FALSE;
	if (!m_socket->SendExactQueue((char *)m_pCacheZipBuf, maxCompSize))
		return FALSE;

	return TRUE;
}


//
//
//
void vncClient::EnableCache(BOOL enabled)
{
	m_encodemgr.EnableCache(enabled);
}

void vncClient::SetProtocolVersion(rfbProtocolVersionMsg *protocolMsg)
{
	if (protocolMsg!=NULL) memcpy(ProtocolVersionMsg,protocolMsg,sz_rfbProtocolVersionMsg);
	else strcpy(ProtocolVersionMsg,"0.0.0.0");
}

void vncClient::Clear_Update_Tracker()
{
	m_update_tracker.clear();
}

void vncClient::TriggerUpdate()
{
	m_encodemgr.m_buffer->m_desktop->TriggerUpdate();
}


////////////////////////////////////////////////
// Asynchronous & Delta File Transfer functions
////////////////////////////////////////////////

//
// sf@2004 - Delta Transfer
// Create the checksums buffer of an open file
//
int vncClient::GenerateFileChecksums(HANDLE hFile, char* lpCSBuffer, int nCSBufferSize)
{
	bool fEof = false;
	bool fError = false;
	DWORD dwNbBytesRead = 0;
	int nCSBufferOffset = 0;

	char* lpBuffer = new char [sz_rfbBlockSize];
	if (lpBuffer == NULL)
		return -1;

	while ( !fEof )
	{
		int nRes = ReadFile(hFile, lpBuffer, sz_rfbBlockSize, &dwNbBytesRead, NULL);
		if (!nRes && dwNbBytesRead != 0)
			fError = true;

		if (nRes && dwNbBytesRead == 0)
			fEof = true;
		else
		{
			unsigned long cs = adler32(0L, Z_NULL, 0);
			cs = adler32(cs, (unsigned char*)lpBuffer, (int)dwNbBytesRead);

			memcpy(lpCSBuffer + nCSBufferOffset, &cs, 4);
			nCSBufferOffset += 4; 
		}
	}

	SetFilePointer(hFile, 0L, NULL, FILE_BEGIN); 
	delete [] lpBuffer;

	if (fError) 
	{
		return -1;
	}

	return nCSBufferOffset;

}


//
// sf@2004 - Delta Transfer
// Destination file already exists
// The server sends the checksums of this file in one shot.
// 
bool vncClient::ReceiveDestinationFileChecksums(int nSize, int nLen)
{
	if (nLen < 0 || nLen > 104857600) // 100 MBytes max
		return false;

	m_lpCSBuffer = new char [nLen+1];
	if (m_lpCSBuffer == NULL) 
	{
		return false;
	}

	memset(m_lpCSBuffer, '\0', nLen+1);

	VBool res = m_socket->ReadExact((char *)m_lpCSBuffer, nLen);
	m_nCSBufferSize = nLen;

	return res == VTrue;
}

//
//
//
bool vncClient::ReceiveFileChunk(int nLen, int nSize)
{
    bool connected = true;
	
	if (!m_fFileDownloadRunning)
		return  connected;

	if (m_fFileDownloadError)
	{
		FinishFileReception();
		return connected;
	}

	if (nLen < 0)
		return false;

	if (nLen > sz_rfbBlockSize) return connected;

	bool fCompressed = true;
	BOOL fRes = true;
	bool fAlreadyHere = (nSize == 2);

	// sf@2004 - Delta Transfer - Empty packet
	if (fAlreadyHere) 
	{
		DWORD dwPtr = SetFilePointer(m_hDestFile, nLen, NULL, FILE_CURRENT); 
		if (dwPtr == 0xFFFFFFFF)
			fRes = false;
	}
	else
	{
        connected = m_socket->ReadExact((char *)m_pBuff, nLen) == VTrue;
		if (connected)
		{
			if (nSize == 0) fCompressed = false;
			unsigned int nRawBytes = sz_rfbBlockSize;
			
			if (fCompressed)
			{
				// Decompress incoming data
				int nRet = uncompress(	(unsigned char*)m_pCompBuff,	// Dest 
										(unsigned long *)&nRawBytes, // Dest len
										(const unsigned char*)m_pBuff,// Src
										nLen	// Src len
									);							
				if (nRet != 0)
				{
					m_fFileDownloadError = true;
					FinishFileReception();
					return connected;
				}
			}

			fRes = WriteFile(m_hDestFile,
							fCompressed ? m_pCompBuff : m_pBuff,
							fCompressed ? nRawBytes : nLen,
							&m_dwNbBytesWritten,
							NULL);
		}
		else
		{
			m_fFileDownloadError = true;
			// FlushFileBuffers(m_client->m_hDestFile);
			FinishFileReception();
		}
	}

	if (!fRes)
	{
		// TODO : send an explicit error msg to the client...
		m_fFileDownloadError = true;
		FinishFileReception();
		return connected;
	}

	m_dwTotalNbBytesWritten += (fAlreadyHere ? nLen : m_dwNbBytesWritten);
	m_dwNbReceivedPackets++;

	return connected;
}


void vncClient::FinishFileReception()
{
	if (!m_fFileDownloadRunning)
		return;

	m_fFileDownloadRunning = false;
	m_socket->SetRecvTimeout(m_server->AutoIdleDisconnectTimeout()*1000);
    SendKeepAlive(true);

	// sf@2004 - Delta transfer
	SetEndOfFile(m_hDestFile);

	// if error ?
	FlushFileBuffers(m_hDestFile);

	// Set the DestFile Time Stamp
	if (strlen(m_szFileTime))
	{
		FILETIME DestFileTime;
		SYSTEMTIME FileTime;
		FileTime.wMonth  = atoi(m_szFileTime);
		FileTime.wDay    = atoi(m_szFileTime + 3);
		FileTime.wYear   = atoi(m_szFileTime + 6);
		FileTime.wHour   = atoi(m_szFileTime + 11);
		FileTime.wMinute = atoi(m_szFileTime + 14);
		FileTime.wMilliseconds = 0;
		FileTime.wSecond = 0;
		SystemTimeToFileTime(&FileTime, &DestFileTime);
		// ToDo: hook error
		SetFileTime(m_hDestFile, &DestFileTime, &DestFileTime, &DestFileTime);
	}

	// CleanUp
	helper::close_handle(m_hDestFile);

	// sf@2004 - Delta Transfer : we can keep the existing file data :)
	// if (m_fFileDownloadError) DeleteFile(m_szFullDestName);

	// sf@2003 - Directory Transfer trick
	// If the file is an Ultra Directory Zip we unzip it here and we delete the
	// received file
	// Todo: make a better free space check above in this particular case. The free space must be at least
	// 3 times the size of the directory zip file (this zip file is ~50% of the real directory size) 
	bool bWasDir = UnzipPossibleDirectory(m_szFullDestName);
	/*
	if (!m_fFileDownloadError && !strncmp(strrchr(m_szFullDestName, '\\') + 1, rfbZipDirectoryPrefix, strlen(rfbZipDirectoryPrefix)))
	{
		char szPath[MAX_PATH + MAX_PATH];
		char szDirName[MAX_PATH]; // Todo: improve this (size) 
		strcpy(szPath, m_szFullDestName);
		// Todo: improve all this (p, p2, p3 NULL test or use a standard substring extraction function)
		char *p = strrchr(szPath, '\\') + 1; 
		char *p2 = strchr(p, '-') + 1; // rfbZipDirectoryPrefix MUST have a "-" at the end...
		strcpy(szDirName, p2);
		char *p3 = strrchr(szDirName, '.');
		*p3 = '\0';
		if (p != NULL) *p = '\0';
		strcat(szPath, szDirName);

		// Create the Directory
		// BOOL fRet = CreateDirectory(szPath, NULL);
		m_pZipUnZip->UnZipDirectory(szPath, m_szFullDestName);
		DeleteFile(m_szFullDestName);
	}
	*/

    if (m_fFileDownloadError && m_fUserAbortedFileTransfer)
    {
        SplitTransferredFileNameAndDate(m_szFullDestName, 0);
        ::DeleteFile(m_szFullDestName);
        FTDownloadCancelledHook();
    }
    else
    {
        std::string realName = get_real_filename(m_szFullDestName);
        if (!m_fFileDownloadError && !bWasDir)
            m_fFileDownloadError = !replaceFile(m_szFullDestName, realName.c_str());
        if (m_fFileDownloadError)
            FTDownloadFailureHook();
        else
            FTDownloadCompleteHook();
    }
	//delete [] m_szFullDestName;

	if (m_pCompBuff != NULL)
		delete [] m_pCompBuff;
	if (m_pBuff != NULL)
		delete [] m_pBuff;


	return;
}



bool vncClient::SendFileChunk()
{
    bool connected = true;
	omni_mutex_lock l(GetUpdateLock());

	if (!m_fFileUploadRunning) return connected;
	if ( m_fEof || m_fFileUploadError)
	{
		FinishFileSending();
		return connected;
	}

	int nRes = ReadFile(m_hSrcFile, m_pBuff, sz_rfbBlockSize, &m_dwNbBytesRead, NULL);
	if (!nRes && m_dwNbBytesRead != 0)
	{
		m_fFileUploadError = true;
	}

	if (nRes && m_dwNbBytesRead == 0)
	{
		m_fEof = true;
	}
	else
	{
		// sf@2004 - Delta Transfer
		bool fAlreadyThere = false;
		unsigned long nCS = 0;
		// if Checksums are available for this file
		if (m_lpCSBuffer != NULL)
		{
			if (m_nCSOffset < m_nCSBufferSize)
			{
				memcpy(&nCS, &m_lpCSBuffer[m_nCSOffset], 4);
				if (nCS != 0)
				{
					m_nCSOffset += 4;
					unsigned long cs = adler32(0L, Z_NULL, 0);
					cs = adler32(cs, (unsigned char*)m_pBuff, (int)m_dwNbBytesRead);
					if (cs == nCS)
						fAlreadyThere = true;
				}
			}
		}

		if (fAlreadyThere)
		{
			// Send the FileTransferMsg with empty rfbFilePacket
			rfbFileTransferMsg ft;
			ft.type = rfbFileTransfer;
			ft.contentType = rfbFilePacket;
			ft.size = Swap32IfLE(2); // Means "Empty packet"// Swap32IfLE(nCS); 
			ft.length = Swap32IfLE(m_dwNbBytesRead);
			m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
		}
		else
		{
			// Compress the data
			// (Compressed data can be longer if it was already compressed)
			unsigned int nMaxCompSize = sz_rfbBlockSize + 1024; // TODO: Improve this...
			bool fCompressed = false;
			if (m_fCompressionEnabled)
			{
				int nRetC = compress((unsigned char*)(m_pCompBuff),
									(unsigned long *)&nMaxCompSize,	
									(unsigned char*)m_pBuff,
									m_dwNbBytesRead
									);

				if (nRetC != 0)
				{
					vnclog.Print(LL_INTINFO, VNCLOG("Compress returned error in File Send :%d\n"), nRetC);
					// Todo: send data uncompressed instead
					m_fFileUploadError = true;
					FinishFileSending();
					return connected;
				}
				fCompressed = true;
			}

			// Test if we have to deal with already compressed data
			if (nMaxCompSize > m_dwNbBytesRead)
				fCompressed = false;
				// m_fCompressionEnabled = false;

			rfbFileTransferMsg ft;

			ft.type = rfbFileTransfer;
			ft.contentType = rfbFilePacket;
			ft.size = fCompressed ? Swap32IfLE(1) : Swap32IfLE(0); 
			ft.length = fCompressed ? Swap32IfLE(nMaxCompSize) : Swap32IfLE(m_dwNbBytesRead);
            connected = m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer) == VTrue;
            if (connected) {
			if (fCompressed)
				m_socket->SendExact((char *)m_pCompBuff , nMaxCompSize);
			else
				m_socket->SendExact((char *)m_pBuff , m_dwNbBytesRead);
			}
            }
		
		m_dwTotalNbBytesRead += m_dwNbBytesRead;
		// TODO : test on nb of bytes written
	}
					
    if (connected)
    {
	// Order next asynchronous packet sending
 	PostToWinVNC( FileTransferSendPacketMessage, (WPARAM)this, (LPARAM)0);
    }
    return connected;
}


void vncClient::FinishFileSending()
{
	omni_mutex_lock l(GetUpdateLock());

	if (!m_fFileUploadRunning)
		return;

    // restore original timeout
	m_socket->SetSendTimeout(m_server->AutoIdleDisconnectTimeout()*1000);

	m_fFileUploadRunning = false;

	rfbFileTransferMsg ft;

	// File Copy OK
	if ( !m_fFileUploadError /*nRet == 1*/)
	{
		ft.type = rfbFileTransfer;
		ft.contentType = rfbEndOfFile;
		m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
	}
	else // Error in file copy
	{
		// TODO : send an error msg to the client...
		ft.type = rfbFileTransfer;
		ft.contentType = rfbAbortFileTransfer;
		m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);
	}
	
	helper::close_handle(m_hSrcFile);
	if (m_pBuff != NULL)
		delete [] m_pBuff;
	if (m_pCompBuff != NULL)
		delete [] m_pCompBuff;

	// sf@2003 - Directory Transfer trick
	// If the transfered file is a Directory zip, we delete it locally, whatever the result of the transfer
	if (!strncmp(strrchr(m_szSrcFileName, '\\') + 1, rfbZipDirectoryPrefix, strlen(rfbZipDirectoryPrefix)))
	{
		char *p = strrchr(m_szSrcFileName, ',');
		if (p != NULL) *p = '\0'; // Remove the time stamp we've added above from the file name
		DeleteFile(m_szSrcFileName);
	}
    //  jdp 8/8/2008
    if (m_fUserAbortedFileTransfer)
        FTUploadCancelledHook();
    else if (m_fFileUploadError)
        FTUploadFailureHook();
    else
        FTUploadCompleteHook();

}


bool vncClient::GetSpecialFolderPath(int nId, char* szPath)
{
	LPITEMIDLIST pidl;
    bool retval = false;

    LPMALLOC pSHMalloc;

    if (FAILED(SHGetMalloc(&pSHMalloc)))
        return false;

	if (SHGetSpecialFolderLocation(0, nId, &pidl) == NOERROR)
    {
        retval = SHGetPathFromIDList(pidl, szPath) ? true : false;

        pSHMalloc->Free(pidl);
    }

    pSHMalloc->Release();

	return retval;
}

//
// Zip a possible directory
//
int vncClient::ZipPossibleDirectory(LPSTR szSrcFileName)
{
//	vnclog.Print(0, _T("ZipPossibleDirectory\n"));
	char* p1 = strrchr(szSrcFileName, '\\') + 1;
	char* p2 = strrchr(szSrcFileName, rfbDirSuffix[0]);
	if (
		p1[0] == rfbDirPrefix[0] && p1[1] == rfbDirPrefix[1]  // Check dir prefix
		&& p2[1] == rfbDirSuffix[1] && p2 != NULL && p1 < p2  // Check dir suffix
		) //
	{
		// sf@2004 - Improving Directory Transfer: Avoids ReadOnly media problem
		char szDirZipPath[MAX_PATH];
		char szWorkingDir[MAX_PATH];
		if (GetModuleFileName(NULL, szWorkingDir, MAX_PATH))
		{
			char* p = strrchr(szWorkingDir, '\\');
			if (p == NULL)
				return -1;
			*(p+1) = '\0';
		}
		else
			return -1;

		char szPath[MAX_PATH];
		char szDirectoryName[MAX_PATH];
		strcpy(szPath, szSrcFileName);
		p1 = strrchr(szPath, '\\') + 1;
		strcpy(szDirectoryName, p1 + 2); // Skip dir prefix (2 chars)
		szDirectoryName[strlen(szDirectoryName) - 2] = '\0'; // Remove dir suffix (2 chars)
		*p1 = '\0';
        m_OrigSourceDirectoryName = std::string(szPath) + szDirectoryName;
		if ((strlen(szPath) + strlen(rfbZipDirectoryPrefix) + strlen(szDirectoryName) + 4) > (MAX_PATH - 1)) return -1;
		sprintf(szDirZipPath, "%s%s%s%s", szWorkingDir, rfbZipDirectoryPrefix, szDirectoryName, ".zip"); 
		strcat(szPath, szDirectoryName);
		strcpy(szDirectoryName, szPath);
		if (strlen(szDirectoryName) > (MAX_PATH - 4)) return -1;
		strcat(szDirectoryName, "\\*.*");
		bool fZip = m_pZipUnZip->ZipDirectory(szPath, szDirectoryName, szDirZipPath, true);
		if (!fZip) return -1;
		strcpy(szSrcFileName, szDirZipPath);
		return 1;
	}
	else
		return 0;
}


int vncClient::CheckAndZipDirectoryForChecksuming(LPSTR szSrcFileName)
{
	if (!m_fFileDownloadError 
		&& 
		!strncmp(strrchr(szSrcFileName, '\\') + 1, rfbZipDirectoryPrefix, strlen(rfbZipDirectoryPrefix))
	   )
	{
		char szPath[MAX_PATH + MAX_PATH];
		char szDirName[MAX_PATH];
		char szDirectoryName[MAX_PATH * 2];
		strcpy(szPath, szSrcFileName);
		char *p = strrchr(szPath, '\\') + 1; 
		char *p2 = strchr(p, '-') + 1;
		strcpy(szDirName, p2);
		char *p3 = strrchr(szDirName, '.');
		*p3 = '\0';
		if (p != NULL) *p = '\0';
		strcat(szPath, szDirName);

		int nRes = CreateDirectory(szPath, NULL);
		DWORD err = GetLastError(); // debug
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			strcpy(szDirectoryName, szPath);
			// p = strrchr(szPath, '\\') + 1; 
			// if (p != NULL) *p = '\0'; else return -1;
			if (strlen(szDirectoryName) > (MAX_PATH - 4)) return -1;
			strcat(szDirectoryName, "\\*.*");
			bool fZip = m_pZipUnZip->ZipDirectory(szPath, szDirectoryName, szSrcFileName, true);
			if (!fZip) return -1;
		}

	}		
	return 0;
}

//
// Unzip possible directory
// Todo: handle unzip error correctly...
//
bool vncClient::UnzipPossibleDirectory(LPSTR szFileName)
{
//	vnclog.Print(0, _T("UnzipPossibleDirectory\n"));
	if (!m_fFileDownloadError 
		&& 
		!strncmp(strrchr(szFileName, '\\') + 1, rfbZipDirectoryPrefix, strlen(rfbZipDirectoryPrefix))
	   )
	{
		char szPath[MAX_PATH + MAX_PATH];
		char szDirName[MAX_PATH]; // Todo: improve this (size) 
		strcpy(szPath, szFileName);
		// Todo: improve all this (p, p2, p3 NULL test or use a standard substring extraction function)
		char *p = strrchr(szPath, '\\') + 1; 
		char *p2 = strchr(p, '-') + 1; // rfbZipDirectoryPrefix MUST have a "-" at the end...
		strcpy(szDirName, p2);
		char *p3 = strrchr(szDirName, '.');
		*p3 = '\0';
		if (p != NULL) *p = '\0';
		strcat(szPath, szDirName);
		// Create the Directory
		bool fUnzip = m_pZipUnZip->UnZipDirectory(szPath, szFileName);
		DeleteFile(szFileName);
        return true;
	}						
	return false;
}


//
// GetFileSize() doesn't handle files > 4GBytes...
// GetFileSizeEx() doesn't exist under Win9x...
// So let's write our own function.
// 
bool vncClient::MyGetFileSize(char* szFilePath, ULARGE_INTEGER *n2FileSize)
{
	WIN32_FIND_DATA fd;
	HANDLE ff;

    DWORD errmode = SetErrorMode(SEM_FAILCRITICALERRORS); // No popup please !
	ff = FindFirstFile(szFilePath, &fd);
	SetErrorMode( errmode );

	if (ff == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	FindClose(ff);

	(*n2FileSize).LowPart = fd.nFileSizeLow;
	(*n2FileSize).HighPart = fd.nFileSizeHigh;
	(*n2FileSize).QuadPart = (((__int64)fd.nFileSizeHigh) << 32 ) + fd.nFileSizeLow;
	
	return true;
}


bool vncClient::DoFTUserImpersonation()
{
	vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::DoFTUserImpersonation - Call\n"));
	omni_mutex_lock l(GetUpdateLock());

	if (m_fFileDownloadRunning) return true;
	if (m_fFileUploadRunning) return true;
	if (m_fFTUserImpersonatedOk) return true;
    // if we're already impersonating the user and have a session open, do nothing.
    if (m_fFileSessionOpen && m_fFTUserImpersonatedOk)
        return true;

	vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::DoFTUserImpersonation - 1\n"));
	bool fUserOk = true;

	if (vncService::IsWSLocked())
	{
		m_fFTUserImpersonatedOk = false;
		vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::DoFTUserImpersonation - WSLocked\n"));
		return false;
	}

	char username[UNLEN+1];
	vncService::CurrentUser((char *)&username, sizeof(username));
	vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::DoFTUserImpersonation - currentUser = %s\n"), username);
	if (strcmp(username, "") != 0)
	{
		// sf@2007 - New method to achieve FTUserImpersonation - Still needs to be further tested...
		HANDLE hProcess, hPToken;
		DWORD pid = GetExplorerLogonPid();
		if (pid != 0) 
		{
			hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, pid);
			if (!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY
									|TOKEN_DUPLICATE|TOKEN_ASSIGN_PRIMARY|TOKEN_ADJUST_SESSIONID
									|TOKEN_READ|TOKEN_WRITE,&hPToken
								 )
			) 
			{
				vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::DoFTUserImpersonation - OpenProcessToken Error\n"));
				fUserOk = false;
			}
			else
			{
				if (!ImpersonateLoggedOnUser(hPToken))
				{
					vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::DoFTUserImpersonation - ImpersonateLoggedOnUser Failed\n"));
					fUserOk = false;
				}
			}

			CloseHandle(hProcess);
			CloseHandle(hPToken);
		}
		
		/* Old method
		// Modif Byteboon (Jeremy C.) - Impersonnation
		if (m_server->m_impersonationtoken)
		{
			vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::DoFTUserImpersonation - Impersonationtoken exists\n"));
			HANDLE newToken;
			if (DuplicateToken(m_server->m_impersonationtoken, SecurityImpersonation, &newToken))
			{
				vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::DoFTUserImpersonation - DuplicateToken ok\n"));
				if(!ImpersonateLoggedOnUser(newToken))
				{
					vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::DoFTUserImpersonation - failed to impersonate [%d]\n"),GetLastError());
					fUserOk = false;
				}
				CloseHandle(newToken);
			}
			else
			{
				vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::DoFTUserImpersonation - DuplicateToken FAILEDk\n"));
				fUserOk = false;
			}
		}
		else
		{
			vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::DoFTUserImpersonation - No impersonationtoken\n"));
			fUserOk = false;
		}
		*/
		
		if (!vncService::RunningAsService())
			fUserOk = true;
	}
	else
	{
		fUserOk = false;
	}

	if (fUserOk)
		m_lLastFTUserImpersonationTime = timeGetTime();

	m_fFTUserImpersonatedOk = fUserOk;

	return fUserOk;
			
}


void vncClient::UndoFTUserImpersonation()
{
	//vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::UNDoFTUserImpersonation - Call\n"));
	omni_mutex_lock l(GetUpdateLock());

	if (!m_fFTUserImpersonatedOk) return;
	if (m_fFileDownloadRunning) return;
	if (m_fFileUploadRunning) return;
    if (m_fFileSessionOpen) return;

	vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::UNDoFTUserImpersonation - 1\n"));
	DWORD lTime = timeGetTime();
	if (lTime - m_lLastFTUserImpersonationTime < 10000) return;
	vnclog.Print(LL_INTERR, VNCLOG("%%%%%%%%%%%%% vncClient::UNDoFTUserImpersonation - Impersonationtoken exists\n"));
	RevertToSelf();
	m_fFTUserImpersonatedOk = false;
}

// 10 April 2008 jdp paquette@atnetsend.net
void vncClient::SendServerStateUpdate(CARD32 state, CARD32 value)
{
    if (m_wants_ServerStateUpdates && m_socket)
    {
        // send message to client
		rfbServerStateMsg rsmsg;
        memset(&rsmsg, 0, sizeof rsmsg);
		rsmsg.type = rfbServerState;
		rsmsg.state  = Swap32IfLE(state);
		rsmsg.value = Swap32IfLE(value);

		m_socket->SendExact((char*)&rsmsg, sz_rfbServerStateMsg, rfbServerState);
    }
}
void vncClient::SendKeepAlive(bool bForce)
{
    if (m_wants_KeepAlive && m_socket)
    {
        static time_t lastSent = 0;
        time_t now = time(&now);
        time_t delta = now - lastSent;
        if (!bForce && delta < m_server->GetKeepAliveInterval())
            return;

        lastSent = now;

        rfbKeepAliveMsg kp;
        memset(&kp, 0, sizeof kp);
        kp.type = rfbKeepAlive;

		m_socket->SendExact((char*)&kp, sz_rfbKeepAliveMsg, rfbKeepAlive);
    }
}

void vncClient::SendFTProtocolMsg()
{
    rfbFileTransferMsg ft;
    memset(&ft, 0, sizeof ft);
    ft.type = rfbFileTransfer;
    ft.contentType = rfbFileTransferProtocolVersion;
    ft.contentParam = FT_PROTO_VERSION_3;
    m_socket->SendExact((char *)&ft, sz_rfbFileTransferMsg, rfbFileTransfer);

}
