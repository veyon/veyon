/*
 * italc_core_server.h - ItalcCoreServer
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef _ITALC_CORE_SERVER_H
#define _ITALC_CORE_SERVER_H

#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QPair>
#include <QtCore/QSignalMapper>
#include <QtCore/QStringList>

#include "italc_core.h"


class QProcess;
class IVS;
class DemoClient;
class LockWidget;

class ItalcCoreServer : public QObject
{
	Q_OBJECT
public:
	static QList<ItalcCore::Command> externalActions;

	enum AccessDialogResult
	{
		AccessYes,
		AccessNo,
		AccessAlways,
		AccessNever
	} ;

	void earlyInit( void );

	ItalcCoreServer( int _argc, char * * _argv );
	virtual ~ItalcCoreServer();

	static ItalcCoreServer * instance( void )
	{
		Q_ASSERT( this != NULL );
		return _this;
	}

	int processClient( socketDispatcher _sd, void * _user );

	bool authSecTypeItalc( socketDispatcher _sd, void * _user,
				ItalcAuthTypes _auth_type = ItalcAuthDSA,
				const QStringList & _allowedHosts =
								QStringList() );

	static int doGuiOp( const ItalcCore::Command & _cmd,
					const ItalcCore::CommandArgs & _args );


private slots:
	void checkForPendingActions( void );

	// client-functions
	void startDemo( const QString & _master_host, bool _fullscreen );
	void stopDemo( void );

	void lockDisplay( void );
	void unlockDisplay( void );

	void displayTextMessage( const QString & _msg );

	AccessDialogResult showAccessDialog( const QString & _host );


private:
	static void errorMsgAuth( const QString & _ip );


	static ItalcCoreServer * _this;

	QMutex m_actionMutex;
	ItalcCore::CommandList m_pendingActions;

	IVS * m_ivs;
	DemoClient * m_demoClient;
	LockWidget * m_lockWidget;

	QMap<QString, QProcess *> m_guiProcs;

} ;


#endif
