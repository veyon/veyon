/*
 *  LockWidget.h - widget for locking a client
 *
 *  Copyright (c) 2006-2019 Tobias Junghans <tobydox@veyon.io>
 *
 *  This file is part of Veyon - http://veyon.io
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

#ifndef LOCK_WIDGET_H
#define LOCK_WIDGET_H

#include <QWidget>
#include <QPixmap>

#include "VeyonCore.h"


class VEYON_CORE_EXPORT LockWidget : public QWidget
{
	Q_OBJECT
public:
	typedef enum Modes
	{
		DesktopVisible,
		BackgroundPixmap,
		NoBackground
	} Mode;

	LockWidget( Mode mode, const QPixmap& background = QPixmap(), QWidget* parent = nullptr );
	~LockWidget() override;


private:
	void paintEvent( QPaintEvent * ) override;

	QPixmap m_background;
	Mode m_mode;

} ;

#endif
