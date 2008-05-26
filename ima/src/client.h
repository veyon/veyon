/*
 * client.h - declaration of class client which represents a client, shows its
 *            display and allows controlling it in several ways
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QQueue>
#include <QtCore/QVector>
#include <QtGui/QWidget>
#include <QtGui/QImage>
#include <QtGui/QMenu>

#include "fast_qimage.h"

class classRoom;
class classRoomItem;
class client;
class isdConnection;
class ivsConnection;
class mainWindow;


typedef void( client:: * execCmd )( const QString & );

const QString CONFIRM_NO = "n";
const QString CONFIRM_YES = "y";


class updateThread : public QThread
{
	Q_OBJECT
public:
	enum queueableCommands
	{
		Cmd_ResetConnection,
		Cmd_StartDemo,
		Cmd_StopDemo,
		Cmd_LockScreen,
		Cmd_UnlockScreen,
		Cmd_SendTextMessage,
		Cmd_LogonUser,
		Cmd_LogoutUser,
		Cmd_Reboot,
		Cmd_PowerDown,
		Cmd_ExecCmds
	} ;

	updateThread( client * _client );
	virtual ~updateThread()
	{
	}

	inline void enqueueCommand( queueableCommands _cmd,
					const QVariant & _data =
							QVariant() )
	{
		m_queueMutex.lock();
		m_queue.enqueue( qMakePair( _cmd, _data ) );
		m_queueMutex.unlock();
	}


private slots:
	void update( void );


private:
	virtual void run( void );

	client * m_client;
	QMutex m_queueMutex;
	typedef QPair<queueableCommands, QVariant> queueItem;
	QQueue<queueItem> m_queue;

	friend class client;

} ;




class clientAction : public QAction
{
	Q_OBJECT
public:
	enum type
	{
		Overview,
		FullscreenDemo,
		WindowDemo,
		Locked,
		ViewLive,
		RemoteControl,
		ClientDemo,
		SendTextMessage,
		LogonUser,
		LogoutUser,
		Snapshot,
		PowerOn,
		Reboot,
		PowerDown,
		ExecCmds
	} ;

	enum targetGroup
	{
		Default,
		SelectedClients,
		VisibleClients
	} ;

	enum flags
	{
		None = 0,
		FullMenu = 1
	} ;

	clientAction( type _type, QObject * _parent = 0, int _flags = 0 );
	clientAction( type _type, const QIcon & _icon, const QString & _text,
			QObject * _parent = 0, int _flags = 0 );
	~clientAction() {};

	void process( QVector<client *> _clients, targetGroup _target = Default );
	static void process( QAction * _action,
			QVector<client *> _clients, targetGroup _target = Default );

	inline bool flags( int _mask = -1 )
	{
		return ( m_flags & _mask );
	}

private:
	type m_type;
	int m_flags;

	bool confirmLogout( targetGroup _target ) const;
	bool confirmReboot( targetGroup _target ) const;
	bool confirmPowerDown( targetGroup _target ) const;

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




class client : public QWidget
{
	Q_OBJECT
public:
	enum modes
	{
		Mode_Overview,
		Mode_FullscreenDemo,
		Mode_WindowDemo,
		Mode_Locked,
		Mode_Unknown
	} ;

	enum states
	{
		State_Unreachable,
		State_NoUserLoggedIn,
		State_Overview,
		State_Demo,
		State_Locked,
		State_Unkown
	} ;

	enum types
	{
		Type_Student,
		Type_Teacher,
		Type_Other
	} ;

	client( const QString & _hostname,
		const QString & _mac, const QString & _name, types _type,
		classRoom * _class_room, mainWindow * _main_window,
								int _id = -1 );

	virtual ~client();

	void quit( void );


	int id( void ) const;
	static client * clientFromID( int _id );


	inline modes mode( void ) const
	{
		return( m_mode );
	}

	// action-handlers
	void changeMode( const modes _new_mode );
	void viewLive( void );
	void remoteControl( void );
	void clientDemo( void );
	void sendTextMessage( const QString & _msg );
	void logonUser( const QString & _username, const QString & _password,
			const QString & _domain );
	void logoutUser( void );
	void snapshot( void );
	void powerOn( void );
	void reboot( void );
	void powerDown( void );
	void execCmds( const QString & _cmds );
	void reload( void );


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

	inline types type( void ) const
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

	inline void setType( types _type )
	{
		if( _type >= Type_Student && _type <= Type_Other )
		{
			m_type = _type;
		}
	}

	void setClassRoom( classRoom * _cr );

	void resetConnection( void );

	virtual void update( void );

	static inline bool reloadSnapshotList( void )
	{
		return( s_reloadSnapshotList );
	}

	static inline void resetReloadOfSnapshotList( void )
	{
		s_reloadSnapshotList = FALSE;
	}


	void zoom( void );
	void zoomBack( void );


	float m_rasterX;
	float m_rasterY;


public slots:


private slots:
	void enlarge( void );


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


	states currentState( void ) const;


	mainWindow * m_mainWindow;
	ivsConnection * m_connection;
	QPoint m_clickPoint;
	QPoint m_origPos;
	QSize m_origSize;

	QString m_hostname;
	QString m_nickname;
	QString m_mac;
	types m_type;
	int m_reloadsAfterReset;

	modes m_mode;
	QString m_user;
	volatile bool m_makeSnapshot;

	states m_state;

	QMutex m_syncMutex;

	classRoomItem * m_classRoomItem;

	updateThread * m_updateThread;


	// static data
	static bool s_reloadSnapshotList;

	static QHash<int, client *> s_clientIDs;

	// static members
	static int freeID( void );


	friend class updateThread;
	friend class classRoomItem;

} ;


#endif
