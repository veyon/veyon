/*
 * LocalSystem.cpp - namespace LocalSystem, providing an interface for
 *				   transparent usage of operating-system-specific functions
 *
 * Copyright (c) 2006-2017 Tobias Junghans <tobydox@users.sf.net>
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

#include <QGuiApplication>
#include <qpa/qplatformnativeinterface.h>

#include <shlobj.h>
#include <wtsapi32.h>
#include <userenv.h>

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
#include "PlatformCoreFunctions.h"
#include "PlatformUserFunctions.h"




namespace LocalSystem
{


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



QString Path::expand( QString path )
{
	QString p = QDTNS( path.replace( QStringLiteral( "$HOME" ), QDir::homePath() ).
						replace( QStringLiteral( "%HOME%" ), QDir::homePath() ).
						replace( QStringLiteral( "$PROFILE" ), QDir::homePath() ).
						replace( QStringLiteral( "%PROFILE%" ), QDir::homePath() ).
						replace( QStringLiteral( "$APPDATA" ), VeyonCore::platform().coreFunctions().personalAppDataPath() ).
						replace( QStringLiteral( "%APPDATA%" ), VeyonCore::platform().coreFunctions().personalAppDataPath() ).
						replace( QStringLiteral( "$GLOBALAPPDATA" ), VeyonCore::platform().coreFunctions().globalAppDataPath() ).
						replace( QStringLiteral( "%GLOBALAPPDATA%" ), VeyonCore::platform().coreFunctions().globalAppDataPath() ).
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
	const auto personalAppDataPath = VeyonCore::platform().coreFunctions().personalAppDataPath();
	const auto globalAppDataPath = VeyonCore::platform().coreFunctions().globalAppDataPath();
	if( path.startsWith( personalAppDataPath, cs ) )
	{
		path.replace( personalAppDataPath, envVar.arg( QStringLiteral( "APPDATA" ) ) );
	}
	else if( path.startsWith( globalAppDataPath, cs ) )
	{
		path.replace( globalAppDataPath, envVar.arg( QStringLiteral( "GLOBALAPPDATA" ) ) );
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




#ifdef VEYON_BUILD_WIN32
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

