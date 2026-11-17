// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QHash>

#include "IpcChannel.h"

#include <functional>

// Grace period of established, already authorised connections while the directory needed by access
// control is not available (04-directory-broker.md §2): a re-check that fails for this reason keeps
// the previous decision, the connection is checked again every RecheckInterval and closed after
// GracePeriod without a successful check. New connections are not affected (always fail closed).
class VEYON_CORE_EXPORT AccessControlGracePeriod
{
public:
	using Clock = std::function<qint64()>;
	using ClientId = const void*;

	static constexpr int RecheckInterval = 15 * 1000;
	static constexpr int GracePeriod = 120 * 1000;

	explicit AccessControlGracePeriod(const Clock& clock = IpcChannel::steadyNow);

	// a re-check of the client failed because the directory is not available: true while the
	// previous decision may be kept, false once the grace period is over
	[[nodiscard]] bool keepAccess(ClientId client, const QString& description);

	// a re-check succeeded or failed for another reason, or the client disconnected
	void remove(ClientId client, const QString& description = {});

	bool contains(ClientId client) const
	{
		return m_graceStart.contains(client);
	}

	bool isEmpty() const
	{
		return m_graceStart.isEmpty();
	}

	QList<ClientId> clients() const
	{
		return m_graceStart.keys();
	}

private:
	const Clock m_clock;
	QHash<ClientId, qint64> m_graceStart;
};
