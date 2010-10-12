/*
 * ItalcCoreConnection.h - declaration of class ItalcCoreConnection
 *
 * Copyright (c) 2008-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef _ITALC_CORE_CONNECTION_H
#define _ITALC_CORE_CONNECTION_H

#include "ItalcCore.h"
#include "ItalcVncConnection.h"


class ItalcCoreConnection : public QObject
{
	Q_OBJECT
public:
	ItalcCoreConnection( ItalcVncConnection * _ivc );
	~ItalcCoreConnection();

	ItalcVncConnection * vncConnection( void )
	{
		return m_ivc;
	}

	inline bool isConnected( void ) const
	{
		return m_ivc->isConnected();
	}

	inline const QString & user( void ) const
	{
		return m_user;
	}

	inline const QString & userHomeDir( void ) const
	{
		return m_userHomeDir;
	}

	void sendGetUserInformationRequest( void );
	void execCmds( const QString & _cmd );
	void startDemo( int _port, bool _full_screen = false );
	void stopDemo( void );
	void lockDisplay( void );
	void unlockDisplay( void );
	void logonUser( const QString & _uname, const QString & _pw,
						const QString & _domain );
	void logoutUser( void );
	void displayTextMessage( const QString & _msg );
	void sendFile( const QString & _fname );
	void collectFiles( const QString & _nfilter );

	void powerOnComputer( const QString & _mac );
	void powerDownComputer( void );
	void restartComputer( void );
	void disableLocalInputs( bool _disabled );

	void setRole( const ItalcCore::UserRoles _role );


private slots:
	void initNewClient( rfbClient * _cl );


private:
	static rfbBool handleItalcMessage( rfbClient *cl,
						rfbServerToClientMsg *msg );

	bool handleServerMessage( rfbClient *cl, uint8_t msg );
	void enqueueMessage( const ItalcCore::Msg & _msg );


	ItalcVncConnection * m_ivc;
	SocketDevice m_socketDev;

	QString m_user;
	QString m_userHomeDir;

} ;


#endif
