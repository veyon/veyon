// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QMutex>
#include <QString>

#include "IpcChannel.h"

#include <map>
#include <memory>

// Limits the password checks (PAM conversations) the service performs for the session servers
// (IPC command Authenticate, docs/specs/02-ipc.md §6b): per server session at most one
// outstanding request and an exponential backoff after failed checks, plus a global cap of
// admitted checks. A successful check only resets failures of the same user name, so a caller
// cannot reset its backoff with an account it knows. Requests are rejected, never queued.
// Thread-safe (the IPC thread of ServiceDataManager runs the checks, permits may be released
// elsewhere).
// clazy:excludeall=rule-of-three
class AuthenticationLimiter : public std::enable_shared_from_this<AuthenticationLimiter>
{
public:
	using Clock = IpcChannel::Clock;

	// timeout of one call of veyon-auth-helper (one PAM conversation)
	static constexpr int AuthenticateTimeout = 10000;
	// backoff after the first failed check of a caller, doubled with each further failure
	static constexpr int InitialFailureBackoff = 1000;
	static constexpr int MaximumFailureBackoff = 30000;
	// a caller's failure count is forgotten after this long without request; this is the only
	// way failures for more than one user name decay
	static constexpr int FailureResetInterval = 10 * 60 * 1000;
	// checks admitted at the same time (veyon-auth-helper works them off one after the other)
	static constexpr int MaximumConcurrentAuthentications = 4;
	// rejections of a caller are logged at most once per interval
	static constexpr int RejectionLogInterval = 30000;
	// UTF-8 bytes; requests outside these limits are rejected before any check
	static constexpr qsizetype MaximumUsernameSize = 256;
	static constexpr qsizetype MaximumPasswordSize = 1024;

	enum class Rejection {
		None,
		Busy,		// the caller has an outstanding request
		BackingOff, // the caller's last check failed less than its backoff ago
		Overloaded, // MaximumConcurrentAuthentications checks are running
	};

	// Admission of one check of username. The first of finish(), finishWithoutResult() or the
	// destruction (e.g. when the reply can't be delivered any more) releases it; only finish()
	// updates the caller's backoff.
	class Permit
	{
	public:
		~Permit();

		Permit(const Permit&) = delete;
		Permit& operator=(const Permit&) = delete;

		void finish(bool authenticated);

		// the check gave no verdict (e.g. helper error or timeout): neither failure nor success
		void finishWithoutResult();

	private:
		friend class AuthenticationLimiter;

		Permit(std::shared_ptr<AuthenticationLimiter> limiter, const QString& callerKey, const QString& username);

		std::shared_ptr<AuthenticationLimiter> m_limiter;
		QString m_callerKey;
		QString m_username;
		bool m_finished = false;
	};

	// permits keep the limiter alive, so it is always owned by a shared pointer
	[[nodiscard]] static std::shared_ptr<AuthenticationLimiter> create(const Clock& clock = IpcChannel::steadyNow);

	// check of username for the caller; returns nullptr and sets rejection if the request has to
	// be rejected
	[[nodiscard]] std::shared_ptr<Permit> admit(const QString& callerKey, const QString& username,
												Rejection* rejection = nullptr);

	// true if a rejection of the caller is to be logged now; suppressedCount: rejections of this
	// caller that were not logged since the last logged one
	[[nodiscard]] bool shouldLogRejection(const QString& callerKey, int* suppressedCount = nullptr);

	// non-empty username without control characters and a non-empty password without NUL bytes,
	// both within the size limits
	[[nodiscard]] static bool isValidRequest(const QString& username, const char* password, qsizetype passwordSize);

	static QLatin1String rejectionName(Rejection rejection);

	int runningCount() const;
	int trackedCallerCount() const;

private:
	explicit AuthenticationLimiter(const Clock& clock);

	struct Caller
	{
		bool outstanding = false;
		int failures = 0;
		// user name of the first of the current failures, and whether others failed since
		QString failedUsername{};
		bool mixedUsernames = false;
		qint64 blockedUntil = 0;
		qint64 lastActivity = 0;
		qint64 nextRejectionLog = 0;
		int suppressedRejections = 0;
	};

	void release(const QString& callerKey, const QString& username, bool hasResult, bool authenticated);
	void removeIdleCallers(qint64 now);

	const Clock m_clock;
	mutable QMutex m_mutex;
	std::map<QString, Caller> m_callers;
	int m_running = 0;
};
