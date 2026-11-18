// SPDX-FileCopyrightText: 2011-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <QObject>
#include "VeyonCore.h"
class QWidget;
class VEYON_CORE_EXPORT AboutDialog : public QObject
{
	Q_OBJECT
public:
	explicit AboutDialog(QWidget* parent);
	int exec();
};
