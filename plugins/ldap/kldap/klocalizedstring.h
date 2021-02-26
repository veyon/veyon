/*
 * klocalizedstring.h - dummy replacements for i18n() functions of KDE
 *
 * Copyright (c) 2016-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

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

