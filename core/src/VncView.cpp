/*
 * VncView.cpp - VNC viewer widget
 *
 * Copyright (c) 2006-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of Veyon - http://veyon.io
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

#define XK_KOREAN
#include "rfb/keysym.h"

#include "VncView.h"
#include "ProgressWidget.h"
#include "SystemKeyTrapper.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>


VncView::VncView( const QString &host, int port, QWidget *parent, Mode mode ) :
	QWidget( parent ),
	m_vncConn( new VeyonVncConnection( QCoreApplication::instance() ) ),
	m_mode( mode ),
	m_cursorShape(),
	m_cursorX( 0 ),
	m_cursorY( 0 ),
	m_framebufferSize( 0, 0 ),
	m_cursorHotX( 0 ),
	m_cursorHotY( 0 ),
	m_viewOnly( true ),
	m_viewOnlyFocus( true ),
	m_initDone( false ),
	m_buttonMask( 0 ),
	m_establishingConnectionWidget( nullptr ),
	m_sysKeyTrapper( new SystemKeyTrapper( false ) )
{
	m_vncConn->setHost( host );
	m_vncConn->setPort( port );

	if( m_mode == DemoMode )
	{
		m_vncConn->setQuality( VeyonVncConnection::DefaultQuality );
		m_vncConn->setVeyonAuthType( RfbVeyonAuth::HostWhiteList );
		m_establishingConnectionWidget = new ProgressWidget(
			tr( "Establishing connection to %1 ..." ).arg( host ),
					QStringLiteral( ":/resources/watch%1.png" ), 16, this );
		connect( m_vncConn, &VeyonVncConnection::stateChanged,
				 this, &VncView::updateConnectionState );
	}
	else if( m_mode == RemoteControlMode )
	{
		m_vncConn->setQuality( VeyonVncConnection::RemoteControlQuality );
	}

	connect( m_vncConn, &VeyonVncConnection::imageUpdated, this, &VncView::updateImage );
	connect( m_vncConn, &VeyonVncConnection::framebufferSizeChanged, this, &VncView::updateFramebufferSize );

	connect( m_vncConn, &VeyonVncConnection::cursorPosChanged, this, &VncView::updateCursorPos );
	connect( m_vncConn, &VeyonVncConnection::cursorShapeUpdated, this, &VncView::updateCursorShape );

	// forward trapped special keys
	connect( m_sysKeyTrapper, &SystemKeyTrapper::keyEvent,
			 m_vncConn, &VeyonVncConnection::keyEvent );
	connect( m_sysKeyTrapper, &SystemKeyTrapper::keyEvent,
			 this, &VncView::checkKeyEvent );


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

	m_vncConn->start();
}



VncView::~VncView()
{
	// do not receive any signals during connection shutdown
	m_vncConn->disconnect( this );

	unpressModifiers();
	delete m_sysKeyTrapper;

	m_vncConn->stop( true );
}



bool VncView::eventFilter(QObject *obj, QEvent *event)
{
	if( m_viewOnly )
	{
		if( event->type() == QEvent::KeyPress ||
			event->type() == QEvent::KeyRelease ||
			event->type() == QEvent::MouseButtonDblClick ||
			event->type() == QEvent::MouseButtonPress ||
			event->type() == QEvent::MouseButtonRelease ||
			event->type() == QEvent::Wheel )
		return true;
	}

	return QWidget::eventFilter(obj, event);
}



QSize VncView::sizeHint() const
{
	return framebufferSize();
}



QSize VncView::scaledSize() const
{
	if( isScaledView() == false )
	{
		return m_framebufferSize;
	}

	return m_framebufferSize.scaled( size(), Qt::KeepAspectRatio );
}



void VncView::setViewOnly( bool viewOnly )
{
	if( viewOnly == m_viewOnly )
	{
		return;
	}
	m_viewOnly = viewOnly;

	if( m_viewOnly )
	{
		releaseKeyboard();
		m_sysKeyTrapper->setEnabled( false );
		updateLocalCursor();
	}
	else
	{
		grabKeyboard();
		updateLocalCursor();
		m_sysKeyTrapper->setEnabled( true );
	}
}



void VncView::sendShortcut( VncView::Shortcut shortcut )
{
	if( isViewOnly() )
	{
		return;
	}

	unpressModifiers();


	switch( shortcut )
	{
	case ShortcutCtrlAltDel:
		pressKey( XK_Control_L );
		pressKey( XK_Alt_L );
		pressKey( XK_Delete );
		unpressKey( XK_Delete );
		unpressKey( XK_Alt_L );
		unpressKey( XK_Control_L );
		break;
	case ShortcutCtrlEscape:
		pressKey( XK_Control_L );
		pressKey( XK_Escape );
		unpressKey( XK_Escape );
		unpressKey( XK_Control_L );
		break;
	case ShortcutAltTab:
		pressKey( XK_Alt_L );
		pressKey( XK_Tab );
		unpressKey( XK_Tab );
		unpressKey( XK_Alt_L );
		break;
	case ShortcutAltF4:
		pressKey( XK_Alt_L );
		pressKey( XK_F4 );
		unpressKey( XK_F4 );
		unpressKey( XK_Alt_L );
		break;
	case ShortcutWinTab:
		pressKey( XK_Meta_L );
		pressKey( XK_Tab );
		unpressKey( XK_Tab );
		unpressKey( XK_Meta_L );
		break;
	case ShortcutWin:
		pressKey( XK_Meta_L );
		unpressKey( XK_Meta_L );
		break;
	case ShortcutMenu:
		pressKey( XK_Menu );
		unpressKey( XK_Menu );
		break;
	case ShortcutAltCtrlF1:
		pressKey( XK_Control_L );
		pressKey( XK_Alt_L );
		pressKey( XK_F1 );
		unpressKey( XK_F1 );
		unpressKey( XK_Alt_L );
		unpressKey( XK_Control_L );
		break;
	default:
		qWarning( "VncView::sendShortcut(): unknown shortcut %d", (int) shortcut );
		break;
	}
}




void VncView::checkKeyEvent( unsigned int key, bool pressed )
{
#if 0
	if( key == XK_Super_L )
	{
		if( pressed )
		{
			m_keyModifiers[key] = true;
		}
		else if( m_keyModifiers.contains( key ) )
		{
			m_keyModifiers.remove( key );
		}
	}
#endif
}



void VncView::updateCursorPos( int x, int y )
{
	if( isViewOnly() )
	{
		if( !m_cursorShape.isNull() )
		{
			update( m_cursorX, m_cursorY,
					m_cursorShape.width(), m_cursorShape.height() );
		}
		m_cursorX = x;
		m_cursorY = y;
		if( !m_cursorShape.isNull() )
		{
			update( m_cursorX, m_cursorY,
					m_cursorShape.width(), m_cursorShape.height() );
		}
	}
}



void VncView::updateCursorShape( const QPixmap& cursorShape, int xh, int yh )
{
	const auto scale = scaleFactor();

	m_cursorHotX = xh*scale;
	m_cursorHotY = yh*scale;
	m_cursorShape = cursorShape.scaled( cursorShape.width()*scale, cursorShape.height()*scale,
										Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

	if( isViewOnly() )
	{
		update( m_cursorX, m_cursorY, m_cursorShape.width(), m_cursorShape.height() );
	}

	updateLocalCursor();
}



void VncView::focusInEvent( QFocusEvent * _e )
{
	if( !m_viewOnlyFocus )
	{
		setViewOnly( false );
	}
	QWidget::focusInEvent( _e );
}



void VncView::focusOutEvent( QFocusEvent * _e )
{
	m_viewOnlyFocus = isViewOnly();
	if( !isViewOnly() )
	{
		setViewOnly( true );
	}
	QWidget::focusOutEvent( _e );
}



// our builtin keyboard-handler
void VncView::keyEventHandler( QKeyEvent *event )
{
	const Qt::Key qtKey = static_cast<Qt::Key>( event->key() );

	bool pressed = ( event->type() == QEvent::KeyPress );

	// handle modifiers
	if( m_keyMapper.isModifier( qtKey ) )
	{
		if( pressed )
		{
			m_keyModifiers[qtKey] = true;
		}
		else if( m_keyModifiers.contains( qtKey ) )
		{
			m_keyModifiers.remove( qtKey );
		}
		else
		{
			unpressModifiers();
		}
	}

	auto xKey = m_keyMapper.qtToXKey( qtKey );
	if( xKey == XK_VoidSymbol )
	{
		QString text = event->text();

		if( m_keyModifiers.contains( Qt::Key_Control ) &&
				QKeySequence( qtKey ).toString().length() == 1 )
		{
			text = QKeySequence( qtKey ).toString();
			if( m_keyModifiers.contains( Qt::Key_Shift ) == false )
			{
				text = text.toLower();
			}
		}

		xKey = m_keyMapper.unicodeToXKey( text.utf16()[0] );
	}

	// handle Ctrl+Alt+Del replacement (Meta/Super key+Del)
	if( ( m_keyModifiers.contains( Qt::Key_Super_L ) ||
			m_keyModifiers.contains( Qt::Key_Super_R ) ||
			m_keyModifiers.contains( Qt::Key_Meta ) ) &&
				qtKey == Qt::Key_Delete )
	{
		if( pressed )
		{
			unpressModifiers();
			m_vncConn->keyEvent( XK_Control_L, true );
			m_vncConn->keyEvent( XK_Alt_L, true );
			m_vncConn->keyEvent( XK_Delete, true );
			m_vncConn->keyEvent( XK_Delete, false );
			m_vncConn->keyEvent( XK_Alt_L, false );
			m_vncConn->keyEvent( XK_Control_L, false );
			xKey = XK_VoidSymbol;
		}
	}

	if( xKey != XK_VoidSymbol )
	{
		// forward key event to the VNC connection
		m_vncConn->keyEvent( xKey, pressed );

		// signal key event - used by RemoteControlWidget to close itself
		// when pressing Esc
		emit keyEvent( xKey, pressed );

		// inform Qt that we handled the key event
		event->accept();
	}
}




void VncView::unpressModifiers()
{
	const auto keys = m_keyModifiers.keys();
	for( auto key : keys )
	{
		m_vncConn->keyEvent( m_keyMapper.qtToXKey( key ), false );
	}
	m_keyModifiers.clear();
}



bool VncView::isScaledView() const
{
	return width() < m_framebufferSize.width() ||
			height() < m_framebufferSize.height();
}



float VncView::scaleFactor() const
{
	if( isScaledView() )
	{
		return (float) scaledSize().width() / m_framebufferSize.width();
	}

	return 1;
}



QPoint VncView::mapToFramebuffer( QPoint pos )
{
	if( m_framebufferSize.isEmpty() )
	{
		return QPoint( 0, 0 );
	}

	return QPoint( pos.x() * m_framebufferSize.width() / scaledSize().width(),
				   pos.y() * m_framebufferSize.height() / scaledSize().height() );
}



QRect VncView::mapFromFramebuffer( QRect r )
{
	if( m_framebufferSize.isEmpty() )
	{
		return QRect();
	}

	const float dx = scaledSize().width() / (float) m_framebufferSize.width();
	const float dy = scaledSize().height() / (float) m_framebufferSize.height();

	return( QRect( (int)(r.x()*dx), (int)(r.y()*dy),
				   (int)(r.width()*dx), (int)(r.height()*dy) ) );
}



void VncView::updateLocalCursor()
{
	if( isViewOnly()  )
	{
		setCursor( Qt::ArrowCursor );
	}
	else if( m_cursorShape.isNull() == false )
	{
		setCursor( QCursor( m_cursorShape, m_cursorHotX, m_cursorHotY ) );
	}
	else
	{
		setCursor( Qt::BlankCursor );
	}
}



void VncView::pressKey( unsigned int key )
{
	m_vncConn->keyEvent( key, true );
}



void VncView::unpressKey( unsigned int key )
{
	m_vncConn->keyEvent( key, false );
}



bool VncView::event( QEvent * event )
{
	switch( event->type() )
	{
		case QEvent::KeyPress:
		case QEvent::KeyRelease:
			keyEventHandler( static_cast<QKeyEvent*>( event ) );
			return true;
			break;
		case QEvent::MouseButtonDblClick:
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonRelease:
		case QEvent::MouseMove:
			mouseEventHandler( static_cast<QMouseEvent*>( event ) );
			return true;
			break;
		case QEvent::Wheel:
			wheelEventHandler( static_cast<QWheelEvent*>( event ) );
			return true;
			break;
		default:
			return QWidget::event(event);
	}
}



void VncView::paintEvent( QPaintEvent *paintEvent )
{
	QPainter p( this );
	p.setRenderHint( QPainter::SmoothPixmapTransform );

	const auto& image = m_vncConn->image();

	if( image.isNull() || image.format() == QImage::Format_Invalid )
	{
		p.fillRect( paintEvent->rect(), Qt::black );
		return;
	}

	if( isScaledView() )
	{
		// repaint everything in scaled mode to avoid artifacts at rectangle boundaries
		p.drawImage( QRect( QPoint( 0, 0 ), scaledSize() ), image );
	}
	else
	{
		p.drawImage( 0, 0, image );
	}

	if( isViewOnly() && !m_cursorShape.isNull() )
	{
		const QRect cursorRect = mapFromFramebuffer(
			QRect( QPoint( m_cursorX - m_cursorHotX,
							m_cursorY - m_cursorHotY ),
					m_cursorShape.size() ) );
		// parts of cursor within updated region?
		if( paintEvent->region().intersects( cursorRect ) )
		{
			// then repaint it
			p.drawPixmap( cursorRect.topLeft(), m_cursorShape );
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



void VncView::resizeEvent( QResizeEvent *event )
{
	update();

	if( m_establishingConnectionWidget )
	{
		m_establishingConnectionWidget->move( 10, 10 );
	}

	updateLocalCursor();

	QWidget::resizeEvent( event );
}



void VncView::wheelEventHandler( QWheelEvent * _we )
{
	const QPoint p = mapToFramebuffer( _we->pos() );
	m_vncConn->mouseEvent( p.x(), p.y(), m_buttonMask |
		( ( _we->delta() < 0 ) ? rfbButton5Mask : rfbButton4Mask ) );
	m_vncConn->mouseEvent( p.x(), p.y(), m_buttonMask );
}



void VncView::mouseEventHandler( QMouseEvent * _me )
{
	struct buttonXlate
	{
		Qt::MouseButton qt;
		int rfb;
	} const map[] =
		{
			{ Qt::LeftButton, rfbButton1Mask },
			{ Qt::MidButton, rfbButton2Mask },
			{ Qt::RightButton, rfbButton3Mask }
		} ;

	if( _me->type() != QEvent::MouseMove )
	{
		for( auto i : map )
		{
			if( _me->button() == i.qt )
			{
				if( _me->type() == QEvent::MouseButtonPress ||
				_me->type() == QEvent::MouseButtonDblClick )
				{
					m_buttonMask |= i.rfb;
				}
				else
				{
					m_buttonMask &= ~i.rfb;
				}
			}
		}
	}
	else
	{
		if( _me->pos().y() < 2 )
		{
			// special signal for allowing parent-widgets to
			// show a toolbar etc.
			emit mouseAtTop();
		}
	}

	if( !m_viewOnly )
	{
		const QPoint p = mapToFramebuffer( _me->pos() );
		m_vncConn->mouseEvent( p.x(), p.y(), m_buttonMask );
	}
}



void VncView::updateImage( int x, int y, int w, int h )
{
	if( m_initDone == false )
	{
		setAttribute( Qt::WA_OpaquePaintEvent );
		installEventFilter( this );

		setMouseTracking( true ); // get mouse events even when there is no mousebutton pressed
		setFocusPolicy( Qt::WheelFocus );

		resize( sizeHint() );

		emit connectionEstablished();
		m_initDone = true;

	}

	const auto scale = scaleFactor();

	update( qMax( 0, qFloor( x*scale - 1 ) ), qMax( 0, qFloor( y*scale - 1 ) ),
			qCeil( w*scale + 2 ), qCeil( h*scale + 2 ) );
}



void VncView::updateFramebufferSize( int w, int h )
{
	m_framebufferSize = QSize( w, h );

	resize( w, h );

	emit sizeHintChanged();
}



void VncView::updateConnectionState()
{
	if( m_establishingConnectionWidget )
	{
		m_establishingConnectionWidget->setVisible( m_vncConn->state() != VeyonVncConnection::Connected );
	}
}
