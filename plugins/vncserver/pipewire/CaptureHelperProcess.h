// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QElapsedTimer>
#include <QProcess>
#include <QTimer>

#include "CaptureHelperCredentials.h"

#include <sys/types.h>

// clazy:excludeall=copyable-polymorphic

// Starts and supervises veyon-capture-helper (02-ipc.md §1a.5) from the non-dumpable Wayland
// veyon-server: a direct child, so it runs as the session user in the same scope/cgroup as the
// server (portal app ID detection, 02 §2a.1). The helper gets the per-start VNC password and the
// name of its Unix domain socket via stdin and nothing else; whenever it exits it is restarted
// with an increasing delay while the server stays up.
class CaptureHelperProcess : public QProcess
{
	Q_OBJECT
public:
	static constexpr int MinimumRestartDelay = 1000;
	static constexpr int MaximumRestartDelay = 30000;
	// a helper that ran at least this long is considered to have started successfully
	static constexpr int StableRunTime = 60000;

	CaptureHelperProcess(const QString& helperFilePath, CaptureHelperCredentials&& credentials,
						 QObject* parent = nullptr);
	~CaptureHelperProcess() override;

	// veyon-capture-helper next to the running executable (veyon-server)
	static QString defaultHelperFilePath();

	// the helper's socket in the private endpoint directory ($XDG_RUNTIME_DIR/veyon), empty if
	// there is none (no runtime directory or running as root)
	static QString socketPath(const QString& socketName);

	// delay before the next start after the given number of consecutive failed starts (>= 1)
	static int restartDelay(int consecutiveFailures);

	// fails (log line) when running as root or without a valid private endpoint directory
	[[nodiscard]] bool startSupervised();
	void stopSupervised();

	int consecutiveFailures() const
	{
		return m_consecutiveFailures;
	}

private:
	void startHelper();
	void removeSocket();
	void removeStaleSockets();
	void sendCredentials();
	void handleExit(bool failedToStart);

	// runs in the forked child before execve(): the helper must not outlive the server, otherwise
	// it would keep the RFB port with a password the restarted server doesn't know
	static void prepareChildProcess(pid_t parentProcessId);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	void setupChildProcess() override
	{
		prepareChildProcess(m_parentProcessId);
	}
#endif

	const QString m_helperFilePath;
	const CaptureHelperCredentials m_credentials;
	const QString m_socketPath;
	const pid_t m_parentProcessId;

	QTimer m_restartTimer;
	QElapsedTimer m_runTime;
	int m_consecutiveFailures = 0;
	bool m_supervised = false;
};
