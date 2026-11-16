// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CommandLineIO.h"
#include "WebApiConfigurationPage.h"
#include "WebApiHttpServer.h"
#include "WebApiPlugin.h"


WebApiPlugin::WebApiPlugin(QObject* parent) :
	QObject(parent),
	m_configuration(&VeyonCore::config()),
	m_httpServerThread(this),
	m_commands({{QStringLiteral("runserver"), tr("Run WebAPI server")}})
{
	if (VeyonCore::component() == VeyonCore::Component::Service &&
		m_configuration.httpServerEnabled())
	{
		connect(VeyonCore::instance(), &VeyonCore::initialized,
				this, &WebApiPlugin::startHttpServerThread);
	}
}



WebApiPlugin::~WebApiPlugin()
{
	if (m_httpServerThread.isRunning())
	{
		m_httpServerThread.quit();
		m_httpServerThread.wait(ServerThreadTerminationTimeout);
	}
}



ConfigurationPage* WebApiPlugin::createConfigurationPage()
{
	return new WebApiConfigurationPage(m_configuration);
}



CommandLinePluginInterface::RunResult WebApiPlugin::handle_runserver(const QStringList& arguments)
{
	Q_UNUSED(arguments)

	m_httpServer = new WebApiHttpServer{m_configuration};

	if (m_httpServer->start() == false)
	{
		CommandLineIO::error(tr("Failed to start WebAPI server at port %1").arg(m_configuration.httpServerPort()));

		return Failed;
	}

	CommandLineIO::info(tr("WebAPI server running at port %1").arg(m_configuration.httpServerPort()));

	return VeyonCore::instance()->exec() == 0 ? Successful : Failed;
}



void WebApiPlugin::startHttpServerThread()
{
	m_httpServer = new WebApiHttpServer{m_configuration};
	m_httpServer->moveToThread(&m_httpServerThread);

	connect(&m_httpServerThread, &QThread::started, m_httpServer, &WebApiHttpServer::start);

	m_httpServerThread.start();
}


IMPLEMENT_CONFIG_PROXY(WebApiConfiguration)
