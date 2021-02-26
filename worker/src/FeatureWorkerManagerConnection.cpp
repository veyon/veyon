/*
 * FeatureWorkerManagerConnection.cpp - class which handles communication between service and feature
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
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

#include <QCoreApplication>
#include <QHostAddress>

#include "FeatureManager.h"
#include "FeatureWorkerManagerConnection.h"
#include "VeyonConfiguration.h"


FeatureWorkerManagerConnection::FeatureWorkerManagerConnection( VeyonWorkerInterface& worker,
																FeatureManager& featureManager,
																Feature::Uid featureUid,
																QObject* parent ) :
	QObject( parent ),
	m_worker( worker ),
	m_featureManager( featureManager ),
	m_socket( this ),
	m_featureUid( featureUid )
{
	connect( &m_connectTimer, &QTimer::timeout, this, &FeatureWorkerManagerConnection::tryConnection );

	connect( &m_socket, &QTcpSocket::connected,
			 this, &FeatureWorkerManagerConnection::sendInitMessage );

	connect( &m_socket, &QTcpSocket::disconnected,
			 QCoreApplication::instance(), &QCoreApplication::quit );

	connect( &m_socket, &QTcpSocket::readyRead,
			 this, &FeatureWorkerManagerConnection::receiveMessage );

	tryConnection();
}



bool FeatureWorkerManagerConnection::sendMessage( const FeatureMessage& message )
{
	return message.send( &m_socket );
}



void FeatureWorkerManagerConnection::tryConnection()
{
	if( m_socket.state() != QTcpSocket::ConnectedState )
	{
		const auto port = quint16( VeyonCore::config().featureWorkerManagerPort() + VeyonCore::sessionId() );

		vDebug() << "connecting to FeatureWorkerManager at port" << port;

		m_socket.connectToHost( QHostAddress::LocalHost, port );
		m_connectTimer.start( ConnectTimeout );
	}
}



void FeatureWorkerManagerConnection::sendInitMessage()
{
	vDebug() << m_featureUid;

	m_connectTimer.stop();

	FeatureMessage( m_featureUid, FeatureMessage::InitCommand ).send( &m_socket );
}



void FeatureWorkerManagerConnection::receiveMessage()
{
	FeatureMessage featureMessage;

	while( featureMessage.isReadyForReceive( &m_socket ) )
	{
		if( featureMessage.receive( &m_socket ) )
		{
			m_featureManager.handleFeatureMessage( m_worker, featureMessage );
		}
	}
}
