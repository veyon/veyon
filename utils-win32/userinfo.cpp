#include <windows.h>
#include <stdio.h>
#include <lm.h>
#include <psapi.h>
#include <ctype.h>


void getUserName( char * * _str)
{
	if( !_str )
	{
		return;
	}
	*_str = NULL;

	DWORD aProcesses[1024], cbNeeded;

	if( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
	{
		return;
	}

	DWORD cProcesses = cbNeeded / sizeof(DWORD);

	for( DWORD i = 0; i < cProcesses; i++ )
	{
		HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
								PROCESS_VM_READ,
							FALSE, aProcesses[i] );
		HMODULE hMod;
		if( hProcess == NULL ||
			!EnumProcessModules( hProcess, &hMod, sizeof( hMod ),
								&cbNeeded ) )
	        {
			continue;
		}
		TCHAR szProcessName[MAX_PATH];
		GetModuleBaseName( hProcess, hMod, szProcessName, 
                             		  sizeof(szProcessName)/sizeof(TCHAR) );
		for( TCHAR * ptr = szProcessName; *ptr; ++ptr )
		{
			*ptr = tolower( *ptr );
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
		char * buf = new char[len];
		if ( !GetTokenInformation( hToken, TokenUser, buf, len, &len ) )
		{
			CloseHandle( hProcess );
			continue;
		}

		PSID psid = ((TOKEN_USER*) buf)->User.Sid;

		DWORD accname_len = 0;
		DWORD domname_len = 0;
		SID_NAME_USE nu;
		LookupAccountSid( NULL, psid, NULL, &accname_len, NULL,
							&domname_len, &nu );
		char * accname = new char[accname_len];
		char * domname = new char[domname_len];
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
			char * mbstr = new char[len];
			len = WideCharToMultiByte( CP_ACP, 0,
							pBuf->usri2_full_name,
						-1, mbstr, len, NULL, NULL );
			*_str = new char[len+accname_len+4];
			sprintf( *_str, "%s (%s)", mbstr, accname );
			delete[] mbstr;
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
	char * name;
	getUserName( &name );
	if( name )
	{
		printf( "%s\n", name );
	}
	return( 0 );
}
