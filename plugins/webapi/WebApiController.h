/*
 * WebApiController.h - declaration of WebApiController class
 *
 * Copyright (c) 2020-2025 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include <QReadWriteLock>
#include <QThread>

#include "EnumHelper.h"
#include "LockingPointer.h"
#include "WebApiConnection.h"

#define waDebug() if (VeyonCore::isDebugging()==false); else qDebug() << "[WebAPI]"

class WebApiConfiguration;

class WebApiController : public QObject
{
	Q_OBJECT
public:
	enum class Key
	{
		State,
		Method,
		Methods,
		Credentials,
		Format,
		Compression,
		Quality,
		Width,
		Height,
		Feature,
		Name,
		Uid,
		ParentUid,
		Active,
		Arguments,
		Login,
		FullName,
		SessionId,
		SessionUptime,
		SessionClientAddress,
		SessionClientName,
		SessionHostName,
		ValidUntil
	};
	Q_ENUM(Key)

	enum class Error
	{
		NoError,
		InvalidData,
		InvalidConnection,
		InvalidFeature,
		InvalidCredentials,
		AuthenticationMethodNotAvailable,
		AuthenticationFailed,
		ConnectionLimitReached,
		ConnectionTimedOut,
		UnsupportedImageFormat,
		FramebufferNotAvailable,
		FramebufferEncodingError,
		ProtocolMismatch,
	};

	struct Request
	{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		using Headers = QList<QPair<QByteArray, QByteArray>>;
#else
		using Headers = QVariantMap;
#endif
		Request(const QString& p = {}, const Headers& h = {}, const QVariantMap& d = {}) : path(p), headers(h), data(d) { }
		QString path{};
		Headers headers{};
		QVariantMap data{};
	};

	struct Response
	{
		Response( const QVariantMap& md = {} ) : mapData(md) { }
		Response( const QVariantList& ad ) : arrayData(ad) { }
		Response( const QByteArray& bd ) : binaryData(bd) { }
		Response( Error e, const QString& ed = {} ) : error(e), errorDetails(ed) { }
		QVariantList arrayData{};
		QVariantMap mapData{};
		QByteArray binaryData{};
		Error error{Error::NoError};
		QString errorDetails{};
	};

	explicit WebApiController( const WebApiConfiguration& configuration, QObject* parent = nullptr );
	~WebApiController() override;

	Response getHostState(const Request& request, const QString& host);

	Response getAuthenticationMethods( const Request& request, const QString& host );
	Response performAuthentication( const Request& request, const QString& host );
	Response closeConnection( const Request& request, const QString& host );

	Response getFramebuffer( const Request& request );

	Response listFeatures( const Request& request );
	Response setFeatureStatus( const Request& request, const QString& feature );
	Response getFeatureStatus( const Request& request, const QString& feature );

	Response getUserInformation( const Request& request );
	Response getSessionInformation(const Request& request);

	QString getStatistics();
	QString getConnectionDetails();
	Response sleep(const Request& request, const int& seconds);

	static QString errorString( Error error );

private:
	void runInWorkerThread(const std::function<void()>& functor) const;
	void runInWorkerThreadNonBlocking(const std::function<void()>& functor) const;

	template<class T>
	T runInWorkerThread(const std::function<T()>& functor) const;

	void removeConnection( QUuid connectionUuid );

	void incrementVncFramebufferUpdatesCounter();
	void updateStatistics();

	static constexpr auto MillisecondsPerSecond = 1000;
	static constexpr auto MillisecondsPerHour = MillisecondsPerSecond * 60 * 60;
	static constexpr auto StatisticsUpdateIntervalSeconds = 10;

	static QString k2s( Key key )
	{
		return EnumHelper::toCamelCaseString( key );
	}

	static QByteArray connectionUidHeaderFieldName()
	{
		return QByteArrayLiteral("Connection-Uid");
	}

	static QByteArray lookupHeaderField(const Request& request, const QByteArray& fieldName);

	using WebApiConnectionPointer = QSharedPointer<WebApiConnection>;
	using LockingConnectionPointer = LockingPointer<WebApiConnectionPointer>;

	LockingConnectionPointer lookupConnection( const Request& request );

	using CheckFunction = std::function<Response(const WebApiController*, const QVariantMap &)>;

	Response checkConnection( const Request& request );
	Response checkFeature( const QString& featureUid );

	const WebApiConfiguration& m_configuration;
	QMap<QUuid, WebApiConnectionPointer> m_connections{};
	QReadWriteLock m_connectionsLock;

	QThread* m_workerThread = nullptr;
	QObject* m_workerObject = nullptr;

	QTimer m_updateStatisticsTimer{this};
	QAtomicInt m_apiTotalRequestsCounter = 0;
	QAtomicInt m_framebufferRequestsCounter = 0;
	QAtomicInt m_vncFramebufferUpdatesCounter = 0;
	QAtomicInt m_apiTotalRequestsLast = 0;
	QAtomicInt m_framebufferRequestsLast = 0;
	QAtomicInt m_vncFramebufferUpdatesLast = 0;
	QAtomicInt m_apiTotalRequestsPerSecond = 0;
	QAtomicInt m_framebufferRequestsPerSecond = 0;
	QAtomicInt m_vncFramebufferUpdatesPerSecond = 0;

};
