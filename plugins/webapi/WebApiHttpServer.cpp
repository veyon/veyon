// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtHttpServer/qhttpserver.h>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtHttpServer/qhttpserverfutureresponse.h>
#endif
#include <QJsonDocument>
#include <QSslCertificate>
#include <QSslKey>
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#include <QSslServer>
#endif

#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <memory>

#include "Filesystem.h"
#include "HostAddress.h"
#include "ProcessHelper.h"
#include "WebApiConfiguration.h"
#include "WebApiController.h"
#include "WebApiHttpServer.h"
#include "WebApiLogRedaction.h"

using WebApiLogRedaction::redactedHeaders;
using WebApiLogRedaction::redactedRequestData;
using WebApiLogRedaction::redactedResponseData;
using WebApiLogRedaction::toJson;

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
static inline QByteArray redactedHeaders(const QHttpHeaders& headers)
{
	return redactedHeaders(headers.toListOfPairs());
}
#endif



static QHttpServerResponse convertResponse(const WebApiController::Request& request,
										   const WebApiController::Response& response)
{
	if (response.error == WebApiController::Error::NoError)
	{
		if (response.binaryData.isEmpty() == false)
		{
			waDebug() << "[RESP]"
					  << request.path.toUtf8().constData()
					  << redactedHeaders(request.headers).constData()
					  << "[binary data]";
			return QHttpServerResponse{response.binaryData};
		}

		if (response.arrayData.isEmpty() == false)
		{
			waDebug() << "[RESP]"
					  << request.path.toUtf8().constData()
					  << redactedHeaders(request.headers).constData()
					  << toJson(response.arrayData).constData();
			return {QJsonArray::fromVariantList(response.arrayData)};
		}

		waDebug() << "[RESP]"
				  << request.path.toUtf8().constData()
				  << redactedHeaders(request.headers).constData()
				  << redactedResponseData(response.mapData).toUtf8().constData();
		return {QJsonObject::fromVariantMap(response.mapData)};
	}

	const auto statusCode = [&response]() {
		switch (response.error)
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
		{QStringLiteral("code"), int(response.error)},
		{QStringLiteral("message"), WebApiController::errorString(response.error)}};

	if (response.errorDetails.isEmpty() == false)
	{
		errorObject[QStringLiteral("details")] = response.errorDetails;
	}

	waDebug() << "[RESP] [ERROR]" << request.path.toUtf8().constData() << errorObject << int(statusCode);

	return {
		QByteArrayLiteral("application/json"),
		QJsonDocument{QJsonObject{{QStringLiteral("error"), errorObject}}}.toJson(QJsonDocument::Compact),
		statusCode};
}



static WebApiHttpServer* __serverInstance = nullptr;

WebApiHttpServer::WebApiHttpServer(const WebApiConfiguration& configuration, QObject* parent) :
	QObject(parent),
	m_configuration(configuration),
	m_controller(new WebApiController(configuration, this)),
	m_server(new QHttpServer(this))
{
	__serverInstance = this;

	m_threadPool.setMaxThreadCount(m_configuration.connectionLimit());
}



WebApiHttpServer::~WebApiHttpServer()
{
	delete m_server;

	delete m_controller;
}



template<>
QVariantMap WebApiHttpServer::dataFromRequest<WebApiHttpServer::Method::Post>(const QHttpServerRequest& request)
{
	QVariantMap data;

	const auto bodyData = QJsonDocument::fromJson(request.body()).object();
	for (auto it = bodyData.constBegin(), end = bodyData.constEnd(); it != end; ++it)
	{
		data[it.key()] = it.value().toVariant();
	}

	waDebug() << "[REQ] [POST]"
			  << request.url().toString().toUtf8().constData()
			  << redactedHeaders(request.headers()).constData()
			  << redactedRequestData(data).toUtf8().constData();

	return data;
}



template<>
QVariantMap WebApiHttpServer::dataFromRequest<WebApiHttpServer::Method::Put>(const QHttpServerRequest& request)
{
	QVariantMap data;

	const auto bodyData = QJsonDocument::fromJson(request.body()).object();
	for (auto it = bodyData.constBegin(), end = bodyData.constEnd(); it != end; ++it)
	{
		data[it.key()] = it.value().toVariant();
	}

	waDebug() << "[REQ] [PUT]"
			  << request.url().toString().toUtf8().constData()
			  << redactedHeaders(request.headers()).constData()
			  << redactedRequestData(data).toUtf8().constData();

	return data;
}



template<>
QVariantMap WebApiHttpServer::dataFromRequest<WebApiHttpServer::Method::Get>(const QHttpServerRequest& request)
{
	waDebug() << "[REQ] [GET]"
			  << request.url().toString().toUtf8().constData()
			  << redactedHeaders(request.headers()).constData();

	QVariantMap data;

	const auto items = request.query().queryItems(); // clazy:exclude=inefficient-qlist
	for (const auto& item : items)
	{
		data[item.first] = item.second;
	}

	return data;
}



template<>
QVariantMap WebApiHttpServer::dataFromRequest<WebApiHttpServer::Method::Delete>(const QHttpServerRequest& request)
{
	waDebug() << "[REQ] [DELETE]"
			  << request.url().toString().toUtf8().constData()
			  << redactedHeaders(request.headers()).constData();

	QVariantMap data;

	const auto items = request.query().queryItems(); // clazy:exclude=inefficient-qlist
	for (const auto& item : items)
	{
		data[item.first] = item.second;
	}

	return data;
}



template<WebApiHttpServer::Method M, typename... Args>
bool WebApiHttpServer::addRoute(const QString& path,
								WebApiController::Response (WebApiController::*controllerMethod)(const WebApiController::Request& request,
																								 Args... args))
{
	return m_server->route(QStringLiteral("/api/v1/%1").arg(path), []() {
		switch (M)
		{
		case Method::Get: return QHttpServerRequest::Method::Get;
		case Method::Post: return QHttpServerRequest::Method::Post;
		case Method::Put: return QHttpServerRequest::Method::Put;
		case Method::Delete: return QHttpServerRequest::Method::Delete;
		}
	}(), [=, this](Args... args, const QHttpServerRequest& request) ->
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
						   QFuture<QHttpServerResponse>
#else
						   QHttpServerFutureResponse
#endif
	{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
		const auto headers = request.headers().toListOfPairs();
#else
		const auto headers = request.headers();
#endif
		const auto data = dataFromRequest<M>(request);
		const auto controllerRequest = WebApiController::Request{path, headers, data};

		if (m_threadPool.activeThreadCount() >= m_threadPool.maxThreadCount())
		{
			auto response = convertResponse(controllerRequest, WebApiController::Error::ConnectionLimitReached);
			QFutureInterface<QHttpServerResponse> fi;
			fi.reportAndMoveResult(std::move(response));
			fi.reportFinished();
			return QFuture<QHttpServerResponse>{&fi};
		}

		return QtConcurrent::run(&m_threadPool, [=, this] {
			return convertResponse(controllerRequest,
								   (m_controller->*controllerMethod)(controllerRequest, std::forward<Args>(args)...));
		});
	});
}



bool WebApiHttpServer::start()
{
	if (m_server == nullptr || m_controller == nullptr)
	{
		return false;
	}

	if (m_configuration.httpsEnabled())
	{
		if (setupTls() == false)
		{
			return false;
		}
	}
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
	else
	{
		m_tcpServer = new QTcpServer(m_server);
	}
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
	if (m_tcpServer->listen(QHostAddress::Any, m_configuration.httpServerPort()) == false ||
		m_server->bind(m_tcpServer) == false)
#else
	if (m_server->listen(QHostAddress::Any, m_configuration.httpServerPort()) != m_configuration.httpServerPort())
#endif
	{
		vCritical() << "can't listen at port" << m_configuration.httpServerPort();
		return false;
	}

	auto success = true;

	success &= addRoute<Method::Get>(QStringLiteral("hoststate/<arg>"), &WebApiController::getHostState);
	success &= addRoute<Method::Get>(QStringLiteral("authentication/"), &WebApiController::getAuthenticationMethods);
	success &= addRoute<Method::Post>(QStringLiteral("authentication/<arg>"), &WebApiController::performAuthentication);
	success &= addRoute<Method::Delete>(QStringLiteral("authentication/<arg>"), &WebApiController::closeConnection);
	success &= addRoute<Method::Get>(QStringLiteral("framebuffer"), &WebApiController::getFramebuffer);
	success &= addRoute<Method::Get>(QStringLiteral("feature"), &WebApiController::listFeatures);
	success &= addRoute<Method::Get>(QStringLiteral("feature/<arg>"), &WebApiController::getFeatureStatus);
	success &= addRoute<Method::Put>(QStringLiteral("feature/<arg>"), &WebApiController::setFeatureStatus);
	success &= addRoute<Method::Get>(QStringLiteral("user"), &WebApiController::getUserInformation);
	success &= addRoute<Method::Get>(QStringLiteral("session"), &WebApiController::getSessionInformation);

	if (m_debug)
	{
		success &= addRoute<Method::Get>(QStringLiteral("debug/sleep/<arg>"), &WebApiController::sleep);
		success &= bool(m_server->route(QStringLiteral("/api/v1/debug/info"), [this]() { return getDebugInformation(); }));
	}

	success &= bool(m_server->route(QStringLiteral(".*"), [] {
		return QHttpServerResponse{
			QByteArrayLiteral("text/plain"),
			QStringLiteral("Invalid command or non-matching HTTP method").toUtf8(),
			QHttpServerResponse::StatusCode::NotFound};
	}));

	vInfo() << "listening at port" << m_configuration.httpServerPort();

	return success;
}



static bool privateKeyMatchesCertificate(const QSslKey& privateKey, const QSslCertificate& certificate)
{
	const auto keyPem = privateKey.toPem();
	const auto certPem = certificate.toPem();

	const std::unique_ptr<BIO, decltype(&BIO_free)> keyBio(BIO_new_mem_buf(keyPem.constData(), int(keyPem.size())),
														   &BIO_free);
	const std::unique_ptr<BIO, decltype(&BIO_free)> certBio(BIO_new_mem_buf(certPem.constData(), int(certPem.size())),
															&BIO_free);
	if (keyBio == nullptr || certBio == nullptr)
	{
		return false;
	}

	// never prompt for a passphrase
	const auto noPassphrase = [](char*, int, int, void*) { return 0; };

	const std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key(
		PEM_read_bio_PrivateKey(keyBio.get(), nullptr, noPassphrase, nullptr), &EVP_PKEY_free);
	const std::unique_ptr<X509, decltype(&X509_free)> cert(
		PEM_read_bio_X509(certBio.get(), nullptr, noPassphrase, nullptr), &X509_free);

	const auto matches = key && cert && X509_check_private_key(cert.get(), key.get()) == 1;

	ERR_clear_error();

	return matches;
}



bool WebApiHttpServer::setupTls()
{
	QFile certFile(VeyonCore::filesystem().expandPath(m_configuration.tlsCertificateFile()));

	if (certFile.exists() == false)
	{
		vCritical() << "TLS certificate file" << certFile.fileName() << "does not exist";
		return false;
	}

	if (certFile.open(QFile::ReadOnly) == false)
	{
		vCritical() << "TLS certificate file" << certFile.fileName() << "is not readable";
		return false;
	}

	// the file may contain the server certificate followed by intermediate certificates
	const auto certificateChain = QSslCertificate::fromData(certFile.readAll(), QSsl::Pem);
	if (certificateChain.isEmpty() || certificateChain.first().isNull())
	{
		vCritical() << certFile.fileName() << "does not contain a valid TLS certificate";
		return false;
	}

	QFile privateKeyFile(VeyonCore::filesystem().expandPath(m_configuration.tlsPrivateKeyFile()));

	if (privateKeyFile.exists() == false)
	{
		vCritical() << "TLS private key file" << privateKeyFile.fileName() << "does not exist";
		return false;
	}

	if (privateKeyFile.open(QFile::ReadOnly) == false)
	{
		vCritical() << "TLS private key file" << privateKeyFile.fileName() << "is not readable";
		return false;
	}

	const auto privateKeyFileData = privateKeyFile.readAll();

	QSslKey privateKey;
	for (const auto algorithm : {QSsl::KeyAlgorithm::Rsa, QSsl::KeyAlgorithm::Ec, QSsl::KeyAlgorithm::Dh})
	{
		QSslKey currentPrivateKey(privateKeyFileData, algorithm);
		if (currentPrivateKey.isNull() == false)
		{
			privateKey = currentPrivateKey;
			break;
		}
	}

	if (privateKey.isNull())
	{
		vCritical() << privateKeyFile.fileName() << "contains an invalid or unsupported TLS private key";
		return false;
	}

	if (privateKeyMatchesCertificate(privateKey, certificateChain.first()) == false)
	{
		vCritical() << "TLS private key" << privateKeyFile.fileName() << "does not match certificate"
					<< certFile.fileName();
		return false;
	}

	auto sslConfig = QSslConfiguration::defaultConfiguration();
	sslConfig.setLocalCertificateChain(certificateChain);
	sslConfig.setPrivateKey(privateKey);
	sslConfig.setProtocol(QSsl::TlsV1_3OrLater);

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
	auto sslServer = new QSslServer(this);
	sslServer->setSslConfiguration(sslConfig);

	m_tcpServer = sslServer;
#else
	m_server->sslSetup(sslConfig);
#endif

	return true;
}



QString WebApiHttpServer::getDebugInformation()
{
	const QString sysInfo =
		QStringLiteral("Veyon WebAPI server version: %1<br/>\n").arg(VeyonCore::versionString()) +
		QStringLiteral("Local hostname: %1<br/>\n").arg(HostAddress::localFQDN()) +
		QStringLiteral("Operating system: %1 %2 %3<br/>\n").arg(QSysInfo::prettyProductName(), QSysInfo::productType(), QSysInfo::productVersion()) +
		QStringLiteral("Kernel: %1 %2<br/>\n").arg(QSysInfo::kernelType(), QSysInfo::kernelVersion());
	const QString processLimits =
		QStringLiteral("<pre>%1</pre>\n").arg(QString::fromUtf8(ProcessHelper(QStringLiteral("prlimit"), {}).runAndReadAll()));
	const QString stats =
		QStringLiteral("Number of active HTTP server threads: %1 / %2\n<br/>").arg(m_threadPool.activeThreadCount()).arg(m_threadPool.maxThreadCount()) +
		m_controller->getStatistics();

	return QStringLiteral("<!DOCTYPE html>\n"
						  "<html lang=\"en-US\">\n"
						  "<head>\n"
						  "<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\">\n"
						  "<title>Veyon WebAPI Debug Information</title>\n"
						  "</head>\n"
						  "<body>\n"
						  "<h1>Veyon WebAPI Debug Information</h1>\n"
						  "<h2>System</h2>\n"
						  "%1\n"
						  "<h2>Process limits</h2>\n"
						  "%2\n"
						  "<h2>Statistics</h2>\n"
						  "%3"
						  "<h2>Client connection details</h2>\n"
						  "%4"
						  "</body>\n"
						  "</html>")
		.arg(sysInfo, processLimits, stats, m_controller->getConnectionDetails());
}
