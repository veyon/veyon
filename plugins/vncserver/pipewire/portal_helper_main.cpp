/*
 * portal_helper_main.cpp - Entry point for the veyon-portal-helper executable
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

/**
 * @file portal_helper_main.cpp
 * @brief veyon-portal-helper — out-of-process D-Bus portal helper for PipeWire VNC.
 *
 * This small executable is spawned by the main veyon-server process whenever the
 * PipeWire VNC server plugin needs a new XDG Desktop Portal screen-sharing session.
 * It runs under the session user's credentials so that it can access the user's
 * session D-Bus, which is required for the RemoteDesktop / ScreenCast portal APIs.
 *
 * Communication with the parent process
 * ======================================
 * The parent creates a Unix socketpair(2) and passes the child's FD number as the
 * command-line argument "--socket-fd=<N>".  After the helper is exec'd, both sides
 * use the binary protocol defined in PortalHelperProtocol.h:
 *
 *   Parent → Helper  (M2H):
 *     M2HStart         – begin the portal session flow
 *     M2HClose         – close the session and exit
 *     M2HNotifyKey     – forward a keyboard keysym event
 *     M2HNotifyPointer – forward a pointer (mouse) event
 *
 *   Helper → Parent  (H2M):
 *     H2MStarted + SCM_RIGHTS(pipewireFd) – session ready; FD transferred
 *     H2MFailed                            – session failed
 *
 * Exit codes
 * ==========
 *   0 – normal shutdown after receiving M2HClose
 *   1 – bad arguments or socket setup error
 */

#include "PortalHelperProtocol.h"
#include "PortalSession.h"
#include "VeyonCore.h"

#include <QCoreApplication>
#include <QSocketNotifier>

#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

// ---------------------------------------------------------------------------
// PortalHelperRunner — wires PortalSession to the IPC socket
// ---------------------------------------------------------------------------

class PortalHelperRunner : public QObject
{
    Q_OBJECT

public:
    PortalHelperRunner(int socketFd, QObject* parent = nullptr)
        : QObject(parent)
        , m_socketFd(socketFd)
        , m_session(new PortalSession(this))
    {
        // Respond to portal session outcomes
        connect(m_session, &PortalSession::started,
                this, &PortalHelperRunner::onSessionStarted);
        connect(m_session, &PortalSession::failed,
                this, &PortalHelperRunner::onSessionFailed);

        // Listen for commands from the parent process
        m_notifier = new QSocketNotifier(m_socketFd, QSocketNotifier::Read, this);
        connect(m_notifier, &QSocketNotifier::activated,
                this, &PortalHelperRunner::onCommandReceived);
    }

private Q_SLOTS:
    void onSessionStarted()
    {
        const int     fd     = m_session->pipewireFd();
        const quint32 nodeId = m_session->pipeWireNodeId();

        vDebug() << "Portal session started; node ID" << nodeId
                 << "; sending FD to parent";

        PortalHelperProtocol::H2MStarted msg;
        msg.nodeId = nodeId;

        if (!PortalHelperProtocol::sendMessage(m_socketFd, &msg, fd))
        {
            vCritical() << "Failed to send H2MStarted to parent";
            QCoreApplication::exit(1);
        }
    }

    void onSessionFailed()
    {
        vWarning() << "Portal session failed; notifying parent";

        PortalHelperProtocol::H2MFailed msg;
        PortalHelperProtocol::sendMessage(m_socketFd, &msg);
    }

    void onCommandReceived()
    {
        // Read one fixed-size envelope; SOCK_SEQPACKET gives us one record per call
        uint8_t buf[PortalHelperProtocol::EnvelopeSize]{};

        const ssize_t n = PortalHelperProtocol::recvMessage(m_socketFd, buf);

        if (n <= 0)
        {
            // Parent closed the socket – exit cleanly
            vDebug() << "Parent socket closed; exiting";
            m_session->close();
            QCoreApplication::exit(0);
            return;
        }

        const auto type = static_cast<PortalHelperProtocol::M2HType>(buf[0]);

        switch (type)
        {
        case PortalHelperProtocol::M2HType::Start:
            vDebug() << "Received M2HStart – beginning portal session";
            m_session->start();
            break;

        case PortalHelperProtocol::M2HType::Close:
            vDebug() << "Received M2HClose – shutting down";
            m_session->close();
            QCoreApplication::exit(0);
            break;

        case PortalHelperProtocol::M2HType::NotifyKey:
        {
            const auto* msg =
                reinterpret_cast<const PortalHelperProtocol::M2HNotifyKey*>(buf);
            m_session->notifyKeyboardKeysym(msg->keySym, msg->down != 0);
            break;
        }

        case PortalHelperProtocol::M2HType::NotifyPointer:
        {
            const auto* msg =
                reinterpret_cast<const PortalHelperProtocol::M2HNotifyPointer*>(buf);
            m_session->notifyPointer(msg->buttonMask, msg->x, msg->y);
            break;
        }

        default:
            vWarning() << "PortalHelperRunner: unknown M2H message type"
                       << static_cast<int>(buf[0]);
            break;
        }
    }

private:
    int              m_socketFd;
    PortalSession*   m_session;
    QSocketNotifier* m_notifier{nullptr};
};

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // Parse --socket-fd=<N> from the command line
    int socketFd = -1;
    for (int i = 1; i < argc; ++i)
    {
        const QByteArray arg(argv[i]);
        if (arg.startsWith("--socket-fd="))
        {
            bool ok = false;
            socketFd = arg.mid(12).toInt(&ok);
            if (!ok)
            {
                socketFd = -1;
            }
        }
    }

    if (socketFd < 0)
    {
        ::fprintf(stderr,
                  "veyon-portal-helper: missing or invalid --socket-fd argument\n");
        return 1;
    }

    // Make the socket non-inheritable from here onwards (we own it explicitly)
    ::fcntl(socketFd, F_SETFD, FD_CLOEXEC);

    VeyonCore::setupApplicationParameters();

    // Use QCoreApplication – the portal D-Bus calls do not require a display
    // connection, and the application identifier is now a compile-time constant
    // in PortalSession (see AppId in PortalSession.cpp).
    QCoreApplication app(argc, argv);

    VeyonCore core(&app, VeyonCore::Component::Server,
                   QStringLiteral("PortalHelper"));

    vDebug() << "veyon-portal-helper started; socket FD =" << socketFd;

    PortalHelperRunner runner(socketFd);

    return app.exec();
}

#include "portal_helper_main.moc"
