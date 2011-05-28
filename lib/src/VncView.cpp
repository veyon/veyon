/*
 * VncView.cpp - VNC viewer widget
 *
 * Copyright (c) 2006-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#define XK_KOREAN
#include "rfb/keysym.h"

#include "VncView.h"
#include "QtUserEvents.h"
#include "ProgressWidget.h"
#include "SystemKeyTrapper.h"

#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>



VncView::VncView( const QString &host, QWidget *parent, Mode mode ) :
	QWidget( parent ),
	m_vncConn( this ),
	m_mode( mode ),
	m_frame(),
	m_cursorShape(),
	m_cursorX( 0 ),
	m_cursorY( 0 ),
	m_framebufferSize( 0, 0 ),
	m_cursorHotX( 0 ),
	m_cursorHotY( 0 ),
	m_viewOnly( true ),
	m_viewOnlyFocus( true ),
	m_scaledView( true ),
	m_initDone( false ),
	m_buttonMask( 0 ),
	m_establishingConnection( NULL ),
	m_sysKeyTrapper( new SystemKeyTrapper( false ) )
{
	m_vncConn.setHost( host );
	if( m_mode == DemoMode )
	{
		m_vncConn.setQuality( ItalcVncConnection::DemoClientQuality );
		m_vncConn.setItalcAuthType( ItalcAuthHostBased );
		m_establishingConnection = new ProgressWidget(
			tr( "Establishing connection to %1 ..." ).arg( host ),
					":/resources/watch%1.png", 16, this );
		connect( &m_vncConn, SIGNAL( connected() ),
					m_establishingConnection, SLOT( hide() ) );

	}
	else if( m_mode == RemoteControlMode )
	{
		m_vncConn.setQuality( ItalcVncConnection::RemoteControlQuality );
	}

	connect( &m_vncConn, SIGNAL( imageUpdated( int, int, int, int ) ),
			this, SLOT( updateImage( int, int, int, int ) ),
						Qt::BlockingQueuedConnection );

	connect( &m_vncConn, SIGNAL( framebufferSizeChanged( int, int ) ),
				this, SLOT( updateSizeHint( int, int ) ), Qt::QueuedConnection );

	connect( &m_vncConn, SIGNAL( cursorPosChanged( int, int ) ),
				this, SLOT( updateCursorPos( int, int ) ) );

	connect( &m_vncConn, SIGNAL( cursorShapeUpdated( const QImage &, int, int ) ),
				this, SLOT( updateCursorShape( const QImage &, int, int ) ) );

	// forward trapped special keys
	connect( m_sysKeyTrapper, SIGNAL( keyEvent( unsigned int, bool ) ),
				&m_vncConn, SLOT( keyEvent( unsigned int, bool ) ) );
	connect( m_sysKeyTrapper, SIGNAL( keyEvent( unsigned int, bool ) ),
				this, SLOT( checkKeyEvent( unsigned int, bool ) ) );


	// set up background color
	if( parent == NULL )
	{
		parent = this;
	}
	QPalette pal = parent->palette();
	pal.setColor( parent->backgroundRole(), Qt::black );
	parent->setPalette( pal );

	show();

	resize( QApplication::desktop()->
						availableGeometry( this ).size() - QSize( 10, 30 ) );

	setFocusPolicy( Qt::StrongFocus );
	setFocus();

	m_vncConn.start();
}




VncView::~VncView()
{
	disconnect( &m_vncConn, SIGNAL( imageUpdated( int, int, int, int ) ),
			this, SLOT( updateImage( int, int, int, int ) ) );

	unpressModifiers();
	delete m_sysKeyTrapper;

	m_vncConn.stop();
	m_vncConn.wait( 500 );
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
	if( m_scaledView )
	{
		return scaledSize();
	}
	return framebufferSize();
}




QSize VncView::scaledSize() const
{
	const QSize s = size();
	QSize fbs = framebufferSize();
	if( ( s.width() >= fbs.width() && s.height() >= fbs.height() ) ||
							m_scaledView == false )
	{
		return fbs;
	}
	fbs.scale( s, Qt::KeepAspectRatio );
	return fbs;
}




void VncView::setViewOnly( bool _vo )
{
	if( _vo == m_viewOnly )
	{
		return;
	}
	m_viewOnly = _vo;

	if( m_viewOnly )
	{
		releaseKeyboard();
		m_sysKeyTrapper->setEnabled( false );
		setCursor( Qt::ArrowCursor );
	}
	else
	{
		grabKeyboard();
		updateLocalCursor();
		m_sysKeyTrapper->setEnabled( true );
	}
}




void VncView::setScaledView( bool scaledView )
{
	m_scaledView = scaledView;
	m_vncConn.setScaledSize( scaledSize() );
	update();
}




void VncView::checkKeyEvent( unsigned int key, bool pressed )
{
	if( key == XK_Super_L )
	{
		if( pressed )
		{
			m_mods[key] = true;
		}
		else if( m_mods.contains( key ) )
		{
			m_mods.remove( key );
		}
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




void VncView::updateCursorShape( const QImage &cursorShape, int xh, int yh )
{
	float scale = 1;
	if( !scaledSize().isEmpty() && !framebufferSize().isEmpty() )
	{
		scale = (float) scaledSize().width() / framebufferSize().width();
	}
	m_cursorHotX = xh*scale;
	m_cursorHotY = yh*scale;
	m_cursorShape = cursorShape.scaled( m_cursorShape.width()*scale,
										m_cursorShape.height()*scale );

	if( isViewOnly() )
	{
		update( m_cursorX, m_cursorY,
					m_cursorShape.width(), m_cursorShape.height() );
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
void VncView::keyEventHandler( QKeyEvent * _ke )
{
	bool pressed = _ke->type() == QEvent::KeyPress;

#ifdef ITALC_BUILD_LINUX
	// Starting with Qt 4.2 there's a nice function returning the key-code
	// of the key-event (platform-dependent) so when operating under Linux/X11,
	// key-codes are equal to the ones used by RFB protocol
	int key = _ke->nativeVirtualKey();

	// we do not handle Key_Backtab separately as the Shift-modifier
	// is already enabled
	if( _ke->key() == Qt::Key_Backtab )
	{
		key = XK_Tab;
	}

#else
	// hmm, either Win32-platform or too old Qt so we have to handle and
	// translate Qt-key-codes to X-keycodes
	unsigned int key = 0;
	switch( _ke->key() )
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

	if( _ke->key() >= Qt::Key_F1 && _ke->key() <= Qt::Key_F35 )
	{
		key = XK_F1 + _ke->key() - Qt::Key_F1;
	}
	else if( key == 0 )
	{
		if( m_mods.contains( XK_Control_L ) &&
			QKeySequence( _ke->key() ).toString().length() == 1 )
		{
			QString s = QKeySequence( _ke->key() ).toString();
			if( !m_mods.contains( XK_Shift_L ) )
			{
				s = s.toLower();
			}
			key = s.utf16()[0];
		}
		else
		{
			key = _ke->text().utf16()[0];
		}
	}
	// correct translation of AltGr+<character key> (non-US-keyboard layout
	// such as German keyboard layout)
	if( m_mods.contains( XK_Alt_L ) && m_mods.contains( XK_Control_L ) &&
						key >= 64 && key < 0xF000 )
	{
		unpressModifiers();
		m_vncConn.keyEvent( XK_ISO_Level3_Shift, true );
	}
#endif

	// handle Ctrl+Alt+Del replacement (Meta/Super key+Del)
	if( ( m_mods.contains( XK_Super_L ) ||
			m_mods.contains( XK_Super_R ) ||
			m_mods.contains( XK_Meta_L ) ) &&
				_ke->key() == Qt::Key_Delete )
	{
		if( pressed )
		{
			unpressModifiers();
			m_vncConn.keyEvent( XK_Control_L, true );
			m_vncConn.keyEvent( XK_Alt_L, true );
			m_vncConn.keyEvent( XK_Delete, true );
			m_vncConn.keyEvent( XK_Delete, false );
			m_vncConn.keyEvent( XK_Alt_L, false );
			m_vncConn.keyEvent( XK_Control_L, false );
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
		m_vncConn.keyEvent( key, pressed );

		// signal key event - used by RemoteControlWidget to close itself
		// when pressing Esc
		emit keyEvent( key, pressed );

		// inform Qt that we handled the key event
		_ke->accept();
	}
}




void VncView::unpressModifiers()
{
	QList<unsigned int> keys = m_mods.keys();
	QList<unsigned int>::const_iterator it = keys.begin();
	while( it != keys.end() )
	{
		m_vncConn.keyEvent( *it, false );
		it++;
	}
	m_mods.clear();
}




QPoint VncView::mapToFramebuffer( const QPoint &pos )
{
	const QSize fbs = framebufferSize();
	if( fbs.isEmpty() )
	{
		return QPoint( 0, 0 );
	}

	if( m_scaledView )
	{
		return QPoint( pos.x() * fbs.width() / scaledSize().width(),
						pos.y() * fbs.height() / scaledSize().height() );
	}

	return pos;
}




QRect VncView::mapFromFramebuffer( const QRect &r )
{
	if( framebufferSize().isEmpty() )
	{
		return QRect();
	}
	if( m_scaledView )
	{
		const float dx = width() / (float) framebufferSize().width();
		const float dy = height() / (float) framebufferSize().height();
		return( QRect( (int)(r.x()*dx), (int)(r.y()*dy),
					(int)(r.width()*dx), (int)(r.height()*dy) ) );
	}
	return r;
}



void VncView::updateLocalCursor()
{
	if( !isViewOnly() && !m_cursorShape.isNull() )
	{
		setCursor( QCursor( QPixmap::fromImage( m_cursorShape ),
								m_cursorHotX, m_cursorHotY ) );
	}
	else
	{
		setCursor( Qt::ArrowCursor );
	}
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
	paintEvent->accept();

	QPainter p( this );

	p.fillRect( paintEvent->rect(), Qt::black );
	if( m_frame.isNull() || m_frame.format() == QImage::Format_Invalid )
	{
		return;
	}

	const QSize sSize = scaledSize();
	const float scale = sSize.isEmpty() ? 1 :
			(float) sSize.width() / framebufferSize().width();
	if( m_repaint )
	{
		if( sSize.isEmpty() )
		{
			p.drawImage( QRect( m_x, m_y, m_w, m_h ),
					m_frame.copy( m_x, m_y, m_w, m_h ) );
		}
		else
		{
			FastQImage i = m_frame;
			p.drawImage( 0, 0, m_frame.scaled( sSize ) );
		}
	}
	else
	{
		QRect rect = paintEvent->rect();
		if( rect.width() != m_frame.width() || rect.height() != m_frame.height() )
		{
			int sx = qRound( rect.x()/scale );
			int sy = qRound( rect.y()/scale );
			int sw = qRound( rect.width()/scale );
			int sh = qRound( rect.height()/scale );
			p.drawImage( rect, m_frame.copy( sx, sy, sw, sh ).
				scaled( rect.width(), rect.height(),
						Qt::IgnoreAspectRatio,
						Qt::SmoothTransformation ) );
		}
		else
		{
			// even we just have to update a part of the screen, update
			// everything when in scaled mode as otherwise there're annoying
			// artifacts
			p.drawImage( QPoint( 0, 0 ),
		m_frame.scaled( qRound( m_frame.width() * scale ),
					qRound( m_frame.height()*scale ),
			Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );
		}
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
			p.drawImage( cursorRect.topLeft(), m_cursorShape );
		}
	}



	// draw black borders if neccessary
	const int fbw = sSize.isValid() ? sSize.width() :
				m_vncConn.framebufferSize().width();
	if( fbw < width() )
	{
		p.fillRect( fbw, 0, width() - fbw, height(), Qt::black );
	}
	const int fbh = sSize.isValid() ? sSize.height() :
				m_vncConn.framebufferSize().height();
	if( fbh < height() )
	{
		p.fillRect( 0, fbh, fbw, height() - fbh, Qt::black );
	}

}




void VncView::resizeEvent( QResizeEvent *event )
{
	m_vncConn.setScaledSize( scaledSize() );

	update();

	if( m_establishingConnection )
	{
		m_establishingConnection->move( 10, 10 );
	}

	updateLocalCursor();

	QWidget::resizeEvent( event );
}




void VncView::wheelEventHandler( QWheelEvent * _we )
{
	const QPoint p = mapToFramebuffer( _we->pos() );
	m_vncConn.mouseEvent( p.x(), p.y(), m_buttonMask |
		( ( _we->delta() < 0 ) ? rfbButton5Mask : rfbButton4Mask ) );
	m_vncConn.mouseEvent( p.x(), p.y(), m_buttonMask );
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
		for( uint8_t i = 0; i < sizeof(map)/sizeof(buttonXlate); ++i )
		{
			if( _me->button() == map[i].qt )
			{
				if( _me->type() == QEvent::MouseButtonPress ||
				_me->type() == QEvent::MouseButtonDblClick )
				{
					m_buttonMask |= map[i].rfb;
				}
				else
				{
					m_buttonMask &= ~map[i].rfb;
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
		m_vncConn.mouseEvent( p.x(), p.y(), m_buttonMask );
	}
}





void VncView::updateImage(int x, int y, int w, int h)
{
	m_x = x;
	m_y = y;
	m_w = w;
	m_h = h;

	const QSize sSize = scaledSize();
	const float scale = sSize.isEmpty() ? 1 :
			(float) sSize.width() / framebufferSize().width();
	if( !sSize.isEmpty() )
	{
		m_x-=1;
		m_y-=1;
		m_w+=2;
		m_h+=2;
	}

	m_frame = m_vncConn.image();

	if( !m_initDone )
	{
		setAttribute( Qt::WA_StaticContents );
		setAttribute( Qt::WA_OpaquePaintEvent );
		installEventFilter( this );

		setMouseTracking( true ); // get mouse events even when there is no mousebutton pressed
		setFocusPolicy( Qt::WheelFocus );

		resize( sizeHint() );
		m_vncConn.setScaledSize( scaledSize() );

		emit connectionEstablished();
		m_initDone = true;

	}

	m_repaint = true;
	repaint( qRound( m_x*scale ), qRound( m_y*scale ),
			qRound( m_w*scale ), qRound( m_h*scale ) );
	m_repaint = false;
}



void VncView::updateSizeHint( int w, int h )
{
	m_framebufferSize = QSize( w, h );
	if( isScaledView() )
	{
		resize( w, h );
	}
	emit sizeHintChanged();
}


