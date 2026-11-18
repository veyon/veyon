// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "SniffClassifier.h"

// First bytes of a connection to the TLS sniffing listener (05-tls.md §1.2, threat model T-NET-09):
// untrusted bytes from any network peer, arriving in chunks at arbitrary times as in
// SniffedConnection::sniff(). The first input byte selects the chunk size and the time step
// between chunks.
extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	if (size < 1)
	{
		return 0;
	}

	static constexpr qint64 SniffTimeout = 200;

	const auto mode = quint8(data[0]);
	const auto chunkSize = qsizetype((mode & 0x0f) + 1);
	const auto timeStep = qint64((mode >> 4) * 20);
	const QByteArray input(data + 1, qsizetype(size - 1));

	QByteArray received;
	qint64 elapsed = 0;
	auto result = SniffClassifier::classify(received, elapsed, SniffTimeout);

	for (qsizetype offset = 0; offset < input.size() && result == SniffClassifier::Result::NeedMoreData; offset += chunkSize)
	{
		received += input.mid(offset, chunkSize);
		elapsed += timeStep;
		result = SniffClassifier::classify(received, elapsed, SniffTimeout);
	}

	// the sniff timer always ends the sniffing
	if (result == SniffClassifier::Result::NeedMoreData)
	{
		result = SniffClassifier::classify(received, SniffTimeout, SniffTimeout);
		if (result == SniffClassifier::Result::NeedMoreData)
		{
			__builtin_trap();
		}
	}

	// TLS only for a TLS handshake record header, decided on the first two bytes only
	const auto isTlsRecord = received.size() >= 2 && quint8(received.at(0)) == 0x16 && quint8(received.at(1)) == 0x03;
	if ((result == SniffClassifier::Result::Tls) != isTlsRecord)
	{
		__builtin_trap();
	}

	// more data or more time never changes a decision taken on two bytes
	if (received.size() >= 2 &&
		(SniffClassifier::classify(received.left(2), 0, SniffTimeout) != result ||
		 SniffClassifier::classify(received + input, SniffTimeout * 2, SniffTimeout) != result))
	{
		__builtin_trap();
	}

	return 0;
}
