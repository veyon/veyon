/*
 * isd_base.h - basics concerning ISD (iTALC Service Daemon)
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef _ISD_BASE_H
#define _ISD_BASE_H

#include <config.h>

#include <QtCore/QtGlobal>
#include <QtCore/QIODevice>
#include <QtCore/QString>
#include <QtCore/QSysInfo>
#include <QtCore/QVariant>

#include "types.h"
#include "italc_rfb_ext.h"
#include "rfb/rfbproto.h"


// isd-server-stuff
#define isdProtocolVersionFormat "ISD %03d.%03d\n"
#define isdProtocolMajorVersion 1
#define isdProtocolMinorVersion 0

#define sz_isdProtocolVersionMsg sz_rfbProtocolVersionMsg

typedef char isdProtocolVersionMsg[sz_isdProtocolVersionMsg+1];


// demo-server-stuff
#define idsProtocolVersionFormat "IDS %03d.%03d\n"
#define idsProtocolMajorVersion 1
#define idsProtocolMinorVersion 0

#define sz_idsProtocolVersionMsg sz_rfbProtocolVersionMsg

typedef char idsProtocolVersionMsg[sz_idsProtocolVersionMsg+1];



inline const uint16_t swap16IfLE( const uint16_t & _val )
{
	return( QSysInfo::ByteOrder == QSysInfo::LittleEndian ?
			( ( _val & 0xff ) << 8 ) |
			( ( _val >> 8 ) & 0xff )
			:
			_val );
}


inline const uint32_t swap32IfLE( const uint32_t & _val )
{
	return( QSysInfo::ByteOrder == QSysInfo::LittleEndian ?
			( ( _val & 0xff000000 ) >> 24 ) |
			( ( _val & 0x00ff0000 ) >> 8 ) |
			( ( _val & 0x0000ff00 ) << 8 ) |
			( ( _val & 0x000000ff ) << 24 )
		:
			_val );
}

/*
#ifndef Swap16IfLE
#define Swap16IfLE(s) ( QSysInfo::ByteOrder == QSysInfo::LittleEndian ?	\
			((((s) & 0xff) << 8) | (((s) >> 8) & 0xff)) : (s))
#endif

#ifndef Swap32IfLE
#define Swap32IfLE(l) ( QSysInfo::ByteOrder == QSysInfo::LittleEndian ?	\
				((((l) & 0xff000000) >> 24) |	\
				(((l) & 0x00ff0000) >> 8)  |	\
				(((l) & 0x0000ff00) << 8)  |	\
				(((l) & 0x000000ff) << 24))  : (l))
#endif*/



/* ============================================================================
 * ISD dispatching
 * ============================================================================ 
 */


typedef enum
{
	SocketRead,
	SocketWrite,
	SocketGetIPBoundTo
} socketOpCodes;


typedef qint64 ( * socketDispatcher )( char * _buffer, const qint64 _bytes,
						const socketOpCodes _op_code,
						void * _user );



class socketDevice : public QIODevice
{
public:
	inline socketDevice( socketDispatcher _sd, void * _user ) :
		QIODevice(),
		m_socketDispatcher( _sd ),
		m_user( _user )
	{
		open( ReadWrite | Unbuffered );
	}

	inline QVariant read( void )
	{
		QDataStream d( this );
		return( d );
	}

	inline void write( const QVariant & _v )
	{
		QDataStream d( this );
		d << _v;
	}

	inline socketDispatcher sockDispatcher( void )
	{
		return( m_socketDispatcher );
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
		return( readData( _buf, _bytes ) );
	}

	inline qint64 write( const char * _buf, qint64 _bytes )
	{
		return( writeData( _buf, _bytes ) );
	}


protected:
	inline qint64 readData( char * _buf, qint64 _bytes )
	{
		return( m_socketDispatcher( _buf, _bytes, SocketRead,
								m_user ) );
	}

	inline qint64 writeData( const char * _buf, qint64 _bytes )
	{
		return( m_socketDispatcher( const_cast<char *>( _buf ), _bytes,
						SocketWrite, m_user ) );
	}


private:
	socketDispatcher m_socketDispatcher;
	void * m_user;

} ;



qint64 qtcpsocketDispatcher( char * _buf, const qint64 _len,
						const socketOpCodes _op_code,
						void * _user );




typedef struct
{
	enum commands
	{
		DummyCmd = rfbItalcServiceRequest + 1,
		GetUserInformation,
		UserInformation,
		StartFullScreenDemo,
		StartWindowDemo,
		StopDemo,
		LockDisplay,
		UnlockDisplay,
		LogonUserCmd,
		LogoutUser,
		DisplayTextMessage,
		SendFile,
		CollectFiles,
/*		ViewRemoteDisplay,
		RemoteControlDisplay,*/
		ExecCmds,

		// system-stuff
		WakeOtherComputer = 48,
		PowerDownComputer,
		RestartComputer,

		SetRole = 64,

		DemoServer_Run = 80,
		DemoServer_Stop,
		DemoServer_AllowClient,
		DemoServer_DenyClient

	} ;



	class msg
	{
		commands m_cmd;
		socketDevice * m_socketDevice;

		typedef QMap<QString, QVariant> argMap;
		argMap m_argMap;

	public:
		msg( socketDevice * _sd, const commands _cmd = DummyCmd ) :
			m_cmd( _cmd ),
			m_socketDevice( _sd )
		{
		}

		msg & addArg( const QString & _name, const QVariant & _content )
		{
			m_argMap[_name] = _content;
			return( *this );
		}

		QVariant arg( const QString & _name ) const
		{
			return( m_argMap[_name] );
		}

		bool send( void )
		{
			QDataStream d( m_socketDevice );
			d << (Q_UINT8) rfbItalcServiceRequest;
			d << (Q_UINT8) m_cmd;
			d << m_argMap;
			return( TRUE );
		}

		msg & receive( void )
		{
			QDataStream d( m_socketDevice );
			d >> m_argMap;
			return( *this );
		}


	} ;

	enum userRoles
	{
		RoleNone,
		RoleTeacher,
		RoleAdmin,
		RoleSupporter,
		RoleOther,
		RoleCount
	} ;


} ISD;


extern ISD::userRoles __role;


#endif
