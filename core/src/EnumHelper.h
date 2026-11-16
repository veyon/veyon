// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QMetaEnum>

// clazy:excludeall=rule-of-three

class EnumHelper
{
public:
	template<class T>
	static QString toString(T item)
	{
		return QLatin1String(QMetaEnum::fromType<T>().valueToKey(int(item)));
	}

	template<class T>
	static QString toCamelCaseString(T item)
	{
		const auto name = toString(item);
		return name.mid(0, 1).toLower() + name.mid(1);
	}
};
