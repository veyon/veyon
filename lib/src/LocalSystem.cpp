/*
 * LocalSystem.cpp - namespace LocalSystem, providing an interface for
 *				   transparent usage of operating-system-specific functions
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifdef ITALC_BUILD_WIN32
#define _WIN32_WINNT 0x0501
#endif

#include <italcconfig.h>

#ifdef ITALC_BUILD_WIN32
#define _WIN32_WINNT 0x0501
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QMutex>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QDateTime>
#include <QtGui/QWidget>
#include <QtNetwork/QHostInfo>


#ifdef ITALC_BUILD_WIN32

#include <QtCore/QLibrary>

static const char * tr_accels = QT_TRANSLATE_NOOP(
		"QObject",
		"UPL (note for translators: the first three characters of "
		"this string are the accellerators (underlined characters) "
		"of the three input-fields in logon-dialog of windows - "
		"please keep this note as otherwise there are strange errors "
					"concerning logon-feature)" );

#include <windows.h>
#include <shlobj.h>
#include <psapi.h>		// TODO: remove when rewriting logonUser()
#include <wtsapi32.h>
#include <sddl.h>
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

#include "LocalSystem.h"
#include "minilzo.h"



static LocalSystem::p_pressKey __pressKey;
static QString __log_file;
static QFile * __debug_out = NULL;



static QString properLineEnding( QString _out )
{
	if( _out.right( 1 ) != "\012" )
	{
		_out += "\012";
	}
#ifdef ITALC_BUILD_WIN32
	if( _out.right( 1 ) != "\015" )
	{
		_out.replace( QString( "\012" ), QString( "\015\012" ) );
	}
#elif ITALC_BUILD_LINUX
	if( _out.right( 1 ) != "\015" ) // MAC
	{
		_out.replace( QString( "\012" ), QString( "\015" ) );
	}
#endif
	return( _out );
}



void msgHandler( QtMsgType _type, const char * _msg )
{
	if( LocalSystem::logLevel == 0 )
	{
		return ;
	}
#ifdef ITALC_BUILD_WIN32
	if( QString( _msg ).contains( "timers cannot be stopped",
							Qt::CaseInsensitive ) )
	{
		exit( 0 );
	}
#endif
	if( __debug_out == NULL )
	{
		QString tmp_path = QDir::rootPath() +
#ifdef ITALC_BUILD_WIN32
						"temp"
#else
						"tmp"
#endif
				;
		foreach( const QString s, QProcess::systemEnvironment() )
		{
			if( s.toLower().left( 5 ) == "temp=" )
			{
				tmp_path = s.toLower().mid( 5 );
				break;
			}
			else if( s.toLower().left( 4 ) == "tmp=" )
			{
				tmp_path = s.toLower().mid( 4 );
				break;
			}
		}
		if( !QDir( tmp_path ).exists() )
		{
			if( QDir( QDir::rootPath() ).mkdir( tmp_path ) )
			{
				QFile::setPermissions( tmp_path,
						QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
						QFile::ReadUser | QFile::WriteUser | QFile::ExeUser |
						QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup |
						QFile::ReadOther | QFile::WriteOther | QFile::ExeOther );
			}
		}
		const QString log_path = tmp_path + QDir::separator();
		__debug_out = new QFile( log_path + __log_file );
		__debug_out->open( QFile::WriteOnly | QFile::Append |
							QFile::Unbuffered );
	}

	QString out;
	switch( _type )
	{
		case QtDebugMsg:
			if( LocalSystem::logLevel > 8)
			{
				out = QDateTime::currentDateTime().toString() + QString( ": [debug] %1" ).arg( _msg ) + "\n";
			}
			break;
		case QtWarningMsg:
			if( LocalSystem::logLevel > 5 )
			{
				out = QDateTime::currentDateTime().toString() + QString( ": [warning] %1" ).arg( _msg ) + "\n";
			}
			break;
		case QtCriticalMsg:
			if( LocalSystem::logLevel > 3 )
			{
				out = QDateTime::currentDateTime().toString() + QString( ": [critical] %1" ).arg( _msg ) + "\n";
			}
			break;
		case QtFatalMsg:
			if( LocalSystem::logLevel > 1 )
			{
				out = QDateTime::currentDateTime().toString() + QString( ": [fatal] %1" ).arg( _msg ) + "\n";
			}
		default:
			out = QDateTime::currentDateTime().toString() + QString( ": [unknown] %1" ).arg( _msg ) + "\n";
			break;
	}
	if( out.trimmed().size() )
	{
		out = properLineEnding( out );
		__debug_out->write( out.toUtf8() );
		fprintf( stderr, "%s\n", out.toUtf8().constData() );
	}
}


void initResources( void )
{
	Q_INIT_RESOURCE(italc_core);
}

namespace LocalSystem
{

int logLevel = 6;


void initialize( p_pressKey _pk, const QString & _log_file )
{
	__pressKey = _pk;
	__log_file = _log_file;

	lzo_init();

	QCoreApplication::setOrganizationName( "iTALC Solutions" );
	QCoreApplication::setOrganizationDomain( "italcsolutions.org" );
	QCoreApplication::setApplicationName( "iTALC" );

	QSettings settings( QSettings::SystemScope, "iTALC Solutions", "iTALC" );

	if( settings.contains( "settings/LogLevel" ) )
	{
		logLevel = settings.value( "settings/LogLevel" ).toInt();
	}

	qInstallMsgHandler( msgHandler );

	initResources();
}


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




User::User( const QString &name, const QString &dom, const QString &fullname ) :
	m_userToken( 0 ),
	m_name( name ),
	m_domain( dom ),
	m_fullName( fullname )
{
#ifdef ITALC_BUILD_WIN32
	// try to look up the user -> domain
	DWORD sidLen = 256;
	char domain[256];
	domain[0] = 0;
	char *sid = new char[sidLen];
	DWORD domainLen = sizeof( domain );
	SID_NAME_USE snu;
	m_userToken = sid;

	if( !LookupAccountName( NULL,		// system name
							m_name.toAscii().constData(),
							m_userToken,		// SID
							&sidLen,
							domain,
							&domainLen,
							&snu ) )
	{
		qCritical( "Could not look up SID structure" );
		return;
	}

	if( m_domain.isEmpty() )
	{
		m_domain = domain;
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
		m_domain = domainName;
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
															User *processOwner )
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
															User *processOwner )
{
	DWORD pid = 0;
	PROCESSENTRY32 procEntry;

	HANDLE hSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if( hSnap == INVALID_HANDLE_VALUE )
	{
		return 0;
	}

	procEntry.dwSize = sizeof( PROCESSENTRY32 );

	if( !Process32First( hSnap, &procEntry ) )
	{
		CloseHandle( hSnap );
		return 0;
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
							int sessionId, User *processOwner )
{
	int pid = 0;
#ifdef ITALC_BUILD_WIN32
	pid = findProcessId_WTS( processName, sessionId, processOwner );
	if( pid < 0 )
	{
		pid = findProcessId_TH32( processName, sessionId, processOwner );
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

	OpenProcessToken( processHandle(), MAXIMUM_ALLOWED, &hToken );
	ImpersonateLoggedOnUser( hToken );

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

	CreateProcessAsUser(
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
				);

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
	if( WSAStartup( MAKEWORD( 1, 1 ), &info ) != 0 )
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
	__pressKey( _key, TRUE );
	__pressKey( _key, FALSE );
}


void logonUser( const QString & _uname, const QString & _passwd,
						const QString & _domain )
{
#ifdef ITALC_BUILD_WIN32

	// first check for process "explorer.exe" - if we find it, a user
	// is logged in and we do not send our key-sequences as it probably
	// disturbes user
	DWORD aProcesses[1024], cbNeeded;

	if( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
	{
		return;
	}

	DWORD cProcesses = cbNeeded / sizeof(DWORD);

	bool user_logged_on = FALSE;
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
		if( strcmp( szProcessName, "explorer.exe" ) == 0 )
		{
			user_logged_on = TRUE;
			break;
		}
	}

	if( user_logged_on )
	{
		return;
	}
/*
	// does not work :(
	enablePrivilege( SE_TCB_NAME, TRUE );
	HANDLE hToken;
	if( !LogonUser(
		(CHAR *)_uname.toAscii().constData(),
		(CHAR*)(_domain.isEmpty() ? "." : _domain.toAscii().constData()),
		(CHAR *)_passwd.toAscii().constData(),
		LOGON32_LOGON_INTERACTIVE,
		LOGON32_PROVIDER_DEFAULT,
		&hToken ) )
	{
		CloseHandle( hToken );
	}
	enablePrivilege( SE_TCB_NAME, FALSE );*/

	// disable caps lock
	if( GetKeyState( VK_CAPITAL ) & 1 )
	{
		INPUT input[2];
		ZeroMemory( input, sizeof( input ) );
		input[0].type = input[1].type = INPUT_KEYBOARD;
		input[0].ki.wVk = input[1].ki.wVk = VK_CAPITAL;
		input[1].ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput( 2, input, sizeof( INPUT ) );
	}

	pressAndReleaseKey( XK_Escape );
	pressAndReleaseKey( XK_Escape );

	// send Secure Attention Sequence (SAS) for making sure we can enter
	// username and password
	__pressKey( XK_Alt_L, TRUE );
	__pressKey( XK_Control_L, TRUE );
	pressAndReleaseKey( XK_Delete );
	__pressKey( XK_Control_L, FALSE );
	__pressKey( XK_Alt_L, FALSE );

	const ushort * accels = QObject::tr( tr_accels ).utf16();

	/* Need to handle 2 cases here; if an interactive login message is
		 * defined in policy, this window will be displayed with an "OK" button;
		 * if not the login window will be displayed. Sending a space will
		 * dismiss the message, but will also add a space to the currently
		 * selected field if the login windows is active. The solution is:
		 *  1. send the "username" field accelerator (which won't do anything
		 *	 to the message window, but will select the "username" field of
		 *	 the login window if it is active)
		 *  2. Send a space keypress to dismiss the message. (adds a space
		 *	 to username field of login window if active)
		 *  3. Send the "username" field accelerator again; which will select
		 *	 the username field for the case where the message was displayed
		 *  4. Send a backspace keypress to remove any space that was added to
		 *	 the username field if there is no message.
		 */
	__pressKey( XK_Alt_L, TRUE );
	pressAndReleaseKey( accels[0] );
	__pressKey( XK_Alt_L, FALSE );

	pressAndReleaseKey( XK_space );

	__pressKey( XK_Alt_L, TRUE );
	pressAndReleaseKey( accels[0] );
	__pressKey( XK_Alt_L, FALSE );

	pressAndReleaseKey( XK_BackSpace );
#endif

	for( int i = 0; i < _uname.size(); ++i )
	{
		pressAndReleaseKey( _uname.utf16()[i] );
	}

#ifdef ITALC_BUILD_WIN32
	__pressKey( XK_Alt_L, TRUE );
	pressAndReleaseKey( accels[1] );
	__pressKey( XK_Alt_L, FALSE );
#else
	pressAndReleaseKey( XK_Tab );
#endif

	for( int i = 0; i < _passwd.size(); ++i )
	{
		pressAndReleaseKey( _passwd.utf16()[i] );
	}

#ifdef ITALC_BUILD_WIN32
	if( !_domain.isEmpty() )
	{
		__pressKey( XK_Alt_L, TRUE );
		pressAndReleaseKey( accels[2] );
		__pressKey( XK_Alt_L, FALSE );
		for( int i = 0; i < _domain.size(); ++i )
		{
			pressAndReleaseKey( _domain.utf16()[i] );
		}
	}
#endif

	pressAndReleaseKey( XK_Return );
}




static const QString userRoleNames[] =
{
	"none",
	"teacher",
	"admin",
	"supporter",
	"other"
} ;


QString userRoleName( const ItalcCore::UserRoles _role )
{
	return( userRoleNames[_role] );
}


inline QString keyPath( const ItalcCore::UserRoles _role, const QString _group,
							bool _only_path )
{
	QSettings settings( QSettings::SystemScope, "iTALC Solutions",
								"iTALC" );
	if( _role <= ItalcCore::RoleNone || _role >= ItalcCore::RoleCount )
	{
		qWarning( "invalid role" );
		return( "" );
	}
	const QString fallback_dir =
#ifdef ITALC_BUILD_LINUX
		"/etc/italc/keys/"
#else
#ifdef ITALC_BUILD_WIN32
		"c:\\italc\\keys\\"
#endif
#endif
		+ _group + QDir::separator() + userRoleNames[_role] +
						QDir::separator() +
						( _only_path ? "" : "key" );
	const QString val = settings.value( "keypaths" + _group + "/" +
					userRoleNames[_role] ).toString();
	if( val.isEmpty() )
	{
		settings.setValue( "keypaths" + _group + "/" +
					userRoleNames[_role], fallback_dir );
		return( fallback_dir );
	}
	else
	{
		if( _only_path && val.right( 4 ) == "\\key" )
		{
			return( val.left( val.size() - 4 ) );
		}
	}
	return( val );
}


QString privateKeyPath( const ItalcCore::UserRoles _role, bool _only_path )
{
	return( keyPath( _role, "private", _only_path ) );
}


QString publicKeyPath( const ItalcCore::UserRoles _role, bool _only_path )
{
	return( keyPath( _role, "public", _only_path ) );
}




void setKeyPath( QString _path, const ItalcCore::UserRoles _role,
							const QString _group )
{
	_path = _path.left( 1 ) + _path.mid( 1 ).
		replace( QString( QDir::separator() ) + QDir::separator(),
							QDir::separator() );

	QSettings settings( QSettings::SystemScope, "iTALC Solutions",
								"iTALC" );
	if( _role <= ItalcCore::RoleNone || _role >= ItalcCore::RoleCount )
	{
		qWarning( "invalid role" );
		return;
	}
	settings.setValue( "keypaths" + _group + "/" +
						userRoleNames[_role], _path );
}


void setPrivateKeyPath( const QString & _path, const ItalcCore::UserRoles _role )
{
	setKeyPath( _path, _role, "private" );
}




void setPublicKeyPath( const QString & _path, const ItalcCore::UserRoles _role )
{
	setKeyPath( _path, _role, "public" );
}




QString snapshotDir( void )
{
	QSettings settings;
	return( settings.value( "paths/snapshots",
#ifdef ITALC_BUILD_WIN32
				windowsConfigPath( CSIDL_PERSONAL ) +
					QDir::separator() +
					QObject::tr( "iTALC-snapshots" )
#else
				personalConfigDir() + "snapshots"
#endif
					).toString() + QDir::separator() );
}




QString globalConfigPath( void )
{
	QSettings settings;
	return( settings.value( "paths/globalconfig", personalConfigDir() +
					"globalconfig.xml" ).toString() );
}




QString personalConfigDir( void )
{
	QSettings settings;
	const QString d = settings.value( "paths/personalconfigdir" ).toString();
	return( d.isEmpty() ?
#ifdef ITALC_BUILD_WIN32
				windowsConfigPath( CSIDL_APPDATA ) +
						QDir::separator() + "iTALC"
#else
				QDir::homePath() + QDir::separator() +
								".italc"
#endif
				+ QDir::separator()
		:
				d + QDir::separator() );
}




QString personalConfigPath( void )
{
	QSettings settings;
	const QString d = settings.value( "paths/personalconfig" ).toString();
	return( d.isEmpty() ?
				personalConfigDir() + "personalconfig.xml"
			:
				d );
}




QString globalStartmenuDir( void )
{
#ifdef ITALC_BUILD_WIN32
	return( windowsConfigPath( CSIDL_COMMON_STARTMENU ) +
							QDir::separator() );
#else
	return( "/usr/share/applnk/Applications/" );
#endif
}




QString parameter( const QString & _name )
{
	return( QSettings().value( "parameters/" + _name ).toString() );
}




bool ensurePathExists( const QString & _path )
{
	if( _path.isEmpty() || QDir( _path ).exists() )
	{
		return( TRUE );
	}

	QString p = QDir( _path ).absolutePath();
	if( !QFileInfo( _path ).isDir() )
	{
		p = QFileInfo( _path ).absolutePath();
	}
	QStringList dirs;
	while( !QDir( p ).exists() && !p.isEmpty() )
	{
		dirs.push_front( QDir( p ).dirName() );
		p.chop( dirs.front().size() + 1 );
	}
	if( !p.isEmpty() )
	{
		return( QDir( p ).mkpath( dirs.join( QDir::separator() ) ) );
	}
	return( FALSE );
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

