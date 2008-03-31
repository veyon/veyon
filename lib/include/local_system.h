/*
 * local_system.h - misc. platform-specific stuff
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "isd_base.h"

#ifdef BUILD_WIN32
#include <windef.h>
#endif

class QWidget;

extern IC_DllExport QByteArray __appInternalChallenge;


namespace localSystem
{
	extern int IC_DllExport logLevel;

	typedef void (*p_pressKey)( int _key, bool _down );


	int IC_DllExport freePort( int _default_port );

	void IC_DllExport initialize( p_pressKey _pk, const QString & _log_file );

	void IC_DllExport sleep( const int _ms );

	void IC_DllExport execInTerminal( const QString & _cmds );

	void IC_DllExport broadcastWOLPacket( const QString & _mac );

	void /*IC_DllExport*/ powerDown( void );
	void /*IC_DllExport*/ reboot( void );

	void IC_DllExport logonUser( const QString & _uname, const QString & _pw,
						const QString & _domain );
	void /*IC_DllExport*/ logoutUser( void );

	QString /*IC_DllExport*/ currentUser( void );

	QString IC_DllExport privateKeyPath( const ISD::userRoles _role,
						bool _only_path = FALSE );
	QString IC_DllExport publicKeyPath( const ISD::userRoles _role,
						bool _only_path = FALSE );

	void IC_DllExport setPrivateKeyPath( const QString & _path,
						const ISD::userRoles _role );
	void IC_DllExport setPublicKeyPath( const QString & _path,
						const ISD::userRoles _role );

	QString IC_DllExport snapshotDir( void );
	QString IC_DllExport globalConfigPath( void );
	QString IC_DllExport personalConfigDir( void );
	QString IC_DllExport personalConfigPath( void );

	QString IC_DllExport globalStartmenuDir( void );

	QString IC_DllExport parameter( const QString & _name );

	bool IC_DllExport ensurePathExists( const QString & _path );

	QString IC_DllExport ip( void );

	QString IC_DllExport userRoleName( const ISD::userRoles _role );

#ifdef BUILD_WIN32
	BOOL enablePrivilege( LPCTSTR lpszPrivilegeName, BOOL bEnable );
	QString windowsConfigPath( int _type );
#endif

	void IC_DllExport activateWindow( QWidget * _window );

}


#endif
