/*
 *  LockWidget.h - widget for locking a client
 *
 *  Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 *  This file is part of iTALC - http://italc.sourceforge.net
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#ifndef _LOCK_WIDGET_H
#define _LOCK_WIDGET_H

#include <italcconfig.h>

#include <QtGui/QWidget>
#include <QtGui/QPixmap>

#include "SystemKeyTrapper.h"


class LockWidget : public QWidget
{
	Q_OBJECT
public:
	enum Modes
	{
		DesktopVisible,
		Black,
		NoBackground
	} ;

	LockWidget( Modes _mode = Black );
	virtual ~LockWidget();


private:
	virtual void paintEvent( QPaintEvent * );
#ifdef ITALC_BUILD_LINUX
	virtual bool x11Event( XEvent * _e );
#endif

	QPixmap m_background;
	Modes m_mode;
	SystemKeyTrapper m_sysKeyTrapper;

} ;

#endif

