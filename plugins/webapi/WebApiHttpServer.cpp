/*
 * WebApiHttpServer.cpp - implementation of WebApiHttpServer class
 *
 * Copyright (c) 2020-2023 Tobias Junghans <tobydox@veyon.io>
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

#include <QtHttpServer/qhttpserver.h>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtHttpServer/qhttpserverfutureresponse.h>
#endif
#include <QJsonDocument>
#include <QSslCertificate>
#include <QSslKey>

#include "Filesystem.h"
#include "WebApiHttpServer.h"
#include "WebApiConfiguration.h"
#include "WebApiController.h"

static inline QByteArray toJson(const QVariant& data)
{
	return QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact);
}


static QHttpServerResponse convertResponse(const WebApiController::Request& request,
										   const WebApiController::Response& response)
{
	if( response.error == WebApiController::Error::NoError )
	{
		if( response.binaryData.isEmpty() == false )
		{
			waDebug() << "[RESP]"
					  << request.path.toUtf8().constData()
					  << toJson(request.headers).constData()
					  << "[binary data]";
			return QHttpServerResponse{ response.binaryData };
		}

		if( response.arrayData.isEmpty() == false )
		{
			waDebug() << "[RESP]"
					  << request.path.toUtf8().constData()
					  << toJson(request.headers).constData()
					  << toJson(response.arrayData).constData();
			return { QJsonArray::fromVariantList(response.arrayData) };
		}

		waDebug() << "[RESP]"
				  << request.path.toUtf8().constData()
				  << toJson(request.headers).constData()
				  << toJson(response.mapData).constData();
		return { QJsonObject::fromVariantMap(response.mapData) };
	}

	const auto statusCode = [&response]() {
		switch( response.error )
		{
		case WebApiController::Error::NoError: return QHttpServerResponse::StatusCode::Ok;
		case WebApiController::Error::InvalidData: return QHttpServerResponse::StatusCode::BadRequest;
		case WebApiController::Error::InvalidConnection: return QHttpServerResponse::StatusCode::Unauthorized;
		case WebApiController::Error::InvalidFeature: return QHttpServerResponse::StatusCode::BadRequest;
		case WebApiController::Error::InvalidCredentials: return QHttpServerResponse::StatusCode::BadRequest;
		case WebApiController::Error::AuthenticationMethodNotAvailable: return QHttpServerResponse::StatusCode::BadRequest;
		case WebApiController::Error::AuthenticationFailed: return QHttpServerResponse::StatusCode::Unauthorized;
		case WebApiController::Error::ConnectionLimitReached: return QHttpServerResponse::StatusCode::TooManyRequests;
		case WebApiController::Error::ConnectionTimedOut: return QHttpServerResponse::StatusCode::RequestTimeout;
		case WebApiController::Error::UnsupportedImageFormat: return QHttpServerResponse::StatusCode::ServiceUnavailable;
		case WebApiController::Error::FramebufferNotAvailable: return QHttpServerResponse::StatusCode::ServiceUnavailable;
		case WebApiController::Error::FramebufferEncodingError: return QHttpServerResponse::StatusCode::InternalServerError;
		case WebApiController::Error::ProtocolMismatch: return QHttpServerResponse::StatusCode::NotImplemented;
		}
		return QHttpServerResponse::StatusCode::BadRequest;
	}();

	QJsonObject errorObject{
		{ QStringLiteral("code"), int(response.error) },
		{ QStringLiteral("message"), WebApiController::errorString( response.error ) }
	};

	if( response.errorDetails.isEmpty() == false )
	{
		errorObject[QStringLiteral("details")] = response.errorDetails;
	}

	waDebug() << "[RESP] [ERROR]" << request.path.toUtf8().constData() << errorObject << int(statusCode);

	return {
		QByteArrayLiteral("application/json"),
		QJsonDocument{ QJsonObject{ { QStringLiteral("error"), errorObject } } }.toJson( QJsonDocument::Compact ),
		statusCode
	};
}



WebApiHttpServer::WebApiHttpServer( const WebApiConfiguration& configuration, QObject* parent ) :
	QObject( parent ),
	m_configuration( configuration ),
	m_controller( new WebApiController( configuration, this ) ),
	m_server( new QHttpServer( this ) )
{
	m_threadPool.setMaxThreadCount( m_configuration.connectionLimit() );
}



WebApiHttpServer::~WebApiHttpServer()
{
	delete m_server;

	delete m_controller;
}



template<>
QVariantMap WebApiHttpServer::dataFromRequest<WebApiHttpServer::Method::Post>( const QHttpServerRequest& request )
{
	waDebug() << "[REQ] [POST]"
			  << request.url().toString().toUtf8().constData()
			  << toJson(request.headers()).constData()
			  << request.body().constData();

	QVariantMap data;

	const auto bodyData = QJsonDocument::fromJson( request.body() ).object();
	for( auto it = bodyData.constBegin(), end = bodyData.constEnd(); it != end; ++it )
	{
		data[it.key()] = it.value().toVariant();
	}

	return data;
}



template<>
QVariantMap WebApiHttpServer::dataFromRequest<WebApiHttpServer::Method::Put>( const QHttpServerRequest& request )
{
	waDebug() << "[REQ] [PUT]"
			  << request.url().toString().toUtf8().constData()
			  << toJson(request.headers()).constData()
			  << request.body().constData();

	QVariantMap data;

	const auto bodyData = QJsonDocument::fromJson( request.body() ).object();
	for( auto it = bodyData.constBegin(), end = bodyData.constEnd(); it != end; ++it )
	{
		data[it.key()] = it.value().toVariant();
	}

	return data;
}



template<>
QVariantMap WebApiHttpServer::dataFromRequest<WebApiHttpServer::Method::Get>( const QHttpServerRequest& request )
{
	waDebug() << "[REQ] [GET]"
			  << request.url().toString().toUtf8().constData()
			  << toJson(request.headers()).constData();

	QVariantMap data;

	const auto items = request.query().queryItems(); // clazy:exclude=inefficient-qlist
	for( const auto& item : items )
	{
		data[item.first] = item.second;
	}

	return data;
}



template<>
QVariantMap WebApiHttpServer::dataFromRequest<WebApiHttpServer::Method::Delete>( const QHttpServerRequest& request )
{
	waDebug() << "[REQ] [DELETE]"
			  << request.url().toString().toUtf8().constData()
			  << toJson(request.headers()).constData();

	QVariantMap data;

	const auto items = request.query().queryItems(); // clazy:exclude=inefficient-qlist
	for( const auto& item : items )
	{
		data[item.first] = item.second;
	}

	return data;
}



template<WebApiHttpServer::Method M, typename ... Args>
bool WebApiHttpServer::addRoute( const QString& path,
								WebApiController::Response(WebApiController::* controllerMethod)( const WebApiController::Request& request,
																									Args... args ) )
{
	return m_server->route( QStringLiteral("/api/v1/%1").arg(path), []()
		{
			switch(M)
			{
			case Method::Get: return QHttpServerRequest::Method::Get;
			case Method::Post: return QHttpServerRequest::Method::Post;
			case Method::Put: return QHttpServerRequest::Method::Put;
			case Method::Delete: return QHttpServerRequest::Method::Delete;
			}
		}(),
		[=](Args... args, const QHttpServerRequest& request) ->
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	QFuture<QHttpServerResponse>
#else
	QHttpServerFutureResponse
#endif
		{
			const auto headers = request.headers();
			const auto data = dataFromRequest<M>( request );
			const auto controllerRequest = WebApiController::Request{path, headers, data};

			if( m_threadPool.activeThreadCount() >= m_threadPool.maxThreadCount() )
			{
				auto response = convertResponse(controllerRequest, WebApiController::Error::ConnectionLimitReached);
				QFutureInterface<QHttpServerResponse> fi;
				fi.reportAndMoveResult( std::move(response) );
				fi.reportFinished();
				return QFuture<QHttpServerResponse>{ &fi };
			}

			return QtConcurrent::run( &m_threadPool, [=] {
				return convertResponse(controllerRequest,
									   (m_controller->*controllerMethod)(controllerRequest, std::forward<Args>(args)... ));
			} );
		} );
}



bool WebApiHttpServer::start()
{
	if( m_server == nullptr || m_controller == nullptr )
	{
		return false;
	}

	if( m_configuration.httpsEnabled() &&
		setupTls() == false )
	{
		return false;
	}

	if( m_server->listen( QHostAddress::Any, m_configuration.httpServerPort() ) != m_configuration.httpServerPort() )
	{
		vCritical() << "can't listen at port" << m_configuration.httpServerPort();
		return false;
	}

	auto success = true;

	success &= addRoute<Method::Get>( QStringLiteral("authentication/"), &WebApiController::getAuthenticationMethods );
	success &= addRoute<Method::Post>( QStringLiteral("authentication/<arg>"), &WebApiController::performAuthentication );
	success &= addRoute<Method::Delete>( QStringLiteral("authentication/<arg>"), &WebApiController::closeConnection );
	success &= addRoute<Method::Get>( QStringLiteral("framebuffer"), &WebApiController::getFramebuffer );
	success &= addRoute<Method::Get>( QStringLiteral("feature"), &WebApiController::listFeatures );
	success &= addRoute<Method::Get>( QStringLiteral("feature/<arg>"), &WebApiController::getFeatureStatus );
	success &= addRoute<Method::Put>( QStringLiteral("feature/<arg>"), &WebApiController::setFeatureStatus );
	success &= addRoute<Method::Get>( QStringLiteral("user"), &WebApiController::getUserInformation );
	success &= addRoute<Method::Get>(QStringLiteral("session"), &WebApiController::getSessionInformation);

	success &= m_server->route( QStringLiteral(".*"), [] {
		return QHttpServerResponse{
			QByteArrayLiteral("text/plain"),
			QStringLiteral("Invalid command or non-matching HTTP method").toUtf8(),
			QHttpServerResponse::StatusCode::NotFound
		};
	} );

	vInfo() << "listening at port" << m_configuration.httpServerPort();

	return success;
}



bool WebApiHttpServer::setupTls()
{
	QFile certFile( VeyonCore::filesystem().expandPath( m_configuration.tlsCertificateFile() ) );

	if( certFile.exists() == false )
	{
		vCritical() << "TLS certificate file" << certFile.fileName() << "does not exist";
		return false;
	}

	if( certFile.open( QFile::ReadOnly ) == false )
	{
		vCritical() << "TLS certificate file" << certFile.fileName() << "is not readable";
		return false;
	}

	QSslCertificate certificate( certFile.readAll() );
	if( certificate.isNull() )
	{
		vCritical() << certFile.fileName() << "does not contain a valid TLS certificate";
		return false;
	}

	QFile privateKeyFile( VeyonCore::filesystem().expandPath( m_configuration.tlsPrivateKeyFile() ) );

	if( privateKeyFile.exists() == false )
	{
		vCritical() << "TLS private key file" << privateKeyFile.fileName() << "does not exist";
		return false;
	}

	if( privateKeyFile.open( QFile::ReadOnly ) == false )
	{
		vCritical() << "TLS private key file" << privateKeyFile.fileName() << "is not readable";
		return false;
	}

	const auto privateKeyFileData = privateKeyFile.readAll();

	QSslKey privateKey;
	for( auto algorithm : { QSsl::KeyAlgorithm::Rsa, QSsl::KeyAlgorithm::Ec
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
				 , QSsl::KeyAlgorithm::Dh
#endif
		 } )
	{
		QSslKey currentPrivateKey( privateKeyFileData, algorithm );
		if( currentPrivateKey.isNull() == false )
		{
			privateKey = currentPrivateKey;
			break;
		}
	}

	if( privateKey.isNull() )
	{
		vCritical() << privateKeyFile.fileName() << "contains an invalid or unsupported TLS private key";
		return false;
	}

	m_server->sslSetup( certificate, privateKey, QSsl::TlsV1_3OrLater );

	return true;
}
