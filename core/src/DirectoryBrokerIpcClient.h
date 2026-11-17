// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>

#include "DirectoryBrokerClient.h"

#include <atomic>

// Directory broker client of the session server: sends the broker commands over the authenticated
// local IPC connection to veyon-service (02-ipc.md §6). The IPC client is not thread-safe, so
// requests are only accepted in its thread (the main thread); other threads get "unavailable".
class VEYON_CORE_EXPORT DirectoryBrokerIpcClient : public QObject, public DirectoryBrokerClient
{
	Q_OBJECT
public:
	explicit DirectoryBrokerIpcClient(QObject* parent = nullptr);

	Reply request(const QString& command, const QVariantMap& arguments) override;
	void requestAsync(const QString& command, const QVariantMap& arguments, const ReplyHandler& handler) override;

private:
	static constexpr int RateLimitRetryDelay = 100;

	bool isUsable() const;

	std::atomic<quint32> m_nextRequestId{1};
};
