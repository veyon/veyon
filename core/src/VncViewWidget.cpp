/*
 * VncViewWidget.cpp - VNC viewer widget
 *
 * Copyright (c) 2006-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QApplication>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

#include "ProgressWidget.h"
#include "VeyonConnection.h"
#include "VncConnection.h"
#include "VncViewWidget.h"


VncViewWidget::VncViewWidget( const QString& host, int port, QWidget* parent, Mode mode, const QRect& viewport ) :
	QWidget( parent ),
	VncView( new VncConnection( QCoreApplication::instance() ) ),
	m_veyonConnection( new VeyonConnection( connection() ) )
{
	setViewport( viewport );

	connectUpdateFunctions( this );

	connection()->setHost( host );
	connection()->setPort( port );

	if( mode == DemoMode )
	{
		connection()->setQuality( VncConnection::Quality::Default );
		m_establishingConnectionWidget = new ProgressWidget(
			tr( "Establishing connection to %1 ..." ).arg( connection()->host() ),
			QStringLiteral( ":/core/watch%1.png" ), 16, this );
		connect( connection(), &VncConnection::stateChanged,
				 this, &VncViewWidget::updateConnectionState );
	}
	else if( mode == RemoteControlMode )
	{
		connection()->setQuality( VncConnection::Quality::RemoteControl );
	}

	// set up mouse border signal timer
	m_mouseBorderSignalTimer.setSingleShot( true );
	m_mouseBorderSignalTimer.setInterval( MouseBorderSignalDelay );
	connect( &m_mouseBorderSignalTimer, &QTimer::timeout, this, &VncViewWidget::mouseAtBorder );

	// set up background color
	if( parent == nullptr )
	{
		parent = this;
	}
	QPalette pal = parent->palette();
	pal.setColor( parent->backgroundRole(), Qt::black );
	parent->setPalette( pal );

	show();

	resize( QApplication::desktop()->availableGeometry( this ).size() - QSize( 10, 30 ) );

	setFocusPolicy( Qt::StrongFocus );
	setFocus();

	connection()->start();
}



VncViewWidget::~VncViewWidget()
{
	// do not receive any signals during connection shutdown
	connection()->disconnect( this );

	unpressModifiers();

	delete m_veyonConnection;
	m_veyonConnection = nullptr;

	connection()->stopAndDeleteLater();
}



QSize VncViewWidget::sizeHint() const
{
	return effectiveFramebufferSize();
}



void VncViewWidget::setViewOnly( bool enabled )
{
	if( enabled == viewOnly() )
	{
		return;
	}

	if( enabled )
	{
		releaseKeyboard();
	}
	else
	{
		grabKeyboard();
	}

	VncView::setViewOnly( enabled );
}



void VncViewWidget::updateView( int x, int y, int w, int h )
{
	update( x, y, w, h );
}



QSize VncViewWidget::viewSize() const
{
	return size();
}



void VncViewWidget::setViewCursor(const QCursor& cursor)
{
	setCursor( cursor );
}



void VncViewWidget::updateFramebufferSize( int w, int h )
{
	VncView::updateFramebufferSize( w, h );

	resize( w, h );

	Q_EMIT sizeHintChanged();
}



void VncViewWidget::updateImage( int x, int y, int w, int h )
{
	if( m_initDone == false )
	{
		setAttribute( Qt::WA_OpaquePaintEvent );
		installEventFilter( this );

		setMouseTracking( true ); // get mouse events even when there is no mousebutton pressed
		setFocusPolicy( Qt::WheelFocus );

		resize( sizeHint() );

		Q_EMIT connectionEstablished();
		m_initDone = true;

	}

	VncView::updateImage( x, y, w, h );
}



bool VncViewWidget::event( QEvent* event )
{
	return VncView::handleEvent( event ) || QWidget::event( event );
}



bool VncViewWidget::eventFilter( QObject* obj, QEvent* event )
{
	if( viewOnly() )
	{
		if( event->type() == QEvent::KeyPress ||
			event->type() == QEvent::KeyRelease ||
			event->type() == QEvent::MouseButtonDblClick ||
			event->type() == QEvent::MouseButtonPress ||
			event->type() == QEvent::MouseButtonRelease ||
			event->type() == QEvent::Wheel )
		{
			return true;
		}
	}

	return QWidget::eventFilter( obj, event );
}



void VncViewWidget::focusInEvent( QFocusEvent* event )
{
	if( m_viewOnlyFocus == false )
	{
		setViewOnly( false );
	}

	QWidget::focusInEvent( event );
}



void VncViewWidget::focusOutEvent( QFocusEvent* event )
{
	m_viewOnlyFocus = viewOnly();

	if( viewOnly() == false )
	{
		setViewOnly( true );
	}

	QWidget::focusOutEvent( event );
}



void VncViewWidget::mouseEventHandler( QMouseEvent* event )
{
	if( event == nullptr )
	{
		return;
	}

	VncView::mouseEventHandler( event );

	if( event->type() == QEvent::MouseMove )
	{
		if( event->pos().y() == 0 )
		{
			if( m_mouseBorderSignalTimer.isActive() == false )
			{
				m_mouseBorderSignalTimer.start();
			}
		}
		else
		{
			m_mouseBorderSignalTimer.stop();
		}
	}
}



void VncViewWidget::paintEvent( QPaintEvent* paintEvent )
{
	QPainter p( this );
	p.setRenderHint( QPainter::SmoothPixmapTransform );

	const auto& image = connection()->image();

	if( image.isNull() || image.format() == QImage::Format_Invalid )
	{
		p.fillRect( paintEvent->rect(), Qt::black );
		return;
	}

	auto source = viewport();
	if( source.isNull() || source.isEmpty() )
	{
		source = { QPoint{ 0, 0 }, image.size() };
	}

	if( isScaledView() )
	{
		// repaint everything in scaled mode to avoid artifacts at rectangle boundaries
		p.drawImage( QRect( QPoint( 0, 0 ), scaledSize() ), image, source );
	}
	else
	{
		p.drawImage( { 0, 0 }, image, source );
	}

	if( viewOnly() && cursorShape().isNull() == false )
	{
		const auto cursorRect = mapFromFramebuffer( QRect( cursorPos() - cursorHot(), cursorShape().size() ) );
		// parts of cursor within updated region?
		if( paintEvent->region().intersects( cursorRect ) )
		{
			// then repaint it
			p.drawPixmap( cursorRect.topLeft(), cursorShape() );
		}
	}

	// draw black borders if neccessary
	const int screenWidth = scaledSize().width();
	if( screenWidth < width() )
	{
		p.fillRect( screenWidth, 0, width() - screenWidth, height(), Qt::black );
	}

	const int screenHeight = scaledSize().height();
	if( screenHeight < height() )
	{
		p.fillRect( 0, screenHeight, width(), height() - screenHeight, Qt::black );
	}
}



void VncViewWidget::resizeEvent( QResizeEvent* event )
{
	update();

	if( m_establishingConnectionWidget )
	{
		m_establishingConnectionWidget->move( 10, 10 );
	}

	updateLocalCursor();

	QWidget::resizeEvent( event );
}



void VncViewWidget::updateConnectionState()
{
	if( m_establishingConnectionWidget )
	{
		m_establishingConnectionWidget->setVisible( connection()->state() != VncConnection::State::Connected );
	}
}
