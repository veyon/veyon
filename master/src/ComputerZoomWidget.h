/*
 * ComputerZoomWidget.h - fullscreen preview widget
 *
 * Copyright (c) 2021-2022 Tobias Junghans <tobydox@veyon.io>
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

#include "ComputerControlInterface.h"

#include <QWidget>

class VncViewWidget;

// clazy:excludeall=ctor-missing-parent-argument
class ComputerZoomWidget : public QWidget
{
	Q_OBJECT
public:
	ComputerZoomWidget( const ComputerControlInterface::Pointer& computerControlInterface );
	~ComputerZoomWidget() override;

protected:
	bool eventFilter( QObject* object, QEvent* event ) override;

	void resizeEvent( QResizeEvent* event ) override;
	void closeEvent( QCloseEvent* event ) override;

private:
	void updateSize();
	void updateComputerZoomWidgetTitle();

	int m_currentScreen{-1};

	VncViewWidget* m_vncView;

Q_SIGNALS:
	void keypressInComputerZoomWidget( );

} ;
