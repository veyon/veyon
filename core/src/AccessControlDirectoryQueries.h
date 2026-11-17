// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QHash>
#include <QStringList>

#include "IpcChannel.h"

#include <functional>
#include <optional>

class NetworkObjectDirectory;
class UserGroupsBackendInterface;

// Directory queries of access control decisions (04-directory-broker.md §2, fail closed):
// - a failed query is recorded (hasFailed()), the caller has to deny access,
// - results are memoized between beginPass() and endPass() (one access check or one re-check pass
//   of all connections), failures included,
// - after a failed query further queries fail immediately for FailureBackoff (circuit breaker),
//   so that a directory outage does not cost the request timeout for every query.
class VEYON_CORE_EXPORT AccessControlDirectoryQueries
{
public:
	using Clock = std::function<qint64()>;

	static constexpr int FailureBackoff = 3000;

	AccessControlDirectoryQueries(UserGroupsBackendInterface* userGroupsBackend, NetworkObjectDirectory* directory,
								  bool useDomainUserGroups, const Clock& clock = IpcChannel::steadyNow);

	void beginPass();
	void endPass();

	void resetFailure()
	{
		m_failed = false;
	}

	bool hasFailed() const
	{
		return m_failed;
	}

	QStringList groupsOfUser(const QString& user);
	// fullyQualifiedDomainName: of the computer (never resolved here)
	QStringList locationsOfComputer(const QString& fullyQualifiedDomainName);

private:
	std::optional<QStringList> queryGroupsOfUser(const QString& user);
	std::optional<QStringList> queryLocationsOfComputer(const QString& fullyQualifiedDomainName);
	QStringList result(const std::optional<QStringList>& result);
	void recordFailure();
	bool isBackingOff() const;

	UserGroupsBackendInterface* m_userGroupsBackend;
	NetworkObjectDirectory* m_directory;
	const bool m_useDomainUserGroups;
	const Clock m_clock;

	int m_passDepth = 0;
	QHash<QString, std::optional<QStringList>> m_groupsOfUser;
	QHash<QString, std::optional<QStringList>> m_locationsOfComputer;

	bool m_failed = false;
	qint64 m_lastFailure = 0;
	bool m_hasFailedBefore = false;
};
