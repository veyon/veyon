/*
 * Inject.cpp - functions for injecting code into winlogon.exe for disabling
 *              SAS (Alt+Ctrl+Del)
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "Inject.h"
#include "LocalSystem.h"

#ifdef ITALC_BUILD_WIN32

#include <ctype.h>
#include <windows.h>
#include <tlhelp32.h>



DWORD GetPIDFromName( char *szProcessName )
{
	typedef HANDLE (WINAPI *CREATESNAPSHOT) (DWORD, DWORD);
	typedef BOOL   (WINAPI *PROCESSWALK)    (HANDLE, LPPROCESSENTRY32);

	HINSTANCE       hKernel;
	CREATESNAPSHOT  CreateToolhelp32Snapshot;
	PROCESSWALK     Process32First;
	PROCESSWALK     Process32Next;

	HANDLE          hSnapshot;
	PROCESSENTRY32  pe32;
	BOOL            bRes;
	char            *p;
	DWORD           dwPID = -1;

	// Check szProcessName
	if( !szProcessName )
		return -1;

	// Get Kernel32 handle
	if( !( hKernel = GetModuleHandle( "Kernel32.dll" ) ) )
		return -1;

	// We must link to these functions explicitly.
	// Otherwise it will fail on Windows NT which doesn't have Toolhelp
	// functions defined in Kernel32.
	CreateToolhelp32Snapshot = (CREATESNAPSHOT) GetProcAddress( hKernel,
						"CreateToolhelp32Snapshot" );
	Process32First = (PROCESSWALK) GetProcAddress( hKernel,
							"Process32First" );
	Process32Next = (PROCESSWALK) GetProcAddress( hKernel,
							"Process32Next" );
	if (!CreateToolhelp32Snapshot || !Process32First || !Process32Next)
	{
		FreeLibrary( hKernel );
		SetLastError( ERROR_PROC_NOT_FOUND );
		return -1;
	}

	hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if( hSnapshot == INVALID_HANDLE_VALUE )
		return -1;

	pe32.dwSize = sizeof( pe32 );
	bRes = Process32First( hSnapshot, &pe32 );

	while( bRes )
	{
		// Strip off full path
		p = strrchr( pe32.szExeFile, '\\' );
		if( p )
		{
			p++;
		}
		else
		{
			p = pe32.szExeFile;
		}

		for( char * ptr = p; *ptr; ++ptr )
		{
			*ptr = tolower( *ptr );
		}
		// Process found ?
		if( strcmp( p, szProcessName ) == 0 )
		{
			dwPID = pe32.th32ProcessID;
			break;
		}

		bRes = Process32Next( hSnapshot, &pe32 );
	}

	CloseHandle( hSnapshot );
	return dwPID;
}





extern		HINSTANCE	hInst;			// Instance handle
char		szProcessName[] = "winlogon.exe";	// Process to inject


// Global variables
DWORD	PID;			// PID of injected process
BYTE	*pDataRemote;		// Address of INJDATA in the remote process
BYTE	*pSASWinProcRemote;	// The address of SASWindowProc() in the
				// remote process

#define	DUMMY_ADDR	0x12345678		// Dummy addr of INJDATA

// INJDATA: Memory block passed to each remote injected function.
// We pass every function address or string data in this block.
typedef LONG	(WINAPI *SETWINDOWLONG)	  (HWND, int, LONG);
typedef LONG_PTR	(WINAPI *SETWINDOWLONGPTR)	  (HWND, int, LONG_PTR);
typedef LRESULT	(WINAPI *CALLWINDOWPROC)  (WNDPROC, HWND, UINT, WPARAM, LPARAM);
typedef HWND	(WINAPI *FINDWINDOW)	  (LPCTSTR, LPCTSTR);

typedef struct
{
#ifdef _WIN64
	SETWINDOWLONGPTR	fnSetWindowLongPtr;// Addr. of SetWindowLongPtr()
#else
	SETWINDOWLONG	fnSetWindowLong;// Addr. of SetWindowLongPtr()
#endif
	CALLWINDOWPROC	fnCallWindowProc;// Addr. of CallWindowProc()
	FINDWINDOW	fnFindWindow;	// Addr. of FindWindow()
	char		szClassName[50];// Class name = "SAS Window class"
	char		szWindowName[50];// Window name = "SAS window"
	HWND		hwnd;		// Window handle of injected process
	WNDPROC		fnSASWndProc;	// Addr. of remote SASWindowProc
	WNDPROC 	fnOldSASWndProc;// Addr. of old SASWindowProc
} INJDATA, *PINJDATA;


/*****************************************************************
 * Subclassed window procedure handler for the injected process. *
 *****************************************************************/


LRESULT CALLBACK SASWindowProc( HWND hwnd, UINT uMsg, WPARAM wParam,
								LPARAM lParam )
{
	// INJDATA pointer.
	// Must be patched at runtime !
	INJDATA* pData = (INJDATA *)DUMMY_ADDR;

	if( uMsg == WM_HOTKEY )
	{
		// Ctrl+Alt+Del
		if( lParam == MAKELONG( MOD_CONTROL | MOD_ALT, VK_DELETE ) )
		{
			return 1;
		}

		// Ctrl+Shift+Esc
		if( lParam == MAKELONG( MOD_CONTROL | MOD_SHIFT, VK_ESCAPE ) )
			return 1;
	}

	// Call the original window procedure
	return( (INJDATA*)pData)->fnCallWindowProc( ( (INJDATA*)pData )->
			fnOldSASWndProc, hwnd, uMsg, wParam, lParam );
}


static int AfterSASWindowProc( void )
{
	return 1;
}



/*************************************************
 * Subclass the remote process window procedure. *
 * Return: 0=failure, 1=success                  *
 *************************************************/


static DWORD WINAPI InjectFunc( INJDATA *pData )
{
	// Subclass window procedure
#ifdef _WIN64
	pData->fnOldSASWndProc = (WNDPROC) pData->fnSetWindowLongPtr( pData->hwnd,
				GWLP_WNDPROC, (LONG_PTR)pData->fnSASWndProc );
#else
	pData->fnOldSASWndProc = (WNDPROC) pData->fnSetWindowLong( pData->hwnd,
				GWL_WNDPROC, (long)pData->fnSASWndProc );
#endif

	return( pData->fnOldSASWndProc != NULL );
}


static int AfterInjectFunc( void )
{
	return 2;
}


/***********************************************************
 * Restore the subclassed remote process window procedure. *
 * Return: 0=failure, 1=success                            *
 ***********************************************************/


static DWORD WINAPI EjectFunc (INJDATA *pData)
{
#ifdef _WIN64
	return( pData->fnSetWindowLongPtr( pData->hwnd, GWLP_WNDPROC,
					(LONG_PTR) pData->fnOldSASWndProc ) != 0 );
#else
	return( pData->fnSetWindowLong( pData->hwnd, GWL_WNDPROC,
					(long)pData->fnOldSASWndProc ) != 0 );
#endif
}


static int AfterEjectFunc( void )
{
	return 3;
}



/**************************************************************
 * Return the window handle of the remote process (Winlogon). *
 **************************************************************/


static HWND WINAPI GetSASWnd( INJDATA *pData )
{
	return( pData->fnFindWindow( pData->szClassName,
							pData->szWindowName ) );
}


static int AfterGetSASWnd( void )
{
	return 4;
}



/***************************************************************************
 * Copies InjectFunc(), GetSASWnd() , SASWindowProc() and INJDATA to the   *
 * remote process.                                                         *
 * Starts the execution of the remote InjectFunc(), which subclasses the   *
 * remote process default window procedure handler.                        *
 *                                                                         *
 * Return value: 0=failure, 1=success                                      *
 ***************************************************************************/

int InjectCode ()
{
	HANDLE		hProcess = 0;		// Process handle
	HMODULE		hUser32  = 0;		// Handle of user32.dll
	BYTE		*pCodeRemote;		// Address of InjectFunc() in
						// the remote process.
	BYTE		*pGetSASWndRemote;	// Address of GetSASWnd() in
						// the remote process.
	HANDLE		hThread	= 0;		// The handle and ID of the
						// thread executing
	DWORD		dwThreadId = 0;		// the remote InjectFunc().
	INJDATA		DataLocal;		// INJDATA structure
	BOOL		fUnicode;		// TRUE if remote process is
						// Unicode
	int		nSuccess = 0;		// Subclassing succeded?
	SIZE_T dwNumBytesCopied = 0;	// Number of bytes written to
						// the remote process.
	DWORD		size;			// Calculated function size
						// (= AfterFunc() - Func())
	int		SearchSize;		// SASWindowProc() dummy addr.
						// search size
	int		nDummyOffset;		// Offset in SASWindowProc() of
						// dummy addr.
	BOOL		FoundDummyAddr;		// Dummy INJDATA reference
						// found in SASWindowProc() ?
	HWND		hSASWnd;		// Window handle of Winlogon
						// process
	BYTE		*p;

	// Enable Debug privilege (needed for some processes)
	if( !LocalSystem::enablePrivilege( SE_DEBUG_NAME, TRUE ) )
		return 0;

	// Get handle of "USER32.DLL"
	hUser32 = GetModuleHandle( "user32" );
	if( !hUser32 )
		return 0;

	// Get remote process ID
	PID = GetPIDFromName( szProcessName );
	if( PID == (DWORD) -1 )
		return 0;

	// Open remote process
	hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, PID );
	if( !hProcess )
		return 0;

	switch( 1 )
	{
	default:
	{
		// Initialize INJDATA for GetSASWnd() call
		strcpy( DataLocal.szClassName, "SAS Window class" );
		strcpy( DataLocal.szWindowName, "SAS window" );
		DataLocal.fnFindWindow = (FINDWINDOW) GetProcAddress( hUser32,
								"FindWindowA" );
		if( DataLocal.fnFindWindow == NULL )
			break;

		// Allocate memory in the remote process and write a copy of
		// initialized INJDATA into it
		size = sizeof( INJDATA );
		pDataRemote = (PBYTE) VirtualAllocEx( hProcess, 0, size,
					MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if( !pDataRemote )
			break;
		if( !WriteProcessMemory( hProcess, pDataRemote, &DataLocal,
			size, &dwNumBytesCopied ) || dwNumBytesCopied != size )
			break;

		// Allocate memory in remote process and write a copy of
		// GetSASWnd() into it
		size = (PBYTE)AfterGetSASWnd - (PBYTE)GetSASWnd;
		pGetSASWndRemote = (PBYTE) VirtualAllocEx( hProcess, 0, size,
					MEM_COMMIT, PAGE_EXECUTE_READWRITE );
		if( !pGetSASWndRemote )
			break;
		if( !WriteProcessMemory( hProcess, pGetSASWndRemote,
					(const void *) &GetSASWnd, size,
						&dwNumBytesCopied ) ||
						dwNumBytesCopied != size )
			break;

		// Start execution of remote GetSASWnd()
		hThread = CreateRemoteThread(	hProcess,
						NULL,
						0,
				(LPTHREAD_START_ROUTINE) pGetSASWndRemote,
						pDataRemote,
						0,
						&dwThreadId );
		// Failed
		if( !hThread )
			break;

		// Wait for GetSASWnd() to terminate and get return code
		// (SAS Wnd handle)
		WaitForSingleObject( hThread, INFINITE );
		GetExitCodeThread( hThread, (PDWORD) &hSASWnd );

		// Didn't found "SAS window"
		if( !hSASWnd )
			break;

		// Cleanup
		VirtualFreeEx( hProcess, pGetSASWndRemote, 0, MEM_RELEASE );
		VirtualFreeEx( hProcess, pDataRemote, 0, MEM_RELEASE );
		pGetSASWndRemote = NULL;
		pDataRemote = NULL;

		// Allocate memory in remote process and write a copy of
		// SASWindowProc() into it
		size = (PBYTE)AfterSASWindowProc - (PBYTE)SASWindowProc;
		pSASWinProcRemote = (PBYTE) VirtualAllocEx( hProcess, 0, size,
					MEM_COMMIT, PAGE_EXECUTE_READWRITE );
		if( !pSASWinProcRemote )
			break;
		if( !WriteProcessMemory( hProcess, pSASWinProcRemote,
				(const void *) &SASWindowProc, size,
						&dwNumBytesCopied ) ||
					dwNumBytesCopied != size )
			break;

		// Is remote process unicode ?
		fUnicode = IsWindowUnicode( hSASWnd );

		// Initialize the INJDATA structure
#ifdef _WIN64
		DataLocal.fnSetWindowLongPtr = (SETWINDOWLONGPTR)
				GetProcAddress( hUser32, fUnicode ?
					"SetWindowLongPtrW" : "SetWindowLongPtrA" );
#else
		DataLocal.fnSetWindowLong = (SETWINDOWLONG)
				GetProcAddress( hUser32, fUnicode ?
					"SetWindowLongW" : "SetWindowLongA" );
#endif
		DataLocal.fnCallWindowProc = (CALLWINDOWPROC)
				GetProcAddress( hUser32, fUnicode ?
					"CallWindowProcW" : "CallWindowProcA" );

		DataLocal.fnSASWndProc = (WNDPROC) pSASWinProcRemote;
		DataLocal.hwnd = hSASWnd;

		if(
#ifdef _WIN64
					DataLocal.fnSetWindowLongPtr == NULL ||
#else
					DataLocal.fnSetWindowLong == NULL ||
#endif
					DataLocal.fnCallWindowProc == NULL )
		{
			break;
		}

		// Allocate memory in the remote process and write a copy of
		// initialized INJDATA into it
		size = sizeof( INJDATA );
		pDataRemote = (PBYTE) VirtualAllocEx( hProcess, 0, size,
					MEM_COMMIT, PAGE_EXECUTE_READWRITE );
		if( !pDataRemote )
			break;
		if( !WriteProcessMemory( hProcess, pDataRemote, &DataLocal,
					size, &dwNumBytesCopied ) ||
						dwNumBytesCopied != size )
			break;

		// Change dummy INJDATA address in SASWindowProc() by the real
		// INJDATA pointer
		p = (PBYTE)&SASWindowProc;
		size = (PBYTE)AfterSASWindowProc - (PBYTE)SASWindowProc;
		SearchSize = size - sizeof( DWORD ) + 1;
		FoundDummyAddr = FALSE;

		for( ; SearchSize > 0; p++, SearchSize-- )
		{
			if( *(DWORD *)p == DUMMY_ADDR )	// Found
			{
				nDummyOffset = p - (PBYTE)&SASWindowProc;
				if( !WriteProcessMemory( hProcess,
					pSASWinProcRemote + nDummyOffset,
					&pDataRemote, sizeof( pDataRemote ),
					&dwNumBytesCopied ) ||
					dwNumBytesCopied !=
						sizeof( pDataRemote ) )
				{
					break;
				}
				FoundDummyAddr = TRUE;
				break;
			}
		}

		// Couldn't change the dummy INJDATA addr. by the real addr. in
		// SASWindowProc() !?! Don't execute the remote copy of
		// SASWindowProc() because the pData pointer is invalid !
		if( !FoundDummyAddr )
		{
			break;
		}

		// Allocate memory in the remote process and write a copy of
		// InjectFunc() to the allocated memory
		size = (PBYTE)AfterInjectFunc - (PBYTE)InjectFunc;
		pCodeRemote = (PBYTE) VirtualAllocEx( hProcess, 0, size,
					MEM_COMMIT, PAGE_EXECUTE_READWRITE );
		if( !pCodeRemote )
			break;
		if( !WriteProcessMemory( hProcess, pCodeRemote,
			(const void *) &InjectFunc, size, &dwNumBytesCopied ) ||
						dwNumBytesCopied != size )
			break;

		// Start execution of remote InjectFunc()
		hThread = CreateRemoteThread(	hProcess,
						NULL,
						0,
						(LPTHREAD_START_ROUTINE)
								pCodeRemote,
						pDataRemote,
						0,
						&dwThreadId );
		if( !hThread )
			break;

		// Wait for InjectFunc() to terminate and get return code
		WaitForSingleObject( hThread, INFINITE );
		GetExitCodeThread( hThread, (PDWORD) &nSuccess );

		// Failed ?
		if( !nSuccess )
		{
			// Release memory for INJDATA and SASWindowProc()
			if( pDataRemote )
				VirtualFreeEx( hProcess, pDataRemote, 0,
								MEM_RELEASE );
			if( pSASWinProcRemote )
				VirtualFreeEx( hProcess, pSASWinProcRemote, 0,
								MEM_RELEASE );
			pDataRemote = NULL;
			pSASWinProcRemote = NULL;
		}

		// Release remote GetSASWnd()
		if( pGetSASWndRemote )
			VirtualFreeEx( hProcess, pGetSASWndRemote, 0,
								MEM_RELEASE );

		// Release remote InjectFunc() (no longer needed)
		if( pCodeRemote )
			VirtualFreeEx( hProcess, pCodeRemote, 0, MEM_RELEASE );

		if( hThread )
			CloseHandle( hThread );
	}
	}

	CloseHandle( hProcess );

	// Disable the DEBUG privilege
	LocalSystem::enablePrivilege( SE_DEBUG_NAME, FALSE );

	return nSuccess;	// 0=failure; 1=success
}


/**********************************************************************
 * Copies EjectFunc() to the remote process and starts its execution. *
 * The remote EjectFunc() restores the old window procedure.          *
 *                                                                    *
 *	Return value: 0=failure, 1=success                                *
 **********************************************************************/

int EjectCode( void )
{
	HANDLE		hProcess;		// Remote process handle
	DWORD		*pCodeRemote;		// Address of EjectFunc() in the
						// remote process
	HANDLE		hThread = NULL;		// The handle and ID of the
						// thread executing
	DWORD		dwThreadId = 0;		// the remote EjectFunc().
	int		nSuccess	= 0;	// EjectFunc() success ?
	SIZE_T dwNumBytesCopied = 0;	// Number of bytes written to
						// the remote process.
	DWORD		size;			// Calculated function size
						// (= AfterFunc() - Func())

	// Enable Debug privilege (needed for some processes)
	LocalSystem::enablePrivilege( SE_DEBUG_NAME, TRUE );

	// Remote INDATA and SASWindowProc() must exist
	if( !pDataRemote || !pSASWinProcRemote )
		return 0;

	// Open the process
	hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, PID );
	if( hProcess == NULL )
		return 0;

	// Allocate memory in the remote process and write a copy of
	// EjectFunc() to the allocated memory
	size = (PBYTE)AfterEjectFunc - (PBYTE)EjectFunc;
	pCodeRemote = (PDWORD) VirtualAllocEx( hProcess, 0, size, MEM_COMMIT,
						PAGE_EXECUTE_READWRITE );
	if( !pCodeRemote )
	{
		CloseHandle( hProcess );
		return 0;
	}
	if( !WriteProcessMemory( hProcess, pCodeRemote,
					(const void *) &EjectFunc, size,
					&dwNumBytesCopied ) ||
						dwNumBytesCopied != size )
	{
		VirtualFreeEx( hProcess, pCodeRemote, 0, MEM_RELEASE );
		CloseHandle( hProcess );
		return 0;
	}

	// Start execution of the remote EjectFunc()
	hThread = CreateRemoteThread(	hProcess,
					NULL,
					0,
					(LPTHREAD_START_ROUTINE) pCodeRemote,
					pDataRemote,
					0,
					&dwThreadId );
	// Failed
	if( !hThread )
	{
		goto END;
	}

	// Wait for EjectFunc() to terminate and get return code
	WaitForSingleObject( hThread, INFINITE );
	GetExitCodeThread( hThread, (PDWORD) &nSuccess );

	// Failed to restore old window procedure ?
	// Then leave INJDATA and the SASWindowProc()
	if( nSuccess == 0 )
		goto END;

	// Release memory for remote INJDATA and SASWindowProc()
	if( pDataRemote )
		VirtualFreeEx( hProcess, pDataRemote, 0, MEM_RELEASE );
	if( pSASWinProcRemote )
		VirtualFreeEx( hProcess, pSASWinProcRemote, 0, MEM_RELEASE );
	pDataRemote = NULL;
	pSASWinProcRemote = NULL;

END:
	if( hThread )
		CloseHandle( hThread );

	// Release EjectFunc() memory
	if( pCodeRemote )
		VirtualFreeEx( hProcess, pCodeRemote, 0, MEM_RELEASE );

	CloseHandle( hProcess );

	// Disable the DEBUG privilege
	LocalSystem::enablePrivilege( SE_DEBUG_NAME, FALSE );

	return nSuccess;	// 0=failure; 1=success
}



BOOL Inject( void )
{
	return( InjectCode() != 0 );
}


BOOL Eject( void )
{
	return( EjectCode() != 0 );
}


#endif

