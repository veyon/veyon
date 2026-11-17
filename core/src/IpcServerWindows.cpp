// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <windows.h>

#include <QLocalSocket>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QWinEventNotifier>

#include "IpcServerPlatform.h"

#include <sddl.h>


// named pipe \\.\pipe\<endpoint name> with a protected DACL, created with
// FILE_FLAG_FIRST_PIPE_INSTANCE, plus a replacement listening instance
class IpcServer::WindowsListener : public IpcServer::Listener
{
public:
	explicit WindowsListener(IpcServer* server) :
		m_server(server)
	{
	}

	~WindowsListener() override;

	bool listen(const QString& directory) override;
	void close() override;

	bool ensureListener() override
	{
		return ensurePipeListener();
	}

private:
	bool createPipeInstance(bool firstInstance);
	bool ensurePipeListener();

	static constexpr unsigned long MaximumPipeInstances = 2;
	void handlePipeConnected();

	IpcServer* const m_server;

	QString m_pipeName;
	void* m_pipeHandle = nullptr;
	void* m_connectEvent = nullptr;
	std::unique_ptr<OVERLAPPED> m_overlapped;
	QWinEventNotifier* m_connectNotifier = nullptr;
};



std::unique_ptr<IpcServer::Listener> IpcServer::Listener::create(IpcServer* server)
{
	return std::make_unique<WindowsListener>(server);
}



IpcServer::WindowsListener::~WindowsListener()
{
	if (m_connectEvent)
	{
		CloseHandle(m_connectEvent);
	}
}



bool IpcServer::WindowsListener::listen(const QString& directory)
{
	Q_UNUSED(directory)

	m_pipeName = QStringLiteral("\\\\.\\pipe\\") + m_server->m_endpointName;

	m_overlapped = std::make_unique<OVERLAPPED>();
	m_connectEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (m_connectEvent == nullptr)
	{
		vCritical() << "CreateEvent() failed" << GetLastError();
		return false;
	}

	m_connectNotifier = new QWinEventNotifier(m_connectEvent, m_server);
	m_connectNotifier->setEnabled(false);
	QObject::connect(m_connectNotifier, &QWinEventNotifier::activated, m_server, [this]() { handlePipeConnected(); });

	return createPipeInstance(true);
}



std::optional<QString> IpcServer::windowsPipeSecurityDescriptor(const PeerPolicy& peerPolicy)
{
	// (A;;0x4;;;OW): the owner of the pipe (the creating process, normally SYSTEM) may create the
	// replacement instance (FILE_CREATE_PIPE_INSTANCE) also when it runs as a regular user, e.g. a
	// manually started veyon-server; it grants no read/write access
	switch (peerPolicy.windowsAccess)
	{
	case WindowsAccess::SystemAndAdministrators:
		return QStringLiteral("D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;0x4;;;OW)");
	case WindowsAccess::SystemOnly:
		return QStringLiteral("D:P(A;;GA;;;SY)(A;;0x4;;;OW)");
	case WindowsAccess::SystemAndUser:
	{
		static const QRegularExpression sidRX(QRegularExpression::anchoredPattern(QStringLiteral("S-1(-[0-9]+)+")));
		if (sidRX.match(peerPolicy.windowsUserSid).hasMatch() == false)
		{
			return std::nullopt;
		}
		// FILE_GENERIC_READ | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES: no FILE_CREATE_PIPE_INSTANCE
		// (= FILE_APPEND_DATA, part of GENERIC_WRITE), so the user can't add instances to the pipe
		return QStringLiteral("D:P(A;;GA;;;SY)(A;;0x12018b;;;%1)(A;;0x4;;;OW)").arg(peerPolicy.windowsUserSid);
	}
	}

	return std::nullopt;
}



bool IpcServer::WindowsListener::createPipeInstance(bool firstInstance)
{
	const auto securityDescriptor = windowsPipeSecurityDescriptor(m_server->m_peerPolicy);
	if (securityDescriptor.has_value() == false)
	{
		vCritical() << "invalid user SID for endpoint" << m_server->m_endpointName;
		return false;
	}

	PSECURITY_DESCRIPTOR descriptor = nullptr;
	if (ConvertStringSecurityDescriptorToSecurityDescriptorW(reinterpret_cast<LPCWSTR>(securityDescriptor->utf16()),
															 SDDL_REVISION_1, &descriptor, nullptr) == FALSE)
	{
		vCritical() << "failed to create security descriptor for endpoint" << m_server->m_endpointName << GetLastError();
		return false;
	}
	const auto freeDescriptor = qScopeGuard([descriptor]() { LocalFree(descriptor); });

	SECURITY_ATTRIBUTES attributes{};
	attributes.nLength = sizeof(attributes);
	attributes.lpSecurityDescriptor = descriptor;
	attributes.bInheritHandle = FALSE;

	static constexpr DWORD PipeBufferSize = IpcProtocol::FrameHeaderSize + IpcProtocol::MaximumFrameSize;

	// FILE_FLAG_FIRST_PIPE_INSTANCE: fail instead of joining a pipe someone else created first.
	// Two instances: a replacement listening instance is always created before a connected one
	// is closed, so the pipe name never becomes free for others while the endpoint exists. Further
	// instances can only be created with FILE_CREATE_PIPE_INSTANCE, which the DACL grants to
	// SYSTEM and administrators only.
	const DWORD openMode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | (firstInstance ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0);
	const auto handle = CreateNamedPipeW(reinterpret_cast<LPCWSTR>(m_pipeName.utf16()), openMode,
										 PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
										 MaximumPipeInstances, PipeBufferSize, PipeBufferSize, 0, &attributes);
	if (handle == INVALID_HANDLE_VALUE)
	{
		vCritical() << "failed to create named pipe for endpoint" << m_server->m_endpointName << GetLastError();
		return false;
	}

	m_pipeHandle = handle;

	ZeroMemory(m_overlapped.get(), sizeof(OVERLAPPED));
	ResetEvent(m_connectEvent);
	m_overlapped->hEvent = m_connectEvent;

	if (ConnectNamedPipe(handle, m_overlapped.get()) == FALSE)
	{
		const auto error = GetLastError();
		if (error == ERROR_PIPE_CONNECTED)
		{
			SetEvent(m_connectEvent);
		}
		else if (error != ERROR_IO_PENDING)
		{
			vCritical() << "ConnectNamedPipe() failed for endpoint" << m_server->m_endpointName << error;
			CloseHandle(handle);
			m_pipeHandle = nullptr;
			return false;
		}
	}
	else
	{
		SetEvent(m_connectEvent);
	}

	m_connectNotifier->setEnabled(true);

	return true;
}



void IpcServer::WindowsListener::handlePipeConnected()
{
	m_connectNotifier->setEnabled(false);

	const auto handle = m_pipeHandle;
	if (handle == nullptr)
	{
		return;
	}

	m_pipeHandle = nullptr;

	// query the result of this instance's ConnectNamedPipe() before the replacement instance below
	// reuses m_overlapped and m_connectEvent (which would make the result ERROR_IO_INCOMPLETE)
	DWORD transferred = 0;
	const auto connected = GetOverlappedResult(handle, m_overlapped.get(), &transferred, FALSE) != FALSE ||
		GetLastError() == ERROR_PIPE_CONNECTED;
	const auto error = GetLastError();

	// replace the listening instance before handling (and possibly closing) the connected one, so
	// the pipe name is never free
	ensurePipeListener();

	if (connected == false)
	{
		vWarning() << "waiting for connection at" << m_server->m_endpointName << "failed" << error;
		CloseHandle(handle);
		ensurePipeListener();
		return;
	}

	auto socket = new QLocalSocket(m_server);
	if (socket->setSocketDescriptor(reinterpret_cast<qintptr>(handle)) == false)
	{
		vWarning() << "could not wrap pipe handle of endpoint" << m_server->m_endpointName;
		CloseHandle(handle);
		delete socket;
		ensurePipeListener();
		return;
	}

	m_server->acceptConnection(socket);
}



bool IpcServer::WindowsListener::ensurePipeListener()
{
	if (m_pipeHandle || m_server->m_failed || m_server->m_connection || m_server->m_credentials.has_value() == false)
	{
		return true;
	}

	// fails while two instances exist (e.g. a pending and a dropped connection); retried after the
	// next connection was closed
	return createPipeInstance(false);
}



void IpcServer::WindowsListener::close()
{
	if (m_connectNotifier)
	{
		m_connectNotifier->setEnabled(false);
	}
	if (m_pipeHandle)
	{
		CancelIo(m_pipeHandle);
		CloseHandle(m_pipeHandle);
		m_pipeHandle = nullptr;
	}
}



QString IpcServer::endpointAddress(const QString& directory, const QString& endpointName)
{
	Q_UNUSED(directory)
	// QLocalSocket maps the name to \\.\pipe\<name>
	return endpointName;
}



QString IpcServer::serviceEndpointDirectory()
{
	return {};
}



QString IpcServer::workerEndpointDirectory()
{
	return {};
}



IpcServer::DirectoryType IpcServer::workerEndpointDirectoryType()
{
	return DirectoryType::Private;
}



qint64 IpcServer::currentUserId()
{
	return -1;
}



qint64 IpcServer::currentWindowsSessionId()
{
	DWORD sessionId = 0;
	if (ProcessIdToSessionId(GetCurrentProcessId(), &sessionId) == FALSE)
	{
		return -1;
	}
	return qint64(sessionId);
}



bool IpcServer::prepareDirectory(const QString& path, DirectoryType type)
{
	Q_UNUSED(path)
	Q_UNUSED(type)
	return true;
}



bool IpcServer::prepareWorkerEndpointDirectory(const QString& directory)
{
	Q_UNUSED(directory)
	return true;
}



std::optional<IpcServer::PeerIdentity> IpcServer::peerIdentity(QLocalSocket* socket)
{
	const auto handle = reinterpret_cast<HANDLE>(socket->socketDescriptor());
	ULONG processId = 0;
	DWORD sessionId = 0;
	if (handle == INVALID_HANDLE_VALUE ||
		GetNamedPipeClientProcessId(handle, &processId) == FALSE ||
		ProcessIdToSessionId(processId, &sessionId) == FALSE)
	{
		return {};
	}

	return PeerIdentity{ProcessId(processId), -1, qint64(sessionId)};
}



bool IpcServer::checkPeerUser(const PeerIdentity& peer) const
{
	if (m_peerPolicy.windowsSessionId >= 0 && peer.windowsSessionId != m_peerPolicy.windowsSessionId)
	{
		vWarning() << "peer at" << m_endpointName << "runs in session" << peer.windowsSessionId
				   << "instead of" << m_peerPolicy.windowsSessionId;
		return false;
	}

	return true;
}



IpcServer::PeerCheckResult IpcServer::checkUnregisteredPeer()
{
	// the child may connect before its process ID was registered (stdin is written while the
	// process is created), see IpcServer::acceptConnection()
	return PeerCheckResult::Pending;
}
