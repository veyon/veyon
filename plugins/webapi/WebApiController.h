/*
 * WebApiController.h - declaration of WebApiController class
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

#pragma once

#include <QReadWriteLock>

#include "EnumHelper.h"
#include "FeatureManager.h"
#include "LockingPointer.h"
#include "WebApiConnection.h"

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
		Session,
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
	};

	struct Request
	{
		Request( const QVariantMap& h = {}, const QVariantMap& d = {} ) : headers(h), data(d) { }
		QVariantMap headers{};
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

	Response getAuthenticationMethods( const Request& request, const QString& host );
	Response performAuthentication( const Request& request, const QString& host );
	Response closeConnection( const Request& request, const QString& host );

	Response getFramebuffer( const Request& request );

	Response listFeatures( const Request& request );
	Response setFeatureStatus( const Request& request, const QString& feature );
	Response getFeatureStatus( const Request& request, const QString& feature );

	Response getUserInformation( const Request& request );

	static QString errorString( Error error );

private:
	void removeConnection( QUuid connectionUuid );

	static constexpr auto MillisecondsPerSecond = 1000;
	static constexpr auto MillisecondsPerHour = MillisecondsPerSecond * 60 * 60;

	static QString k2s( Key key )
	{
		return EnumHelper::toCamelCaseString( key );
	}

	static QString connectionUidHeaderFieldName()
	{
		return QStringLiteral("Connection-Uid");
	}

	using WebApiConnectionPointer = QSharedPointer<WebApiConnection>;
	using LockingConnectionPointer = LockingPointer<WebApiConnectionPointer>;

	LockingConnectionPointer lookupConnection( const Request& request );

	using CheckFunction = std::function<Response(const WebApiController*, const QVariantMap &)>;

	Response checkConnection( const Request& request );
	Response checkFeature( const QString& featureUid );

	const WebApiConfiguration& m_configuration;
	const FeatureManager m_featureManager;
	QMap<QUuid, WebApiConnectionPointer> m_connections{};
	QReadWriteLock m_connectionsLock;

};
