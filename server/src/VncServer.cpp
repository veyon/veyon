// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AuthenticationCredentials.h"
#include "CryptoCore.h"
#include "IpcClient.h"
#include "PlatformPluginInterface.h"
#include "PlatformSessionFunctions.h"
#include "PluginManager.h"
#include "VeyonConfiguration.h"
#include "VncServer.h"
#include "VncServerPluginInterface.h"
#include "rfb/rfbproto.h"


VncServer::VncServer(QObject* parent) :
	QThread(parent),
	m_pluginInterface(nullptr)
{
	const auto currentSessionType = VeyonCore::platform().sessionFunctions().currentSessionType();

	VeyonCore::authenticationCredentials().setInternalVncServerPassword(
		CryptoCore::generateChallenge().toBase64().left(MAXPWLEN));

	VncServerPluginInterfaceList defaultVncServerPlugins;

	for (auto pluginObject : std::as_const(VeyonCore::pluginManager().pluginObjects()))
	{
		auto pluginInterface = qobject_cast<PluginInterface*>(pluginObject);
		auto vncServerPluginInterface = qobject_cast<VncServerPluginInterface*>(pluginObject);

		if (pluginInterface && vncServerPluginInterface)
		{
			// skip VNC server plugins which support certain session types only and do not support
			// the current session type
			if (vncServerPluginInterface->supportedSessionTypes().isEmpty() == false &&
				vncServerPluginInterface->supportedSessionTypes().contains(currentSessionType, Qt::CaseInsensitive) == false)
			{
				continue;
			}

			if (pluginInterface->uid() == VeyonCore::config().vncServerPlugin())
			{
				m_pluginInterface = vncServerPluginInterface;
			}
			else if (pluginInterface->flags().testFlag(Plugin::ProvidesDefaultImplementation))
			{
				defaultVncServerPlugins.append(vncServerPluginInterface); // clazy:exclude=reserve-candidates
			}
		}
	}

	if (m_pluginInterface == nullptr)
	{
		if (defaultVncServerPlugins.isEmpty())
		{
			vCritical() << "no VNC server plugins found!";
		}
		else
		{
			m_pluginInterface = defaultVncServerPlugins.first();
		}
	}

	queryExternalVncServerPassword();
}



VncServer::~VncServer()
{
	vDebug();
}



void VncServer::prepare()
{
	vDebug();

	if (m_pluginInterface)
	{
		m_pluginInterface->prepareServer();
	}
}



int VncServer::serverBasePort() const
{

	if (m_pluginInterface && m_pluginInterface->configuredServerPort() > 0)
	{
		return m_pluginInterface->configuredServerPort();
	}

	return VeyonCore::config().vncServerPort();
}



int VncServer::serverPort() const
{
	return serverBasePort() + VeyonCore::sessionId();
}



VncServerEndpoint VncServer::endpoint() const
{
	VncServerEndpoint endpoint;
	endpoint.port = serverPort();

	if (m_pluginInterface)
	{
		endpoint.localSocketPath = m_pluginInterface->localSocketPath();
		if (endpoint.isLocalSocket())
		{
			auto pluginInterface = m_pluginInterface;
			endpoint.expectedProcessId = [pluginInterface]() { return pluginInterface->localSocketProcessId(); };
		}
	}

	return endpoint;
}



VncServer::Password VncServer::password() const
{
	if (m_externalVncServerPassword.isEmpty() == false)
	{
		return m_externalVncServerPassword;
	}

	if (m_pluginInterface && m_pluginInterface->configuredPassword().isEmpty() == false)
	{
		return m_pluginInterface->configuredPassword();
	}

	return VeyonCore::authenticationCredentials().internalVncServerPassword();
}



void VncServer::run()
{
	if (m_pluginInterface)
	{
		vDebug() << "running";

		if (m_pluginInterface->configuredServerPort() > 0)
		{
			VeyonCore::config().setVncServerPort(m_pluginInterface->configuredServerPort());
		}

		if (m_externalVncServerPassword.isEmpty() == false)
		{
			VeyonCore::authenticationCredentials().setInternalVncServerPassword(m_externalVncServerPassword);
		}
		else if (m_pluginInterface->configuredPassword().isEmpty() == false)
		{
			VeyonCore::authenticationCredentials().setInternalVncServerPassword(m_pluginInterface->configuredPassword());
		}

		if (m_pluginInterface->runServer(serverPort(), password()) == false)
		{
			vCritical() << "An error occurred while running the VNC server plugin";
		}

		vDebug() << "finished";
	}
}



void VncServer::queryExternalVncServerPassword()
{
	// called in the main thread, the IPC client must not be used from the VNC server thread
	const auto ipcClient = VeyonCore::serviceIpcClient();
	if (ipcClient == nullptr || ipcClient->isConnected() == false)
	{
		return;
	}

	// refused unless the external VNC server plugin is configured
	const auto response = ipcClient->call(IpcProtocol::Command::GetExternalVncServerPassword);
	if (response.ok)
	{
		m_externalVncServerPassword = QByteArray::fromBase64(
			response.result.value(IpcProtocol::Key::Password).toString().toLatin1());
	}
}
