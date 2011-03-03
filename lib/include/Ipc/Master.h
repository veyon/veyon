/*
 * IpcMaster.h - class Ipc::Master which manages Ipc::Slaves
 *
 * Copyright (c) 2010-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 * Copyright (c) 2010 Univention GmbH
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

#ifndef _IPC_MASTER_H
#define _IPC_MASTER_H

#include "Ipc/Core.h"

#include <QtCore/QMutex>
#include <QtCore/QProcess>
#include <QtCore/QSignalMapper>
#include <QtNetwork/QTcpServer>


namespace Ipc
{

class SlaveLauncher;

class Master : public QTcpServer
{
	Q_OBJECT
public:
	Master( const QString &applicationFilePath );
	virtual ~Master();

	const QString & applicationFilePath() const
	{
		return m_applicationFilePath;
	}

	virtual void createSlave( const Ipc::Id &id, SlaveLauncher *slaveLauncher = NULL );
	void stopSlave( const Ipc::Id &id );
	bool isSlaveRunning( const Ipc::Id &id );

	void sendMessage( const Ipc::Id &id, const Ipc::Msg &msg );
	Ipc::Msg receiveMessage( const Ipc::Id &id );

	virtual bool handleMessage( const Ipc::Id &id, const Ipc::Msg &msg ) = 0;


private slots:
	void acceptConnection();
	void receiveMessage( QObject *sock );
	void sendPendingMessages();


private:
	QString m_applicationFilePath;
	QSignalMapper m_socketReceiveMapper;

	struct ProcessInformation
	{
		QTcpSocket *sock;
		SlaveLauncher *slaveLauncher;
		QVector<Ipc::Msg> pendingMessages;

		ProcessInformation() :
			sock( NULL ),
			slaveLauncher( NULL ),
			pendingMessages()
		{
		}

		ProcessInformation( const ProcessInformation &ref ) :
			sock( ref.sock ),
			slaveLauncher( ref.slaveLauncher ),
			pendingMessages( ref.pendingMessages )
		{
		}
	};

	typedef QMap<Ipc::Id, ProcessInformation> ProcessMap;
	ProcessMap m_processes;

	QMutex m_processMapMutex;


signals:
	void messagesPending();

};

}

#endif // _IPC_MASTER_H
