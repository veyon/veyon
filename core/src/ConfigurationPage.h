// SPDX-FileCopyrightText: 2016-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QWidget>

#include "VeyonCore.h"

class VEYON_CORE_EXPORT ConfigurationPage : public QWidget
{
	Q_OBJECT
public:
	explicit ConfigurationPage(QWidget* parent = nullptr);
	~ConfigurationPage() override = default;

	virtual void resetWidgets() = 0;
	virtual void connectWidgetsToProperties() = 0;
	virtual void applyConfiguration() = 0;

Q_SIGNALS:
	void widgetsChanged();
};
