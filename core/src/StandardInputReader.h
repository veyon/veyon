// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VeyonCore.h"

class VEYON_CORE_EXPORT StandardInputReader : public QObject
{
	Q_OBJECT
public:
	StandardInputReader(QObject* parent) :
		QObject(parent) {}

Q_SIGNALS:
	void lineRead(const QString& line);
};
