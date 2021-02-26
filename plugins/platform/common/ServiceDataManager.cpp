/*
 * ServiceDataManager.cpp - implementation of ServiceDataManager class
 *
 * Copyright (c) 2019-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QElapsedTimer>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcessEnvironment>

#include "ServiceDataManager.h"


ServiceDataManager::ServiceDataManager( QObject* parent ) :
	QThread( parent ),
	m_token( CryptoCore::generateChallenge().toHex() )
{
	vDebug();

	start();
}



ServiceDataManager::~ServiceDataManager()
{
	vDebug();

	quit();
	wait();
}



QByteArray ServiceDataManager::read( const Token& token )
{
	QLocalSocket socket;
	socket.connectToServer( serverName() );
	if( socket.waitForConnected() == false )
	{
		vCritical() << "connection timed out";
		return {};
	}

	VariantArrayMessage outMessage( &socket );
	outMessage.write( token.toByteArray() );
	outMessage.write( static_cast<int>( Command::ReadData ) );

	sendMessage( &socket, outMessage );

	if( waitForMessage( &socket ) == false )
	{
		vCritical() << "no response";
		return {};
	}

	VariantArrayMessage response( &socket );
	response.receive();

	return response.read().toByteArray();
}



bool ServiceDataManager::write( const Token& token, const Data& data )
{
	QLocalSocket socket;
	socket.connectToServer( serverName() );

	if( socket.waitForConnected() == false )
	{
		vCritical() << "connection timed out";
		return false;
	}

	VariantArrayMessage outMessage( &socket );
	outMessage.write( token.toByteArray() );
	outMessage.write( static_cast<int>( Command::WriteData ) );
	outMessage.write( data.toByteArray() );

	sendMessage( &socket, outMessage );

	return waitForMessage( &socket );
}



ServiceDataManager::Token ServiceDataManager::serviceDataTokenFromEnvironment()
{
	return QProcessEnvironment::systemEnvironment().value(
				QLatin1String( serviceDataTokenEnvironmentVariable() ) ).toUtf8();
}



void ServiceDataManager::run()
{
	m_server = new QLocalServer;
	m_server->setSocketOptions( QLocalServer::UserAccessOption );

	if( m_server->listen( serverName() ) == false )
	{
		vCritical() << "can't listen" << m_server->errorString();
		return;
	}

	connect( m_server, &QLocalServer::newConnection, m_server, [this]() { acceptConnection(); } );

	QThread::run();
}



bool ServiceDataManager::waitForMessage( QLocalSocket* socket )
{
	// wait for acknowledge
	QElapsedTimer messageTimeoutTimer;
	messageTimeoutTimer.start();

	VariantArrayMessage inMessage( socket );
	while( messageTimeoutTimer.elapsed() < MessageReadTimeout &&
		   inMessage.isReadyForReceive() == false )
	{
		socket->waitForReadyRead( SocketWaitTimeout );
	}

	return inMessage.isReadyForReceive();
}



void ServiceDataManager::sendMessage( QLocalSocket* socket, VariantArrayMessage& message )
{
	message.send();
	socket->flush();
	socket->waitForBytesWritten();
}



void ServiceDataManager::acceptConnection()
{
	auto socket = m_server->nextPendingConnection();

	connect( socket, &QLocalSocket::readyRead,
			 socket, [this, socket]() { handleConnection( socket ); } );
}



void ServiceDataManager::handleConnection( QLocalSocket* socket )
{
	VariantArrayMessage inMessage( socket );

	if( inMessage.isReadyForReceive() && inMessage.receive() )
	{
		const auto token = inMessage.read().toByteArray();
		if( token != m_token.toByteArray() )
		{
			vCritical() << "Invalid token";
			socket->close();
			return;
		}

		const auto command = static_cast<Command>( inMessage.read().toInt() );

		if( command == Command::WriteData )
		{
			m_data = inMessage.read().toByteArray();

			VariantArrayMessage acknowledge( socket );
			acknowledge.send();
		}
		else if( command == Command::ReadData )
		{
			VariantArrayMessage response( socket );
			response.write( m_data.toByteArray() );
			response.send();
		}
		else
		{
			vCritical() << "unknown command";
			socket->close();
		}
		socket->flush();
	}
}
