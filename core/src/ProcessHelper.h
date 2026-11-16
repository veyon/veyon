// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QProcess>

#include "VeyonCore.h"

class VEYON_CORE_EXPORT ProcessHelper
{
public:
	ProcessHelper(const QString& program, const QStringList& arguments);

	int run();
	QByteArray runAndReadAll();

private:
	QProcess m_process;
};
