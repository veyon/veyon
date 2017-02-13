/*
 * FeatureWorkerManager.h - class for managing feature worker instances
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef FEATURE_WORKER_MANAGER_H
#define FEATURE_WORKER_MANAGER_H

#include <QMutex>
#include <QPointer>
#include <QProcess>
#include <QTcpServer>
#include <QTcpSocket>

#include "FeatureMessage.h"

class FeatureManager;

class FeatureWorkerManager : public QObject
{
	Q_OBJECT
public:
	FeatureWorkerManager( FeatureManager& featureManager );
	virtual ~FeatureWorkerManager();

	Q_INVOKABLE void startWorker( const Feature& feature );
	Q_INVOKABLE void stopWorker( const Feature& feature );

	Q_INVOKABLE void sendMessage( const FeatureMessage& message );

	bool isWorkerRunning( const Feature& feature );

private slots:
	void acceptConnection();
	void processConnection( QTcpSocket* socket );

private:
	static QString workerProcessFilePath();

	FeatureManager& m_featureManager;
	QTcpServer m_tcpServer;

	struct Worker
	{
		QPointer<QTcpSocket> socket;
		QPointer<QProcess> process;
		QList<FeatureMessage> pendingMessages;

		Worker() :
			socket( nullptr ),
			process( nullptr ),
			pendingMessages()
		{
		}

		Worker( const Worker &ref ) :
			socket( ref.socket ),
			process( ref.process ),
			pendingMessages( ref.pendingMessages )
		{
		}
	};

	typedef QMap<Feature::Uid, Worker> WorkerMap;
	WorkerMap m_workers;

	QMutex m_workersMutex;


} ;

#endif
