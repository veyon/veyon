// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QLocale>

#include "VeyonCore.h"

class VEYON_CORE_EXPORT TranslationLoader
{
public:
	TranslationLoader(const QString& resourceName);

	static bool load(const QString& resourceName);
};
