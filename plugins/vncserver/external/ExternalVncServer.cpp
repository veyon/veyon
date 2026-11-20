// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QEventLoop>

#include "AuthenticationCredentials.h"
#include "ExternalVncServer.h"
#include "ExternalVncServerConfigurationWidget.h"
#include "SecretStore.h"
#include "VeyonConfiguration.h"


ExternalVncServer::ExternalVncServer(QObject* parent) :
	QObject(parent),
	m_configuration(&VeyonCore::config())
{
}



Configuration::SecretPropertyList ExternalVncServer::secretProperties() const
{
	return {
		{QStringLiteral("ExternalVncServer"), QStringLiteral("Password"), QStringLiteral("vncserver-external/password"), true},
	};
}



QWidget* ExternalVncServer::configurationWidget()
{
	return new ExternalVncServerConfigurationWidget(m_configuration);
}



void ExternalVncServer::prepareServer()
{
}



bool ExternalVncServer::runServer(int serverPort, const Password& password)
{
	Q_UNUSED(serverPort);
	Q_UNUSED(password);

	QEventLoop().exec();

	return true;
}



int ExternalVncServer::configuredServerPort()
{
	return m_configuration.serverPort();
}



ExternalVncServer::Password ExternalVncServer::configuredPassword()
{
	// veyon-server gets the password from veyon-service (02-ipc.md §6); the SecretStore is
	// available in privileged components only
	if (SecretStore::isAvailable() == false)
	{
		return {};
	}

	const auto password = SecretStore::read(m_configuration.password().key());
	if (password.has_value() == false)
	{
		return {};
	}

	return Password(QByteArray::fromRawData(password->constData(), password->size()));
}



IMPLEMENT_CONFIG_PROXY(ExternalVncServerConfiguration)
