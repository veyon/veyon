// SPDX-FileCopyrightText: 2024-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QEventLoop>

#include "CaptureHelperProcess.h"
#include "PipeWireVncConfigurationWidget.h"
#include "PipeWireVncServer.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"

#include <cstring>


PipeWireVncServer::PipeWireVncServer(QObject* parent) :
	QObject(parent),
	m_configuration(&VeyonCore::config())
{
}



PipeWireVncServer::~PipeWireVncServer() = default;



QWidget* PipeWireVncServer::configurationWidget()
{
	return new PipeWireVncConfigurationWidget(m_configuration);
}



void PipeWireVncServer::prepareServer()
{
	// nothing to do: veyon-capture-helper accesses the session bus and the portal
}



bool PipeWireVncServer::runServer(int serverPort, const Password& password)
{
	// the helper listens on localSocketPath() only
	Q_UNUSED(serverPort)

	initializeHelperCredentials();

	auto credentials = CaptureHelperCredentials::create(
		Veyon::Crypto::SecureBuffer(password.constData(), password.size()), m_socketName);
	if (credentials.has_value() == false)
	{
		// fail closed instead of running the helper with a weaker password
		vCritical() << "no valid VNC password for the capture helper";
		return false;
	}

	// runs in the VncServer thread for the lifetime of the server
	CaptureHelperProcess helperProcess(CaptureHelperProcess::defaultHelperFilePath(), std::move(*credentials));
	// like LinuxServerProcess: nobody reads the helper's output pipes
	if (VeyonCore::config().logToSystem())
	{
		helperProcess.setProcessChannelMode(QProcess::ForwardedChannels);
	}
	else
	{
		helperProcess.setStandardOutputFile(QProcess::nullDevice());
		helperProcess.setStandardErrorFile(QProcess::nullDevice());
	}

	// the server knows the process which must listen on the socket (VncServerSocket)
	connect(&helperProcess, &QProcess::started, &helperProcess,
			[this, &helperProcess]() { m_helperProcessId = helperProcess.processId(); });
	connect(&helperProcess, &QProcess::stateChanged, &helperProcess, [this](QProcess::ProcessState state) {
		if (state == QProcess::NotRunning)
		{
			m_helperProcessId = -1;
		}
	});

	if (helperProcess.startSupervised() == false)
	{
		return false;
	}

	QEventLoop eventLoop;
	eventLoop.exec();

	return true;
}



PipeWireVncServer::Password PipeWireVncServer::configuredPassword()
{
	initializeHelperCredentials();

	return m_vncPassword;
}



QString PipeWireVncServer::localSocketPath()
{
	initializeHelperCredentials();

	const auto socketPath = CaptureHelperProcess::socketPath(m_socketName);

	// never empty, so that VncServer never falls back to a loopback TCP port anybody could listen
	// on; runServer() fails in this case
	return socketPath.isEmpty() ? QStringLiteral("/nonexistent/veyon-capture-helper.sock") : socketPath;
}



void PipeWireVncServer::initializeHelperCredentials()
{
	if (m_vncPassword.isEmpty())
	{
		const auto randomPassword = CaptureHelperCredentials::generatePassword();
		if (randomPassword.has_value())
		{
			m_vncPassword = Password(int(randomPassword->size()));
			std::memcpy(m_vncPassword.data(), randomPassword->constData(), size_t(randomPassword->size()));
		}
	}

	if (m_socketName.isEmpty())
	{
		m_socketName = CaptureHelperCredentials::generateSocketName(VeyonCore::sessionId()).value_or(QString{});
	}
}
