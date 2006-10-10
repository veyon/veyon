/*
 *  lock_widget.h - widget for locking a client
 *
 *  Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QtGui/QWidget>
#include <QtGui/QPixmap>

#include "system_key_trapper.h"


class lockWidget : public QWidget
{
public:
	enum types
	{
		DesktopVisible, Black, NoBackground
	} ;

	lockWidget( types _type = Black );
	virtual ~lockWidget();


private:
	virtual void paintEvent( QPaintEvent * );
#ifdef BUILD_LINUX
	virtual bool x11Event( XEvent * _e );
#endif
/*
#ifdef BUILD_WIN32
	virtual bool winEvent( MSG * _message, long * _result );
#endif
*/
	QPixmap m_background;
	types m_type;
	systemKeyTrapper m_sysKeyTrapper;

} ;


#endif

