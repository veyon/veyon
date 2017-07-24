/*
 * LocalSystem.cpp - namespace LocalSystem, providing an interface for
 *				   transparent usage of operating-system-specific functions
 *
 * Copyright (c) 2006-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of Veyon - http://veyon.io
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

#include "VeyonCore.h"

#include <QDir>
#include <QProcess>
#include <QWidget>
#include <QHostInfo>

#ifdef VEYON_BUILD_WIN32

#include <QLibrary>
#include <QGuiApplication>
#include <qpa/qplatformnativeinterface.h>

#include <shlobj.h>
#include <wtsapi32.h>
#include <sddl.h>
#include <userenv.h>
#include <lm.h>
#include <reason.h>


namespace LocalSystem
{

// taken from qt-win-opensource-src-4.2.2/src/corelib/io/qsettings.cpp
QString windowsConfigPath( int type )
{
	QString result;

	QLibrary library( "shell32" );
	typedef BOOL( WINAPI* GetSpecialFolderPath )( HWND, LPTSTR, int, BOOL );
	GetSpecialFolderPath SHGetSpecialFolderPath = (GetSpecialFolderPath)
				library.resolve( "SHGetSpecialFolderPathW" );
	if( SHGetSpecialFolderPath )
	{
		wchar_t path[MAX_PATH];
		SHGetSpecialFolderPath( 0, path, type, FALSE );
		result = QString::fromWCharArray( path );
	}
	return( result );
}

}

#endif


#ifdef VEYON_HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef VEYON_HAVE_PWD_H
#include <pwd.h>
#endif

#include "VeyonConfiguration.h"
#include "LocalSystem.h"
#include "Logger.h"




namespace LocalSystem
{


Desktop::Desktop( const QString &name ) :
	m_name( name )
{
#ifdef VEYON_BUILD_WIN32
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

#ifdef VEYON_BUILD_WIN32
	HDESK desktopHandle = OpenInputDesktop( 0, TRUE, DESKTOP_READOBJECTS );

	wchar_t dname[256];
	dname[0] = 0;
	if( GetUserObjectInformation( desktopHandle, UOI_NAME, dname,
									sizeof( dname ) / sizeof( wchar_t ), NULL ) )
	{
		deskName = QString( QStringLiteral( "winsta0\\%1" ) ).arg( QString::fromWCharArray( dname ) );
	}
	CloseDesktop( desktopHandle );
#endif

	return Desktop( deskName );
}



Desktop Desktop::screenLockDesktop()
{
	return Desktop( QStringLiteral( "ScreenLockSlaveDesktop" ) );
}




User::User( const QString &name, const QString &dom, const QString &fullname ) :
	m_userToken( 0 ),
	m_name( name ),
	m_domain( dom ),
	m_fullName( fullname )
{
#ifdef VEYON_BUILD_WIN32
	// try to look up the user -> domain
	DWORD sidLen = 256;
	wchar_t domainName[MAX_PATH];
	domainName[0] = 0;
	char *sid = new char[sidLen];
	DWORD domainLen = MAX_PATH;
	SID_NAME_USE snu;
	m_userToken = sid;

	if( !LookupAccountName( NULL,		// system name
							(LPCWSTR) m_name.utf16(),
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
		wchar_t computerName[MAX_PATH];
		DWORD size = MAX_PATH;
		GetComputerName( computerName, &size );

		if( QString::fromWCharArray( domainName ) !=
			QString::fromWCharArray( computerName ) )
		{
			m_domain = QString::fromWCharArray( domainName );
		}
	}

#else
	m_userToken = getuid();
#endif
}


#ifdef VEYON_BUILD_WIN32
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
#ifdef VEYON_BUILD_WIN32
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
#ifdef VEYON_BUILD_WIN32
	copySid( user.userToken(), m_userToken );
#endif
}



User::~User()
{
#ifdef VEYON_BUILD_WIN32
	if( m_userToken )
	{
		delete[] (char *) m_userToken;
	}
#endif
}



QString User::stripDomain( QString username )
{
	// remove the domain part of username (e.g. "EXAMPLE.COM\Teacher" -> "Teacher")
	int domainSeparator = username.indexOf( '\\' );
	if( domainSeparator >= 0 )
	{
		return username.mid( domainSeparator + 1 );
	}

	return username;
}



#ifdef VEYON_BUILD_WIN32
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
		result = QString::fromWCharArray( pBuffer );
	}
	WTSFreeMemory( pBuffer );

	return result;
}
#endif



User User::loggedOnUser()
{
	QString username = QStringLiteral( "unknown" );
	QString domainName = QHostInfo::localDomainName();

#ifdef VEYON_BUILD_WIN32

	DWORD sessionId = WTSGetActiveConsoleSessionId();

	username = querySessionInformation( sessionId, WTSUserName );
	domainName = querySessionInformation( sessionId, WTSDomainName );

	// check whether we just got the name of the local computer
	if( !domainName.isEmpty() )
	{
		wchar_t computerName[MAX_PATH];
		DWORD size = MAX_PATH;
		GetComputerName( computerName, &size );

		if( domainName == QString::fromWCharArray( computerName ) )
		{
			// reset domain name as storing the local computer's name
			// doesn't help here
			domainName = QString();
		}
	}

#else

	char * envUser = getenv( "USER" );

#ifdef VEYON_HAVE_PWD_H
	struct passwd * pw_entry = nullptr;
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
		if ( !( shell.endsWith( QStringLiteral( "/false" ) ) ||
				shell.endsWith( QStringLiteral( "/true" ) ) ||
				shell.endsWith( QStringLiteral( "/null" ) ) ||
				shell.endsWith( QStringLiteral( "/nologin" ) ) ) )
		{
			username = QString::fromUtf8( pw_entry->pw_name );
		}
	}
#endif	/* VEYON_HAVE_PWD_H */

	if( username.isEmpty() )
	{
		username = QString::fromUtf8( envUser );
	}

#endif

	return User( username, domainName );
}




QString User::homePath() const
{
	QString homePath = QDir::homePath();

#ifdef VEYON_BUILD_WIN32
	LocalSystem::Process userProcess(
				LocalSystem::Process::findProcessId( QString(), -1, this ) );
	HANDLE hToken;
	if( OpenProcessToken( userProcess.processHandle(),
									MAXIMUM_ALLOWED, &hToken ) )
	{
		wchar_t userProfile[MAX_PATH];
		DWORD size = MAX_PATH;
		if( GetUserProfileDirectory( hToken, userProfile, &size ) )
		{
			homePath = QString::fromWCharArray( userProfile );
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

#ifdef VEYON_BUILD_WIN32
	DWORD accNameLen = 0;
	DWORD domainNameLen = 0;
	SID_NAME_USE snu;
	LookupAccountSid( NULL, userToken(), NULL, &accNameLen, NULL,
							&domainNameLen, &snu );
	if( accNameLen == 0 || domainNameLen == 0 )
	{
		return;
	}

	wchar_t* accName = new wchar_t[accNameLen];
	wchar_t* domainName = new wchar_t[domainNameLen];
	LookupAccountSid( NULL, userToken(), accName, &accNameLen,
						domainName, &domainNameLen, &snu );

	if( m_name.isEmpty() )
	{
		m_name = QString::fromWCharArray( accName );
	}

	if( m_domain.isEmpty() )
	{
		wchar_t computerName[MAX_PATH];
		DWORD size = MAX_PATH;
		GetComputerName( computerName, &size );

		if( QString::fromWCharArray( domainName ) !=
			QString::fromWCharArray( computerName ) )
		{
			m_domain = QString::fromWCharArray( domainName );
		}
	}

	delete[] accName;
	delete[] domainName;

#else	/* VEYON_BUILD_WIN32 */

	struct passwd * pw_entry = getpwuid( m_userToken );
	if( pw_entry )
	{
		QString shell( pw_entry->pw_shell );

		// Skip not real users
		if ( !( shell.endsWith( QStringLiteral( "/false" ) ) ||
				shell.endsWith( QStringLiteral( "/true" ) ) ||
				shell.endsWith( QStringLiteral( "/null" ) ) ||
				shell.endsWith( QStringLiteral( "/nologin" ) ) ) )
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

#ifdef VEYON_BUILD_WIN32
	// try to retrieve user's full name from domain

	PBYTE dc = NULL;	// domain controller
	if( NetGetDCName( NULL, (LPWSTR) m_domain.utf16(), &dc ) != NERR_Success )
	{
		dc = NULL;
	}

	LPUSER_INFO_2 pBuf = NULL;
	NET_API_STATUS nStatus = NetUserGetInfo( (LPWSTR)dc, (LPWSTR) m_name.utf16(), 2,
												(LPBYTE *) &pBuf );
	if( nStatus == NERR_Success && pBuf != NULL )
	{
		m_fullName = QString::fromWCharArray( pBuf->usri2_full_name );
	}

	if( pBuf != NULL )
	{
		NetApiBufferFree( pBuf );
	}
	if( dc != NULL )
	{
		NetApiBufferFree( dc );
	}
#else

#ifdef VEYON_HAVE_PWD_H
	struct passwd * pw_entry = getpwnam( m_name.toUtf8().constData() );
	if( !pw_entry )
	{
		pw_entry = getpwuid( m_userToken );
	}
	if( pw_entry )
	{
		QString shell( pw_entry->pw_shell );

		// Skip not real users
		if ( !( shell.endsWith( QStringLiteral( "/false" ) ) ||
				shell.endsWith( QStringLiteral( "/true" ) ) ||
				shell.endsWith( QStringLiteral( "/null" ) ) ||
				shell.endsWith( QStringLiteral( "/nologin" ) ) ) )
		{
			m_fullName = QString::fromUtf8( pw_entry->pw_gecos ).split( ',' ).first();
		}
	}
#endif

#endif
}





Process::Process( int pid ) :
	m_processHandle( 0 )
{
#ifdef VEYON_BUILD_WIN32
	if( pid >= 0 )
	{
		m_processHandle = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pid );
	}
#endif
}



Process::~Process()
{
#ifdef VEYON_BUILD_WIN32
	if( m_processHandle )
	{
		CloseHandle( m_processHandle );
	}
#endif
}



#ifdef VEYON_BUILD_WIN32
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
			processName.compare( QString::fromWCharArray( pProcessInfo[proc].pProcessName ),
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
			processName.compare( QString::fromWCharArray( procEntry.szExeFile ),
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
	//qDebug() << "Process::findProcessId(" << processName << sessionId << processOwner << ")";

	int pid = -1;
#ifdef VEYON_BUILD_WIN32
	pid = findProcessId_WTS( processName, sessionId, processOwner );
	//ilogf( Debug, "findProcessId_WTS(): %d", pid );
	if( pid < 0 )
	{
		pid = findProcessId_TH32( processName, sessionId, processOwner );
		//ilogf( Debug, "findProcessId_TH32(): %d", pid );
	}
#endif
	return pid;
}




User *Process::getProcessOwner()
{
#ifdef VEYON_BUILD_WIN32
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
	return nullptr;
#endif
}




Process::Handle Process::runAsUser( const QString &proc,
									const QString &desktop )
{
#ifdef VEYON_BUILD_WIN32
	enablePrivilege( QString::fromWCharArray( SE_ASSIGNPRIMARYTOKEN_NAME ), true );
	enablePrivilege( QString::fromWCharArray( SE_INCREASE_QUOTA_NAME ), true );
	HANDLE hToken = NULL;

	if( !OpenProcessToken( processHandle(), MAXIMUM_ALLOWED, &hToken ) )
	{
		ilog_failedf( "OpenProcessToken()", "%d", GetLastError() );
		return NULL;
	}

	LPVOID userEnvironment = nullptr;
	if( CreateEnvironmentBlock( &userEnvironment, hToken, FALSE ) == false )
	{
		ilog_failedf( "CreateEnvironmentBlock()", "%d", GetLastError() );
		return nullptr;
	}

	PWSTR profileDir = nullptr;
	if( SHGetKnownFolderPath( FOLDERID_Profile, 0, hToken, &profileDir ) != S_OK )
	{
		ilog_failedf( "SHGetKnownFolderPath()", "%d", GetLastError() );
		return nullptr;
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
		si.lpDesktop = (LPWSTR) desktop.utf16();
	}
	HANDLE hNewToken = NULL;

	DuplicateTokenEx( hToken, TOKEN_ASSIGN_PRIMARY|TOKEN_ALL_ACCESS, NULL,
						SecurityImpersonation, TokenPrimary, &hNewToken );

	if( !CreateProcessAsUser(
				hNewToken,			// client's access token
				NULL,			  // file to execute
				(LPWSTR) proc.utf16(),	 // command line
				NULL,			  // pointer to process SECURITY_ATTRIBUTES
				NULL,			  // pointer to thread SECURITY_ATTRIBUTES
				FALSE,			 // handles are not inheritable
				CREATE_UNICODE_ENVIRONMENT | NORMAL_PRIORITY_CLASS,   // creation flags
				userEnvironment,			  // pointer to new environment block
				profileDir,			  // name of current directory
				&si,			   // pointer to STARTUPINFO structure
				&pi				// receives information about new process
				) )
	{
		ilog_failedf( "CreateProcessAsUser()", "%d", GetLastError() );
		return NULL;
	}

	CoTaskMemFree( profileDir );
	DestroyEnvironmentBlock( userEnvironment );

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
#ifdef VEYON_BUILD_WIN32
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
#ifdef VEYON_BUILD_WIN32
	SHELLEXECUTEINFO sei = { sizeof(sei) };
	sei.lpVerb = L"runas";
	sei.lpFile = (LPWSTR) appPath.utf16();
	sei.hwnd = GetForegroundWindow();
	sei.lpParameters = (LPWSTR) parameters.utf16();
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
#ifdef VEYON_BUILD_WIN32
	Sleep( static_cast<unsigned int>( _ms ) );
#else
	struct timespec ts = { _ms / 1000, ( _ms % 1000 ) * 1000 * 1000 } ;
	nanosleep( &ts, nullptr );
#endif
}



void logonUser( const QString & _uname, const QString & _passwd,
						const QString & _domain )
{
	// TODO
}



void logoutUser()
{
#ifdef VEYON_BUILD_WIN32
	ExitWindowsEx( EWX_LOGOFF | (EWX_FORCE | EWX_FORCEIFHUNG), SHTDN_REASON_MAJOR_OTHER );
#else
	// Gnome logout, 2 = forced mode (don't wait for unresponsive processes)
	QProcess::startDetached( QStringLiteral( "dbus-send --session --dest=org.gnome.SessionManager --type=method_call /org/gnome/SessionManager org.gnome.SessionManager.Logout uint32:2" ) );
	// KDE 3 logout
	QProcess::startDetached( QStringLiteral( "dcop ksmserver ksmserver logout 0 0 0" ) );
	// KDE 4 logout
	QProcess::startDetached( QStringLiteral( "qdbus org.kde.ksmserver /KSMServer logout 0 0 0" ) );
	// KDE 5 logout
	QProcess::startDetached( QStringLiteral( "dbus-send --dest=org.kde.ksmserver /KSMServer org.kde.KSMServerInterface.logout int32:0 int32:2 int32:0" ) );
	// Xfce logout
	QProcess::startDetached( QStringLiteral("xfce4-session-logout --logout") );
#endif
}



QString Path::expand( QString path )
{
	QString p = QDTNS( path.replace( QStringLiteral( "$HOME" ), QDir::homePath() ).
						replace( QStringLiteral( "%HOME%" ), QDir::homePath() ).
						replace( QStringLiteral( "$PROFILE" ), QDir::homePath() ).
						replace( QStringLiteral( "%PROFILE%" ), QDir::homePath() ).
						replace( QStringLiteral( "$APPDATA" ), personalConfigDataPath() ).
						replace( QStringLiteral( "%APPDATA%" ), personalConfigDataPath() ).
						replace( QStringLiteral( "$GLOBALAPPDATA" ), systemConfigDataPath() ).
						replace( QStringLiteral( "%GLOBALAPPDATA%" ), systemConfigDataPath() ).
						replace( QStringLiteral( "$TMP" ), QDir::tempPath() ).
						replace( QStringLiteral( "$TEMP" ), QDir::tempPath() ).
						replace( QStringLiteral( "%TMP%" ), QDir::tempPath() ).
						replace( QStringLiteral( "%TEMP%" ), QDir::tempPath() ) );
	// remove duplicate directory separators - however skip the first two chars
	// as they might specify an UNC path on Windows
	if( p.length() > 3 )
	{
		return p.left( 2 ) + p.mid( 2 ).replace(
									QString( QStringLiteral( "%1%1" ) ).arg( QDir::separator() ),
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
#ifdef VEYON_BUILD_WIN32
	const Qt::CaseSensitivity cs = Qt::CaseInsensitive;
	const QString envVar( QStringLiteral( "%%1%\\" ) );
#else
	const Qt::CaseSensitivity cs = Qt::CaseSensitive;
	const QString envVar( QStringLiteral( "$%1/" ) );
#endif
	if( path.startsWith( personalConfigDataPath(), cs ) )
	{
		path.replace( personalConfigDataPath(), envVar.arg( QStringLiteral( "APPDATA" ) ) );
	}
	else if( path.startsWith( systemConfigDataPath(), cs ) )
	{
		path.replace( systemConfigDataPath(), envVar.arg( QStringLiteral( "GLOBALAPPDATA" ) ) );
	}
	else if( path.startsWith( QDTNS( QDir::homePath() ), cs ) )
	{
		path.replace( QDTNS( QDir::homePath() ), envVar.arg( QStringLiteral( "HOME" ) ) );
	}
	else if( path.startsWith( QDTNS( QDir::tempPath() ), cs ) )
	{
		path.replace( QDTNS( QDir::tempPath() ), envVar.arg( QStringLiteral( "TEMP" ) ) );
	}

	// remove duplicate directory separators - however skip the first two chars
	// as they might specify an UNC path on Windows
	if( path.length() > 3 )
	{
		return QDTNS( path.left( 2 ) + path.mid( 2 ).replace(
									QString( QStringLiteral( "%1%1" ) ).arg( QDir::separator() ), QDir::separator() ) );
	}

	return QDTNS( path );
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
#ifdef VEYON_BUILD_WIN32
		LocalSystem::windowsConfigPath( CSIDL_APPDATA ) + QDir::separator() + "Veyon";
#else
		QDir::homePath() + QDir::separator() + ".veyon";
#endif

	return QDTNS( appData + QDir::separator() );
}



QString Path::privateKeyPath( VeyonCore::UserRoles role, QString baseDir )
{
	if( baseDir.isEmpty() )
	{
		baseDir = expand( VeyonCore::config().privateKeyBaseDir() );
	}
	else
	{
		baseDir += QDir::separator() + QStringLiteral( "private" );
	}
	QString d = baseDir + QDir::separator() + VeyonCore::userRoleName( role ) +
					QDir::separator() + QStringLiteral( "key" );
	return QDTNS( d );
}



QString Path::publicKeyPath( VeyonCore::UserRoles role, QString baseDir )
{
	if( baseDir.isEmpty() )
	{
		baseDir = expand( VeyonCore::config().publicKeyBaseDir() );
	}
	else
	{
		baseDir += QDir::separator() + QStringLiteral( "public" );
	}
	QString d = baseDir + QDir::separator() + VeyonCore::userRoleName( role ) +
					QDir::separator() + QStringLiteral( "key" );
	return QDTNS( d );
}




QString Path::systemConfigDataPath()
{
#ifdef VEYON_BUILD_WIN32
	return QDTNS( windowsConfigPath( CSIDL_COMMON_APPDATA ) + QDir::separator() + QStringLiteral( "Veyon" ) + QDir::separator() );
#else
	return QStringLiteral( "/etc/veyon/" );
#endif
}





#ifdef VEYON_BUILD_WIN32
BOOL enablePrivilege( const QString& privilegeName, bool enable )
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

	if( !LookupPrivilegeValue( NULL, (LPWSTR) privilegeName.utf16(), &luid ) )
	{
		return FALSE;
	}

	tp.PrivilegeCount		   = 1;
	tp.Privileges[0].Luid	   = luid;
	tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

	ret = AdjustTokenPrivileges( hToken, FALSE, &tp, 0, NULL, NULL );

	CloseHandle( hToken );

	return ret;
}
#endif


#ifdef VEYON_BUILD_WIN32
static QWindow* windowForWidget( const QWidget* widget )
{
	QWindow* window = widget->windowHandle();
	if( window )
	{
		return window;
	}

	const QWidget* nativeParent = widget->nativeParentWidget();
	if( nativeParent )
	{
		return nativeParent->windowHandle();
	}

	return 0;
}


HWND getHWNDForWidget(const QWidget* widget )
{
	QWindow* window = windowForWidget( widget );
	if( window )
	{
		QPlatformNativeInterface* interfacep = QGuiApplication::platformNativeInterface();
		return static_cast<HWND>( interfacep->nativeResourceForWindow( QByteArrayLiteral( "handle" ), window ) );
	}
	return 0;
}
#endif


void activateWindow( QWidget* w )
{
	w->activateWindow();
	w->raise();
#ifdef VEYON_BUILD_WIN32
	SetWindowPos( getHWNDForWidget(w), HWND_TOPMOST, 0, 0, 0, 0,
						SWP_NOMOVE | SWP_NOSIZE );
	SetWindowPos( getHWNDForWidget(w), HWND_NOTOPMOST, 0, 0, 0, 0,
						SWP_NOMOVE | SWP_NOSIZE );
#endif
}

} // end of namespace LocalSystem

