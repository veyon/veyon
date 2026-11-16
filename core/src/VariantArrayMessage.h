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

	explicit VariantArrayMessage(QIODevice* ioDevice);

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
	enum {
		MaxMessageSize = 1024 * 1024 * 32
	};

	QBuffer m_buffer;
	VariantStream m_stream;
	QIODevice* m_ioDevice;
};
