/*
 * types.h - some typedefs
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>

typedef signed char             Q_INT8;         /* 8 bit signed */
typedef unsigned char           Q_UINT8;        /* 8 bit unsigned */
typedef short                   Q_INT16;        /* 16 bit signed */
typedef unsigned short          Q_UINT16;       /* 16 bit unsigned */
typedef int32_t			Q_INT32;        /* 32 bit signed */
typedef uint32_t 		Q_UINT32;       /* 32 bit unsigned */


#ifdef BUILD_WIN32

#ifdef BUILD_LIBRARY
#define IC_DllExport __declspec(dllexport)
#else
#define IC_DllExport __declspec(dllimport)
#endif


#else

#define IC_DllExport

#endif


#endif
