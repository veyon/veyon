/*
 * FeatureWorkerManagerConnection.cpp - class which handles communication between service and feature
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

#include <QCoreApplication>
#include <QDebug>
#include <QHostAddress>

#include "FeatureManager.h"
#include "FeatureWorkerManagerConnection.h"
#include "ItalcCore.h"
#include "ItalcConfiguration.h"


FeatureWorkerManagerConnection::FeatureWorkerManagerConnection( FeatureManager& featureManager, const Feature::Uid& featureUid ) :
	QObject(),
	m_featureManager( featureManager ),
	m_socket( this ),
	m_featureUid( featureUid )
{
	connect( &m_socket, &QTcpSocket::connected,
			 this, &FeatureWorkerManagerConnection::sendInitMessage );

	connect( &m_socket, &QTcpSocket::disconnected,
			 QCoreApplication::instance(), &QCoreApplication::quit );

	connect( &m_socket, &QTcpSocket::readyRead,
			 this, &FeatureWorkerManagerConnection::receiveMessage );

	m_socket.connectToHost( QHostAddress::LocalHost, ItalcCore::config->featureWorkerManagerPort() );
}



void FeatureWorkerManagerConnection::sendInitMessage()
{
	qDebug() << "FeatureWorkerManagerConnection::sendInitMessage():" << m_featureUid;
	FeatureMessage( m_featureUid, FeatureMessage::InitCommand ).send( &m_socket );
}



void FeatureWorkerManagerConnection::receiveMessage()
{
	qDebug() << "FeatureWorkerManagerConnection::receiveMessage():" << m_featureUid;
	while( m_socket.bytesAvailable() > 0 )
	{
		m_featureManager.handleWorkerFeatureMessage( FeatureMessage( &m_socket ).receive(), &m_socket );
	}
}
