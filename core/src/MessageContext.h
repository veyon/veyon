// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QPointer>

#include "VeyonCore.h"

class QIODevice;

class VEYON_CORE_EXPORT MessageContext
{
public:
	using IODevice = QPointer<QIODevice>;
	using Connection = QPointer<QObject>;

	explicit MessageContext(QIODevice* ioDevice = nullptr, QObject* connection = nullptr) :
		m_ioDevice(ioDevice),
		m_connection(connection)
	{
	}

	~MessageContext() = default;

	QIODevice* ioDevice() const
	{
		return m_ioDevice;
	}

	QObject* connection() const
	{
		return m_connection;
	}

private:
	IODevice m_ioDevice;
	Connection m_connection;
};
