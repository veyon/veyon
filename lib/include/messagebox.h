/*
 * messagebox.h - simple messagebox
 *
 * Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef _MESSAGEBOX_H
#define _MESSAGEBOX_H

#include <QtGui/QDialog>
#include <QtGui/QPixmap>
#include "qt_features.h"
#include "types.h"

#ifdef SYSTEMTRAY_SUPPORT
#include <QtGui/QSystemTrayIcon>
#endif


class IC_DllExport messageBox : public QDialog
{
public:
	enum MessageIcon
	{
		NoIcon,
		Information,
		Warning,
		Critical
	} ;

	messageBox( const QString & _title, const QString & _msg,
					const QPixmap & _pixmap = QPixmap() );

	static void information( const QString & _title, const QString & _msg,
					const QPixmap & _pixmap = QPixmap() );

	static void trySysTrayMessage( const QString & _title,
							const QString & _msg,
							MessageIcon _msg_icon );
} ;

#ifdef SYSTEMTRAY_SUPPORT
extern IC_DllExport QSystemTrayIcon * __systray_icon;
#endif

#endif

