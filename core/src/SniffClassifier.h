// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QByteArray>

#include "VeyonCore.h"

// Decides from the first bytes a client sent whether it speaks TLS or plain RFB (05-tls.md §1.2).
// RFB servers speak first and TLS clients speak first: a plain (4.11) client sends nothing until
// it got the server's banner, so silence until the sniff timeout means plain. Only the first
// PeekSize bytes are inspected: a TLS record header starts with the content type handshake (0x16)
// and the major version 3 of the legacy record version (0x03xx), everything else is plain.
class VEYON_CORE_EXPORT SniffClassifier
{
public:
	enum class Result {
		NeedMoreData,
		Tls,
		Plain,
	};

	static constexpr int PeekSize = 2;
	static constexpr char TlsHandshakeRecordType = 0x16;
	static constexpr char TlsMajorVersion = 0x03;

	// firstBytes: the bytes received so far (only the first PeekSize bytes are used), elapsed: time
	// since the connection was accepted
	[[nodiscard]] static Result classify(const QByteArray& firstBytes, qint64 elapsed, qint64 sniffTimeout);
};
