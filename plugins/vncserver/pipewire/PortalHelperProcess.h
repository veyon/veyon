/*
 * PortalHelperProcess.h - Main-process proxy for the veyon-portal-helper subprocess
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

#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

#include <sys/types.h>

class QSocketNotifier;

/**
 * @brief Manages the veyon-portal-helper subprocess and exposes the same
 *        public interface as PortalSession.
 *
 * Architecture overview
 * ---------------------
 * All D-Bus interactions with the XDG Desktop Portal (RemoteDesktop /
 * ScreenCast) are delegated to a separate child process (`veyon-portal-helper`)
 * that runs under the session user's credentials.  This avoids the need for the
 * main veyon-server process to impersonate the user with seteuid(2) just to
 * reach the user's session bus.
 *
 * IPC mechanism
 * -------------
 * A Unix socketpair(AF_UNIX, SOCK_STREAM) is created before fork/exec.
 *   - The parent (main process) holds fds[0].
 *   - The child  (helper)       holds fds[1], passed as "--socket-fd=<N>".
 *
 * Messages are fixed-size packed structs defined in PortalHelperProtocol.h.
 * The PipeWire remote FD is returned from the helper via SCM_RIGHTS ancillary
 * data alongside the H2MStarted payload so that no open() / dup() race can
 * occur between the two processes.
 *
 * Privilege handling
 * ------------------
 * If veyon-server is running as root the helper drops to the session user's
 * UID/GID before exec-ing so that DBUS_SESSION_BUS_ADDRESS resolves correctly
 * for the target user.  The session bus socket path is set explicitly to
 * "unix:path=/run/user/<uid>/bus" in the child's environment.
 * When the server is already running as an unprivileged user (the normal case
 * after LinuxServerProcess::setProcessUserId()) no credential change is needed.
 *
 * Lifecycle
 * ---------
 * 1. Call start()       – spawns the helper and asks it to begin the portal flow.
 * 2. Wait for started() – the helper emits this once OpenPipeWireRemote succeeds
 *                         and the PipeWire FD + node ID are available.
 * 3. Call notifyKeyboardKeysym() / notifyPointer() to forward input events.
 * 4. Call close()       – sends M2HClose and waits for the helper to exit.
 *
 * On any error the helper sends H2MFailed and the failed() signal is emitted.
 */
class PortalHelperProcess : public QObject
{
    Q_OBJECT

public:
    explicit PortalHelperProcess(QObject* parent = nullptr);
    ~PortalHelperProcess() override;

    /**
     * @brief Launch the helper and start the XDG portal flow.
     * Emits started() on success or failed() on error.
     */
    void start();

    /**
     * @brief Ask the helper to close the portal session and terminate.
     */
    void close();

    /**
     * @brief PipeWire remote file descriptor (valid after started() is emitted).
     * The caller owns this FD and must close(2) it when done.
     */
    int pipewireFd() const { return m_pipewireFd; }

    /**
     * @brief PipeWire node ID for the screen stream (valid after started()).
     */
    quint32 pipeWireNodeId() const { return m_pipeWireNodeId; }

    /**
     * @brief Forward a keyboard keysym event to the portal helper.
     * No-op when the helper is not running or the session has not started.
     */
    void notifyKeyboardKeysym(quint32 keySym, bool down);

    /**
     * @brief Forward a pointer event to the portal helper.
     * No-op when the helper is not running or the session has not started.
     */
    void notifyPointer(int buttonMask, int x, int y);

Q_SIGNALS:
    void started();
    void failed();

private Q_SLOTS:
    void onSocketReadable();
    void onHelperFinished(int exitCode, QProcess::ExitStatus status);

private:
    /**
     * @brief QProcess subclass that drops root privileges in the child process
     *        when veyon-server is running as root.
     *
     * On Qt 5 the privilege drop is done by overriding setupChildProcess().
     * On Qt 6 setChildProcessModifier() lambda is used instead.
     */
    class HelperQProcess : public QProcess
    {
    public:
        uid_t targetUid{0}; ///< UID to switch to in the child (0 = no change)

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    protected:
        void setupChildProcess() override;
#endif
    };

    static QString helperFilePath();

    bool createSocketPair();
    void startHelper();

    HelperQProcess*  m_process{nullptr};
    QSocketNotifier* m_notifier{nullptr};

    int      m_mainSocketFd{-1}; ///< Parent-side of the socket pair
    int      m_helperSocketFd{-1}; ///< Child-side (closed in parent after fork)
    int      m_pipewireFd{-1};
    quint32  m_pipeWireNodeId{0};
    bool     m_sessionRunning{false};
};
