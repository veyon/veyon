// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QEventLoop>

#include "AuthenticationCredentials.h"
#include "ExternalVncServer.h"
#include "ExternalVncServerConfigurationWidget.h"
#include "VeyonConfiguration.h"


ExternalVncServer::ExternalVncServer(QObject* parent) :
	QObject(parent),
	m_configuration(&VeyonCore::config())
{
}



void ExternalVncServer::upgrade(const QVersionNumber& oldVersion)
{
	if (oldVersion < QVersionNumber(1, 1))
	{
		// external VNC server password not encrypted yet?
		const auto rawPassword = m_configuration.passwordProperty().variantValue().toString();
		if (rawPassword.size() < MaximumPlaintextPasswordLength)
		{
			// setting it again will encrypt it
			m_configuration.setPassword(Configuration::Password::fromPlainText(rawPassword.toUtf8()));
		}
	}
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
	return m_configuration.password().plainText();
}



IMPLEMENT_CONFIG_PROXY(ExternalVncServerConfiguration)
