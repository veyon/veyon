/*
 * WebApiController.cpp - implementation of WebApiController class
 *
 * Copyright (c) 2020-2021 Tobias Junghans <tobydox@veyon.io>
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
#include "WebApiController.h"


static void runInMainThread( const std::function<void()>& functor )
{
	if( QThread::currentThread() != VeyonCore::instance()->thread() )
	{
		QMetaObject::invokeMethod( VeyonCore::instance(), functor, Qt::BlockingQueuedConnection );
	}
	else
	{
		functor();
	}
}


template<class T>
static T runInMainThread( const std::function<T()>& functor )
{
	if( QThread::currentThread() != VeyonCore::instance()->thread() )
	{
		T retval{};
		QMetaObject::invokeMethod( VeyonCore::instance(), functor, Qt::BlockingQueuedConnection, &retval );
		return retval;
	}

	return functor();
}


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
	m_configuration( configuration ),
	m_connectionsLock( QReadWriteLock::Recursive )
{
}



WebApiController::~WebApiController()
{
	QWriteLocker connectionsWriteLocker{ &m_connectionsLock };

	m_connections.clear();
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
	QReadLocker connectionsReadLocker{&m_connectionsLock};

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

	connectionsReadLocker.unlock();

	auto proxy = new WebApiAuthenticationProxy( m_configuration );

	// create connection (including timer resources) in main thread
	auto connection = runInMainThread<WebApiConnectionPointer>( [host, proxy]() {
		auto connection = new WebApiConnection{host.isEmpty() ? QStringLiteral("localhost") : host};
		connection->controlInterface()->start( {}, ComputerControlInterface::UpdateMode::Monitoring, proxy );

		// make shared pointer destroy the connection in main thread again
		return WebApiConnectionPointer{ connection,
										[]( WebApiConnection* c ) { runInMainThread( [c] { delete c; } ); }
		};
	} );

	const auto authTimeout = m_configuration.connectionAuthenticationTimeout() * MillisecondsPerSecond;
	if( proxy->waitForAuthenticationMethods( authTimeout ) == false )
	{
		vWarning() << "waiting for authentication methods timed out";
		return Error::ConnectionTimedOut;
	}

	if( proxy->authenticationMethods().contains( methodUuid ) == false )
	{
		return Error::AuthenticationMethodNotAvailable;
	}

	if( proxy->populateCredentials( methodUuid, request.data[k2s(Key::Credentials)].toMap() ) == false )
	{
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
			 [&connection, &eventLoop]() { // clazy:exclude=lambda-in-connect
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

	authenticationTimeoutTimer.start( authTimeout );

	const auto result = eventLoop.exec() == ResultAuthSucceeded;

	if( result )
	{
		LockingConnectionPointer connectionLock{connection};

		m_connectionsLock.lockForWrite();
		m_connections[uuid] = connection;
		m_connectionsLock.unlock();

		const auto idleTimer = connection->idleTimer();
		const auto lifetimeTimer = connection->lifetimeTimer();

		connect( idleTimer, &QTimer::timeout, this, [this, uuid]() { removeConnection( uuid ); } );
		connect( lifetimeTimer, &QTimer::timeout, this, [this, uuid]() { removeConnection( uuid ); } );

		const auto connectionIdleTimeout = m_configuration.connectionIdleTimeout() * MillisecondsPerSecond;
		const auto connectionLifetime = m_configuration.connectionLifetime() * MillisecondsPerHour;

		runInMainThread( [=] {
			idleTimer->start( connectionIdleTimeout );
			lifetimeTimer->start( connectionLifetime );
		} );

		connect( connection->controlInterface().data(), &ComputerControlInterface::featureMessageReceived, this,
				 [this]( const FeatureMessage& featureMessage,
						 const ComputerControlInterface::Pointer& computerControlInterface ) {
					 computerControlInterface->lock();
					 m_featureManager.handleFeatureMessage( computerControlInterface, featureMessage );
					 computerControlInterface->unlock();
				 } );

		return QVariantMap{
			{ connectionUidHeaderFieldName().toLower(), uuidToString( uuid ) },
			{ k2s(Key::ValidUntil), QDateTime::currentSecsSinceEpoch() + connectionLifetime / MillisecondsPerSecond }
		};
	}

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
	const auto activeFeatures = lookupConnection( request )->controlInterface()->activeFeatures();

	QVariantList featureList; // clazy:exclude=inefficient-qlist
	featureList.reserve( features.size() );

	for( const auto& feature : features )
	{
		QVariantMap featureObject{ { k2s(Key::Name), feature.name() },
								   { k2s(Key::Uid), uuidToString(feature.uid()) },
								   { k2s(Key::ParentUid), uuidToString(feature.parentUid()) },
								   { k2s(Key::Active), activeFeatures.contains(feature.uid()) } };
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

	if( request.data.contains( k2s(Key::Active) ) == false )
	{
		return Error::InvalidData;
	}

	const auto connection = lookupConnection( request );

	const auto operation = request.data[k2s(Key::Active)].toBool() ? FeatureProviderInterface::Operation::Start
																	 : FeatureProviderInterface::Operation::Stop;
	const auto arguments = request.data[k2s(Key::Arguments)].toMap();

	m_featureManager.controlFeature( feature, operation, arguments, { connection->controlInterface() } );

	return {};
}



WebApiController::Response WebApiController::getFeatureStatus( const Request& request, const QString& feature )
{
	Response checkResponse{};
	if( ( checkResponse = checkConnection( request ) ).error != Error::NoError )
	{
		return checkResponse;
	}

	const auto connection = lookupConnection( request );
	const auto controlInterface = connection->controlInterface();

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

	const auto connection = lookupConnection( request );
	const auto controlInterface = connection->controlInterface();

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
	// destroy connection in main thread
	runInMainThread( [=] {
		QWriteLocker connectionsWriteLocker{ &m_connectionsLock };

		m_connections.remove( connectionUuid );
	} );
}



WebApiController::LockingConnectionPointer WebApiController::lookupConnection( const Request& request )
{
	QReadLocker connectionsReadLocker{&m_connectionsLock};

	return m_connections.value( request.headers[connectionUidHeaderFieldName()].toUuid() );
}



WebApiController::Response WebApiController::checkConnection( const Request& request )
{
	const auto connectionUuid = QUuid( request.headers[connectionUidHeaderFieldName()].toString() );

	return runInMainThread<WebApiController::Response>( [=]() -> WebApiController::Response {
		m_connectionsLock.lockForRead();
		if( connectionUuid.isNull() || m_connections.contains( connectionUuid ) == false )
		{
			m_connectionsLock.unlock();
			return Error::InvalidConnection;
		}

		const auto connection = qAsConst(m_connections)[connectionUuid];
		m_connectionsLock.unlock();

		connection->lock();

		const auto idleTimer = connection->idleTimer();
		idleTimer->stop();
		idleTimer->start();

		connection->unlock();

		return {};
	} );
}



WebApiController::Response WebApiController::checkFeature( const QString& featureUid )
{
	if( m_featureManager.feature( featureUid ).isValid() == false )
	{
		return Error::InvalidFeature;
	}

	return {};
}
