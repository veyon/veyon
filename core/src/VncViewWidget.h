/*
 * VncViewWidget.h - VNC viewer widget
 *
 * Copyright (c) 2006-2023 Tobias Junghans <tobydox@veyon.io>
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

#include <QTimer>
#include <QWidget>

#include "VncView.h"
#include "ComputerControlInterface.h"

class VEYON_CORE_EXPORT VncViewWidget : public QWidget, public VncView
{
	Q_OBJECT
public:
	VncViewWidget( const ComputerControlInterface::Pointer& computerControlInterface, QRect viewport,
				   QWidget* parent );

	~VncViewWidget() override;

	QSize sizeHint() const override;

	void setViewOnly( bool enabled ) override;

Q_SIGNALS:
	void mouseAtBorder();
	void sizeHintChanged();

protected:
	void updateView( int x, int y, int w, int h ) override;
	QSize viewSize() const override;
	void setViewCursor( const QCursor& cursor ) override;

	void updateGeometry() override;

	bool event( QEvent* handleEvent ) override;
	bool eventFilter( QObject* obj, QEvent* handleEvent ) override;
	void focusInEvent( QFocusEvent* handleEvent ) override;
	void focusOutEvent( QFocusEvent* handleEvent ) override;
	void mouseEventHandler( QMouseEvent* handleEvent ) override;
	void paintEvent( QPaintEvent* handleEvent ) override;
	void resizeEvent( QResizeEvent* handleEvent ) override;

private:
	void drawBusyIndicator( QPainter* painter );
	void updateConnectionState();

	bool m_viewOnlyFocus{true};

	static constexpr auto BusyIndicatorUpdateInterval = 25;
	QTimer m_busyIndicatorTimer{this};
	int m_busyIndicatorState{0};

	static constexpr int MouseBorderSignalDelay = 500;
	QTimer m_mouseBorderSignalTimer{this};

} ;
