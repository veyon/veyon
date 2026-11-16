// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>

#include "WebApiController.h"

class QHttpServer;
class QHttpServerRequest;
class QTcpServer;
class WebApiConfiguration;

class WebApiHttpServer : public QObject
{
	Q_OBJECT
public:
	explicit WebApiHttpServer(const WebApiConfiguration& configuration, QObject* parent = nullptr);
	~WebApiHttpServer() override;

	bool start();

private:
	enum class Method {
		Get,
		Post,
		Put,
		Delete
	};

	bool setupTls();

	template<Method M>
	static QVariantMap dataFromRequest(const QHttpServerRequest& request);

	template<Method M, typename... Args>
	bool addRoute(const QString& path,
				  WebApiController::Response (WebApiController::*controllerMethod)(const WebApiController::Request& request,
																				   Args... args));

	QString getDebugInformation();

	const WebApiConfiguration& m_configuration;

	QThreadPool m_threadPool{this};

	WebApiController* m_controller{nullptr};
	QHttpServer* m_server{nullptr};
	QTcpServer* m_tcpServer = nullptr;
	bool m_debug = qEnvironmentVariableIsSet("VEYON_WEBAPI_DEBUG");
};
