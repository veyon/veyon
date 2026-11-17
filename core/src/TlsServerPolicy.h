// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QList>
#include <QSslCertificate>
#include <QSslKey>

#include "VeyonCore.h"

class VeyonConfiguration;

// Configuration of a TlsSniffingServer (05-tls.md §1): the TLS/ServerMode and the server's
// certificate chain (leaf first) and private key. Without credentials (or without a usable TLS
// backend) Optional serves plain connections only and Required refuses every connection.
struct VEYON_CORE_EXPORT TlsServerPolicy
{
	static constexpr int DefaultSniffTimeout = 200;
	static constexpr int MinimumSniffTimeout = 50;
	static constexpr int MaximumSniffTimeout = 2000;
	static constexpr int DefaultHandshakeTimeout = 10000;

	// fail closed: a default-constructed policy refuses every connection
	VeyonCore::TlsServerMode mode = VeyonCore::TlsServerMode::Required;
	QList<QSslCertificate> certificateChain{};
	QSslKey privateKey{};
	int sniffTimeoutMs = DefaultSniffTimeout;
	int handshakeTimeoutMs = DefaultHandshakeTimeout;

	bool hasCredentials() const
	{
		return certificateChain.isEmpty() == false && certificateChain.first().isNull() == false &&
			privateKey.isNull() == false;
	}

	// TLS/ServerMode (fail closed for invalid values) and TLS/SniffTimeoutMs (clamped), no credentials
	[[nodiscard]] static TlsServerPolicy fromConfiguration(const VeyonConfiguration& configuration);

	[[nodiscard]] static int clampedSniffTimeout(int sniffTimeout);
};
