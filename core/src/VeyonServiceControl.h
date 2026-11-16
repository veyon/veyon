// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ServiceControl.h"

// clazy:excludeall=ctor-missing-parent-argument

class VEYON_CORE_EXPORT VeyonServiceControl : public ServiceControl
{
	Q_OBJECT
public:
	explicit VeyonServiceControl(QWidget* parent = nullptr);

	bool setAutostart(bool enabled);

	static QString name();
	static QString filePath();
	static QString displayName();
};
