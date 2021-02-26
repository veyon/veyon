/*
 * WebApiHttpServer.h - declaration of WebApiHttpServer class
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

#include <QObject>

#include "WebApiController.h"

class QHttpServer;
class QHttpServerRequest;
class WebApiConfiguration;

class WebApiHttpServer : public QObject
{
	Q_OBJECT
public:
	explicit WebApiHttpServer( const WebApiConfiguration& configuration, QObject* parent = nullptr );
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
	static QVariantMap dataFromRequest( const QHttpServerRequest& request );

	template<Method M, typename ... Args>
	bool addRoute( const QString& path,
				  WebApiController::Response(WebApiController::* controllerMethod)( const WebApiController::Request& request,
																					  Args... args ) );

	const WebApiConfiguration& m_configuration;

	QThreadPool m_threadPool{this};

	WebApiController* m_controller{nullptr};
	QHttpServer* m_server{nullptr};

};
