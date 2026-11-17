// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>

#include "IpcServerPlatform.h"

#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


// Unix domain socket in an endpoint directory (QLocalServer)
class IpcServer::UnixListener : public IpcServer::Listener
{
public:
	explicit UnixListener(IpcServer* server) :
		m_server(server)
	{
	}

	bool listen(const QString& directory) override;
	void close() override;

	bool ensureListener() override
	{
		return true;
	}

private:
	IpcServer* const m_server;
	QLocalServer* m_localServer = nullptr;
};



std::unique_ptr<IpcServer::Listener> IpcServer::Listener::create(IpcServer* server)
{
	return std::make_unique<UnixListener>(server);
}



bool IpcServer::UnixListener::listen(const QString& directory)
{
	m_localServer = new QLocalServer(m_server);
	// makes Qt bind in a private temporary directory and rename the socket into place, so the
	// socket is never accessible with the default mode
	m_localServer->setSocketOptions(QLocalServer::UserAccessOption);
	m_localServer->setMaxPendingConnections(1);

	const auto path = endpointAddress(directory, m_server->m_endpointName);
	if (m_localServer->listen(path) == false)
	{
		vCritical() << "can't listen at" << path << m_localServer->errorString();
		return false;
	}

	const auto encodedPath = QFile::encodeName(path);
	if (::chmod(encodedPath.constData(), 0600) != 0 ||
		(m_server->m_peerPolicy.socketOwnerUserId >= 0 &&
		 ::chown(encodedPath.constData(), uid_t(m_server->m_peerPolicy.socketOwnerUserId), gid_t(-1)) != 0))
	{
		vCritical() << "failed to set mode or owner of" << path << strerror(errno);
		return false;
	}

	const auto expectedOwner = m_server->m_peerPolicy.socketOwnerUserId >= 0 ? uid_t(m_server->m_peerPolicy.socketOwnerUserId) : ::geteuid();
	struct stat status{};
	if (::lstat(encodedPath.constData(), &status) != 0 ||
		S_ISSOCK(status.st_mode) == false ||
		status.st_uid != expectedOwner ||
		(status.st_mode & 07777) != 0600)
	{
		vCritical() << "unexpected type, owner or mode of" << path;
		return false;
	}

	QObject::connect(m_localServer, &QLocalServer::newConnection, m_server, [this]() {
		while (m_localServer->hasPendingConnections())
		{
			m_server->acceptConnection(m_localServer->nextPendingConnection());
		}
	});

	return true;
}



void IpcServer::UnixListener::close()
{
	if (m_localServer)
	{
		// removes the socket file
		m_localServer->close();
	}
}



QString IpcServer::endpointAddress(const QString& directory, const QString& endpointName)
{
	return directory + QLatin1Char('/') + endpointName + QStringLiteral(".sock");
}



QString IpcServer::serviceEndpointDirectory()
{
	return QStringLiteral("/run/veyon");
}



QString IpcServer::workerEndpointDirectory()
{
	if (::geteuid() == 0)
	{
		return serviceEndpointDirectory();
	}

	// shared by all sessions of the user: endpoint names carry the session ID
	const auto runtimeDirectory = qEnvironmentVariable("XDG_RUNTIME_DIR");
	if (runtimeDirectory.startsWith(QLatin1Char('/')) == false)
	{
		return {};
	}

	return runtimeDirectory + QStringLiteral("/veyon");
}



IpcServer::DirectoryType IpcServer::workerEndpointDirectoryType()
{
	return ::geteuid() == 0 ? DirectoryType::Shared : DirectoryType::Private;
}



qint64 IpcServer::currentUserId()
{
	return qint64(::geteuid());
}



qint64 IpcServer::currentWindowsSessionId()
{
	return -1;
}



std::optional<QString> IpcServer::windowsPipeSecurityDescriptor(const PeerPolicy& peerPolicy)
{
	Q_UNUSED(peerPolicy)

	return std::nullopt;
}



bool IpcServer::prepareDirectory(const QString& path, DirectoryType type)
{
	const auto encodedPath = QFile::encodeName(path);
	const mode_t mode = type == DirectoryType::Shared ? 0711 : 0700;

	if (path.isEmpty() || path.startsWith(QLatin1Char('/')) == false)
	{
		vCritical() << "invalid endpoint directory" << path;
		return false;
	}

	if (::mkdir(encodedPath.constData(), mode) == 0)
	{
		// do not depend on the umask
		if (::chmod(encodedPath.constData(), mode) != 0)
		{
			vCritical() << "failed to set mode of endpoint directory" << path << strerror(errno);
			return false;
		}
	}
	else if (errno != EEXIST)
	{
		vCritical() << "failed to create endpoint directory" << path << strerror(errno);
		return false;
	}

	struct stat status{};
	if (::lstat(encodedPath.constData(), &status) != 0)
	{
		vCritical() << "failed to stat endpoint directory" << path << strerror(errno);
		return false;
	}

	if (S_ISDIR(status.st_mode) == false)
	{
		vCritical() << "endpoint directory" << path << "is not a directory - refusing to use it";
		return false;
	}

	if (status.st_uid != ::geteuid())
	{
		vCritical() << "endpoint directory" << path << "is owned by uid" << status.st_uid << "- refusing to use it";
		return false;
	}

	if (type == DirectoryType::Shared && (status.st_mode & (S_IWGRP | S_IWOTH)))
	{
		vCritical() << "endpoint directory" << path << "is writable by group or others - refusing to use it";
		return false;
	}

	if (type == DirectoryType::Private && (status.st_mode & 07777) != mode)
	{
		vCritical() << "endpoint directory" << path << "has mode" << QString::number(status.st_mode & 07777, 8)
					<< "instead of 0700 - refusing to use it";
		return false;
	}

	return true;
}



bool IpcServer::prepareWorkerEndpointDirectory(const QString& directory)
{
	return directory.isEmpty() == false && prepareDirectory(directory, workerEndpointDirectoryType());
}



std::optional<IpcServer::PeerIdentity> IpcServer::peerIdentity(QLocalSocket* socket)
{
#ifdef Q_OS_LINUX
	ucred credentials{};
	socklen_t size = sizeof(credentials);
	if (getsockopt(int(socket->socketDescriptor()), SOL_SOCKET, SO_PEERCRED, &credentials, &size) != 0 ||
		size != sizeof(credentials))
	{
		return {};
	}

	return PeerIdentity{ProcessId(credentials.pid), qint64(credentials.uid), -1};
#else
	Q_UNUSED(socket)
	return {};
#endif
}



bool IpcServer::checkPeerUser(const PeerIdentity& peer) const
{
	if (m_peerPolicy.userId < 0 || peer.userId != m_peerPolicy.userId)
	{
		vWarning() << "peer at" << m_endpointName << "has uid" << peer.userId << "instead of" << m_peerPolicy.userId;
		return false;
	}

	return true;
}



IpcServer::PeerCheckResult IpcServer::checkUnregisteredPeer()
{
	// the credentials are written to the child's stdin only after its PID was registered
	return PeerCheckResult::Rejected;
}
