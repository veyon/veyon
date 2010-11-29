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

#include "ItalcCore.h"

#ifdef ITALC_BUILD_WIN32
#include <windef.h>
#endif

#define QDTNS(x)	QDir::toNativeSeparators(x)

class QWidget;


namespace LocalSystem
{

	class Desktop
	{
	public:
		Desktop( const QString &name = QString() );
		Desktop( const Desktop &desktop );

		const QString &name() const
		{
			return m_name;
		}

		bool isActive() const
		{
			return name() == activeDesktop().name();
		}

		static Desktop activeDesktop();
		static Desktop screenLockDesktop();


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

		const Token &userToken() const
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

		QString homePath() const;


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
									const User *processOwner = NULL );

		User *getProcessOwner();

		Handle processHandle()
		{
			return m_processHandle;
		}

		Handle runAsUser( const QString &proc,
									const QString &desktop = QString() );
		static bool isRunningAsAdmin();
		static bool runAsAdmin( const QString &proc, const QString &parameters );

	private:
		Handle m_processHandle;

	} ;


	class Path
	{
	public:
		static QString expand( QString path );
		static QString shrink( QString path );
		static bool ensurePathExists( const QString &path );

		static QString personalConfigDataPath();
		static QString systemConfigDataPath();

		static QString privateKeyPath( ItalcCore::UserRoles role,
												QString baseDir = QString() );
		static QString publicKeyPath( ItalcCore::UserRoles role,
												QString baseDir = QString() );
	} ;


	void sleep( const int _ms );

	void broadcastWOLPacket( const QString & _mac );

	void powerDown();
	void reboot();

	void logonUser( const QString & _uname, const QString & _pw,
						const QString & _domain );
	void logoutUser();

#ifdef ITALC_BUILD_WIN32
	BOOL enablePrivilege( LPCTSTR lpszPrivilegeName, BOOL bEnable );
#endif

	void activateWindow( QWidget * _window );

}

#endif
