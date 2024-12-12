/*
 * WebApiController.cpp - implementation of WebApiController class
 *
 * Copyright (c) 2020-2024 Tobias Junghans <tobydox@veyon.io>
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

#include <QTcpSocket>
#include <QBuffer>
#include <QEventLoop>
#include <QImageWriter>

#include "ComputerControlInterface.h"
#include "FeatureManager.h"
#include "PlatformNetworkFunctions.h"
#include "WebApiAuthenticationProxy.h"
#include "WebApiConfiguration.h"
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
	m_configuration( configuration ),
	m_connectionsLock( QReadWriteLock::Recursive )
{
	connect(&m_updateStatisticsTimer, &QTimer::timeout, this, &WebApiController::updateStatistics);
	m_updateStatisticsTimer.start(StatisticsUpdateIntervalSeconds * MillisecondsPerSecond);

	m_workerThread = new QThread(this);
	m_workerThread->setObjectName(QStringLiteral("WebApiController Worker"));
	m_workerThread->start();

	m_workerObject = new QObject;
	m_workerObject->moveToThread(m_workerThread);
}



WebApiController::~WebApiController()
{
	QWriteLocker connectionsWriteLocker{ &m_connectionsLock };

	m_connections.clear();
}



WebApiController::Response WebApiController::getHostState(const Request& request, const QString& host)
{
	Q_UNUSED(request);

	m_apiTotalRequestsCounter++;

	QTcpSocket socket;
	socket.connectToHost(host, VeyonCore::config().veyonServerPort());
	if (socket.waitForConnected(3000))
	{
		return QVariantMap{{k2s(Key::State), QByteArrayLiteral("online")}};
	}

	if (VeyonCore::platform().networkFunctions().ping(host))
	{
		return QByteArrayLiteral("up");
	}

	return QVariantMap{{k2s(Key::State), QByteArrayLiteral("down")}};
}




WebApiController::Response WebApiController::getAuthenticationMethods( const Request& request, const QString& host )
{
	m_apiTotalRequestsCounter++;

	Q_UNUSED(request)

	WebApiConnection connection( host.isEmpty() ? QStringLiteral("localhost") : host );

	const auto proxy = new WebApiAuthenticationProxy( m_configuration );
	proxy->populateCredentials( proxy->dummyAuthenticationMethod(), {} );

	connection.controlInterface()->start( {}, ComputerControlInterface::UpdateMode::Basic, proxy );

	if( proxy->waitForAuthenticationMethods(
			m_configuration.connectionAuthenticationTimeout() * MillisecondsPerSecond ) == false )
	{
		if( proxy->protocolErrorOccurred() )
		{
			return Error::ProtocolMismatch;
		}

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
	m_apiTotalRequestsCounter++;

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
	auto connection = runInWorkerThread<WebApiConnectionPointer>([this, host, proxy]() {
		auto connection = new WebApiConnection{host.isEmpty() ? QStringLiteral("localhost") : host};
		connection->controlInterface()->start({}, ComputerControlInterface::UpdateMode::Basic, proxy);

		// make shared pointer destroy the connection in management thread again
		return WebApiConnectionPointer{connection,
						  [this](WebApiConnection* c) { runInWorkerThreadNonBlocking([c] { delete c; }); } };
	});

	const auto authTimeout = m_configuration.connectionAuthenticationTimeout() * MillisecondsPerSecond;
	if( proxy->waitForAuthenticationMethods( authTimeout ) == false )
	{
		vWarning() << "waiting for authentication methods timed out";
		return Error::ConnectionTimedOut;
	}

	if( proxy->protocolErrorOccurred() )
	{
		return Error::ProtocolMismatch;
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

		connect(connection->controlInterface().get(), &ComputerControlInterface::framebufferUpdated,
				this, &WebApiController::incrementVncFramebufferUpdatesCounter);

		const auto idleTimer = connection->idleTimer();
		const auto lifetimeTimer = connection->lifetimeTimer();

		connect( idleTimer, &QTimer::timeout, this, [this, uuid]() { removeConnection( uuid ); } );
		connect( lifetimeTimer, &QTimer::timeout, this, [this, uuid]() { removeConnection( uuid ); } );

		const auto connectionIdleTimeout = m_configuration.connectionIdleTimeout() * MillisecondsPerSecond;
		const auto connectionLifetime = m_configuration.connectionLifetime() * MillisecondsPerHour;

		runInWorkerThread([=] {
			idleTimer->start(connectionIdleTimeout);
			lifetimeTimer->start(connectionLifetime);
		});

		return QVariantMap{
			{ QString::fromUtf8(connectionUidHeaderFieldName().toLower()), uuidToString(uuid) },
			{ k2s(Key::ValidUntil), QDateTime::currentSecsSinceEpoch() + connectionLifetime / MillisecondsPerSecond }
		};
	}

	return Error::AuthenticationFailed;
}



WebApiController::Response WebApiController::closeConnection( const Request& request, const QString& host )
{
	m_apiTotalRequestsCounter++;

	Q_UNUSED(host)

	Response checkResponse{};
	if( ( checkResponse = checkConnection( request ) ).error != Error::NoError )
	{
		return checkResponse;
	}

	removeConnection(QUuid{lookupHeaderField(request, connectionUidHeaderFieldName())});

	return {};
}



WebApiController::Response WebApiController::getFramebuffer( const Request& request )
{
	m_apiTotalRequestsCounter++;

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

	m_framebufferRequestsCounter++;

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
	m_apiTotalRequestsCounter++;

	Response checkResponse{};
	if( ( checkResponse = checkConnection( request ) ).error != Error::NoError )
	{
		return checkResponse;
	}

	const auto& features = VeyonCore::featureManager().features(); // clazy:exclude=inefficient-qlist
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
	m_apiTotalRequestsCounter++;

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

	runInMainThread([&] {
		VeyonCore::featureManager().controlFeature(Feature::Uid{feature}, operation, arguments, {connection->controlInterface()});
	});

	return {};
}



WebApiController::Response WebApiController::getFeatureStatus( const Request& request, const QString& feature )
{
	m_apiTotalRequestsCounter++;

	Response checkResponse{};
	if( ( checkResponse = checkConnection( request ) ).error != Error::NoError )
	{
		return checkResponse;
	}

	const auto connection = lookupConnection( request );
	const auto controlInterface = connection->controlInterface();

	const auto result = controlInterface->activeFeatures().contains(Feature::Uid{feature});

	return QVariantMap{ { k2s(Key::Active), result } };
}



WebApiController::Response WebApiController::getUserInformation( const Request& request )
{
	m_apiTotalRequestsCounter++;

	Response checkResponse{};
	if( ( checkResponse = checkConnection( request ) ).error != Error::NoError )
	{
		return checkResponse;
	}

	const auto connection = lookupConnection( request );
	const auto controlInterface = connection->controlInterface();

	const auto& userLoginName = controlInterface->userLoginName();
	auto userFullName = controlInterface->userFullName();
	if (userLoginName.isEmpty())
	{
		userFullName.clear();
	}

	return QVariantMap{
		{
			{k2s(Key::Login), userLoginName},
			{k2s(Key::FullName), userFullName}
		}
	};
}



WebApiController::Response WebApiController::getSessionInformation(const Request& request)
{
	m_apiTotalRequestsCounter++;

	Response checkResponse{};
	if((checkResponse = checkConnection(request)).error != Error::NoError)
	{
		return checkResponse;
	}

	const auto connection = lookupConnection(request);
	const auto controlInterface = connection->controlInterface();

	return QVariantMap{
		{
			{k2s(Key::SessionId), controlInterface->sessionInfo().id},
			{k2s(Key::SessionUptime), controlInterface->sessionInfo().uptime},
			{k2s(Key::SessionClientAddress), controlInterface->sessionInfo().clientAddress},
			{k2s(Key::SessionClientName), controlInterface->sessionInfo().clientName},
			{k2s(Key::SessionHostName), controlInterface->sessionInfo().hostName},
		}
	};
}



QString WebApiController::getStatistics()
{
	QReadLocker connectionsLocker{&m_connectionsLock};

	return QStringLiteral("Total API requests: %1 (%2/s in the past %3 s)\n<br/>").arg(m_apiTotalRequestsCounter).arg(m_apiTotalRequestsPerSecond).arg(StatisticsUpdateIntervalSeconds) +
			QStringLiteral("Framebuffer requests: %1 (%2/s in the past %3 s)\n<br/>").arg(m_framebufferRequestsCounter).arg(m_framebufferRequestsPerSecond).arg(StatisticsUpdateIntervalSeconds) +
			QStringLiteral("VNC framebuffer updates: %1 (%2/s in the past %3 s)\n<br/>").arg(m_vncFramebufferUpdatesCounter).arg(m_vncFramebufferUpdatesPerSecond).arg(StatisticsUpdateIntervalSeconds) +
			QStringLiteral("Number of client connections: %1<br/>\n").arg(m_connections.count());
}



QString WebApiController::getConnectionDetails()
{
	QReadLocker connectionsLocker{&m_connectionsLock};

	QStringList columns {QStringLiteral("Connection UUID"),
					   QStringLiteral("State"),
					   QStringLiteral("Host"),
					   QStringLiteral("User"),
					   QStringLiteral("Server version")
					  };
	QList<QStringList> rows;
	rows.reserve(m_connections.count());
	for (auto it = m_connections.constBegin(), end = m_connections.constEnd(); it != end; ++it)
	{
		const auto connection = it.value();

		rows.append({uuidToString(it.key()),
					 EnumHelper::toString(connection->controlInterface()->state()),
					 connection->controlInterface()->computer().hostAddress(),
					 connection->controlInterface()->userLoginName(),
					 EnumHelper::toString(connection->controlInterface()->serverVersion()),
					});
	}

	const auto rowToString = [](const QStringList& row, const QString& tag) {
		return std::accumulate(row.constBegin(), row.constEnd(), QString{}, [&tag](const QString& acc, const QString& cell) -> QString {
			return acc + QStringLiteral("<%1>%2</%1>").arg(tag, cell);
		});
	};

	const auto tableHeader = QStringLiteral("<tr>%1</tr>\n").arg(rowToString(columns, QStringLiteral("th")));
	const auto tableBody = std::accumulate(rows.constBegin(), rows.constEnd(), QString{},
										   [&](const QString& acc, const QStringList& row) -> QString {
		return acc + QStringLiteral("<tr>%1</tr>\n").arg(rowToString(row, QStringLiteral("td")));
	});

	return QStringLiteral("<table border=\"1\">\n%1%2</table>\n").arg(tableHeader, tableBody);
}



WebApiController::Response WebApiController::sleep(const Request& request, const int& seconds) // clazy:exclude=function-args-by-value
{
	Q_UNUSED(request)

	m_apiTotalRequestsCounter++;
	QThread::sleep(seconds);

	return {""};
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
	case Error::ProtocolMismatch: return QStringLiteral("Protocol mismatch error");
	}

	return {};
}



void WebApiController::runInMainThread(const std::function<void()>& functor)
{
	QMetaObject::invokeMethod(VeyonCore::instance(), functor, Qt::BlockingQueuedConnection);
}



void WebApiController::runInWorkerThread(const std::function<void()>& functor) const
{
	QMetaObject::invokeMethod(m_workerObject, functor, Qt::BlockingQueuedConnection);
}



void WebApiController::runInWorkerThreadNonBlocking(const std::function<void()>& functor) const
{
	QMetaObject::invokeMethod(m_workerObject, functor, Qt::QueuedConnection);
}



template<class T>
T WebApiController::runInWorkerThread(const std::function<T()>& functor) const
{
	T retval{};
	QMetaObject::invokeMethod(m_workerObject, functor, Qt::BlockingQueuedConnection, &retval );
	return retval;
}



void WebApiController::removeConnection( QUuid connectionUuid )
{
	QWriteLocker connectionsWriteLocker{ &m_connectionsLock };

	// deleter functor automatically performs actual deletion in worker thread
	m_connections.remove(connectionUuid);
}



void WebApiController::incrementVncFramebufferUpdatesCounter()
{
	++m_vncFramebufferUpdatesCounter;
}



void WebApiController::updateStatistics()
{
	m_apiTotalRequestsPerSecond = (m_apiTotalRequestsCounter - m_apiTotalRequestsLast) / StatisticsUpdateIntervalSeconds;
	m_framebufferRequestsPerSecond = (m_framebufferRequestsCounter - m_framebufferRequestsLast) / StatisticsUpdateIntervalSeconds;
	m_vncFramebufferUpdatesPerSecond = (m_vncFramebufferUpdatesCounter - m_vncFramebufferUpdatesLast) / StatisticsUpdateIntervalSeconds;

	m_apiTotalRequestsLast = m_apiTotalRequestsCounter;
	m_framebufferRequestsLast = m_framebufferRequestsCounter;
	m_vncFramebufferUpdatesLast = m_vncFramebufferUpdatesCounter;
}



QByteArray WebApiController::lookupHeaderField(const Request& request, const QByteArray& fieldName)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	for (const auto& h : request.headers)
	{
		if (h.first.compare(fieldName, Qt::CaseInsensitive) == 0)
		{
			return h.second;
		}
	}
#else
	const auto fieldNameString = QString::fromUtf8(fieldName);
	if (request.headers.contains(fieldNameString))
	{
		return request.headers[fieldNameString].toByteArray();
	}

	for (auto it = request.headers.constBegin(), end = request.headers.constEnd(); it != end; ++it)
	{
		if (it.key().compare(fieldNameString, Qt::CaseInsensitive ) == 0)
		{
			return it.value().toByteArray();
		}
	}
#endif

	return {};
}



WebApiController::LockingConnectionPointer WebApiController::lookupConnection( const Request& request )
{
	QReadLocker connectionsReadLocker{&m_connectionsLock};

	return m_connections.value(QUuid{lookupHeaderField(request, connectionUidHeaderFieldName())});
}



WebApiController::Response WebApiController::checkConnection( const Request& request )
{
	const QUuid connectionUuid{lookupHeaderField(request, connectionUidHeaderFieldName())};

	return runInWorkerThread<WebApiController::Response>([=]() -> WebApiController::Response {
		m_connectionsLock.lockForRead();
		if( connectionUuid.isNull() || m_connections.contains( connectionUuid ) == false )
		{
			m_connectionsLock.unlock();
			return Error::InvalidConnection;
		}

		const auto connection = std::as_const(m_connections)[connectionUuid];
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
	if( VeyonCore::featureManager().feature(Feature::Uid{featureUid}).isValid() == false )
	{
		return Error::InvalidFeature;
	}

	return {};
}
