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

#include "ItalcRfbExt.h"
#include "rfb/rfbproto.h"
#include "rfb/rfbclient.h"


// iTALC Core Server stuff
#define icsProtocolVersionFormat "ICS %03d.%03d\n"
#define icsProtocolMajorVersion 1
#define icsProtocolMinorVersion 0

#define sz_icsProtocolVersionMsg sz_rfbProtocolVersionMsg

typedef char icsProtocolVersionMsg[sz_icsProtocolVersionMsg+1];


// iTALC Demo Server stuff
#define idsProtocolVersionFormat "IDS %03d.%03d\n"
#define idsProtocolMajorVersion 1
#define idsProtocolMinorVersion 0

#define sz_idsProtocolVersionMsg sz_rfbProtocolVersionMsg

typedef char idsProtocolVersionMsg[sz_idsProtocolVersionMsg+1];



inline const uint16_t swap16IfLE( const uint16_t & _val )
{
	return QSysInfo::ByteOrder == QSysInfo::LittleEndian ?
				( ( _val & 0xff ) << 8 ) |
				( ( _val >> 8 ) & 0xff )
			:
				_val;
}


inline const uint32_t swap32( const uint32_t & _val )
{
	return ( ( _val & 0xff000000 ) >> 24 ) |
			( ( _val & 0x00ff0000 ) >> 8 ) |
			( ( _val & 0x0000ff00 ) << 8 ) |
			( ( _val & 0x000000ff ) << 24 );
}


inline const uint32_t swap32IfLE( const uint32_t & _val )
{
	return QSysInfo::ByteOrder == QSysInfo::LittleEndian ?
						swap32( _val ) : _val;
}


inline const uint32_t swap32IfBE( const uint32_t & _val )
{
	return QSysInfo::ByteOrder == QSysInfo::BigEndian ?
						swap32( _val ) : _val;
}



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


typedef qint64 ( * socketDispatcher )( char * _buffer, const qint64 _bytes,
						const SocketOpCodes _op_code,
						void * _user );


extern qint64 libvncClientDispatcher( char * _buf, const qint64 _len,
				const SocketOpCodes _op_code, void * _user );


class SocketDevice : public QIODevice
{
public:
	inline SocketDevice( socketDispatcher _sd, void * _user = NULL ) :
		QIODevice(),
		m_socketDispatcher( _sd ),
		m_user( _user )
	{
		open( ReadWrite | Unbuffered );
	}

	inline QVariant read( void )
	{
		QDataStream d( this );
		return d;
	}

	inline void write( const QVariant & _v )
	{
		QDataStream d( this );
		d << _v;
	}

	inline socketDispatcher sockDispatcher( void )
	{
		return m_socketDispatcher;
	}

	inline void * user( void )
	{
		return m_user;
	}

	inline void setUser( void * _user )
	{
		m_user = _user;
	}

	inline qint64 read( char * _buf, qint64 _bytes )
	{
		return readData( _buf, _bytes );
	}

	inline qint64 write( const char * _buf, qint64 _bytes )
	{
		return writeData( _buf, _bytes );
	}


protected:
	inline qint64 readData( char * _buf, qint64 _bytes )
	{
		return m_socketDispatcher( _buf, _bytes, SocketRead,
								m_user );
	}

	inline qint64 writeData( const char * _buf, qint64 _bytes )
	{
		return m_socketDispatcher( const_cast<char *>( _buf ), _bytes,
							SocketWrite, m_user );
	}


private:
	socketDispatcher m_socketDispatcher;
	void * m_user;

} ;




namespace ItalcCore
{
	typedef QString Command;
	typedef QMap<QString, QVariant> CommandArgs;
	typedef QList<QPair<ItalcCore::Command, ItalcCore::CommandArgs> >
								CommandList;

	// static commands
	extern const Command GetUserInformation;
	extern const Command UserInformation;
	extern const Command StartDemo;
	extern const Command StopDemo;
	extern const Command LockDisplay;
	extern const Command UnlockDisplay;
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

	class Msg
	{
	public:
		Msg( SocketDevice * _sd, const Command & _cmd = Command() ) :
			m_socketDevice( _sd ),
			m_cmd( _cmd )
		{
		}

		Msg( const Command & _cmd ) :
			m_socketDevice( NULL ),
			m_cmd( _cmd )
		{
		}

		inline const Command & cmd( void ) const
		{
			return m_cmd;
		}

		inline const CommandArgs & args( void ) const
		{
			return m_args;
		}

		void setSocketDevice( SocketDevice * _sd )
		{
			m_socketDevice = _sd;
		}

		Msg & addArg( const QString & _key, const QString & _value )
		{
			m_args[_key.toLower()] = _value;
			return *this;
		}

		Msg & addArg( const QString & _key, const int _value )
		{
			m_args[_key.toLower()] = QString::number( _value );
			return *this;
		}

		QString arg( const QString & _key ) const
		{
			return m_args[_key.toLower()].toString();
		}

		bool send( void )
		{
			QDataStream d( m_socketDevice );
			d << (uint8_t) rfbItalcCoreRequest;
			d << m_cmd;
			d << m_args;
			return true;
		}

		Msg & receive( void )
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


	extern int serverPort;
	extern UserRoles role;

}

#endif
