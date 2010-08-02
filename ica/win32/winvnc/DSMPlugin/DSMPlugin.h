/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002 Ultr@Vnc Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
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
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://ultravnc.sourceforge.net/
////////////////////////////////////////////////////////////////////////////
//
// DSMPlugin.h: interface for the CDSMPlugin class.
//
//////////////////////////////////////////////////////////////////////
//
//  (2009)
//  Multithreaded DSM plugin framework created by Adam D. Walling aka adzm

#if !defined(CDSMPlugin_H)
#define CDSMPlugin_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#ifdef UNDER_CE
#include "omnithreadce.h"
#else
#include "omnithread.h"
#endif

#include "windows.h"

//adzm - 2009-06-21
class IPlugin
{
public:
	virtual ~IPlugin() {};

	virtual BYTE* TransformBuffer(BYTE* pDataBuffer, int nDataLen, int* pnTransformedDataLen) = 0;
	virtual BYTE* RestoreBuffer(BYTE* pTransBuffer, int nTransDataLen, int* pnRestoredDataLen) = 0;
};

class IIntegratedPlugin : public IPlugin
{
public:
	virtual ~IIntegratedPlugin() {};

	// Free memory allocated by the plugin
	virtual void FreeMemory(void* pMemory) = 0;

	// Get the last error string
	virtual LPCSTR GetLastErrorString() = 0; // volatile, must be copied or may be invalidated

	// Describe the current encryption settings
	virtual LPCSTR DescribeCurrentSettings() = 0; // volatile, must be copied or may be invalidated

	// Set handshake complete and start to transform/restore buffers
	virtual void SetHandshakeComplete() = 0;

	// Helper methods to decrypt or encrypt an arbitrary array of bytes with a given passphrase
	virtual bool EncryptBytesWithKey(const BYTE* pPlainData, int nPlainDataLength, const BYTE* pPassphrase, int nPassphraseLength, BYTE*& pEncryptedData, int& nEncryptedDataLength, bool bIncludeHash) = 0;
	virtual bool DecryptBytesWithKey(const BYTE* pEncryptedData, int nEncryptedDataLength, const BYTE* pPassphrase, int nPassphraseLength, BYTE*& pPlainData, int& nPlainDataLength, bool bIncludeHash) = 0;

	// server
	virtual void SetServerIdentification(const BYTE* pIdentification, int nLength) = 0;
	virtual void SetServerOptions(LPCSTR szOptions) = 0;
	virtual void SetPasswordData(const BYTE* pPasswordData, int nLength) = 0;
	virtual bool GetChallenge(BYTE*& pChallenge, int& nChallengeLength, int nSequenceNumber) = 0;
	virtual bool HandleResponse(const BYTE* pResponse, int nResponseLength, int nSequenceNumber, bool& bSendChallenge) = 0;

	// client
	virtual void SetViewerOptions(LPCSTR szOptions) = 0;
	virtual bool HandleChallenge(const BYTE* pChallenge, int nChallengeLength, int nSequenceNumber, bool& bPasswordOK, bool& bPassphraseRequired) = 0;
	virtual bool GetResponse(BYTE*& pResponse, int& nResponseLength, int nSequenceNumber, bool& bExpectChallenge) = 0;

};

// A plugin dll must export the following functions (with same convention)
typedef char* (__cdecl  *DESCRIPTION)(void);
typedef int   (__cdecl  *STARTUP)(void);
typedef int   (__cdecl  *SHUTDOWN)(void);
typedef int   (__cdecl  *SETPARAMS)(HWND, char*);
typedef char* (__cdecl  *GETPARAMS)(void);
typedef BYTE* (__cdecl  *TRANSFORMBUFFER)(BYTE*, int, int*);
typedef BYTE* (__cdecl  *RESTOREBUFFER)(BYTE*, int, int*);
typedef void  (__cdecl  *FREEBUFFER)(BYTE*);
typedef int   (__cdecl  *RESET)(void);
//adzm - 2009-06-21
typedef IPlugin* (__cdecl  *CREATEPLUGININTERFACE)(void);
//adzm 2010-05-10
typedef IIntegratedPlugin* (__cdecl  *CREATEINTEGRATEDPLUGININTERFACE)(void);
//adzm 2010-05-12 - dsmplugin config
typedef int   (__cdecl  *CONFIG)(HWND, char*, char*, char**);

//
//
//
class CDSMPlugin  
{
public:
	void SetLoaded(bool fEnable);
	bool IsLoaded(void) { return m_fLoaded; };
	void SetEnabled(bool fEnable);
	bool IsEnabled(void) { return m_fEnabled; };
	bool InitPlugin(void);
	//adzm 2010-05-12 - dsmplugin config
	bool SetPluginParams(HWND hWnd, char* szParams, char* szConfig = NULL, char** szNewConfig = NULL);
	char* GetPluginParams(void);
	char* DescribePlugin(void);
	int  ListPlugins(HWND hComboBox);
	bool LoadPlugin(char* szPlugin, bool fAllowMulti);
	bool UnloadPlugin(void); // Could be private
	BYTE* TransformBuffer(BYTE* pDataBuffer, int nDataLen, int* nTransformedDataLen);
	BYTE* RestoreBufferStep1(BYTE* pDataBuffer, int nDataLen, int* nRestoredDataLen);
	BYTE* RestoreBufferStep2(BYTE* pDataBuffer, int nDataLen, int* nRestoredDataLen);
	void RestoreBufferUnlock();
	char* GetPluginName(void) { return m_szPluginName;} ;
	char* GetPluginVersion(void)  {  return m_szPluginVersion;} ;
	char* GetPluginDate(void) { return m_szPluginDate; } ;
	char* GetPluginAuthor(void) { return m_szPluginAuthor;} ;
	char* GetPluginFileName(void) { return m_szPluginFileName;} ;

	//adzm - 2009-06-21
	IPlugin* CreatePluginInterface();
	bool SupportsMultithreaded();

	//adzm 2010-05-10
	IIntegratedPlugin* CreateIntegratedPluginInterface();
	bool SupportsIntegrated();

	CDSMPlugin();
	virtual ~CDSMPlugin();
	bool ResetPlugin(void);

	long m_lPassLen; 

	omni_mutex m_RestMutex;

private:
	bool m_fLoaded;
	bool m_fEnabled;

	char szPassword[64];

	char m_szPluginName[128]; // Name of the plugin and very short description
	char m_szPluginVersion[16];
	char m_szPluginDate[16];
	char m_szPluginAuthor[64];
	char m_szPluginFileName[128]; // No path, just the filename and possible comment

	HMODULE m_hPDll;
	char m_szDllName[MAX_PATH];

	// Plugin's functions pointers when loaded
	DESCRIPTION     m_PDescription;
	SHUTDOWN		m_PShutdown;
	STARTUP			m_PStartup;
	SETPARAMS		m_PSetParams;
	GETPARAMS		m_PGetParams;
	TRANSFORMBUFFER m_PTransformBuffer;
	RESTOREBUFFER	m_PRestoreBuffer;
	FREEBUFFER		m_PFreeBuffer;
	RESET			m_PReset;
	//adzm - 2009-06-21
	CREATEPLUGININTERFACE m_PCreatePluginInterface;
	//adzm 2010-05-10
	CREATEINTEGRATEDPLUGININTERFACE m_PCreateIntegratedPluginInterface;
	//adzm 2010-05-12 - dsmplugin config
	CONFIG m_PConfig;


	//adzm - 2009-06-21 - Please do not use these! Deprecated with multithreaded DSM
	BYTE* m_pTransBuffer;
	BYTE* m_pRestBuffer;

	omni_mutex m_TransMutex;
	// omni_mutex m_RestMutex;
};

#endif
