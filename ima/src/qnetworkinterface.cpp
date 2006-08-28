/*
 * qnetworkinterface.cpp - QNetworkInterface-class
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#include "qnetworkinterface.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if QT_VERSION < 0x040200

#include "3rdparty/qnetworkinterface.cpp"

#ifdef BUILD_WIN32

#include "3rdparty/qnetworkinterface_win.cpp"

#else

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "3rdparty/qnetworkinterface_unix.cpp"

#endif

#endif

