/*
 * Client.h - declaration of class Client which represents a client, shows its
 *            display and allows controlling it in several ways
 *
 * Copyright (c) 2004-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef _CLIENT_H
#define _CLIENT_H

#include <italcconfig.h>

#include <QtCore/QHash>
#include <QtCore/QThread>
#include <QtCore/QQueue>
#include <QtCore/QVector>
#include <QtGui/QWidget>
#include <QtGui/QImage>
#include <QtGui/QMenu>

#include "FastQImage.h"

class classRoom;
class classRoomItem;
class Client;
class MainWindow;
class ItalcCoreConnection;
class ItalcVncConnection;

typedef void( Client:: * execCmd )( const QString & );

const QString CONFIRM_NO = "n";
const QString CONFIRM_YES = "y";


class ClientAction : public QAction
{
	Q_OBJECT
public:
	enum Type
	{
		Overview,
		FullscreenDemo,
		WindowDemo,
		Locked,
		ViewLive,
		RemoteControl,
		ClientDemo,
		SendTextMessage,
		//LogonUser,
		LogoutUser,
		Snapshot,
		PowerOn,
		Reboot,
		PowerDown,
		ExecCmds,
		RemoteScript,
		LocalScript
	} ;

	enum TargetGroup
	{
		Default,
		SelectedClients,
		VisibleClients
	} ;

	enum Flags
	{
		None = 0,
		FullMenu = 1
	} ;

	ClientAction( Type _type, QObject * _parent = 0, int _flags = 0 );
	ClientAction( Type _type, const QIcon & _icon, const QString & _text,
			QObject * _parent = 0, int _flags = 0 );
	~ClientAction() {};

	void process( QVector<Client *> _clients, TargetGroup _target = Default );
	static void process( QAction * _action,
			QVector<Client *> _clients, TargetGroup _target = Default );

	inline bool flags( int _mask = -1 )
	{
		return ( m_flags & _mask );
	}

private:
	Type m_type;
	int m_flags;

	bool confirmLogout( TargetGroup _target ) const;
	bool confirmReboot( TargetGroup _target ) const;
	bool confirmPowerDown( TargetGroup _target ) const;
	QString dataExpanded( QVector<Client *> _clients ) const;

} ;




class clientMenu : public QMenu
{
	Q_OBJECT
public:
	static const bool FullMenu = TRUE;

	clientMenu( const QString & _title, const QList<QAction *> _actions,
			QWidget * _parent = 0, const bool _fullMenu = FALSE );

	virtual ~clientMenu() {};

	static QMenu * createDefault( QWidget * _parent );
} ;




inline QPixmap scaledIcon( const char * _name )
{
	return scaled( QString( ":/resources/" ) + _name, 16, 16 );
}




class Client : public QWidget
{
	Q_OBJECT
public:
	enum Modes
	{
		Mode_Overview,
		Mode_FullscreenDemo,
		Mode_WindowDemo,
		Mode_Locked,
		Mode_Unknown
	} ;

	enum States
	{
		State_Unreachable,
		State_NoUserLoggedIn,
		State_Overview,
		State_Demo,
		State_Locked,
		State_Unkown
	} ;

	enum Types
	{
		Type_Student,
		Type_Teacher,
		Type_Other
	} ;

	Client( const QString & _hostname,
		const QString & _mac, const QString & _name, Types _type,
		classRoom * _class_room, MainWindow * _main_window,
								int _id = -1 );

	virtual ~Client();


	int id( void ) const;
	static Client * clientFromID( int _id );


	inline Modes mode( void ) const
	{
		return( m_mode );
	}

	// action-handlers
	void changeMode( const Modes _new_mode );
	void viewLive( void );
	void remoteControl( void );
	void clientDemo( void );
	void sendTextMessage( const QString & _msg );
/*	void logonUser( const QString & _username, const QString & _password,
			const QString & _domain );*/
	void logoutUser( void );
	void snapshot( void );
	void powerOn( void );
	void reboot( void );
	void powerDown( void );
	void execCmds( const QString & _cmds );


	inline QString name( void ) const
	{
		return( m_nickname.isEmpty() ? m_hostname : m_nickname );
	}

	inline const QString & hostname( void ) const
	{
		return( m_hostname );
	}

	inline const QString & nickname( void ) const
	{
		return( m_nickname );
	}

	inline const QString & mac( void ) const
	{
		return( m_mac );
	}

	inline Types type( void ) const
	{
		return( m_type );
	}

	inline const QString & user( void ) const
	{
		return( m_user );
	}

	inline void setNickname( const QString & _nickname )
	{
		m_nickname = _nickname;
	}

	inline void setHostname( const QString & _hostname )
	{
		m_hostname = _hostname;
	}

	inline void setMac( const QString & _mac )
	{
		m_mac = _mac;
	}

	inline void setType( Types _type )
	{
		if( _type >= Type_Student && _type <= Type_Other )
		{
			m_type = _type;
		}
	}

	void setClassRoom( classRoom * _cr );

	void resetConnection( void );

	virtual void update( void );


	void zoom( void );
	void zoomBack( void );


	float m_rasterX;
	float m_rasterY;


public slots:


private slots:
	void enlarge( void );
	void reload( void );
	void setUpdateFlag();


private:
	bool userLoggedIn( void );

	virtual void contextMenuEvent( QContextMenuEvent * _cme );
	virtual void closeEvent( QCloseEvent * _ce );
	virtual void hideEvent( QHideEvent * _he );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void mouseDoubleClickEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void resizeEvent( QResizeEvent * _re );
	virtual void showEvent( QShowEvent * _se );


	States currentState( void ) const;


	MainWindow * m_mainWindow;
	ItalcCoreConnection *m_connection;
	ItalcVncConnection *m_vncConn;
	bool m_framebufferUpdated;
	QPoint m_clickPoint;
	QPoint m_origPos;
	QSize m_origSize;

	QString m_hostname;
	QString m_nickname;
	QString m_mac;
	Types m_type;

	Modes m_mode;
	QString m_user;
	volatile bool m_takeSnapshot;

	States m_state;

	classRoomItem * m_classRoomItem;


	// static data
	static QHash<int, Client *> s_clientIDs;

	// static members
	static int freeID( void );


	friend class classRoomItem;

} ;


#endif
