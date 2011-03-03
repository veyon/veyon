/*
 * LocalSystem.cpp - namespace LocalSystem, providing an interface for
 *				   transparent usage of operating-system-specific functions
 *
 * Copyright (c) 2006-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <italcconfig.h>

#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtGui/QWidget>
#include <QtNetwork/QHostInfo>


#ifdef ITALC_BUILD_WIN32

#include <QtCore/QLibrary>

#include <windows.h>
#include <shlobj.h>
#include <wtsapi32.h>
#include <sddl.h>
#include <userenv.h>
#include <lm.h>


namespace LocalSystem
{

// taken from qt-win-opensource-src-4.2.2/src/corelib/io/qsettings.cpp
QString windowsConfigPath( int _type )
{
	QString result;

	QLibrary library( "shell32" );
	typedef BOOL( WINAPI* GetSpecialFolderPath )( HWND, char *, int, BOOL );
	GetSpecialFolderPath SHGetSpecialFolderPath = (GetSpecialFolderPath)
				library.resolve( "SHGetSpecialFolderPathA" );
	if( SHGetSpecialFolderPath )
	{
		char path[MAX_PATH];
		SHGetSpecialFolderPath( 0, path, _type, FALSE );
		result = QString::fromLocal8Bit( path );
	}
	return( result );
}

}

#endif


#ifdef ITALC_HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef ITALC_HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef ITALC_HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef ITALC_HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef ITALC_HAVE_ERRNO_H
#include <errno.h>
#endif

#include "ItalcConfiguration.h"
#include "LocalSystem.h"
#include "Logger.h"




namespace LocalSystem
{


Desktop::Desktop( const QString &name ) :
	m_name( name )
{
#ifdef ITALC_BUILD_WIN32
	if( m_name.isEmpty() )
	{
		m_name = "winsta0\\default";
	}
#endif
}



Desktop::Desktop( const Desktop &desktop ) :
	m_name( desktop.name() )
{
}



Desktop Desktop::activeDesktop()
{
	QString deskName;

#ifdef ITALC_BUILD_WIN32
	HDESK desktopHandle = OpenInputDesktop( 0, TRUE, DESKTOP_READOBJECTS );

	char dname[256];
	dname[0] = 0;
	if( GetUserObjectInformation( desktopHandle, UOI_NAME, dname,
									sizeof( dname ), NULL ) )
	{
		deskName = QString( "winsta0\\%1" ).arg( dname );
	}
	CloseDesktop( desktopHandle );
#endif

	return Desktop( deskName );
}



Desktop Desktop::screenLockDesktop()
{
	return Desktop( "ScreenLockSlaveDesktop" );
}




User::User( const QString &name, const QString &dom, const QString &fullname ) :
	m_userToken( 0 ),
	m_name( name ),
	m_domain( dom ),
	m_fullName( fullname )
{
#ifdef ITALC_BUILD_WIN32
	// try to look up the user -> domain
	DWORD sidLen = 256;
	char domainName[MAX_PATH];
	domainName[0] = 0;
	char *sid = new char[sidLen];
	DWORD domainLen = MAX_PATH;
	SID_NAME_USE snu;
	m_userToken = sid;

	if( !LookupAccountName( NULL,		// system name
							m_name.toAscii().constData(),
							m_userToken,		// SID
							&sidLen,
							domainName,
							&domainLen,
							&snu ) )
	{
		qCritical( "Could not look up SID structure" );
		return;
	}

	if( m_domain.isEmpty() )
	{
		CHAR computerName[MAX_PATH];
		DWORD size = MAX_PATH;
		GetComputerName( computerName, &size );

		if( QString( domainName ) != computerName )
		{
			m_domain = domainName;
		}
	}

#else
	m_userToken = getuid();
#endif
}


#ifdef ITALC_BUILD_WIN32
static void copySid( const PSID &src, PSID &dst )
{
	if( src )
	{
		const int sidLen = GetLengthSid( src );
		if( sidLen )
		{
			dst = new char[sidLen];
			CopySid( sidLen, dst, src );
		}
	}
}

#endif

User::User( Token userToken ) :
	m_userToken( userToken ),
	m_name(),
	m_domain(),
	m_fullName()
{
#ifdef ITALC_BUILD_WIN32
	copySid( userToken, m_userToken );
#endif

	lookupNameAndDomain();
}



User::User( const User &user ) :
	m_userToken( user.userToken() ),
	m_name( user.name() ),
	m_domain( user.domain() ),
	m_fullName( user.m_fullName )
{
#ifdef ITALC_BUILD_WIN32
	copySid( user.userToken(), m_userToken );
#endif
}



User::~User()
{
#ifdef ITALC_BUILD_WIN32
	if( m_userToken )
	{
		delete[] (char *) m_userToken;
	}
#endif
}


#ifdef ITALC_BUILD_WIN32
static QString querySessionInformation( DWORD sessionId,
										WTS_INFO_CLASS infoClass )
{
	QString result;
	LPTSTR pBuffer = NULL;
	DWORD dwBufferLen;
	if( WTSQuerySessionInformation(
					WTS_CURRENT_SERVER_HANDLE,
					sessionId,
					infoClass,
					&pBuffer,
					&dwBufferLen ) )
	{
		result = pBuffer;
	}
	WTSFreeMemory( pBuffer );

	return result;
}
#endif



User User::loggedOnUser()
{
	QString userName = "unknown";
	QString domainName = QHostInfo::localDomainName();

#ifdef ITALC_BUILD_WIN32

	DWORD sessionId = WTSGetActiveConsoleSessionId();

	userName = querySessionInformation( sessionId, WTSUserName );
	domainName = querySessionInformation( sessionId, WTSDomainName );

	// check whether we just got the name of the local computer
	if( !domainName.isEmpty() )
	{
		CHAR computerName[MAX_PATH];
		DWORD size = MAX_PATH;
		GetComputerName( computerName, &size );

		if( domainName == computerName )
		{
			// reset domain name as storing the local computer's name
			// doesn't help here
			domainName = QString();
		}
	}

#else

	char * envUser = getenv( "USER" );

#ifdef ITALC_HAVE_PWD_H
	struct passwd * pw_entry = NULL;
	if( envUser )
	{
		pw_entry = getpwnam( envUser );
	}
	if( !pw_entry )
	{
		pw_entry = getpwuid( getuid() );
	}
	if( pw_entry )
	{
		QString shell( pw_entry->pw_shell );

		// Skip not real users
		if ( !( shell.endsWith( "/false" ) ||
				shell.endsWith( "/true" ) ||
				shell.endsWith( "/null" ) ||
				shell.endsWith( "/nologin") ) )
		{
			userName = QString::fromUtf8( pw_entry->pw_name );
		}
	}
#endif	/* ITALC_HAVE_PWD_H */

	if( userName.isEmpty() )
	{
		userName = QString::fromUtf8( envUser );
	}

#endif

	return User( userName, domainName );
}




QString User::homePath() const
{
	QString homePath = QDir::homePath();

#ifdef ITALC_BUILD_WIN32
	LocalSystem::Process userProcess(
				LocalSystem::Process::findProcessId( QString(), -1, this ) );
	HANDLE hToken;
	if( OpenProcessToken( userProcess.processHandle(),
									MAXIMUM_ALLOWED, &hToken ) )
	{
		CHAR userProfile[MAX_PATH];
		DWORD size = MAX_PATH;
		if( GetUserProfileDirectory( hToken, userProfile, &size ) )
		{
			homePath = userProfile;
			CloseHandle( hToken );
		}
		else
		{
			ilog_failedf( "GetUserProfileDirectory()", "%d", GetLastError() );
		}
	}
	else
	{
		ilog_failedf( "OpenProcessToken()", "%d", GetLastError() );
	}
#endif

	return homePath;
}




void User::lookupNameAndDomain()
{
	if( !m_name.isEmpty() && !m_domain.isEmpty() )
	{
		return;
	}

#ifdef ITALC_BUILD_WIN32
	DWORD accNameLen = 0;
	DWORD domainNameLen = 0;
	SID_NAME_USE snu;
	LookupAccountSid( NULL, userToken(), NULL, &accNameLen, NULL,
							&domainNameLen, &snu );
	if( accNameLen == 0 || domainNameLen == 0 )
	{
		return;
	}

	char * accName = new char[accNameLen];
	char * domainName = new char[domainNameLen];
	LookupAccountSid( NULL, userToken(), accName, &accNameLen,
						domainName, &domainNameLen, &snu );

	if( m_name.isEmpty() )
	{
		m_name = accName;
	}

	if( m_domain.isEmpty() )
	{
		CHAR computerName[MAX_PATH];
		DWORD size = MAX_PATH;
		GetComputerName( computerName, &size );

		if( QString( domainName ) != computerName )
		{
			m_domain = domainName;
		}
	}

	delete[] accName;
	delete[] domainName;

#else	/* ITALC_BUILD_WIN32 */

	struct passwd * pw_entry = getpwuid( m_userToken );
	if( pw_entry )
	{
		QString shell( pw_entry->pw_shell );

		// Skip not real users
		if ( !( shell.endsWith( "/false" ) ||
				shell.endsWith( "/true" ) ||
				shell.endsWith( "/null" ) ||
				shell.endsWith( "/nologin") ) )
		{
			m_name = QString::fromUtf8( pw_entry->pw_name );
		}
	}

	m_domain = QHostInfo::localDomainName();
#endif
}




void User::lookupFullName()
{
	lookupNameAndDomain();

#ifdef ITALC_BUILD_WIN32
	char * accName = qstrdup( m_name.toAscii().constData() );
	char * domainName = qstrdup( m_domain.toAscii().constData() );

	// try to retrieve user's full name from domain
	WCHAR wszDomain[256];
	MultiByteToWideChar( CP_ACP, 0, domainName,
			strlen( domainName ) + 1, wszDomain, sizeof( wszDomain ) /
											sizeof( wszDomain[0] ) );
	WCHAR wszUser[256];
	MultiByteToWideChar( CP_ACP, 0, accName,
			strlen( accName ) + 1, wszUser, sizeof( wszUser ) /
											sizeof( wszUser[0] ) );

	PBYTE dc = NULL;	// domain controller
	if( NetGetDCName( NULL, wszDomain, &dc ) != NERR_Success )
	{
		dc = NULL;
	}

	LPUSER_INFO_2 pBuf = NULL;
	NET_API_STATUS nStatus = NetUserGetInfo( (LPWSTR)dc, wszUser, 2,
												(LPBYTE *) &pBuf );
	if( nStatus == NERR_Success && pBuf != NULL )
	{
		int len = WideCharToMultiByte( CP_ACP, 0, pBuf->usri2_full_name,
											-1, NULL, 0, NULL, NULL );
		if( len > 0 )
		{
			char *mbstr = new char[len];
			len = WideCharToMultiByte( CP_ACP, 0, pBuf->usri2_full_name,
										-1, mbstr, len, NULL, NULL );
			if( strlen( mbstr ) >= 1 )
			{
				m_fullName = mbstr;
			}
			delete[] mbstr;
		}
	}

	if( pBuf != NULL )
	{
		NetApiBufferFree( pBuf );
	}
	if( dc != NULL )
	{
		NetApiBufferFree( dc );
	}

	delete[] accName;
	delete[] domainName;

#else

#ifdef ITALC_HAVE_PWD_H
	struct passwd * pw_entry = getpwnam( m_name.toUtf8().constData() );
	if( !pw_entry )
	{
		pw_entry = getpwuid( m_userToken );
	}
	if( pw_entry )
	{
		QString shell( pw_entry->pw_shell );

		// Skip not real users
		if ( !( shell.endsWith( "/false" ) ||
				shell.endsWith( "/true" ) ||
				shell.endsWith( "/null" ) ||
				shell.endsWith( "/nologin") ) )
		{
			m_fullName = QString::fromUtf8( pw_entry->pw_gecos );
		}
	}
#endif

#endif
}





Process::Process( int pid ) :
	m_processHandle( 0 )
{
#ifdef ITALC_BUILD_WIN32
	if( pid >= 0 )
	{
		m_processHandle = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pid );
	}
#endif
}



Process::~Process()
{
#ifdef ITALC_BUILD_WIN32
	if( m_processHandle )
	{
		CloseHandle( m_processHandle );
	}
#endif
}



#ifdef ITALC_BUILD_WIN32
static DWORD findProcessId_WTS( const QString &processName, DWORD sessionId,
													const User *processOwner )
{
	PWTS_PROCESS_INFO pProcessInfo = NULL;
	DWORD processCount = 0;
	DWORD pid = -1;

	if( !WTSEnumerateProcesses( WTS_CURRENT_SERVER_HANDLE, 0, 1, &pProcessInfo,
								&processCount ) )
	{
		return pid;
	}

	for( DWORD proc = 0; (int)pid < 0 && proc < processCount; ++proc )
	{
		if( pProcessInfo[proc].ProcessId == 0 )
		{
			continue;
		}
		if( processName.isEmpty() ||
				processName.compare( pProcessInfo[proc].pProcessName,
												Qt::CaseInsensitive	) == 0 )
		{
			if( (int) sessionId < 0 ||
						sessionId == pProcessInfo[proc].SessionId )
			{
				if( processOwner == NULL )
				{
					pid = pProcessInfo[proc].ProcessId;
				}
				else if( pProcessInfo[proc].pUserSid != NULL )
				{
					if( EqualSid( pProcessInfo[proc].pUserSid,
									processOwner->userToken() ) )
					{
						pid = pProcessInfo[proc].ProcessId;
					}
				}
			}
		}
	}

	WTSFreeMemory( pProcessInfo );

	return pid;
}


#include <tlhelp32.h>

static DWORD findProcessId_TH32( const QString &processName, DWORD sessionId,
													const User *processOwner )
{
	DWORD pid = -1;
	PROCESSENTRY32 procEntry;

	HANDLE hSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if( hSnap == INVALID_HANDLE_VALUE )
	{
		return -1;
	}

	procEntry.dwSize = sizeof( PROCESSENTRY32 );

	if( !Process32First( hSnap, &procEntry ) )
	{
		CloseHandle( hSnap );
		return -1;
	}

	do
	{
		if( processName.isEmpty() ||
				processName.compare( procEntry.szExeFile,
									Qt::CaseInsensitive ) == 0 )
		{
			if( processOwner == NULL )
			{
				pid = procEntry.th32ProcessID;
				break;
			}
			else
			{
				User *u = Process( procEntry.th32ProcessID ).getProcessOwner();
				if( u && EqualSid( u->userToken(), processOwner->userToken() ) )
				{
					pid = procEntry.th32ProcessID;
					break;
				}
				delete u;
			}
		}
	}
	while( Process32Next( hSnap, &procEntry ) );

	CloseHandle( hSnap );

	return pid;
}
#endif



int Process::findProcessId( const QString &processName,
							int sessionId, const User *processOwner )
{
	LogStream( Logger::LogLevelDebug )
			<< "Process::findProcessId("
				<< processName
				<< sessionId
				<< processOwner
			<< ")";

	int pid = -1;
#ifdef ITALC_BUILD_WIN32
	pid = findProcessId_WTS( processName, sessionId, processOwner );
	ilogf( Debug, "findProcessId_WTS(): %d", pid );
	if( pid < 0 )
	{
		pid = findProcessId_TH32( processName, sessionId, processOwner );
		ilogf( Debug, "findProcessId_TH32(): %d", pid );
	}
#endif
	return pid;
}




User *Process::getProcessOwner()
{
#ifdef ITALC_BUILD_WIN32
	HANDLE hToken;
	OpenProcessToken( m_processHandle, TOKEN_READ, &hToken );

	DWORD len = 0;

	GetTokenInformation( hToken, TokenUser, NULL, 0, &len ) ;
	if( len <= 0 )
	{
		ilog_failedf( "GetTokenInformation()", "%d", GetLastError() );
		CloseHandle( hToken );
		return NULL;
	}

	char *buf = new char[len];
	if( buf == NULL )
	{
		CloseHandle( hToken );
		return NULL;
	}

	if ( !GetTokenInformation( hToken, TokenUser, buf, len, &len ) )
	{
		ilog_failedf( "GetTokenInformation() (2)", "%d", GetLastError() );
		delete[] buf;
		CloseHandle( hToken );
		return NULL;
	}

	User *user = new User( ((TOKEN_USER*) buf)->User.Sid );

	delete[] buf;
	CloseHandle( hToken );

	return user;
#else
	return NULL;
#endif
}




Process::Handle Process::runAsUser( const QString &proc,
									const QString &desktop )
{
#ifdef ITALC_BUILD_WIN32
	enablePrivilege( SE_ASSIGNPRIMARYTOKEN_NAME, true );
	enablePrivilege( SE_INCREASE_QUOTA_NAME, true );
	HANDLE hToken = NULL;

	if( !OpenProcessToken( processHandle(), MAXIMUM_ALLOWED, &hToken ) )
	{
		ilog_failedf( "OpenProcessToken()", "%d", GetLastError() );
		return NULL;
	}

	if( !ImpersonateLoggedOnUser( hToken ) )
	{
		ilog_failedf( "ImpersonateLoggedOnUser()", "%d", GetLastError() );
		return NULL;
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory( &si, sizeof( STARTUPINFO ) );
	si.cb = sizeof( STARTUPINFO );
	if( !desktop.isEmpty() )
	{
		si.lpDesktop = (CHAR *) qstrdup( desktop.toAscii().constData() );
	}
	HANDLE hNewToken = NULL;

	DuplicateTokenEx( hToken, TOKEN_ASSIGN_PRIMARY|TOKEN_ALL_ACCESS, NULL,
						SecurityImpersonation, TokenPrimary, &hNewToken );

	if( !CreateProcessAsUser(
				hNewToken,			// client's access token
				NULL,			  // file to execute
				(CHAR *) proc.toUtf8().constData(),	 // command line
				NULL,			  // pointer to process SECURITY_ATTRIBUTES
				NULL,			  // pointer to thread SECURITY_ATTRIBUTES
				FALSE,			 // handles are not inheritable
				NORMAL_PRIORITY_CLASS,   // creation flags
				NULL,			  // pointer to new environment block
				NULL,			  // name of current directory
				&si,			   // pointer to STARTUPINFO structure
				&pi				// receives information about new process
				) )
	{
		ilog_failedf( "CreateProcessAsUser()", "%d", GetLastError() );
		return NULL;
	}

	delete[] si.lpDesktop;

	CloseHandle( hNewToken );
	RevertToSelf();
	CloseHandle( hToken );

	return pi.hProcess;
#else
	QProcess::startDetached( proc );
	return 0;
#endif
}




bool Process::isRunningAsAdmin()
{
#ifdef ITALC_BUILD_WIN32
	BOOL runningAsAdmin = false;
	PSID adminGroupSid = NULL;

	// allocate and initialize a SID of the administrators group.
	SID_IDENTIFIER_AUTHORITY NtAuthority = { SECURITY_NT_AUTHORITY };
	if( AllocateAndInitializeSid(
		&NtAuthority,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&adminGroupSid ) )
	{
		// determine whether the SID of administrators group is enabled in
		// the primary access token of the process.
		CheckTokenMembership( NULL, adminGroupSid, &runningAsAdmin );
	}

	if( adminGroupSid )
	{
		FreeSid( adminGroupSid );
	}

	return runningAsAdmin;
#else
	return true;
#endif
}




bool Process::runAsAdmin( const QString &appPath, const QString &parameters )
{
#ifdef ITALC_BUILD_WIN32
	SHELLEXECUTEINFO sei = { sizeof(sei) };
	sei.lpVerb = "runas";
	sei.lpFile = appPath.toUtf8().constData();
	sei.hwnd = GetForegroundWindow();
	sei.lpParameters = parameters.toUtf8().constData();
	sei.nShow = SW_NORMAL;

	if( !ShellExecuteEx( &sei ) )
	{
		if( GetLastError() == ERROR_CANCELLED )
		{
			// the user refused the elevation - do nothing
		}
		return false;
	}
#endif

	return true;
}





void sleep( const int _ms )
{
#ifdef ITALC_BUILD_WIN32
	Sleep( static_cast<unsigned int>( _ms ) );
#else
	struct timespec ts = { _ms / 1000, ( _ms % 1000 ) * 1000 * 1000 } ;
	nanosleep( &ts, NULL );
#endif
}




void broadcastWOLPacket( const QString & _mac )
{
	const int PORT_NUM = 65535;
	const int MAC_SIZE = 6;
	const int OUTBUF_SIZE = MAC_SIZE*17;
	unsigned char mac[MAC_SIZE];
	char out_buf[OUTBUF_SIZE];

	if( sscanf( _mac.toAscii().constData(),
				"%2x:%2x:%2x:%2x:%2x:%2x",
				(unsigned int *) &mac[0],
				(unsigned int *) &mac[1],
				(unsigned int *) &mac[2],
				(unsigned int *) &mac[3],
				(unsigned int *) &mac[4],
				(unsigned int *) &mac[5] ) != MAC_SIZE )
	{
		qWarning( "invalid MAC-address" );
		return;
	}

	for( int i = 0; i < MAC_SIZE; ++i )
	{
		out_buf[i] = 0xff;
	}

	for( int i = 1; i < 17; ++i )
	{
		for(int j = 0; j < MAC_SIZE; ++j )
		{
			out_buf[i*MAC_SIZE+j] = mac[j];
		}
	}

#ifdef ITALC_BUILD_WIN32
	WSADATA info;
	if( WSAStartup( MAKEWORD( 2, 0 ), &info ) != 0 )
	{
		qCritical( "cannot initialize WinSock!" );
		return;
	}
#endif

	// UDP-broadcast the MAC-address
	unsigned int sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	struct sockaddr_in my_addr;
	my_addr.sin_family	  = AF_INET;			// Address family to use
	my_addr.sin_port		= htons( PORT_NUM );	// Port number to use
	my_addr.sin_addr.s_addr = inet_addr( "255.255.255.255" ); // send to
								  // IP_ADDR

	int optval = 1;
	if( setsockopt( sock, SOL_SOCKET, SO_BROADCAST, (char *) &optval,
							sizeof( optval ) ) < 0 )
	{
		qCritical( "can't set sockopt (%d).", errno );
		return;
	}

	sendto( sock, out_buf, sizeof( out_buf ), 0,
			(struct sockaddr*) &my_addr, sizeof( my_addr ) );
#ifdef ITALC_BUILD_WIN32
	closesocket( sock );
	WSACleanup();
#else
	close( sock );
#endif


#if 0
#ifdef ITALC_BUILD_LINUX
	QProcess::startDetached( "etherwake " + _mac );
#endif
#endif
}



static inline void pressAndReleaseKey( int _key )
{
}


void logonUser( const QString & _uname, const QString & _passwd,
						const QString & _domain )
{
	// TODO
}




QString Path::expand( QString path )
{
	QString p = QDTNS( path.replace( "$HOME", QDir::homePath() ).
						replace( "%HOME%", QDir::homePath() ).
						replace( "$PROFILE", QDir::homePath() ).
						replace( "%PROFILE%", QDir::homePath() ).
						replace( "$APPDATA", personalConfigDataPath() ).
						replace( "%APPDATA%", personalConfigDataPath() ).
						replace( "$GLOBALAPPDATA", systemConfigDataPath() ).
						replace( "%GLOBALAPPDATA%", systemConfigDataPath() ).
						replace( "$TMP", QDir::tempPath() ).
						replace( "$TEMP", QDir::tempPath() ).
						replace( "%TMP%", QDir::tempPath() ).
						replace( "%TEMP%", QDir::tempPath() ) );
	// remove duplicate directory separators - however skip the first two chars
	// as they might specify an UNC path on Windows
	if( p.length() > 3 )
	{
		return p.left( 2 ) + p.mid( 2 ).replace(
									QString( "%1%1" ).arg( QDir::separator() ),
									QDir::separator() );
	}
	return p;
}




QString Path::shrink( QString path )
{
	if( QFileInfo( path ).isDir() )
	{
		// we replace parts of the path with strings returned by
		// personalConfigDataPath() & friends which always return a path with
		// a trailing dir separator - therefore add one so we don't miss a
		// replace
		path += QDir::separator();
	}
	path = QDTNS( path );
#ifdef ITALC_BUILD_WIN32
	const Qt::CaseSensitivity cs = Qt::CaseInsensitive;
	const QString envVar = "%%1%\\";
#else
	const Qt::CaseSensitivity cs = Qt::CaseSensitive;
	const QString envVar = "$%1/";
#endif
	if( path.startsWith( personalConfigDataPath(), cs ) )
	{
		path.replace( personalConfigDataPath(), envVar.arg( "APPDATA" ) );
	}
	else if( path.startsWith( systemConfigDataPath(), cs ) )
	{
		path.replace( systemConfigDataPath(), envVar.arg( "GLOBALAPPDATA" ) );
	}
	else if( path.startsWith( QDTNS( QDir::homePath() ), cs ) )
	{
		path.replace( QDTNS( QDir::homePath() ), envVar.arg( "HOME" ) );
	}
	else if( path.startsWith( QDTNS( QDir::tempPath() ), cs ) )
	{
		path.replace( QDTNS( QDir::tempPath() ), envVar.arg( "TEMP" ) );
	}

	return QDTNS( path.replace( QString( "%1%1" ).
								arg( QDir::separator() ), QDir::separator() ) );
}




bool Path::ensurePathExists( const QString &path )
{
	const QString expandedPath = expand( path );

	if( path.isEmpty() || QDir( expandedPath ).exists() )
	{
		return true;
	}

	qDebug() << "LocalSystem::Path::ensurePathExists(): creating "
				<< path << "=>" << expandedPath;

	QString p = expandedPath;

	QStringList dirs;
	while( !QDir( p ).exists() && !p.isEmpty() )
	{
		dirs.push_front( QDir( p ).dirName() );
		p.chop( dirs.front().size() + 1 );
	}

	if( !p.isEmpty() )
	{
		return QDir( p ).mkpath( dirs.join( QDir::separator() ) );
	}

	return false;
}



QString Path::personalConfigDataPath()
{
	const QString appData = 
#ifdef ITALC_BUILD_WIN32
		LocalSystem::windowsConfigPath( CSIDL_APPDATA ) + QDir::separator() + "iTALC";
#else
		QDir::homePath() + QDir::separator() + ".italc";
#endif

	return QDTNS( appData + QDir::separator() );
}



QString Path::privateKeyPath( ItalcCore::UserRoles role, QString baseDir )
{
	if( baseDir.isEmpty() )
	{
		baseDir = expand( ItalcCore::config->privateKeyBaseDir() );
	}
	else
	{
		baseDir += "/private";
	}
	QString d = baseDir + QDir::separator() + ItalcCore::userRoleName( role ) +
					QDir::separator() + "key";
	return QDTNS( d );
}



QString Path::publicKeyPath( ItalcCore::UserRoles role, QString baseDir )
{
	if( baseDir.isEmpty() )
	{
		baseDir = expand( ItalcCore::config->publicKeyBaseDir() );
	}
	else
	{
		baseDir += "/public";
	}
	QString d = baseDir + QDir::separator() + ItalcCore::userRoleName( role ) +
					QDir::separator() + "key";
	return QDTNS( d );
}




QString Path::systemConfigDataPath()
{
#ifdef ITALC_BUILD_WIN32
	return QDTNS( windowsConfigPath( CSIDL_COMMON_APPDATA ) + QDir::separator() + "iTALC/" );
#else
	return "/etc/italc/";
#endif
}





#ifdef ITALC_BUILD_WIN32
BOOL enablePrivilege( LPCTSTR lpszPrivilegeName, BOOL bEnable )
{
	HANDLE			hToken;
	TOKEN_PRIVILEGES	tp;
	LUID			luid;
	BOOL			ret;

	if( !OpenProcessToken( GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_READ, &hToken ) )
	{
		return FALSE;
	}

	if( !LookupPrivilegeValue( NULL, lpszPrivilegeName, &luid ) )
	{
		return FALSE;
	}

	tp.PrivilegeCount		   = 1;
	tp.Privileges[0].Luid	   = luid;
	tp.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;

	ret = AdjustTokenPrivileges( hToken, FALSE, &tp, 0, NULL, NULL );

	CloseHandle( hToken );

	return ret;
}
#endif



void activateWindow( QWidget * _w )
{
	_w->activateWindow();
	_w->raise();
#ifdef ITALC_BUILD_WIN32
	SetWindowPos( _w->winId(), HWND_TOPMOST, 0, 0, 0, 0,
						SWP_NOMOVE | SWP_NOSIZE );
	SetWindowPos( _w->winId(), HWND_NOTOPMOST, 0, 0, 0, 0,
						SWP_NOMOVE | SWP_NOSIZE );
#endif
}


} // end of namespace LocalSystem

