// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AuthenticationLimiter.h"

#include <cstring>
#include <utility>


AuthenticationLimiter::Permit::Permit(std::shared_ptr<AuthenticationLimiter> limiter, const QString& callerKey,
									  const QString& username) :
	m_limiter(std::move(limiter)),
	m_callerKey(callerKey),
	m_username(username)
{
}



AuthenticationLimiter::Permit::~Permit()
{
	finishWithoutResult();
}



void AuthenticationLimiter::Permit::finish(bool authenticated)
{
	if (m_finished == false)
	{
		m_finished = true;
		m_limiter->release(m_callerKey, m_username, true, authenticated);
	}
}



void AuthenticationLimiter::Permit::finishWithoutResult()
{
	if (m_finished == false)
	{
		m_finished = true;
		m_limiter->release(m_callerKey, m_username, false, false);
	}
}



AuthenticationLimiter::AuthenticationLimiter(const Clock& clock) :
	m_clock(clock ? clock : Clock(IpcChannel::steadyNow))
{
}



std::shared_ptr<AuthenticationLimiter> AuthenticationLimiter::create(const Clock& clock)
{
	return std::shared_ptr<AuthenticationLimiter>(new AuthenticationLimiter(clock));
}



std::shared_ptr<AuthenticationLimiter::Permit> AuthenticationLimiter::admit(const QString& callerKey,
																			const QString& username, Rejection* rejection)
{
	auto result = Rejection::None;

	{
		QMutexLocker locker(&m_mutex);

		const auto now = m_clock();
		removeIdleCallers(now);

		auto& caller = m_callers[callerKey];
		caller.lastActivity = now;

		if (caller.outstanding)
		{
			result = Rejection::Busy;
		}
		else if (now < caller.blockedUntil)
		{
			result = Rejection::BackingOff;
		}
		else if (m_running >= MaximumConcurrentAuthentications)
		{
			result = Rejection::Overloaded;
		}
		else
		{
			caller.outstanding = true;
			++m_running;
		}
	}

	if (rejection)
	{
		*rejection = result;
	}

	if (result != Rejection::None)
	{
		return {};
	}

	return std::shared_ptr<Permit>(new Permit(shared_from_this(), callerKey, username));
}



bool AuthenticationLimiter::shouldLogRejection(const QString& callerKey, int* suppressedCount)
{
	QMutexLocker locker(&m_mutex);

	const auto now = m_clock();
	auto& caller = m_callers[callerKey];
	caller.lastActivity = now;

	if (now < caller.nextRejectionLog)
	{
		++caller.suppressedRejections;
		return false;
	}

	if (suppressedCount)
	{
		*suppressedCount = caller.suppressedRejections;
	}

	caller.suppressedRejections = 0;
	caller.nextRejectionLog = now + RejectionLogInterval;

	return true;
}



bool AuthenticationLimiter::isValidRequest(const QString& username, const char* password, qsizetype passwordSize)
{
	if (username.isEmpty() || username.toUtf8().size() > MaximumUsernameSize)
	{
		return false;
	}

	for (const auto character : username)
	{
		const auto value = character.unicode();
		if (value < 0x20 || (value >= 0x7f && value <= 0x9f))
		{
			return false;
		}
	}

	// PAM takes the password as a C string, so a NUL byte would silently truncate it
	return password != nullptr &&
		passwordSize > 0 &&
		passwordSize <= MaximumPasswordSize &&
		std::memchr(password, 0, size_t(passwordSize)) == nullptr;
}



QLatin1String AuthenticationLimiter::rejectionName(Rejection rejection)
{
	switch (rejection)
	{
	case Rejection::None: return QLatin1String("none");
	case Rejection::Busy: return QLatin1String("busy");
	case Rejection::BackingOff: return QLatin1String("backing off");
	case Rejection::Overloaded: return QLatin1String("overloaded");
	}

	return QLatin1String("unknown");
}



int AuthenticationLimiter::runningCount() const
{
	QMutexLocker locker(&m_mutex);
	return m_running;
}



int AuthenticationLimiter::trackedCallerCount() const
{
	QMutexLocker locker(&m_mutex);
	return int(m_callers.size());
}



void AuthenticationLimiter::release(const QString& callerKey, const QString& username, bool hasResult,
									bool authenticated)
{
	QMutexLocker locker(&m_mutex);

	const auto now = m_clock();

	--m_running;

	auto& caller = m_callers[callerKey];
	caller.outstanding = false;
	caller.lastActivity = now;

	if (hasResult == false)
	{
		return;
	}

	if (authenticated)
	{
		// only a success for the account whose check failed resets (a typo, then the right
		// password); a success for another account must not reset the backoff of guesses
		if (caller.failures == 0 || (caller.mixedUsernames == false && username == caller.failedUsername))
		{
			caller.failures = 0;
			caller.failedUsername.clear();
			caller.mixedUsernames = false;
			caller.blockedUntil = 0;
		}
		return;
	}

	if (caller.failures == 0)
	{
		caller.failedUsername = username;
		caller.mixedUsernames = false;
	}
	else if (username != caller.failedUsername)
	{
		caller.mixedUsernames = true;
	}

	// 1 s, 2 s, 4 s, … capped; the exponent is bounded before shifting
	caller.failures = qMin(caller.failures + 1, 31);
	qint64 backoff = InitialFailureBackoff;
	for (int i = 1; i < caller.failures && backoff < MaximumFailureBackoff; ++i)
	{
		backoff *= 2;
	}

	caller.blockedUntil = now + qMin<qint64>(backoff, MaximumFailureBackoff);
}



void AuthenticationLimiter::removeIdleCallers(qint64 now)
{
	for (auto it = m_callers.begin(); it != m_callers.end();)
	{
		if (it->second.outstanding == false && now - it->second.lastActivity >= FailureResetInterval)
		{
			it = m_callers.erase(it);
		}
		else
		{
			++it;
		}
	}
}
