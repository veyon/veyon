/*
 * VncView.cpp - VNC viewer widget
 *
 * Copyright (c) 2006-2019 Tobias Junghans <tobydox@veyon.io>
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
#include "PlatformInputDeviceFunctions.h"
#include "KeyboardShortcutTrapper.h"
#include "ProgressWidget.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>


VncView::VncView( const QString &host, int port, QWidget *parent, Mode mode ) :
	QWidget( parent ),
	m_vncConn( new VncConnection( QCoreApplication::instance() ) ),
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
	m_keyboardShortcutTrapper( VeyonCore::platform().inputDeviceFunctions().createKeyboardShortcutTrapper( this ) ),
	m_mouseBorderSignalTimer( this )
{
	m_vncConn->setHost( host );
	m_vncConn->setPort( port );

	if( m_mode == DemoMode )
	{
		m_vncConn->setQuality( VncConnection::DefaultQuality );
		m_vncConn->setVeyonAuthType( RfbVeyonAuth::HostWhiteList );
		m_establishingConnectionWidget = new ProgressWidget(
			tr( "Establishing connection to %1 ..." ).arg( m_vncConn->host() ),
					QStringLiteral( ":/resources/watch%1.png" ), 16, this );
		connect( m_vncConn, &VncConnection::stateChanged,
				 this, &VncView::updateConnectionState );
	}
	else if( m_mode == RemoteControlMode )
	{
		m_vncConn->setQuality( VncConnection::RemoteControlQuality );
	}

	connect( m_vncConn, &VncConnection::imageUpdated, this, &VncView::updateImage );
	connect( m_vncConn, &VncConnection::framebufferSizeChanged, this, &VncView::updateFramebufferSize );

	connect( m_vncConn, &VncConnection::cursorPosChanged, this, &VncView::updateCursorPos );
	connect( m_vncConn, &VncConnection::cursorShapeUpdated, this, &VncView::updateCursorShape );

	// handle/forward trapped keyboard shortcuts
	connect( m_keyboardShortcutTrapper, &KeyboardShortcutTrapper::shortcutTrapped,
			 this, &VncView::handleShortcut );

	// set up mouse border signal timer
	m_mouseBorderSignalTimer.setSingleShot( true );
	m_mouseBorderSignalTimer.setInterval( MouseBorderSignalDelay );
	connect( &m_mouseBorderSignalTimer, &QTimer::timeout, this, &VncView::mouseAtBorder );

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
	delete m_keyboardShortcutTrapper;

	m_vncConn->stopAndDeleteLater();
	m_vncConn = nullptr;
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
		m_keyboardShortcutTrapper->setEnabled( false );
		updateLocalCursor();
	}
	else
	{
		grabKeyboard();
		updateLocalCursor();
		m_keyboardShortcutTrapper->setEnabled( true );
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
		qWarning( "VncView::sendShortcut(): unknown shortcut %d", static_cast<int>( shortcut ) );
		break;
	}
}



void VncView::handleShortcut( KeyboardShortcutTrapper::Shortcut shortcut )
{
	unsigned int key = 0;

	switch( shortcut )
	{
	case KeyboardShortcutTrapper::SuperKeyDown:
		m_mods[XK_Super_L] = true;
		break;
	case KeyboardShortcutTrapper::SuperKeyUp:
		m_mods.remove( XK_Super_L );
		break;
	case KeyboardShortcutTrapper::AltTab: key = XK_Tab; break;
	case KeyboardShortcutTrapper::AltEsc: key = XK_Escape; break;
	case KeyboardShortcutTrapper::AltSpace: key = XK_KP_Space; break;
	case KeyboardShortcutTrapper::AltF4: key = XK_F4; break;
	case KeyboardShortcutTrapper::CtrlEsc: key = XK_Escape; break;
	default:
		break;
	}

	if( key )
	{
		m_vncConn->keyEvent( key, true );
		m_vncConn->keyEvent( key, false );
	}
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

	m_cursorHotX = static_cast<int>( xh*scale );
	m_cursorHotY = static_cast<int>( yh*scale );
	m_cursorShape = cursorShape.scaled( static_cast<int>( cursorShape.width()*scale ),
										static_cast<int>( cursorShape.height()*scale ),
										Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

	if( isViewOnly() )
	{
		update( m_cursorX, m_cursorY, m_cursorShape.width(), m_cursorShape.height() );
	}

	updateLocalCursor();
}



void VncView::focusInEvent( QFocusEvent* event )
{
	if( !m_viewOnlyFocus )
	{
		setViewOnly( false );
	}
	QWidget::focusInEvent( event );
}



void VncView::focusOutEvent( QFocusEvent* event )
{
	m_viewOnlyFocus = isViewOnly();
	if( !isViewOnly() )
	{
		setViewOnly( true );
	}
	QWidget::focusOutEvent( event );
}



// our builtin keyboard-handler
void VncView::keyEventHandler( QKeyEvent* event )
{
	bool pressed = event->type() == QEvent::KeyPress;

#ifdef Q_OS_LINUX
	// on Linux/X11 native key codes are equal to the ones used by RFB protocol
	unsigned int key = event->nativeVirtualKey();

	// we do not handle Key_Backtab separately as the Shift-modifier
	// is already enabled
	if( event->key() == Qt::Key_Backtab )
	{
		key = XK_Tab;
	}

#else
	// hmm, either Win32-platform or too old Qt so we have to handle and
	// translate Qt-key-codes to X-keycodes
	unsigned int key = 0;
	switch( event->key() )
	{
		// modifiers are handled separately
		case Qt::Key_Shift: key = XK_Shift_L; break;
		case Qt::Key_Control: key = XK_Control_L; break;
		case Qt::Key_Meta: key = XK_Meta_L; break;
		case Qt::Key_Alt: key = XK_Alt_L; break;
		case Qt::Key_Escape: key = XK_Escape; break;
		case Qt::Key_Tab: key = XK_Tab; break;
		case Qt::Key_Backtab: key = XK_Tab; break;
		case Qt::Key_Backspace: key = XK_BackSpace; break;
		case Qt::Key_Return: key = XK_Return; break;
		case Qt::Key_Insert: key = XK_Insert; break;
		case Qt::Key_Delete: key = XK_Delete; break;
		case Qt::Key_Pause: key = XK_Pause; break;
		case Qt::Key_Print: key = XK_Print; break;
		case Qt::Key_Home: key = XK_Home; break;
		case Qt::Key_End: key = XK_End; break;
		case Qt::Key_Left: key = XK_Left; break;
		case Qt::Key_Up: key = XK_Up; break;
		case Qt::Key_Right: key = XK_Right; break;
		case Qt::Key_Down: key = XK_Down; break;
		case Qt::Key_PageUp: key = XK_Prior; break;
		case Qt::Key_PageDown: key = XK_Next; break;
		case Qt::Key_CapsLock: key = XK_Caps_Lock; break;
		case Qt::Key_NumLock: key = XK_Num_Lock; break;
		case Qt::Key_ScrollLock: key = XK_Scroll_Lock; break;
		case Qt::Key_Super_L: key = XK_Super_L; break;
		case Qt::Key_Super_R: key = XK_Super_R; break;
		case Qt::Key_Menu: key = XK_Menu; break;
		case Qt::Key_Hyper_L: key = XK_Hyper_L; break;
		case Qt::Key_Hyper_R: key = XK_Hyper_R; break;
		case Qt::Key_Help: key = XK_Help; break;
		case Qt::Key_AltGr: key = XK_ISO_Level3_Shift; break;
		case Qt::Key_Multi_key: key = XK_Multi_key; break;
		case Qt::Key_SingleCandidate: key = XK_SingleCandidate; break;
		case Qt::Key_MultipleCandidate: key = XK_MultipleCandidate; break;
		case Qt::Key_PreviousCandidate: key = XK_PreviousCandidate; break;
		case Qt::Key_Mode_switch: key = XK_Mode_switch; break;
		case Qt::Key_Kanji: key = XK_Kanji; break;
		case Qt::Key_Muhenkan: key = XK_Muhenkan; break;
		case Qt::Key_Henkan: key = XK_Henkan; break;
		case Qt::Key_Romaji: key = XK_Romaji; break;
		case Qt::Key_Hiragana: key = XK_Hiragana; break;
		case Qt::Key_Katakana: key = XK_Katakana; break;
		case Qt::Key_Hiragana_Katakana: key = XK_Hiragana_Katakana; break;
		case Qt::Key_Zenkaku: key = XK_Zenkaku; break;
		case Qt::Key_Hankaku: key = XK_Hankaku; break;
		case Qt::Key_Zenkaku_Hankaku: key = XK_Zenkaku_Hankaku; break;
		case Qt::Key_Touroku: key = XK_Touroku; break;
		case Qt::Key_Massyo: key = XK_Massyo; break;
		case Qt::Key_Kana_Lock: key = XK_Kana_Lock; break;
		case Qt::Key_Kana_Shift: key = XK_Kana_Shift; break;
		case Qt::Key_Eisu_Shift: key = XK_Eisu_Shift; break;
		case Qt::Key_Eisu_toggle: key = XK_Eisu_toggle; break;
		case Qt::Key_Hangul: key = XK_Hangul; break;
		case Qt::Key_Hangul_Start: key = XK_Hangul_Start; break;
		case Qt::Key_Hangul_End: key = XK_Hangul_End; break;
		case Qt::Key_Hangul_Hanja: key = XK_Hangul_Hanja; break;
		case Qt::Key_Hangul_Jamo: key = XK_Hangul_Jamo; break;
		case Qt::Key_Hangul_Romaja: key = XK_Hangul_Romaja; break;
		case Qt::Key_Hangul_Jeonja: key = XK_Hangul_Jeonja; break;
		case Qt::Key_Hangul_Banja: key = XK_Hangul_Banja; break;
		case Qt::Key_Hangul_PreHanja: key = XK_Hangul_PreHanja; break;
		case Qt::Key_Hangul_PostHanja: key = XK_Hangul_PostHanja; break;
		case Qt::Key_Hangul_Special: key = XK_Hangul_Special; break;
		case Qt::Key_Dead_Grave: key = XK_dead_grave; break;
		case Qt::Key_Dead_Acute: key = XK_dead_acute; break;
		case Qt::Key_Dead_Circumflex: key = XK_dead_circumflex; break;
		case Qt::Key_Dead_Tilde: key = XK_dead_tilde; break;
		case Qt::Key_Dead_Macron: key = XK_dead_macron; break;
		case Qt::Key_Dead_Breve: key = XK_dead_breve; break;
		case Qt::Key_Dead_Abovedot: key = XK_dead_abovedot; break;
		case Qt::Key_Dead_Diaeresis: key = XK_dead_diaeresis; break;
		case Qt::Key_Dead_Abovering: key = XK_dead_abovering; break;
		case Qt::Key_Dead_Doubleacute: key = XK_dead_doubleacute; break;
		case Qt::Key_Dead_Caron: key = XK_dead_caron; break;
		case Qt::Key_Dead_Cedilla: key = XK_dead_cedilla; break;
		case Qt::Key_Dead_Ogonek: key = XK_dead_ogonek; break;
		case Qt::Key_Dead_Iota: key = XK_dead_iota; break;
		case Qt::Key_Dead_Voiced_Sound: key = XK_dead_voiced_sound; break;
		case Qt::Key_Dead_Semivoiced_Sound: key = XK_dead_semivoiced_sound; break;
		case Qt::Key_Dead_Belowdot: key = XK_dead_belowdot; break;
	}

	if( event->key() >= Qt::Key_F1 && event->key() <= Qt::Key_F35 )
	{
		key = XK_F1 + event->key() - Qt::Key_F1;
	}
	else if( key == 0 )
	{
		if( m_mods.contains( XK_Control_L ) &&
			QKeySequence( event->key() ).toString().length() == 1 )
		{
			QString s = QKeySequence( event->key() ).toString();
			if( !m_mods.contains( XK_Shift_L ) )
			{
				s = s.toLower();
			}
			key = s.utf16()[0];
		}
		else
		{
			key = event->text().utf16()[0];
		}
	}
	// correct translation of AltGr+<character key> (non-US-keyboard layout
	// such as German keyboard layout)
	if( m_mods.contains( XK_Alt_L ) && m_mods.contains( XK_Control_L ) &&
						key >= 64 && key < 0xF000 )
	{
		unpressModifiers();
		m_vncConn->keyEvent( XK_ISO_Level3_Shift, true );
	}
#endif

	// handle Ctrl+Alt+Del replacement (Meta/Super key+Del)
	if( ( m_mods.contains( XK_Super_L ) ||
			m_mods.contains( XK_Super_R ) ||
			m_mods.contains( XK_Meta_L ) ) &&
				event->key() == Qt::Key_Delete )
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
			key = 0;
		}
	}

	// handle modifiers
	if( key == XK_Shift_L || key == XK_Control_L || key == XK_Meta_L ||
			key == XK_Alt_L || key == XK_Super_L || key == XK_Super_R )
	{
		if( pressed )
		{
			m_mods[key] = true;
		}
		else if( m_mods.contains( key ) )
		{
			m_mods.remove( key );
		}
		else
		{
			unpressModifiers();
		}
	}

	if( key )
	{
		// forward key event to the VNC connection
		m_vncConn->keyEvent( key, pressed );

		// signal key event - used by RemoteControlWidget to close itself
		// when pressing Esc
		emit keyEvent( key, pressed );

		// inform Qt that we handled the key event
		event->accept();
	}
}




void VncView::unpressModifiers()
{
	const auto keys = m_mods.keys();
	for( auto key : keys )
	{
		m_vncConn->keyEvent( key, false );
	}
	m_mods.clear();
}



bool VncView::isScaledView() const
{
	return width() < m_framebufferSize.width() ||
			height() < m_framebufferSize.height();
}



qreal VncView::scaleFactor() const
{
	if( isScaledView() )
	{
		return static_cast<qreal>( scaledSize().width() ) / m_framebufferSize.width();
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

	const auto dx = scaledSize().width() / static_cast<qreal>( m_framebufferSize.width() );
	const auto dy = scaledSize().height() / static_cast<qreal>( m_framebufferSize.height() );

	return( QRect( static_cast<int>(r.x()*dx), static_cast<int>(r.y()*dy),
				   static_cast<int>(r.width()*dx), static_cast<int>(r.height()*dy) ) );
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
		case QEvent::MouseButtonDblClick:
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonRelease:
		case QEvent::MouseMove:
			mouseEventHandler( static_cast<QMouseEvent*>( event ) );
			return true;
		case QEvent::Wheel:
			wheelEventHandler( static_cast<QWheelEvent*>( event ) );
			return true;
		default:
			return QWidget::event(event);
	}
}



void VncView::paintEvent( QPaintEvent* paintEvent )
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



void VncView::resizeEvent( QResizeEvent* event )
{
	update();

	if( m_establishingConnectionWidget )
	{
		m_establishingConnectionWidget->move( 10, 10 );
	}

	updateLocalCursor();

	QWidget::resizeEvent( event );
}



void VncView::wheelEventHandler( QWheelEvent* event )
{
	const QPoint p = mapToFramebuffer( event->pos() );
	m_vncConn->mouseEvent( p.x(), p.y(), m_buttonMask | ( ( event->delta() < 0 ) ? rfbButton5Mask : rfbButton4Mask ) );
	m_vncConn->mouseEvent( p.x(), p.y(), m_buttonMask );
}



void VncView::mouseEventHandler( QMouseEvent* event )
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

	if( event->type() != QEvent::MouseMove )
	{
		for( auto i : map )
		{
			if( event->button() == i.qt )
			{
				if( event->type() == QEvent::MouseButtonPress ||
				event->type() == QEvent::MouseButtonDblClick )
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

	if( !m_viewOnly )
	{
		const QPoint p = mapToFramebuffer( event->pos() );
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
		m_establishingConnectionWidget->setVisible( m_vncConn->state() != VncConnection::Connected );
	}
}
