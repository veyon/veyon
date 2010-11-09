/*
 * ItalcCore.h - definitions for iTALC Core
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

#ifndef _ITALC_CORE_H
#define _ITALC_CORE_H

#include <italcconfig.h>

#include <QtCore/QIODevice>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QSysInfo>
#include <QtCore/QVariant>

#include "Ipc/Core.h"

#include "ItalcRfbExt.h"
#include "rfb/rfbproto.h"
#include "rfb/rfbclient.h"


/* ============================================================================
 * socket dispatching
 * ============================================================================
 */


typedef enum
{
	SocketRead,
	SocketWrite,
	SocketGetPeerAddress
} SocketOpCodes;


typedef qint64 ( * socketDispatcher )( char *buffer, const qint64 bytes,
									const SocketOpCodes opCode, void *user );


extern qint64 libvncClientDispatcher( char *buffer, const qint64 bytes,
									const SocketOpCodes opCode, void *user );


class SocketDevice : public QIODevice
{
public:
	inline SocketDevice( socketDispatcher sockDisp, void *user = NULL ) :
		QIODevice(),
		m_socketDispatcher( sockDisp ),
		m_user( user )
	{
		open( ReadWrite | Unbuffered );
	}

	inline QVariant read()
	{
		QDataStream d( this );
		return d;
	}

	inline void write( const QVariant &v )
	{
		QDataStream d( this );
		d << v;
	}

	inline socketDispatcher sockDispatcher()
	{
		return m_socketDispatcher;
	}

	inline void *user()
	{
		return m_user;
	}

	inline void setUser( void *user )
	{
		m_user = user;
	}

	inline qint64 read( char *buf, qint64 bytes )
	{
		return readData( buf, bytes );
	}

	inline qint64 write( const char *buf, qint64 bytes )
	{
		return writeData( buf, bytes );
	}


protected:
	inline qint64 readData( char *buf, qint64 bytes )
	{
		return m_socketDispatcher( buf, bytes, SocketRead, m_user );
	}

	inline qint64 writeData( const char *buf, qint64 bytes )
	{
		return m_socketDispatcher( const_cast<char *>( buf ), bytes,
												SocketWrite, m_user );
	}


private:
	socketDispatcher m_socketDispatcher;
	void * m_user;

} ;



class ItalcConfiguration;

namespace ItalcCore
{
	bool init();
	void destroy();

	extern ItalcConfiguration *config;

	typedef QString Command;
	typedef QMap<QString, QVariant> CommandArgs;
	typedef QList<QPair<ItalcCore::Command, ItalcCore::CommandArgs> >
								CommandList;

	// static commands
	extern const Command GetUserInformation;
	extern const Command UserInformation;
	extern const Command StartDemo;
	extern const Command StopDemo;
	extern const Command LockScreen;
	extern const Command UnlockScreen;
	extern const Command LockInput;
	extern const Command UnlockInput;
	extern const Command LogonUserCmd;
	extern const Command LogoutUser;
	extern const Command DisplayTextMessage;
	extern const Command AccessDialog;
	extern const Command ExecCmds;
	extern const Command PowerOnComputer;
	extern const Command PowerDownComputer;
	extern const Command RestartComputer;
	extern const Command DisableLocalInputs;
	extern const Command SetRole;
	extern const Command StartDemoServer;
	extern const Command StopDemoServer;
	extern const Command DemoServerAllowHost;
	extern const Command DemoServerUnallowHost;
	extern const Command ReportSlaveStateFlags;

	class Msg
	{
	public:
		Msg( SocketDevice *sockDev, const Command &cmd = Command() ) :
			m_socketDevice( sockDev ),
			m_cmd( cmd )
		{
		}

		Msg( const Command &cmd ) :
			m_socketDevice( NULL ),
			m_cmd( cmd )
		{
		}

		inline const Command & cmd() const
		{
			return m_cmd;
		}

		inline const CommandArgs & args() const
		{
			return m_args;
		}

		void setSocketDevice( SocketDevice *sockDev )
		{
			m_socketDevice = sockDev;
		}

		Msg & addArg( const QString &key, const QString &value )
		{
			m_args[key.toLower()] = value;
			return *this;
		}

		Msg & addArg( const QString & key, const int value )
		{
			m_args[key.toLower()] = QString::number( value );
			return *this;
		}

		QString arg( const QString & key ) const
		{
			return m_args[key.toLower()].toString();
		}

		bool send()
		{
			QDataStream d( m_socketDevice );
			d << (uint8_t) rfbItalcCoreRequest;
			d << m_cmd;
			d << m_args;
			return true;
		}

		Msg & receive()
		{
			QDataStream d( m_socketDevice );
			d >> m_cmd;
			d >> m_args;
			return *this;
		}


	private:
		SocketDevice * m_socketDevice;

		Command m_cmd;
		CommandArgs m_args;

	} ;

	enum UserRoles
	{
		RoleNone,
		RoleTeacher,
		RoleAdmin,
		RoleSupporter,
		RoleOther,
		RoleCount
	} ;
	typedef UserRoles UserRole;

	bool initAuthentication();

	QString userRoleName( UserRole role );


	extern int serverPort;
	extern UserRoles role;

}

#endif
