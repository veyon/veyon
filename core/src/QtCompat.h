/*
 * QtCompat.h - functions and templates for compatibility with older Qt versions
 *
 * Copyright (c) 2017-2022 Tobias Junghans <tobydox@veyon.io>
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

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
#error Qt < 5.7 not supported
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#define Q_DISABLE_MOVE(Class) \
	Class(const Class &&) Q_DECL_EQ_DELETE;\
	Class &operator=(const Class &&) Q_DECL_EQ_DELETE;
#endif
