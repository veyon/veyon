/*
 * LocalSystem.h - misc. platform-specific stuff
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

#ifndef _LOCAL_SYSTEM_H
#define _LOCAL_SYSTEM_H

#include <QtCore/QString>

#include "ItalcCore.h"

#ifdef ITALC_BUILD_WIN32
#include <windef.h>
#endif

class QWidget;


namespace LocalSystem
{
	extern int logLevel;

	typedef void (*p_pressKey)( int _key, bool _down );


	void initialize( p_pressKey _pk, const QString & _log_file );

	class Desktop
	{
	public:
		Desktop( const QString &name = QString() );
		Desktop( const Desktop &desktop );

		const QString &name() const
		{
			return m_name;
		}

		static Desktop activeDesktop();

	private:
		QString m_name;
	} ;

	class User
	{
	public:
#ifdef ITALC_BUILD_WIN32
		typedef PSID Token;
#else
		typedef int Token;
#endif
		User( const QString &name, const QString &domain = QString(),
									const QString &fullname = QString() );
		User( Token token );
		User( const User &user );
		~User();

		static User loggedOnUser();

		const Token userToken() const
		{
			return m_userToken;
		}

		Token userToken()
		{
			return m_userToken;
		}

		const QString &name() const
		{
			return m_name;
		}

		const QString &domain() const
		{
			return m_domain;
		}

		const QString &fullName()
		{
			// full name lookups are quite expensive, therefore do them
			// on-demand and only if not done before
			if( m_fullName.isEmpty() )
			{
				lookupFullName();
				if( m_fullName.isEmpty() )
				{
					m_fullName = name();
				}
			}
			return m_fullName;
		}


	private:
		void lookupNameAndDomain();
		void lookupFullName();

		Token m_userToken;
		QString m_name;
		QString m_domain;
		QString m_fullName;

	} ;

	class Process
	{
	public:
#ifdef ITALC_BUILD_WIN32
		typedef HANDLE Handle;
#else
		typedef int Handle;
#endif
		Process( int pid = -1 );
		~Process();

		static int findProcessId( const QString &processName,
									int sessionId = -1,
									User *processOwner = NULL );

		User *getProcessOwner();

		Handle processHandle()
		{
			return m_processHandle;
		}

		Handle runAsUser( const QString &proc,
									const QString &desktop = QString() );

	private:
		Handle m_processHandle;

	} ;


	void sleep( const int _ms );

	void broadcastWOLPacket( const QString & _mac );

	void powerDown( void );
	void reboot( void );

	void logonUser( const QString & _uname, const QString & _pw,
						const QString & _domain );
	void logoutUser( void );

	QString privateKeyPath( const ItalcCore::UserRoles _role,
						bool _only_path = FALSE );
	QString publicKeyPath( const ItalcCore::UserRoles _role,
						bool _only_path = FALSE );

	void setPrivateKeyPath( const QString & _path,
						const ItalcCore::UserRoles _role );
	void setPublicKeyPath( const QString & _path,
						const ItalcCore::UserRoles _role );

	QString snapshotDir( void );
	QString globalConfigPath( void );
	QString personalConfigDir( void );
	QString personalConfigPath( void );

	QString globalStartmenuDir( void );

	QString parameter( const QString & _name );

	bool ensurePathExists( const QString & _path );

	QString ip( void );

	QString userRoleName( const ItalcCore::UserRoles _role );

#ifdef ITALC_BUILD_WIN32
	BOOL enablePrivilege( LPCTSTR lpszPrivilegeName, BOOL bEnable );
	QString windowsConfigPath( int _type );
#endif

	void activateWindow( QWidget * _window );

}

#endif
