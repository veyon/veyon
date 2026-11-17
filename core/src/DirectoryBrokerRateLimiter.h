// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QtGlobal>

#include <deque>

// Request rate and outstanding request limit of one broker connection
// (docs/specs/04-directory-broker.md §6): at most maximumRequestsPerSecond requests within any
// window of one second and at most maximumOutstandingRequests requests waiting for their reply.
class DirectoryBrokerRateLimiter
{
public:
	DirectoryBrokerRateLimiter(int maximumRequestsPerSecond, int maximumOutstandingRequests) :
		m_maximumRequestsPerSecond(maximumRequestsPerSecond),
		m_maximumOutstandingRequests(maximumOutstandingRequests)
	{
	}

	// rejected requests do not count against the rate
	[[nodiscard]] bool acceptRequest(qint64 now)
	{
		if (wouldAcceptRequest(now) == false)
		{
			return false;
		}

		m_requestTimes.push_back(now);

		return true;
	}

	// like acceptRequest(), but without counting the request, e.g. to check several limiters
	// before counting the request in all of them
	[[nodiscard]] bool wouldAcceptRequest(qint64 now)
	{
		while (m_requestTimes.empty() == false && now - m_requestTimes.front() >= WindowSize)
		{
			m_requestTimes.pop_front();
		}

		return int(m_requestTimes.size()) < m_maximumRequestsPerSecond;
	}

	[[nodiscard]] bool acquireOutstandingRequest()
	{
		if (m_outstandingRequests >= m_maximumOutstandingRequests)
		{
			return false;
		}

		++m_outstandingRequests;

		return true;
	}

	void releaseOutstandingRequest()
	{
		if (m_outstandingRequests > 0)
		{
			--m_outstandingRequests;
		}
	}

	int outstandingRequests() const
	{
		return m_outstandingRequests;
	}

private:
	static constexpr qint64 WindowSize = 1000;

	int m_maximumRequestsPerSecond;
	int m_maximumOutstandingRequests;
	std::deque<qint64> m_requestTimes;
	int m_outstandingRequests = 0;
};
