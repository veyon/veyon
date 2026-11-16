// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QByteArray>
#include <QHash>
#include <QHostAddress>
#include <QMutex>

#include "VeyonCore.h"

#include <functional>
#include <list>

class VeyonConfiguration;

// Per-source-address limiter for pre-authentication connections (token bucket,
// exponential backoff after failed authentications, global cap of concurrent
// unauthenticated connections). One instance per listener.
//
// Contract: every admit() call returning true must be followed by exactly one
// call of either authenticated() or closed() for the same address.
// clazy:excludeall=rule-of-three
class VEYON_CORE_EXPORT ConnectionLimiter
{
public:
	struct Config
	{
		int preAuthMax = 32;
		int perIpBurst = 10;
		int perIpRefillSeconds = 6;
		int backoffMaxSeconds = 300;
	};

	using Clock = std::function<qint64()>;
	using OnLinkResolver = std::function<bool(const QHostAddress&)>;

	static constexpr int MaxTrackedAddresses = 4096;
	static constexpr int TrustedPeerSeconds = 1800;

	// nowMs: monotonic clock in milliseconds; isOnLink: decides whether a non-link-local IPv6 address
	// lies inside a prefix of a local interface (default: query QNetworkInterface)
	explicit ConnectionLimiter(const Config& config,
							   const Clock& nowMs = steadyNow,
							   const OnLinkResolver& isOnLink = isOnLinkOfLocalInterface);

	// builds the Config from Network/ConnectionLimit/*
	static Config configFromConfiguration(const VeyonConfiguration& configuration);

	static qint64 steadyNow();
	static bool isOnLinkOfLocalInterface(const QHostAddress& address);

	// consumes a token and a pre-auth slot; returns false (nothing consumed) if the connection must be refused
	[[nodiscard]] bool admit(const QHostAddress& address);

	// frees the slot, resets failures and reserves a budget for this address (TrustedPeerSeconds)
	void authenticated(const QHostAddress& address);

	// frees the slot; on authentication failure doubles the backoff (base 1 s, capped by Config::backoffMaxSeconds)
	void closed(const QHostAddress& address, bool authFailed);

	int preAuthCount() const;
	int trackedAddressCount() const;

private:
	struct Bucket
	{
		int tokens = 0;
		qint64 lastRefillMs = 0;
	};

	struct Entry
	{
		QByteArray key;
		Bucket bucket;
		Bucket trustedBucket;
		qint64 trustedUntilMs = 0;
		qint64 blockedUntilMs = 0;
		int failures = 0;
	};

	using EntryList = std::list<Entry>;

	static QByteArray keyFor(const QHostAddress& address, const OnLinkResolver& isOnLink);
	static bool isLoopback(const QHostAddress& address);

	Entry& entryFor(const QByteArray& key, qint64 now);
	bool takeToken(Bucket& bucket, qint64 now) const;
	bool isTrusted(Entry& entry, qint64 now) const;
	void releaseSlot();

	const Config m_config;
	const Clock m_nowMs;
	const OnLinkResolver m_isOnLink;

	mutable QMutex m_mutex;
	EntryList m_entries;
	QHash<QByteArray, EntryList::iterator> m_index;
	int m_preAuthCount = 0;

	Q_DISABLE_COPY(ConnectionLimiter)
};
