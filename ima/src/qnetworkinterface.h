/*
 * qnetworkinterface.h - QNetworkInterface-class
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


#ifndef _QNETWORKINTERFACE_H
#define _QNETWORKINTERFACE_H

#define QT_MAKEDLL
#define Q_NETWORK_EXPORT
#include <qglobal.h>

#if QT_VERSION < 0x040200
#include "3rdparty/qnetworkinterface.h"
#endif

#undef QT_MAKEDLL

#endif
