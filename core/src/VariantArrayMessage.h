// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QBuffer>
#include <QVariant>

#include "VariantStream.h"

// clazy:excludeall=rule-of-three

class VEYON_CORE_EXPORT VariantArrayMessage
{
public:
	using MessageSize = quint32;

	static constexpr MessageSize MaxMessageSize = 32 * 1024 * 1024;
	// cap for every message of the authentication phase on both sides: the peer is not
	// authenticated yet, and no legitimate message comes close
	static constexpr MessageSize MaxAuthenticationMessageSize = 64 * 1024;

	explicit VariantArrayMessage(QIODevice* ioDevice, MessageSize maximumMessageSize = MaxMessageSize);

	bool send();

	bool isReadyForReceive();

	bool receive();

	QVariant read(); // Flawfinder: ignore

	VariantArrayMessage& write(const QVariant& v);

	QIODevice* ioDevice() const
	{
		return m_ioDevice;
	}

private:
	QBuffer m_buffer;
	VariantStream m_stream;
	QIODevice* m_ioDevice;
	MessageSize m_maximumMessageSize;
};
