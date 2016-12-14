/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2013 UltraVNC Team Members. All Rights Reserved.
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
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////
//
//
// DSMPlugin.cpp: implementation of the CDSMPlugin class.
//
//////////////////////////////////////////////////////////////////////
//
// This class is the interface between UltraVNC and plugins 
// (third party dlls) that may be developed (written, exported and 
// provided by authorized individuals - according to the law of their 
// country) to alter/modify/process/encrypt rfb data streams between 
// vnc viewer and vnc server.
//
// The goal here is not to design and develop an extensive, versatile
// and powerfull plugin system but to provide people a way
// to easely customize the VNC communication data between client and server.
//
// It handles the following tasks:
// 
// - Listing of all plugins found in the vnc directory (dlls, with ".dsm" extension)
//
// - Loading of a given plugin
//
// - Interface between vnc and the plugin functions: 
//   - Init()               Initialize the plugin
//   - SetParams()          Set Password, or key or other. If more than one param, uses ',' as separator
//   - GetParams()          Give Password, or key or other. If more than one param, uses ',' as separator
//   - DescribePlugin()     Give the Plugin ID string (Name, Author, Date, Version, FileName)
//   - TransformBuffer()    Tell the plugin to do it's transformation against the data in the buffer
//   - RestoreBuffer()      Tell the plugin to restore data to its original state   
//   - Shutdown()           Cleanup and shutdown the plugin
//   - FreeBuffer()         Free a buffer used in the plugin
//	 - Reset()
//
// - Unloading of the current loaded plugin
//
//  (2009)
//  Multithreaded DSM plugin framework created by Adam D. Walling aka adzm
// - Creating a new (threadsafe) instance of a DSM plugin encryptor/decryptor
//   - CreatePluginInterface -- returns a pointer to a new IPlugin-derived class,
//       which is then used to transform and restore buffers in a threadsafe manner.
// 
// WARNING: For the moment, only ONE instance of this class must exist in Vncviewer and WinVNC
// Consequently, WinVNc will impose all its clients to use the same plugin. Maybe we'll 
// improve that soon. It depends on the demand/production of DSM plugins.

#include <winsock2.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include "DSMPlugin.h"

#include <stdlib.h>
#include <limits.h>
//
// Utils
//
BOOL MyStrToken(LPSTR szToken, LPSTR lpString, int nTokenNum, char cSep)
{
	int i = 1;
	while (i < nTokenNum)
	{
		while ( *lpString && (*lpString != cSep) &&(*lpString != '\0'))
		{
			lpString++;
		}
		i++;
		lpString++;
	}
	while ((*lpString != cSep) && (*lpString != '\0'))
	{
		*szToken = *lpString;
		szToken++;
		lpString++;
	}
	*szToken = '\0' ;
	if (( ! *lpString ) || (! *szToken)) return NULL;
	return FALSE;
}



//
//
//
CDSMPlugin::CDSMPlugin()
{
	m_fLoaded = false;
	m_fEnabled = false;
	m_lPassLen = 0;

	m_pTransBuffer = NULL;
	m_pRestBuffer = NULL;

	sprintf(m_szPluginName, "Unknown");
	sprintf(m_szPluginVersion, "0.0.0");
	sprintf(m_szPluginDate, "12-12-2002");
	sprintf(m_szPluginAuthor, "Someone");
	sprintf(m_szPluginFileName, "Plugin.dsm"); // No path, just the filename

	m_hPDll = NULL;

	// Plugin's functions pointers init
	/*
	STARTUP			m_PStartup = NULL;
	SHUTDOWN		m_PShutdown = NULL;
	SETPARAMS		m_PSetParams = NULL;
	GETPARAMS		m_PGetParams = NULL;
	TRANSFORMBUFFER m_PTransformBuffer = NULL;
	RESTOREBUFFER	m_PRestoreBuffer = NULL;
	TRANSFORMBUFFER	m_PFreeBuffer = NULL;
	RESET			m_PReset = NULL;
	//adzm - 2009-06-21
	CREATEPLUGININTERFACE	m_PCreatePluginInterface = NULL;
	*/

	//adzm - 2009-07-05
	// Plugin's functions pointers init
	m_PDescription = NULL;
	m_PStartup = NULL;
	m_PShutdown = NULL;
	m_PSetParams = NULL;
	m_PGetParams = NULL;
	m_PTransformBuffer = NULL;
	m_PRestoreBuffer = NULL;
	m_PFreeBuffer = NULL;
	m_PReset = NULL;
	//adzm - 2009-06-21
	m_PCreatePluginInterface = NULL;
	//adzm 2010-05-10
	m_PCreateIntegratedPluginInterface = NULL;
	//adzm 2010-05-12 - dsmplugin config
	m_PConfig = NULL;
}

//
//
//
CDSMPlugin::~CDSMPlugin()
{
	// TODO: Log events
	if (IsLoaded())
		UnloadPlugin();
}


//
//
//
void CDSMPlugin::SetEnabled(bool fEnable)
{
	m_fEnabled =  fEnable;
}


//
//
//
void CDSMPlugin::SetLoaded(bool fEnable)
{
	m_fLoaded =  fEnable;
}


//
//
//
char* CDSMPlugin::DescribePlugin(void)
{
	// TODO: Log events
	char* szDescription = NULL;
	if (m_PDescription)
	{
		 szDescription = (*m_PDescription)();
		 if (szDescription != NULL)
		 {
			 //adzm 2010-05-10 - this was inconsistent with the way the plugins are written
			MyStrToken(m_szPluginName, szDescription, 1, ',');
			MyStrToken(m_szPluginVersion, szDescription, 2, ',');
			MyStrToken(m_szPluginDate, szDescription, 3, ',');
			MyStrToken(m_szPluginAuthor, szDescription, 4, ',');
			MyStrToken(m_szPluginFileName, szDescription, 5, ',');
		 }
		 return szDescription;
	}

	else 
		return "No Plugin loaded";
}


//
// Init the DSMPlugin system
//
bool CDSMPlugin::InitPlugin(void)
{
	// TODO: Log events
	int nRes = (*m_PStartup)();
	if (nRes < 0) return false;
	else return true;
}

//
// Reset the DSMPlugin
//
bool CDSMPlugin::ResetPlugin(void)
{
	// TODO: Log events
	int nRes = (*m_PReset)();
	if (nRes < 0) return false;
	else return true;
}


//
// szParams is the key (or password) that is transmitted to the loaded DSMplugin
//
bool CDSMPlugin::SetPluginParams(HWND hWnd, char* szParams, char* szConfig, char** pszNewConfig)
{
	// TODO: Log events

	//adzm 2010-05-12 - dsmplugin config
	if (m_PConfig) {
		char* szNewConfig = NULL;
		int nRes = (*m_PConfig)(hWnd, szParams, szConfig, pszNewConfig);		
		if (nRes > 0) return true; else return false;
	} else {
		int nRes = (*m_PSetParams)(hWnd, szParams);
		if (nRes > 0) return true; else return false;
	}
}


//
// Return the loaded DSMplugin current param(s)
//
char* CDSMPlugin::GetPluginParams(void)
{
	// 
	return (*m_PGetParams)();

}


//
// List all the plugins is the current APP directory in the given ComboBox
//
int CDSMPlugin::ListPlugins(HWND hComboBox)
{
	// TODO: Log events
	WIN32_FIND_DATA fd;
	HANDLE ff;
	int fRet = 1;
	int nFiles = 0;
	char szCurrentDir[MAX_PATH];

	if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
	{
		char* p = strrchr(szCurrentDir, '\\');
		if (p == NULL)
			return 0;
		*p = '\0';
	}
	else
		return 0;
	// MessageBoxSecure(NULL, szCurrentDir, "Current directory", MB_OK);

    if (szCurrentDir[strlen(szCurrentDir) - 1] != '\\') strcat(szCurrentDir, "\\");
	strcat(szCurrentDir, "*.dsm"); // The DSMplugin dlls must have this extension
	
	ff = FindFirstFile(szCurrentDir, &fd);
	if (ff == INVALID_HANDLE_VALUE)
	{
		// Todo: Log error here
		return 0;
	}

	while (fRet != 0)
	{
		SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)(fd.cFileName)); 
		nFiles++;
		fRet = FindNextFile(ff, &fd);
	}

	FindClose(ff);

	return nFiles;
}



//
// Load the given DSMplugin and map its functions
//
bool CDSMPlugin::LoadPlugin(char* szPlugin, bool fAllowMulti)
{
	// sf@2003 - Multi dll trick 
	// I really don't like doing this kind of dirty workaround but I have no time to do
	// better for now. Used only by a listening viewer.
	// Create a numbered temporary copy of the original plugin dll and load it (viewer only, for now)
	if (fAllowMulti)
	{
		bool fDllCopyCreated = false;
		int i = 1;
		char szDllCopyName[MAX_PATH];
		char szCurrentDir[MAX_PATH];
		char szCurrentDir_szPlugin[MAX_PATH];
		char szCurrentDir_szDllCopyName[MAX_PATH];
		while (!fDllCopyCreated)
		{
			strcpy_s(szDllCopyName, 260,szPlugin);
			szDllCopyName[strlen(szPlugin) - 4] = '\0'; //remove the ".dsm" extension
			sprintf(szDllCopyName, "%s-tmp.d%d", szDllCopyName, i++);
			//fDllCopyCreated = (FALSE != CopyFile(szPlugin, szDllCopyName, false));
			// Note: Let's be really dirty; Overwrite if it's possible only (dll not loaded). 
			// This way if for some reason (abnormal process termination) the dll wasn't previously 
			// normally deleted we overwrite/clean it with the new one at the same time.
			//strcpy (szCurrentDir_szDllCopyName,szDllCopyName);
			//DWORD error=GetLastError();
			//if (error==2)
			{
				if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
					{
						char* p = strrchr(szCurrentDir, '\\');
						*p = '\0';
					}
				char lpPathBuffer[MAX_PATH];
				DWORD dwBufSize=MAX_PATH;
				DWORD dwRetVal;
				dwRetVal = GetTempPath(dwBufSize,lpPathBuffer);
				if (dwRetVal > dwBufSize || (dwRetVal == 0))
				{
					strcpy (lpPathBuffer,szCurrentDir);
				}
				else
				{
					
				}
				
				strcpy (szCurrentDir_szPlugin,szCurrentDir);
				strcat (szCurrentDir_szPlugin,"\\");
				strcat (szCurrentDir_szPlugin,szPlugin);

				strcpy (szCurrentDir_szDllCopyName,lpPathBuffer);
				//strcat (szCurrentDir_szDllCopyName,"\\");
				strcat (szCurrentDir_szDllCopyName,szDllCopyName);
				fDllCopyCreated = (FALSE != CopyFile(szCurrentDir_szPlugin, szCurrentDir_szDllCopyName, false));
			}
			if (i > 99) break; // Just in case...
		}
		strcpy(m_szDllName, szCurrentDir_szDllCopyName);
		m_hPDll = LoadLibrary(m_szDllName);
	}
	else // Use the original plugin dll
	{
		ZeroMemory(m_szDllName, strlen(m_szDllName));
		m_hPDll = LoadLibrary(szPlugin);
		//Try current PATH
		if (m_hPDll==NULL)
		{
			char szCurrentDir[MAX_PATH]="";
			char szCurrentDir_szPlugin[MAX_PATH]="";
			if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
				{
					char* p = strrchr(szCurrentDir, '\\');
					*p = '\0';
				}
			strcpy (szCurrentDir_szPlugin,szCurrentDir);
			strcat (szCurrentDir_szPlugin,"\\");
			strcat (szCurrentDir_szPlugin,szPlugin);
			m_hPDll = LoadLibrary(szCurrentDir_szPlugin);
		}
	}

	if (m_hPDll == NULL) return false;

	m_PDescription     = (DESCRIPTION)     GetProcAddress(m_hPDll, "Description");
	m_PStartup         = (STARTUP)         GetProcAddress(m_hPDll, "Startup");
	m_PShutdown        = (SHUTDOWN)        GetProcAddress(m_hPDll, "Shutdown");
	m_PSetParams       = (SETPARAMS)       GetProcAddress(m_hPDll, "SetParams");
	m_PGetParams       = (GETPARAMS)       GetProcAddress(m_hPDll, "GetParams");
	m_PTransformBuffer = (TRANSFORMBUFFER) GetProcAddress(m_hPDll, "TransformBuffer");
	m_PRestoreBuffer   = (RESTOREBUFFER)   GetProcAddress(m_hPDll, "RestoreBuffer");
	m_PFreeBuffer      = (FREEBUFFER)      GetProcAddress(m_hPDll, "FreeBuffer");
	m_PReset           = (RESET)           GetProcAddress(m_hPDll, "Reset");
	//adzm - 2009-06-21
	m_PCreatePluginInterface	= (CREATEPLUGININTERFACE)	GetProcAddress(m_hPDll, "CreatePluginInterface");
	//adzm 2010-05-10
	m_PCreateIntegratedPluginInterface	= (CREATEINTEGRATEDPLUGININTERFACE)	GetProcAddress(m_hPDll, "CreateIntegratedPluginInterface");
	//adzm 2010-05-12 - dsmplugin config
	m_PConfig         = (CONFIG)           GetProcAddress(m_hPDll, "Config");

	if (m_PStartup == NULL || m_PShutdown == NULL || m_PSetParams == NULL || m_PGetParams == NULL
		|| m_PTransformBuffer == NULL || m_PRestoreBuffer == NULL || m_PFreeBuffer == NULL)
	{
		FreeLibrary(m_hPDll); 
		if (*m_szDllName) DeleteFile(m_szDllName);
		return false;
	}

	// return ((*m_PStartup)());
	SetLoaded(true);
	return true;
}


//
// Unload the current DSMPlugin from memory
//
bool CDSMPlugin::UnloadPlugin(void)
{
	// TODO: Log events
	// Force the DSMplugin to free the buffers it allocated
	if (m_pTransBuffer != NULL) (*m_PFreeBuffer)(m_pTransBuffer);
	if (m_pRestBuffer != NULL) (*m_PFreeBuffer)(m_pRestBuffer);
	
	m_pTransBuffer = NULL;
	m_pRestBuffer = NULL;

	SetLoaded(false);

	if ((*m_PShutdown)())
	{
		bool fFreed = false;
		fFreed = (FALSE != FreeLibrary(m_hPDll));
		if (*m_szDllName) DeleteFile(m_szDllName);
		return fFreed;
	}
	else
		return false;
	
}

//adzm - 2009-06-21
IPlugin* CDSMPlugin::CreatePluginInterface()
{
	if (m_PCreatePluginInterface) {
		return m_PCreatePluginInterface();
	} else {
		return NULL;
	}
}

bool CDSMPlugin::SupportsMultithreaded()
{
	//adzm 2010-05-10
	return m_PCreatePluginInterface != NULL || m_PCreateIntegratedPluginInterface != NULL;
}

//adzm 2010-05-10
IIntegratedPlugin* CDSMPlugin::CreateIntegratedPluginInterface()
{
	if (m_PCreateIntegratedPluginInterface) {
		return m_PCreateIntegratedPluginInterface();
	} else {
		return NULL;
	}
}

bool CDSMPlugin::SupportsIntegrated()
{
	return m_PCreateIntegratedPluginInterface != NULL;
}


//
// Tell the plugin to do its transformation on the source data buffer
// Return: pointer on the new transformed buffer (allocated by the plugin)
// nTransformedDataLen is the number of bytes contained in the transformed buffer
//
BYTE* CDSMPlugin::TransformBuffer(BYTE* pDataBuffer, int nDataLen, int* pnTransformedDataLen)
{
	// FixME: possible pb with this mutex in WinVNC
#ifdef _VIEWER
	omni_mutex_lock l(m_TransMutex);
#else
	omni_mutex_lock l(m_TransMutex,105);
#endif

	m_pTransBuffer = (*m_PTransformBuffer)(pDataBuffer, nDataLen, pnTransformedDataLen);

	return m_pTransBuffer;
}


// - If pRestoredDataBuffer = NULL, the plugin check its local buffer and return the pointer
// - Otherwise, restore data contained in its rest. buffer and put the result in pRestoredDataBuffer
//   pnRestoredDataLen is the number bytes put in pRestoredDataBuffers
BYTE* CDSMPlugin::RestoreBufferStep1(BYTE* pRestoredDataBuffer, int nDataLen, int* pnRestoredDataLen)
{
	//m_RestMutex.lock();
	m_pRestBuffer = (*m_PRestoreBuffer)(pRestoredDataBuffer, nDataLen, pnRestoredDataLen);
	return m_pRestBuffer;
}

BYTE* CDSMPlugin::RestoreBufferStep2(BYTE* pRestoredDataBuffer, int nDataLen, int* pnRestoredDataLen)
{
	m_pRestBuffer = (*m_PRestoreBuffer)(pRestoredDataBuffer, nDataLen, pnRestoredDataLen);
	//m_RestMutex.unlock();
	return NULL;
}

void CDSMPlugin::RestoreBufferUnlock()
{
	//m_RestMutex.unlock();
}


const char Base64::cb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
** Translation Table to decode (created by author)
*/
const char Base64::cd64[] = "|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";


void Base64::encodeblock(BYTE in[3], BYTE out[4], int len)
{
    out[0] = cb64[ in[0] >> 2 ];
    out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    out[2] = (BYTE) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (BYTE) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}

void Base64::decodeblock(BYTE in[4], BYTE out[3])
{
    out[ 0 ] = (BYTE) (in[0] << 2 | in[1] >> 4);
    out[ 1 ] = (BYTE) (in[1] << 4 | in[2] >> 2);
    out[ 2 ] = (BYTE) (((in[2] << 6) & 0xc0) | in[3]);
}

void Base64::encode(const char* szIn, char* szOut)
{
    BYTE in[3], out[4];
    int i, len, blocksout = 0;

	const char* pIn = szIn;
	char* pOut = szOut;

    while(*pIn != '\0') {
        len = 0;
        for( i = 0; i < 3; i++ ) {
            in[i] = (BYTE)*pIn;
			if (*pIn != '\0') {
				len++;
				pIn++;
			}
            else {
                in[i] = 0;
            }
        }
        if( len ) {
            encodeblock( in, out, len );
            for( i = 0; i < 4; i++ ) {
				*pOut = (char)out[i];
				pOut++;
            }
        }
    }
	*pOut = '\0';
}

void Base64::decode(const char* szIn, char* szOut)
{
    BYTE in[4], out[3], v, o;
    int i, len;

	const char* pIn = szIn;
	char* pOut = szOut;

    while(*pIn != '\0') {
        for( len = 0, i = 0; i < 4 && *pIn != '\0'; i++ ) {
            v = 0;
			o = 0;
            while( *pIn != '\0' && v == 0 ) {
                v = (BYTE)*pIn;
				o = v;
				pIn++;

                v = (BYTE) ((v < 43 || v > 122) ? 0 : cd64[ v - 43 ]);
                if( v ) {
                    v = (BYTE) ((v == '$') ? 0 : v - 61);
                }
            }
            if( o != '\0' && v != 0) {
                len++;
                //if( v ) {
                    in[ i ] = (BYTE) (v - 1);
                //}
            }
            else {
                in[i] = 0;
            }
        }
        if( len ) {
            decodeblock( in, out );
            for( i = 0; i < len - 1; i++ ) {
				*pOut = (char)out[i];
				pOut++;
            }
        }
    }
	*pOut = '\0';
}


void ConfigHelper::SetConfigHelper(DWORD dwFlags, char* szPassphrase)
{
	m_szConfig = new char[512];
	m_szConfig[0] = '\0';

	char szEncoded[256];
	szEncoded[0] = '\0';
	if (szPassphrase[0] != '\0') {
		Base64::encode(szPassphrase, szEncoded);
	}

	_snprintf_s(m_szConfig, 512 - 1 - 1, _TRUNCATE, "SecureVNC;0;0x%08x;%s", dwFlags, szEncoded);
}

ConfigHelper::ConfigHelper(const char* szConfig)
	: m_szConfig(NULL)
	, m_szPassphrase(NULL)
	, m_dwFlags(0x01 | 0x4000 | 0x00100000)
{
	m_szPassphrase = new char[256];
	m_szPassphrase[0] = '\0';

	if (szConfig == NULL) return;

	const char* szHeader = "SecureVNC;0;";

	if (strncmp(szConfig, "SecureVNC;0;", strlen(szHeader)) != 0) {
		return;
	}
	
	szConfig += strlen(szHeader);

	char* szEnd = NULL;
	DWORD dwFlags = strtoul(szConfig, &szEnd, 16);

	if (dwFlags != ULONG_MAX) {
		m_dwFlags = dwFlags;

		if (szEnd && szEnd != szConfig) {
			//m_szPassphrase = new char[256];
			//m_szPassphrase[0] = '\0';

			char szEncoded[256];
			szEncoded[0] = '\0';

			strcpy_s(szEncoded, 256 - 1, szEnd + 1);

			if (szEncoded[0] != '\0') {
				Base64::decode(szEncoded, m_szPassphrase);
			}
		}
	}
}

ConfigHelper::~ConfigHelper()
{
	if (m_szConfig) {
		delete[] m_szConfig;
		m_szConfig = NULL;
	}

	if (m_szPassphrase) {
		delete[] m_szPassphrase;
		m_szPassphrase = NULL;
	}
}

