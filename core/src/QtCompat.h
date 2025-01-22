/*
 * QtCompat.h - functions and templates for compatibility with older Qt versions
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
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

#include <QtGlobal>
#include <QSet>
#include <QVariant>
#include <QVector>

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
#error Qt < 5.7 not supported
#endif

template<class T>
static inline QSet<T> qsetFromList(const QList<T>& list)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	return QSet<T>(list.constBegin(), list.constEnd());
#else
	return list.toSet();
#endif
}


template<class T>
static inline QList<T> qlistFromSet(const QSet<T>& set)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	return QList<T>(set.constBegin(), set.constEnd());
#else
	return set.toList();
#endif
}


template <typename T, typename Predicate>
static inline void qsetRemoveIf(QSet<T>& set, Predicate pred)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 1, 0)
	set.removeIf(pred);
#else
	auto it = set.begin();
	const auto e = set.end();
	while (it != e)
	{
		if (pred(*it))
		{
			it = set.erase(it);
		}
		else
		{
			++it;
		}
	}
#endif
}


#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#define Q_DISABLE_MOVE(Class) \
	Class(const Class &&) Q_DECL_EQ_DELETE;\
	Class &operator=(const Class &&) Q_DECL_EQ_DELETE;
#endif
