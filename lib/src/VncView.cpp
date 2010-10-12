/*
 * VncView.cpp - VNC-viewer-widget
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#include "QtFeatures.h"

#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>



VncView::VncView( const QString & _host, QWidget * _parent,
						bool _progress_widget ) :
	QWidget( _parent ),
	m_vncConn( this ),
	m_viewOnly( true ),
	m_viewOnlyFocus( true ),
	m_scaledView( true ),
	m_initDone( false ),
	m_viewOffset( QPoint( 0, 0 ) ),
	m_buttonMask( 0 ),
	m_establishingConnection( NULL ),
	m_sysKeyTrapper( new SystemKeyTrapper( false ) )
{
/*	if( _progress_widget )
	{
		m_establishingConnection = new progressWidget(
			tr( "Establishing connection to %1 ..." ).arg( _host ),
					":/resources/watch%1.png", 16, this );
	}*/

	m_vncConn.setHost( _host );
	m_vncConn.setQuality( ItalcVncConnection::QualityHigh );
	connect( &m_vncConn, SIGNAL( imageUpdated( int, int, int, int ) ),
			this, SLOT( updateImage( int, int, int, int ) ),
						Qt::BlockingQueuedConnection );

/*	connect( m_connection, SIGNAL( cursorShapeChanged() ),
					this, SLOT( updateCursorShape() ) );*/
/*	setMouseTracking( true );
	//setWidgetAttribute( Qt::WA_OpaquePaintEvent );
	setAttribute( Qt::WA_NoSystemBackground, true );
	setAttribute( Qt::WA_DeleteOnClose, true );*/
	show();

	QSize parent_size = size();
	if( parentWidget() != NULL )
	{
		parent_size = parentWidget()->size();
	}
	resize( parent_size );

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
			event->type() == QEvent::Wheel ||
			event->type() == QEvent::MouseMove )
		return true;
	}

	return QWidget::eventFilter(obj, event);
}




QSize VncView::framebufferSize() const
{
    return m_frame.size();
}




QSize VncView::sizeHint() const
{
    return size();
}




QSize VncView::minimumSizeHint() const
{
    return size();
}




QSize VncView::scaledSize( const QSize & _default ) const
{
	const QSize s = size();
	QSize fbs = framebufferSize();//m_connection->framebufferSize();
	if( ( s.width() >= fbs.width() && s.height() >= fbs.height() ) ||
							m_scaledView == false )
	{
		return _default;
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
#ifdef ITALC_BUILD_LINUX
		// for some reason we have to grab mouse and then release
		// again to make complete keyboard-grabbing working ... ??
		grabMouse();
		releaseMouse();
#endif
		grabKeyboard();
//		m_sysKeyTrapper->setEnabled( true );
		updateCursorShape();
	}
}




void VncView::setScaledView( bool _sv )
{
	m_scaledView = _sv;
	m_vncConn.setScaledSize( scaledSize() );
	update();
}




/*void VncView::framebufferUpdate( void )
{
	if( m_connection == NULL )
	{
		QTimer::singleShot( 40, this, SLOT( framebufferUpdate() ) );
		return;
	}

	const QPoint mp = mapFromGlobal( QCursor::pos() );
	// not yet connected or connection lost while handling messages?
	if( m_connection->state() != ivsConnection::Connected && m_running )
	{
		m_running = false;
		if( m_establishingConnection )
		{
			m_establishingConnection->show();
		}
		emit startConnection();
		QTimer::singleShot( 40, this, SLOT( framebufferUpdate() ) );
		if( mp.y() < 2 )
		{
			// special signal for allowing parent-widgets to
			// show a toolbar etc.
			emit mouseAtTop();
		}
		return;
	}

	if( m_connection->state() == ivsConnection::Connected && !m_running )
	{
		if( m_establishingConnection )
		{
			m_establishingConnection->hide();
		}
		m_running = true;
		emit connectionEstablished();

		m_connection->setScaledSize( scaledSize() );

		if( parentWidget() )
		{
			// if we have a parent it's likely remoteControlWidget
			// which needs resize-events for updating its toolbar
			// properly
			parentWidget()->resize( parentWidget()->size() );
		}
	}

	if( m_scaledView == false )
	{
		// check whether to scroll because mouse-cursor is at an egde which
		// doesn't correspond to the framebuffer's edge
		const QPoint old_vo = m_viewOffset;
		const int MAGIC_MARGIN = 15;
		if( mp.x() <= MAGIC_MARGIN && m_viewOffset.x() > 0 )
		{
			m_viewOffset.setX( qMax( 0, m_viewOffset.x() -
						( MAGIC_MARGIN - mp.x() ) ) );
		}
		else if( mp.x() > width() - MAGIC_MARGIN && m_viewOffset.x() <=
				m_connection->framebufferSize().width() -
								width() )
		{
			m_viewOffset.setX( qMin( m_viewOffset.x() +
					( MAGIC_MARGIN + mp.x() - width() ),
				m_connection->framebufferSize().width() -
								width() ) );
		}

		if( mp.y() <= MAGIC_MARGIN )
		{
			if( m_viewOffset.y() > 0 )
			{
				m_viewOffset.setY( qMax( 0, m_viewOffset.y() -
						( MAGIC_MARGIN - mp.y() ) ) );
			}
			else if( mp.y() < 2 )
			{
				// special signal for allowing parent-widgets to
				// show a toolbar etc.
				emit mouseAtTop();
			}
		}
		else if( mp.y() > height() - MAGIC_MARGIN && m_viewOffset.y() <=
				m_connection->framebufferSize().height() -
								height() )
		{
			m_viewOffset.setY( qMin( m_viewOffset.y() +
					( MAGIC_MARGIN + mp.y() - height() ),
				m_connection->framebufferSize().height() -
								height() ) );
		}

		if( old_vo != m_viewOffset )
		{
			update();
		}
	}
	else if( mp.y() <= 2 )
	{
		emit mouseAtTop();
	}

	QTimer::singleShot( 40, this, SLOT( framebufferUpdate() ) );
}*/




void VncView::updateCursorShape( void )
{
/*	if( !viewOnly() && !m_connection->cursorShape().isNull() )
	{
		setCursor( QCursor( QPixmap::fromImage(
						m_connection->cursorShape() ),
			m_connection->cursorHotSpot().x(),
			m_connection->cursorHotSpot().y() ) );
	}*/
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
	m_viewOnlyFocus = viewOnly();
	if( !viewOnly() )
	{
		setViewOnly( true );
	}
	QWidget::focusOutEvent( _e );
}




// our builtin keyboard-handler
void VncView::keyEventHandler( QKeyEvent * _ke )
{
	bool pressed = _ke->type() == QEvent::KeyPress;

#ifdef NATIVE_VIRTUAL_KEY_SUPPORT
	// the Trolls seem to love us! With Qt 4.2 they introduced this cute
	// function returning the key-code of the key-event (platform-dependent)
	// so when operating under Linux/X11, key-codes are equal to the ones
	// used by VNC-protocol
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
	int key = 0;
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

	// handle modifiers
	if( key == XK_Shift_L || key == XK_Control_L || key == XK_Meta_L ||
							key == XK_Alt_L )
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
		m_vncConn.keyEvent( key, pressed );
		_ke->accept();
	}
}




void VncView::unpressModifiers( void )
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




QPoint VncView::mapToFramebuffer( const QPoint & _pos )
{
	const QSize fbs = framebufferSize();//m_connection ? m_connection->framebufferSize() :
									QSize();
	const int x = m_scaledView && fbs.isValid() ?
			_pos.x() * fbs.width() / scaledSize( fbs ).width()
		:
			_pos.x() + m_viewOffset.x();
	const int y = m_scaledView && fbs.isValid() ?
			_pos.y() * fbs.height() / scaledSize( fbs ).height()
		:
			_pos.y() + m_viewOffset.y();
	return( QPoint( x, y ) );
}




QRect VncView::mapFromFramebuffer( const QRect & _r )
{
	if( m_scaledView )
	{
		const float dx = width() / (float) framebufferSize().width();
		const float dy = height() / (float) framebufferSize().height();
		return( QRect( (int)(_r.x()*dx), (int)(_r.y()*dy),
					(int)(_r.width()*dx), (int)(_r.height()*dy) ) );
	}
	return( _r.translated( -m_viewOffset ) );
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




void VncView::paintEvent( QPaintEvent * _pe )
{
	if( m_frame.isNull() || m_frame.format() == QImage::Format_Invalid )
	{
		QWidget::paintEvent( _pe );
		return;
	}

	_pe->accept();

	QPainter p( this );

	const QSize ss = scaledSize();

	// avoid nasty through-shining-window-effect when not connected yet
/*	if( !ss.isValid() && m_frame.isNull() )
	{
		p.fillRect( _pe->rect(), Qt::black );
		return;
	}*/

	const float scale = ss.isValid() ?
			(float) ss.width() / framebufferSize().width() : 1;
	if( m_repaint )
	{
		if( ss.isValid() )
		{
			FastQImage i = m_frame;
			p.drawImage( 0, 0, m_frame.scaled( ss ) );
		}
		else
		{
			p.drawImage( QRect( m_x, m_y, m_w, m_h ),
					m_frame.copy( m_x, m_y, m_w, m_h ) );
		}
	}
	else
	{
		QRect rect = _pe->rect();
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
			p.drawImage( QPoint( 0, 0 ),
		m_frame.scaled( qRound( m_frame.width() * scale ),
					qRound( m_frame.height()*scale ),
			Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );
		}
	}
/*
	// only paint requested region of image
	p.drawImage( _pe->rect().topLeft(),
			ss.isValid() ?
				m_frame. :
						m_frame,
			_pe->rect().translated( m_viewOffset ),
			Qt::ThresholdDither );
*/
/*	if( viewOnly() && !m_connection->cursorShape().isNull() )
	{
		const QImage & cursor = m_connection->cursorShape();
		const QRect cursor_rect = mapFromFramebuffer(
			QRect( m_connection->cursorPos() -
						m_connection->cursorHotSpot(),
							cursor.size() ) );
		// parts of cursor within updated region?
		if( _pe->rect().intersects( cursor_rect ) )
		{
			// then repaint it
			p.drawImage( cursor_rect.topLeft(), cursor );
		}
	}*/

	// draw black borders if neccessary
	const int fbw = ss.isValid() ? ss.width() :
				m_vncConn.framebufferSize().width();
	if( fbw < width() )
	{
		p.fillRect( fbw, 0, width() - fbw, height(), Qt::black );
	}
	const int fbh = ss.isValid() ? ss.height() :
				m_vncConn.framebufferSize().height();
	if( fbh < height() )
	{
		p.fillRect( 0, fbh, fbw, height() - fbh, Qt::black );
	}

}




void VncView::resizeEvent( QResizeEvent * _re )
{
	m_vncConn.setScaledSize( scaledSize() );
	const int max_x = framebufferSize().width() - width();
	const int max_y = framebufferSize().height() - height();
	if( m_viewOffset.x() > max_x || m_viewOffset.y() > max_y )
	{
		m_viewOffset = QPoint(
				qMax( 0, qMin( m_viewOffset.x(), max_x ) ),
				qMax( 0, qMin( m_viewOffset.y(), max_y ) ) );
		update();
	}

	if( m_establishingConnection )
	{
		m_establishingConnection->move( 10, 10 );
	}

	QWidget::resizeEvent( _re );
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

	const QPoint p = mapToFramebuffer( _me->pos() );
	m_vncConn.mouseEvent( p.x(), p.y(), m_buttonMask );
}





void VncView::updateImage(int x, int y, int w, int h)
{
	m_x = x;
	m_y = y;
	m_w = w;
	m_h = h;

	const QSize ss = scaledSize();
	const float scale = ss.isValid() ?
			(float) ss.width() / framebufferSize().width() : 1;
	if( ss.isValid() )
	{
		m_x-=1;
		m_y-=1;
		m_w+=2;
		m_h+=2;
	}

	m_frame = m_vncConn.image();

    if (!m_initDone) {
        setAttribute(Qt::WA_StaticContents);
        setAttribute(Qt::WA_OpaquePaintEvent);
	setCursor( Qt::BlankCursor );
        installEventFilter(this);

//        setCursor(((m_dotCursorState == CursorOn) || m_forceLocalCursor) ? localDotCursor() : Qt::BlankCursor);

        setMouseTracking(true); // get mouse events even when there is no mousebutton pressed
        setFocusPolicy(Qt::WheelFocus);
//        resize(m_frame.width(), m_frame.height());
//        setStatus(Connected);
 /*       emit changeSize(m_frame.width(), m_frame.height());*/
		m_vncConn.setScaledSize( scaledSize() );
        emit connectionEstablished();
        m_initDone = true;

    }

/*    if (!m_scale && (y == 0 && x == 0) && (m_frame.size() != size())) {
        resize(m_frame.width(), m_frame.height());
        emit changeSize(m_frame.width(), m_frame.height());
    }*/

	m_repaint = true;
	repaint( qRound( m_x*scale ), qRound( m_y*scale ),
			qRound( m_w*scale ), qRound( m_h*scale ) );
	m_repaint = false;
}


