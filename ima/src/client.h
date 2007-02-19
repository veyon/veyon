/*
 * client.h - declaration of class client which represents a client, shows its
 *            display and allows controlling it in several ways
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

class QMenu;

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
		ResetConnection,
		StartDemo,
		StopDemo,
		LockScreen,
		UnlockScreen,
		SendTextMessage,
		LogonUserCmd,
		LogoutUser,
		Reboot,
		PowerDown,
		ExecCmds
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



class client : public QWidget
{
	Q_OBJECT
public:
	enum clientCmds
	{
		None,
		Reload,
		ViewLive,
		RemoteControl,
		ClientDemo,
		SendTextMessage,
		LogonUserCmd,
		LogoutUser,
		Snapshot,
		PowerOn,
		Reboot,
		PowerDown,
		ExecCmds,
		CmdCount
	} ;

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

	client( const QString & _local_ip, const QString & _remote_ip,
		const QString & _mac, const QString & _name,
		classRoom * _class_room, mainWindow * _main_window,
		int _id = -1 );

	virtual ~client();


	int id( void ) const;
	static client * clientFromID( int _id );


	inline modes mode( void ) const
	{
		return( m_mode );
	}

	void changeMode( const modes _new_mode, isdConnection * _conn );


/*	inline QString fullName( void ) const
	{
		return( m_name + " (" + m_localIP + ")" );
	}*/

	inline QString name( void ) const
	{
		return( m_name );
	}

	inline const QString & localIP( void ) const
	{
		return( m_localIP );
	}

	inline const QString & remoteIP( void ) const
	{
		return( m_remoteIP );
	}

	inline const QString & mac( void ) const
	{
		return( m_mac );
	}

	inline const QString & user( void ) const
	{
		return( m_user );
	}

	inline void setName( const QString & _name )
	{
		m_name = _name;
	}

	inline void setLocalIP( const QString & _local_ip )
	{
		m_localIP = _local_ip;
	}

	inline void setRemoteIP( const QString & _remote_ip )
	{
		m_remoteIP = _remote_ip;
		if( m_remoteIP == QString::null || m_remoteIP == "" )
		{
			m_remoteIP = m_localIP;
		}
	}

	inline void setMac( const QString & _mac )
	{
		m_mac = _mac;
	}

	void setClassRoom( classRoom * _cr );

	void resetConnection( void );

	virtual void update( void );

	void createActionMenu( QMenu * _m );


	void processCmd( clientCmds _cmd, const QString & _u_data = "" );

	static inline bool reloadSnapshotList( void )
	{
		return( s_reloadSnapshotList );
	}

	static inline void resetReloadOfSnapshotList( void )
	{
		s_reloadSnapshotList = FALSE;
	}



	float m_rasterX;
	float m_rasterY;


	struct clientCommand
	{
		// stuff for cmd-exec
		clientCmds	m_cmd;
		execCmd 	m_exec;
		// stuff for UI
		char *		m_name;
		char *		m_icon;
		bool		m_insertSep;
	} ;


	static const clientCommand s_commands[CmdCount];



public slots:
	void processCmdSlot( QAction * _action );


private:
	void updateStatePixmap( void );
	bool userLoggedIn( void );

	virtual void contextMenuEvent( QContextMenuEvent * _cme );
	virtual void closeEvent( QCloseEvent * _ce );
	virtual void hideEvent( QHideEvent * _he );
	virtual void mouseDoubleClickEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void resizeEvent( QResizeEvent * _re );
	virtual void showEvent( QShowEvent * _se );

	// action-handlers
	void reload( const QString & _update = CONFIRM_YES );
	void clientDemo( const QString & );
	void viewLive( const QString & );
	void remoteControl( const QString & );
	void sendTextMessage( const QString & _msg );
/*	void distributeFile( const QString & _file );
	void collectFiles( const QString & _name_filter );*/
	void logonUser( const QString & _uname_and_pw );
	void logoutUser( const QString & _confirm );
	void snapshot( const QString & );
/*	void execCmdsIRFB( const QString & _cmds );
	void sshLogin( const QString & _user );*/
	void powerOn( const QString & );
	void reboot( const QString & _confirm );
	void powerDown( const QString & _confirm );
	void execCmds( const QString & _cmds );

	states currentState( void ) const;


	mainWindow * m_mainWindow;
	ivsConnection * m_connection;

	QString m_name;
	QString m_localIP;
	QString m_remoteIP;
	QString m_mac;

	modes m_mode;
	QString m_user;
	volatile bool m_makeSnapshot;

	states m_state;
	QPixmap m_statePixmap;

	QMutex m_syncMutex;

	classRoomItem * m_classRoomItem;

	QMenu * m_contextMenu;

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
