/*
 * QtCompat.h - functions and templates for compatibility with older Qt versions
 *
 * Copyright (c) 2017-2020 Tobias Junghans <tobydox@veyon.io>
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

#if QT_VERSION < QT_VERSION_CHECK(5, 6, 0)
#error Qt < 5.6 not supported
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#define Q_DISABLE_MOVE(Class) \
	Class(const Class &&) Q_DECL_EQ_DELETE;\
	Class &operator=(const Class &&) Q_DECL_EQ_DELETE;
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
template <typename T> struct QAddConst { using Type = const T; };
template <typename T> constexpr typename QAddConst<T>::Type &qAsConst(T &t) { return t; }
template <typename T> void qAsConst(const T &&) = delete;

template <typename... Args>
struct QNonConstOverload
{
	template <typename R, typename T>
	Q_DECL_CONSTEXPR auto operator()(R (T::*ptr)(Args...)) const Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }

	template <typename R, typename T>
	static Q_DECL_CONSTEXPR auto of(R (T::*ptr)(Args...)) Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }
};

template <typename... Args>
struct QConstOverload
{
	template <typename R, typename T>
	Q_DECL_CONSTEXPR auto operator()(R (T::*ptr)(Args...) const) const Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }

	template <typename R, typename T>
	static Q_DECL_CONSTEXPR auto of(R (T::*ptr)(Args...) const) Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }
};

template <typename... Args>
struct QOverload : QConstOverload<Args...>, QNonConstOverload<Args...>
{
	using QConstOverload<Args...>::of;
	using QConstOverload<Args...>::operator();
	using QNonConstOverload<Args...>::of;
	using QNonConstOverload<Args...>::operator();

	template <typename R>
	Q_DECL_CONSTEXPR auto operator()(R (*ptr)(Args...)) const Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }

	template <typename R>
	static Q_DECL_CONSTEXPR auto of(R (*ptr)(Args...)) Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }
};
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 12, 0)
#define QDeadlineTimer(x) static_cast<unsigned long int>(x)
#else
#include <QDeadlineTimer>
#endif
