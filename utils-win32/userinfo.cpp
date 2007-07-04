/*
 * userinfo.cpp - userinfo-module for iTALC
 *
 * Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#include <windows.h>
#include <stdio.h>
#include <lm.h>
#include <psapi.h>
#include <ctype.h>


bool __found_ica;


void getUserName( char * * _str )
{
	if( !_str )
	{
		return;
	}
	*_str = NULL;

	DWORD aProcesses[1024], cbNeeded;

	if( !EnumProcesses( aProcesses, sizeof( aProcesses ), &cbNeeded ) )
	{
		return;
	}

	DWORD cProcesses = cbNeeded / sizeof(DWORD);

	for( DWORD i = 0; i < cProcesses; i++ )
	{
		HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
								PROCESS_VM_READ,
							false, aProcesses[i] );
		HMODULE hMod;
		if( hProcess == NULL ||
			!EnumProcessModules( hProcess, &hMod, sizeof( hMod ),
								&cbNeeded ) )
	        {
			continue;
		}

		TCHAR szProcessName[MAX_PATH];
		GetModuleBaseName( hProcess, hMod, szProcessName, 
                       		  sizeof( szProcessName ) / sizeof( TCHAR) );
		for( TCHAR * ptr = szProcessName; *ptr; ++ptr )
		{
			*ptr = tolower( *ptr );
		}

		if( strcmp( szProcessName, "ica.exe" ) == 0 )
		{
			__found_ica = true;
		}

		if( strcmp( szProcessName, "explorer.exe" ) )
		{
			CloseHandle( hProcess );
			continue;
		}
	
		HANDLE hToken;
		OpenProcessToken( hProcess, TOKEN_READ, &hToken );
		DWORD len = 0;

		GetTokenInformation( hToken, TokenUser, NULL, 0, &len ) ;
		if( len <= 0 )
		{
			CloseHandle( hToken );
			CloseHandle( hProcess );
			continue;
		}
		char * buf = new char[len];
		if( buf == NULL )
		{
			CloseHandle( hToken );
			CloseHandle( hProcess );
			continue;
		}
		if ( !GetTokenInformation( hToken, TokenUser, buf, len, &len ) )
		{
			delete[] buf;
			CloseHandle( hToken );
			CloseHandle( hProcess );
			continue;
		}

		PSID psid = ((TOKEN_USER*) buf)->User.Sid;

		DWORD accname_len = 0;
		DWORD domname_len = 0;
		SID_NAME_USE nu;
		LookupAccountSid( NULL, psid, NULL, &accname_len, NULL,
							&domname_len, &nu );
		if( accname_len == 0 || domname_len == 0 )
		{
			delete[] buf;
			CloseHandle( hToken );
			CloseHandle( hProcess );
			continue;
		}
		char * accname = new char[accname_len];
		char * domname = new char[domname_len];
		if( accname == NULL || domname == NULL )
		{
			delete[] buf;
			delete[] accname;
			delete[] domname;
			CloseHandle( hToken );
			CloseHandle( hProcess );
			continue;
		}
		LookupAccountSid( NULL, psid, accname, &accname_len,
						domname, &domname_len, &nu );
		WCHAR wszDomain[256];
		MultiByteToWideChar( CP_ACP, 0, domname,
			strlen( domname ) + 1, wszDomain, sizeof( wszDomain ) /
						sizeof( wszDomain[0] ) );
		WCHAR wszUser[256];
		MultiByteToWideChar( CP_ACP, 0, accname,
			strlen( accname ) + 1, wszUser, sizeof( wszUser ) /
							sizeof( wszUser[0] ) );
		PBYTE domcontroller;
		NetGetDCName( NULL, wszDomain, &domcontroller );
		LPUSER_INFO_2 pBuf = NULL;
		NET_API_STATUS nStatus = NetUserGetInfo( (LPCWSTR)domcontroller,
						wszUser, 2, (PBYTE *) &pBuf );
		if( nStatus == NERR_Success && pBuf != NULL )
		{
			len = WideCharToMultiByte( CP_ACP, 0,
							pBuf->usri2_full_name,
						-1, NULL, 0, NULL, NULL );
			if( len > 0 )
			{
				char * mbstr = new char[len];
				len = WideCharToMultiByte( CP_ACP, 0,
							pBuf->usri2_full_name,
						-1, mbstr, len, NULL, NULL );
				if( strlen( mbstr ) < 1 )
				{
					*_str = new char[2*accname_len+4];
					sprintf( *_str, "%s (%s)", accname,
								accname );
				}
				else
				{
					*_str = new char[len+accname_len+4];
					sprintf( *_str, "%s (%s)", mbstr,
								accname );
				}
				delete[] mbstr;
			}
			else
			{
				*_str = new char[2*accname_len+4];
				sprintf( *_str, "%s (%s)", accname, accname );
			}
		}
		if( pBuf != NULL )
		{
			NetApiBufferFree( pBuf );
		}
		if( domcontroller != NULL )
		{
			NetApiBufferFree( domcontroller );
		}
		delete[] accname;
		delete[] domname;
		FreeSid( psid );
		delete[] buf;
		CloseHandle( hToken );
		CloseHandle( hProcess );
	}
}



int main( void )
{
	do
	{
		__found_ica = false;
		char * name = NULL;
		getUserName( &name );
		if( name )
		{
			printf( "%s:\n", name );
			delete[] name;
		}
		fflush( stdout );
		Sleep( 10*1000 );
	}
	while( __found_ica );

	return( 0 );
}
