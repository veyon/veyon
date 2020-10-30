/*
 * WebApiController.cpp - implementation of WebApiController class
 *
 * Copyright (c) 2020 Tobias Junghans <tobydox@veyon.io>
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

#include <QBuffer>
#include <QEventLoop>
#include <QImageWriter>

#include "ComputerControlInterface.h"
#include "PlatformSessionFunctions.h"
#include "WebApiAuthenticationProxy.h"
#include "WebApiConfiguration.h"
#include "WebApiConnection.h"
#include "WebApiController.h"


static QString uuidToString( QUuid uuid )
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
	return uuid.toString( QUuid::WithoutBraces );
#else
	return uuid.toString().remove( QLatin1Char('{') ).remove( QLatin1Char('}') );
#endif
}


WebApiController::WebApiController( const WebApiConfiguration& configuration, QObject* parent ) :
	QObject( parent ),
	m_configuration( configuration )
{
}



WebApiController::~WebApiController()
{
	qDeleteAll( m_connections );
}




WebApiController::Response WebApiController::getAuthenticationMethods( const Request& request, const QString& host )
{
	Q_UNUSED(request)

	WebApiConnection connection( host.isEmpty() ? QStringLiteral("localhost") : host );

	const auto proxy = new WebApiAuthenticationProxy( m_configuration );
	proxy->populateCredentials( proxy->dummyAuthenticationMethod(), {} );

	connection.controlInterface()->start( {}, ComputerControlInterface::UpdateMode::Monitoring, proxy );

	if( proxy->waitForAuthenticationMethods(
			m_configuration.connectionAuthenticationTimeout() * MillisecondsPerSecond ) == false )
	{
		vWarning() << "waiting for authentication methods timed out";
		return Error::ConnectionTimedOut;
	}

	const auto authMethodUuids = proxy->authenticationMethods();

	QVariantList methods; // clazy:exclude=inefficient-qlist
	methods.reserve( authMethodUuids.size() );

	for( const auto& authMethodUuid : authMethodUuids )
	{
		methods.append( uuidToString( authMethodUuid ) );
	}

	return QVariantMap{ { k2s(Key::Methods), methods } };
}



WebApiController::Response WebApiController::performAuthentication( const Request& request, const QString& host )
{
	if( m_connections.size() >= m_configuration.connectionLimit() )
	{
		return Error::ConnectionLimitReached;
	}

	const auto methodUuid = QUuid( request.data[k2s(Key::Method)].toString() );
	if( methodUuid.isNull() )
	{
		return Error::InvalidData;
	}

	auto uuid = QUuid::createUuid();
	while( m_connections.contains( uuid ) )
	{
		uuid = QUuid::createUuid();
	}

	auto connection = new WebApiConnection( host.isEmpty() ? QStringLiteral("localhost") : host );
	auto proxy = new WebApiAuthenticationProxy( m_configuration );

	connection->controlInterface()->start( {}, ComputerControlInterface::UpdateMode::Monitoring, proxy );

	connection->idleTimer()->setInterval( m_configuration.connectionIdleTimeout() * MillisecondsPerSecond );
	connection->idleTimer()->start();
	connect( connection->idleTimer(), &QTimer::timeout, this, [this, uuid]() { removeConnection( uuid ); } );

	connection->lifetimeTimer()->setInterval( m_configuration.connectionAuthenticationTimeout() * MillisecondsPerSecond );
	connection->lifetimeTimer()->start();
	connect( connection->lifetimeTimer(), &QTimer::timeout, this, [this, uuid]() { removeConnection( uuid ); } );

	if( proxy->waitForAuthenticationMethods(
			m_configuration.connectionAuthenticationTimeout() * MillisecondsPerSecond ) == false )
	{
		vWarning() << "waiting for authentication methods timed out";
		delete connection;
		return Error::ConnectionTimedOut;
	}

	if( proxy->authenticationMethods().contains( methodUuid ) == false )
	{
		delete connection;
		return Error::AuthenticationMethodNotAvailable;
	}

	if( proxy->populateCredentials( methodUuid, request.data[k2s(Key::Credentials)].toMap() ) == false )
	{
		delete connection;
		return Error::InvalidCredentials;
	}

	QEventLoop eventLoop;
	QTimer authenticationTimeoutTimer;

	static constexpr auto ResultAuthSucceeded = 0;
	static constexpr auto ResultAuthFailed = 1;
	static constexpr auto ResultAuthTimedOut = 2;

	connect( &authenticationTimeoutTimer, &QTimer::timeout, &eventLoop,
			 [&eventLoop]() { eventLoop.exit(ResultAuthTimedOut); } );
	connect( connection->controlInterface().data(), &ComputerControlInterface::stateChanged, &eventLoop,
			 [connection, &eventLoop]() { // clazy:exclude=lambda-in-connect
				 switch( connection->controlInterface()->state() )
				 {
				 case ComputerControlInterface::State::AuthenticationFailed:
					 eventLoop.exit( ResultAuthFailed );
					 break;
				 case ComputerControlInterface::State::Connected:
					 eventLoop.exit( ResultAuthSucceeded );
				 default:
					 break;
				 }
			 } );

	authenticationTimeoutTimer.start( m_configuration.connectionAuthenticationTimeout() * MillisecondsPerSecond );

	const auto result = eventLoop.exec() == ResultAuthSucceeded;

	if( result )
	{
		m_connections[uuid] = connection;

		const auto lifetimeTimer = connection->lifetimeTimer();
		lifetimeTimer->stop();
		lifetimeTimer->setInterval( m_configuration.connectionLifetime() * MillisecondsPerHour );
		lifetimeTimer->start();

		connect( connection->controlInterface().data(), &ComputerControlInterface::featureMessageReceived, this,
				 [this]( const FeatureMessage& featureMessage, ComputerControlInterface::Pointer computerControlInterface ) {
					 m_featureManager.handleFeatureMessage( computerControlInterface, featureMessage );
				 } );

		return QVariantMap{
			{ connectionUidHeaderFieldName().toLower(), uuidToString( uuid ) },
			{ k2s(Key::ValidUntil), QDateTime::currentSecsSinceEpoch() + lifetimeTimer->remainingTime() / MillisecondsPerSecond }
		};
	}

	delete connection;

	return Error::AuthenticationFailed;
}



WebApiController::Response WebApiController::closeConnection( const Request& request, const QString& host )
{
	Q_UNUSED(host)

	Response checkResponse{};
	if( ( checkResponse = checkConnection( request ) ).error != Error::NoError )
	{
		return checkResponse;
	}

	removeConnection( QUuid( request.headers[connectionUidHeaderFieldName()].toString() ) );

	return {};
}



WebApiController::Response WebApiController::getFramebuffer( const Request& request )
{
	Response checkResponse{};
	if( ( checkResponse = checkConnection( request ) ).error != Error::NoError )
	{
		return checkResponse;
	}

	const auto connection = lookupConnection( request );

	if( connection->controlInterface()->hasValidFramebuffer() == false )
	{
		return Error::FramebufferNotAvailable;
	}

	const auto width = request.data[k2s(Key::Width)].toInt();
	const auto height = request.data[k2s(Key::Height)].toInt();
	const auto size = connection->scaledFramebufferSize( width, height );

	const auto compression = request.data[k2s(Key::Compression)].toString().toInt();
	const auto quality = request.data[k2s(Key::Quality)].toString().toInt();

	auto format = request.data[k2s(Key::Format)].toString().toUtf8();
	if( format.isEmpty() )
	{
		format = QByteArrayLiteral("png");
	}

	if( QImageWriter::supportedImageFormats().contains( format ) == false )
	{
		return Error::UnsupportedImageFormat;
	}

	const auto imageData = connection->encodedFramebufferData( size, format, compression, quality );

	if( imageData.isNull() )
	{
		return { Error::FramebufferEncodingError, connection->framebufferEncodingError() };
	}

	return imageData;
}



WebApiController::Response WebApiController::listFeatures( const Request& request )
{
	Response checkResponse{};
	if( ( checkResponse = checkConnection( request ) ).error != Error::NoError )
	{
		return checkResponse;
	}

	const auto& features = m_featureManager.features(); // clazy:exclude=inefficient-qlist

	QVariantList featureList; // clazy:exclude=inefficient-qlist
	featureList.reserve( features.size() );

	for( const auto& feature : features )
	{
		QVariantMap featureObject{ { k2s(Key::Name), feature.name() },
								  { k2s(Key::Uid), uuidToString(feature.uid()) },
								  { k2s(Key::ParentUid), uuidToString(feature.parentUid()) } };
		featureList.append( featureObject );
	}

	return featureList;
}



WebApiController::Response WebApiController::setFeatureStatus( const Request& request, const QString& feature )
{
	Response checkResponse{};
	if( ( checkResponse = checkConnection( request ) ).error != Error::NoError ||
		( checkResponse = checkFeature( feature ) ).error != Error::NoError )
	{
		return checkResponse;
	}

	const auto controlInterface = lookupConnection( request )->controlInterface();

	if( request.data.contains( k2s(Key::Active) ) == false )
	{
		return Error::InvalidData;
	}

	const auto operation = request.data[k2s(Key::Active)].toBool() ? FeatureProviderInterface::Operation::Start
																	 : FeatureProviderInterface::Operation::Stop;
	const auto arguments = request.data[k2s(Key::Arguments)].toMap();

	m_featureManager.controlFeature( feature, operation, arguments, { controlInterface } );

	return {};
}



WebApiController::Response WebApiController::getFeatureStatus( const Request& request, const QString& feature )
{
	Response checkResponse{};
	if( ( checkResponse = checkConnection( request ) ).error != Error::NoError )
	{
		return checkResponse;
	}

	const auto controlInterface = lookupConnection( request )->controlInterface();

	const auto result = controlInterface->activeFeatures().contains( feature );

	return QVariantMap{ { k2s(Key::Active), result } };
}



WebApiController::Response WebApiController::getUserInformation( const Request& request )
{
	Response checkResponse{};
	if( ( checkResponse = checkConnection( request ) ).error != Error::NoError )
	{
		return checkResponse;
	}

	const auto controlInterface = lookupConnection( request )->controlInterface();

	const auto& userLoginName = controlInterface->userLoginName();
	auto userFullName = controlInterface->userFullName();
	auto sessionId = controlInterface->userSessionId();
	if( userLoginName.isEmpty() )
	{
		userFullName.clear();
		sessionId = PlatformSessionFunctions::InvalidSessionId;
	}

	return QVariantMap{ {
		{ k2s(Key::Login), userLoginName },
		{ k2s(Key::FullName), userFullName },
		{ k2s(Key::Session), sessionId }
	} };
}



QString WebApiController::errorString( WebApiController::Error error )
{
	switch( error )
	{
	case Error::NoError: return {};
	case Error::InvalidData: return QStringLiteral("Invalid data");
	case Error::InvalidConnection: return QStringLiteral("Invalid connection");
	case Error::InvalidFeature: return QStringLiteral("Invalid feature");
	case Error::AuthenticationMethodNotAvailable: return QStringLiteral("Authentication method not offered by server");
	case Error::InvalidCredentials: return QStringLiteral("Invalid or incomplete credentials");
	case Error::AuthenticationFailed: return QStringLiteral("Authentication failed");
	case Error::ConnectionLimitReached: return QStringLiteral("Limit for maximum number of connections reached");
	case Error::ConnectionTimedOut: return QStringLiteral("Connection timed out");
	case Error::UnsupportedImageFormat: return QStringLiteral("Unsupported image format");
	case Error::FramebufferNotAvailable: return QStringLiteral("Framebuffer not yet available");
	case Error::FramebufferEncodingError: return QStringLiteral("Framebuffer encoding error");
	}

	return {};
}



void WebApiController::removeConnection( QUuid connectionUuid )
{
	delete m_connections.take( connectionUuid );
}



WebApiConnection* WebApiController::lookupConnection( const Request& request ) const
{
	return m_connections.value( request.headers[connectionUidHeaderFieldName()].toUuid() );
}



WebApiController::Response WebApiController::checkConnection( const Request& request ) const
{
	const auto connectionUuid = QUuid( request.headers[connectionUidHeaderFieldName()].toString() );
	if( connectionUuid.isNull() || m_connections.contains( connectionUuid ) == false )
	{
		return Error::InvalidConnection;
	}

	const auto idleTimer = m_connections[connectionUuid]->idleTimer();
	idleTimer->stop();
	idleTimer->start();

	return {};
}



WebApiController::Response WebApiController::checkFeature( const QString& featureUid ) const
{
	if( m_featureManager.feature( featureUid ).isValid() == false )
	{
		return Error::InvalidFeature;
	}

	return {};
}
