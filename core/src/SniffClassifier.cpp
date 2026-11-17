// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "SniffClassifier.h"


SniffClassifier::Result SniffClassifier::classify(const QByteArray& firstBytes, qint64 elapsed, qint64 sniffTimeout)
{
	if (firstBytes.size() >= PeekSize)
	{
		if (firstBytes.at(0) == TlsHandshakeRecordType && firstBytes.at(1) == TlsMajorVersion)
		{
			return Result::Tls;
		}
		return Result::Plain;
	}

	// a single byte at the end of the sniff window is not a TLS record header either
	if (elapsed >= sniffTimeout)
	{
		return Result::Plain;
	}

	return Result::NeedMoreData;
}
