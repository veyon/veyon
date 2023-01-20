// Copyright (c) 2019-2023 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <QObject>

static inline QString i18n(const char* str)
{
	return QObject::tr(str);
}

template<class T>
static inline QString i18n(const char* str, T arg)
{
	return QObject::tr(str).arg(arg);
}

template<class T>
static inline QString i18np(const char* singular, const char* plural, T arg)
{
	if( arg > 1 )
	{
		return QObject::tr(plural).arg(arg);
	}
	return QObject::tr(singular).arg(arg);
}

