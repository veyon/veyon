/*
 * PortalHelperProcess.cpp - Main-process proxy for the veyon-portal-helper subprocess
 *
 * Copyright (c) 2024-2026 Tobias Junghans <tobydox@veyon.io>
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

#include "PortalHelperProcess.h"
#include "PortalHelperProtocol.h"

#include "PlatformUserFunctions.h"
#include "VeyonCore.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcessEnvironment>
#include <QSocketNotifier>

#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// ---------------------------------------------------------------------------
// HelperQProcess – privilege-dropping QProcess subclass (Qt 5 compatibility)
// ---------------------------------------------------------------------------

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void PortalHelperProcess::HelperQProcess::setupChildProcess()
{
    if (targetUid == 0)
    {
        return; // Already running as the target user – nothing to do
    }

    // Drop root privileges before exec so the helper uses the session user's
    // identity when connecting to the session D-Bus.
    const struct passwd* pw = ::getpwuid(targetUid);
    if (pw != nullptr)
    {
        ::initgroups(pw->pw_name, pw->pw_gid);
        ::setgid(pw->pw_gid);
    }
    ::setuid(targetUid);
}
#endif

// ---------------------------------------------------------------------------
// PortalHelperProcess
// ---------------------------------------------------------------------------

PortalHelperProcess::PortalHelperProcess(QObject* parent)
    : QObject(parent)
{
}

PortalHelperProcess::~PortalHelperProcess()
{
    close();
}

void PortalHelperProcess::start()
{
    close(); // Clean up any previous run

    m_pipewireFd     = -1;
    m_pipeWireNodeId = 0;
    m_sessionRunning = false;

    if (!createSocketPair())
    {
        Q_EMIT failed();
        return;
    }

    startHelper();
}

void PortalHelperProcess::close()
{
    m_sessionRunning = false;

    // Ask the helper to exit gracefully
    if (m_process && m_process->state() == QProcess::Running && m_mainSocketFd >= 0)
    {
        PortalHelperProtocol::M2HClose msg;
        PortalHelperProtocol::sendMessage(m_mainSocketFd, &msg);
        m_process->waitForFinished(2000);
    }

    delete m_notifier;
    m_notifier = nullptr;

    if (m_mainSocketFd >= 0)
    {
        ::close(m_mainSocketFd);
        m_mainSocketFd = -1;
    }

    if (m_helperSocketFd >= 0)
    {
        ::close(m_helperSocketFd);
        m_helperSocketFd = -1;
    }

    if (m_process)
    {
        if (m_process->state() != QProcess::NotRunning)
        {
            m_process->kill();
            m_process->waitForFinished(3000);
        }
        delete m_process;
        m_process = nullptr;
    }

    if (m_pipewireFd >= 0)
    {
        ::close(m_pipewireFd);
        m_pipewireFd = -1;
    }
}

void PortalHelperProcess::notifyKeyboardKeysym(quint32 keySym, bool down)
{
    if (!m_sessionRunning || m_mainSocketFd < 0)
    {
        return;
    }

    PortalHelperProtocol::M2HNotifyKey msg;
    msg.keySym = keySym;
    msg.down   = down ? 1u : 0u;
    PortalHelperProtocol::sendMessage(m_mainSocketFd, &msg);
}

void PortalHelperProcess::notifyPointer(int buttonMask, int x, int y)
{
    if (!m_sessionRunning || m_mainSocketFd < 0)
    {
        return;
    }

    PortalHelperProtocol::M2HNotifyPointer msg;
    msg.buttonMask = buttonMask;
    msg.x          = x;
    msg.y          = y;
    PortalHelperProtocol::sendMessage(m_mainSocketFd, &msg);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

QString PortalHelperProcess::helperFilePath()
{
    return QDir::toNativeSeparators(
        QCoreApplication::applicationDirPath() + QDir::separator() +
        QStringLiteral("veyon-portal-helper"));
}

bool PortalHelperProcess::createSocketPair()
{
    int fds[2]{-1, -1};
    if (::socketpair(AF_UNIX, SOCK_SEQPACKET, 0, fds) != 0)
    {
        vCritical() << "socketpair() failed:" << errno;
        return false;
    }

    // Set CLOEXEC on the parent's end so it is not accidentally inherited by
    // any other child process that may be spawned later.
    ::fcntl(fds[0], F_SETFD, FD_CLOEXEC);
    // The child's end (fds[1]) intentionally has no CLOEXEC so it is inherited
    // across the exec() call in QProcess.

    m_mainSocketFd   = fds[0];
    m_helperSocketFd = fds[1];
    return true;
}

void PortalHelperProcess::startHelper()
{
    m_process = new HelperQProcess;

    // Determine whether we need to drop root privileges in the child.
    // The helper must run as the session user so that its session-bus
    // connection is authenticated correctly.
    if (::getuid() == 0)
    {
        const auto currentUser = VeyonCore::platform().userFunctions()
            .queryCurrentUserProperty(PlatformUserFunctions::UserProperty::LoginName);

        const struct passwd* pw = ::getpwnam(currentUser.toLocal8Bit().constData());
        if (pw != nullptr)
        {
            m_process->targetUid = pw->pw_uid;
        }
        else
        {
            vWarning() << "PortalHelperProcess: could not resolve UID for user"
                       << currentUser << "– helper will run as root";
        }
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt 6: use setChildProcessModifier for privilege dropping
    const uid_t targetUid = m_process->targetUid;
    m_process->setChildProcessModifier([targetUid]()
    {
        if (targetUid == 0)
        {
            return;
        }
        const struct passwd* pw = ::getpwuid(targetUid);
        if (pw != nullptr)
        {
            ::initgroups(pw->pw_name, pw->pw_gid);
            ::setgid(pw->pw_gid);
        }
        ::setuid(targetUid);
    });
#endif

    // Build the child's environment.  If we are dropping privileges, also
    // set DBUS_SESSION_BUS_ADDRESS to the well-known socket path for that user.
    auto env = QProcessEnvironment::systemEnvironment();
    if (m_process->targetUid > 0)
    {
        env.insert(QStringLiteral("DBUS_SESSION_BUS_ADDRESS"),
                   QStringLiteral("unix:path=/run/user/%1/bus")
                   .arg(m_process->targetUid));
    }
    m_process->setProcessEnvironment(env);

    if (VeyonCore::config().logToSystem())
    {
        m_process->setProcessChannelMode(QProcess::ForwardedChannels);
    }

    connect(m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PortalHelperProcess::onHelperFinished);

    // Monitor the main-process end of the socket pair for incoming messages
    m_notifier = new QSocketNotifier(m_mainSocketFd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated,
            this, &PortalHelperProcess::onSocketReadable);

    const QString helperPath = helperFilePath();
    const QString socketArg  = QStringLiteral("--socket-fd=%1").arg(m_helperSocketFd);

    vDebug() << "Launching portal helper" << helperPath << socketArg;
    m_process->start(helperPath, {socketArg});

    if (!m_process->waitForStarted(5000))
    {
        vCritical() << "Failed to start portal helper:" << m_process->errorString();
        // close() cleans up and the caller will retry
        Q_EMIT failed();
        return;
    }

    // The child's end of the socket pair is now inherited; close our copy so
    // the helper owns the only reference and we can detect when it exits.
    ::close(m_helperSocketFd);
    m_helperSocketFd = -1;

    // Ask the helper to begin the portal session flow
    PortalHelperProtocol::M2HStart msg;
    PortalHelperProtocol::sendMessage(m_mainSocketFd, &msg);
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void PortalHelperProcess::onSocketReadable()
{
    int receivedFd{-1};

    // Read one fixed-size envelope; SOCK_SEQPACKET guarantees one record per call
    uint8_t buf[PortalHelperProtocol::EnvelopeSize]{};

    const ssize_t n = PortalHelperProtocol::recvMessage(
        m_mainSocketFd, buf, &receivedFd);

    if (n <= 0)
    {
        // Peer closed or error – treat as failure
        if (m_sessionRunning)
        {
            m_sessionRunning = false;
            vWarning() << "Portal helper socket closed unexpectedly";
            Q_EMIT failed();
        }
        return;
    }

    switch (static_cast<PortalHelperProtocol::H2MType>(buf[0]))
    {
    case PortalHelperProtocol::H2MType::Started:
    {
        const auto* msg = reinterpret_cast<const PortalHelperProtocol::H2MStarted*>(buf);
        m_pipeWireNodeId = msg->nodeId;

        if (receivedFd < 0)
        {
            vCritical() << "H2MStarted received without PipeWire FD – treating as failure";
            Q_EMIT failed();
            return;
        }

        // dup() the FD so our ownership is independent of the socket lifetime
        m_pipewireFd = ::dup(receivedFd);
        ::close(receivedFd);

        if (m_pipewireFd < 0)
        {
            vCritical() << "dup() of PipeWire FD failed";
            Q_EMIT failed();
            return;
        }

        m_sessionRunning = true;
        vDebug() << "Portal helper reported session started; PipeWire node"
                 << m_pipeWireNodeId;
        Q_EMIT started();
        break;
    }

    case PortalHelperProtocol::H2MType::Failed:
        if (receivedFd >= 0)
        {
            ::close(receivedFd); // Should not happen, but be safe
        }
        vWarning() << "Portal helper reported session failure";
        Q_EMIT failed();
        break;

    default:
        vWarning() << "PortalHelperProcess: unknown H2M message type" << buf[0];
        if (receivedFd >= 0)
        {
            ::close(receivedFd);
        }
        break;
    }
}

void PortalHelperProcess::onHelperFinished(int exitCode, QProcess::ExitStatus status)
{
    vDebug() << "Portal helper exited; code=" << exitCode
             << "status=" << static_cast<int>(status);

    if (m_sessionRunning)
    {
        m_sessionRunning = false;
        Q_EMIT failed();
    }
}
