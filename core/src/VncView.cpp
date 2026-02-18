/*
 * VncView.cpp - abstract base for all VNC views
 *
 * Copyright (c) 2006-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <rfb/keysym.h>
#include <rfb/rfbproto.h>

#include <array>

#include <QCursor>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QtMath>

#include "PlatformInputDeviceFunctions.h"
#include "KeyboardShortcutTrapper.h"
#include "VeyonConnection.h"
#include "VncConnection.h"
#include "VncView.h"


VncView::VncView( const ComputerControlInterface::Pointer& computerControlInterface ) :
	m_computerControlInterface( [computerControlInterface]() {
		if( computerControlInterface->state() == ComputerControlInterface::State::Disconnected ||
			computerControlInterface->connection() == nullptr )
		{
			computerControlInterface->start();
		}
		return computerControlInterface;
	}() ),
	m_previousUpdateMode( m_computerControlInterface->updateMode() ),
	m_connection( computerControlInterface->connection()->vncConnection() ),
	m_framebufferSize( m_connection->image().size() ),
	m_keyboardShortcutTrapper( VeyonCore::platform().inputDeviceFunctions().createKeyboardShortcutTrapper( nullptr ) )
{
	// handle/forward trapped keyboard shortcuts
	QObject::connect( m_keyboardShortcutTrapper, &KeyboardShortcutTrapper::shortcutTrapped,
					  m_keyboardShortcutTrapper, [this]( KeyboardShortcutTrapper::Shortcut shortcut ) {
						  handleShortcut( shortcut );
					  } );

	m_computerControlInterface->setUpdateMode( ComputerControlInterface::UpdateMode::Live );
}



VncView::~VncView()
{
	unpressAllModifiers();

	m_computerControlInterface->setUpdateMode( m_previousUpdateMode );

	delete m_keyboardShortcutTrapper;
}



QSize VncView::scaledSize() const
{
	if( isScaledView() == false )
	{
		return effectiveFramebufferSize();
	}

	return effectiveFramebufferSize().scaled( viewSize(), Qt::KeepAspectRatio );
}



QSize VncView::effectiveFramebufferSize() const
{
	const auto viewportSize = m_viewport.size();

	if( viewportSize.isEmpty() == false )
	{
		return viewportSize;
	}

	return m_framebufferSize;
}



void VncView::setViewport(QRect viewport)
{
	if( m_viewport != viewport )
	{
		m_viewport = viewport;

		updateGeometry();
	}
}



void VncView::setViewOnly( bool viewOnly )
{
	if( viewOnly == m_viewOnly )
	{
		return;
	}

	m_viewOnly = viewOnly;

	if( m_connection )
	{
		m_connection->setUseRemoteCursor( !viewOnly );
	}

	if( m_viewOnly )
	{
		m_keyboardShortcutTrapper->setEnabled( false );
	}
	else
	{
		m_keyboardShortcutTrapper->setEnabled( true );
	}

	updateLocalCursor();
}



void VncView::sendShortcut( VncView::Shortcut shortcut )
{
	if( viewOnly() )
	{
		return;
	}

	unpressAllModifiers();

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
		pressKey( XK_Super_L );
		pressKey( XK_Tab );
		unpressKey( XK_Tab );
		unpressKey( XK_Super_L );
		break;
	case ShortcutWin:
		pressKey( XK_Super_L );
		unpressKey( XK_Super_L );
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
		vWarning() << "unknown shortcut" << static_cast<int>( shortcut );
		break;
	}
}



bool VncView::isScaledView() const
{
	return viewSize().width() < effectiveFramebufferSize().width() ||
		   viewSize().height() < effectiveFramebufferSize().height();
}



qreal VncView::scaleFactor() const
{
	if( isScaledView() )
	{
		return qreal( scaledSize().width() ) / effectiveFramebufferSize().width();
	}

	return 1;
}



QPoint VncView::mapToFramebuffer( QPoint pos )
{
	if( effectiveFramebufferSize().isEmpty() )
	{
		return { 0, 0 };
	}

	return { pos.x() * effectiveFramebufferSize().width() / scaledSize().width() + viewport().x(),
			 pos.y() * effectiveFramebufferSize().height() / scaledSize().height() + viewport().y() };
}



QRect VncView::mapFromFramebuffer( QRect r )
{
	if( effectiveFramebufferSize().isEmpty() )
	{
		return {};
	}

	r.translate( -viewport().x(), -viewport().y() );

	const auto dx = scaledSize().width() / qreal( effectiveFramebufferSize().width() );
	const auto dy = scaledSize().height() / qreal( effectiveFramebufferSize().height() );

	return { int(r.x()*dx), int(r.y()*dy),
			int(r.width()*dx), int(r.height()*dy) };
}



void VncView::updateCursorShape( const QPixmap& cursorShape, int xh, int yh )
{
	const auto scale = scaleFactor();

	m_cursorHot = { int( xh*scale ), int( yh*scale ) };
	m_cursorShape = cursorShape.scaled( int( cursorShape.width()*scale ),
										int( cursorShape.height()*scale ),
										Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

	updateLocalCursor();
}



void VncView::updateFramebufferSize( int w, int h )
{
	m_framebufferSize = QSize( w, h );

	updateGeometry();
}



void VncView::updateImage( int x, int y, int w, int h )
{
	x -= viewport().x();
	y -= viewport().y();

	const auto scale = scaleFactor();
	updateView( qMax( 0, qFloor( x*scale - 1 ) ), qMax( 0, qFloor( y*scale - 1 ) ),
				qCeil( w*scale + 2 ), qCeil( h*scale + 2 ) );
}



void VncView::pressModifiers(const QList<KeyCode>& modifierKeyCodes)
{
	for(auto keyCode : modifierKeyCodes)
	{
		m_modifierKeys[keyCode] = true;
		m_connection->keyEvent(keyCode, true);
	}
}



void VncView::unpressModifiers(const QList<KeyCode>& modifierKeyCodes)
{
	for(auto keyCode : modifierKeyCodes)
	{
		m_modifierKeys.remove(keyCode);
		m_connection->keyEvent(keyCode, false);
	}
}



void VncView::unpressAllModifiers()
{
	const auto keys = m_modifierKeys.keys();
	for( auto key : keys )
	{
		m_connection->keyEvent( key, false );
	}
	m_modifierKeys.clear();
}



void VncView::handleShortcut( KeyboardShortcutTrapper::Shortcut shortcut )
{
	KeyCode key = 0;

	switch( shortcut )
	{
	case KeyboardShortcutTrapper::SuperKeyDown:
		pressModifiers({XK_Super_L});
		break;
	case KeyboardShortcutTrapper::SuperKeyUp:
		unpressModifiers({XK_Super_L});
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
		m_connection->keyEvent( key, true );
		m_connection->keyEvent( key, false );
	}
}



bool VncView::handleEvent( QEvent* event )
{
	switch( event->type() )
	{
	case QEvent::KeyPress:
	case QEvent::KeyRelease:
		keyEventHandler( dynamic_cast<QKeyEvent*>( event ) );
		return true;
	case QEvent::HoverMove:
		hoverEventHandler( dynamic_cast<QHoverEvent *>( event ) );
		return true;
	case QEvent::MouseButtonDblClick:
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonRelease:
	case QEvent::MouseMove:
		mouseEventHandler( dynamic_cast<QMouseEvent*>( event ) );
		return true;
	case QEvent::Wheel:
		wheelEventHandler( dynamic_cast<QWheelEvent*>( event ) );
		return true;
	case QEvent::FocusOut:
		unpressAllModifiers();
		break;
	default:
		break;
	}

	return false;
}



void VncView::hoverEventHandler( QHoverEvent* event )
{
	if( event && m_viewOnly == false )
	{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		const auto pos = mapToFramebuffer(event->position().toPoint());
#else
		const auto pos = mapToFramebuffer(event->pos());
#endif
		m_connection->mouseEvent( pos.x(), pos.y(), m_buttonMask );
	}
}



void VncView::keyEventHandler( QKeyEvent* event )
{
	if( event == nullptr )
	{
		return;
	}

	const auto pressed = event->type() == QEvent::KeyPress;

#ifdef Q_OS_LINUX
	// on Linux/X11 native key codes are equal to the ones used by RFB protocol
	KeyCode key = event->nativeVirtualKey();

	// we do not handle Key_Backtab separately as the Shift-modifier
	// is already enabled
	if( event->key() == Qt::Key_Backtab )
	{
		key = XK_Tab;
	}
#else
	// translate Qt key codes to X keycodes
	static const auto qtKeyToXKey = [](int qtKey) -> KeyCode {
		switch (qtKey)
		{
		// modifiers are handled separately
		case Qt::Key_Shift: return XK_Shift_L;
		case Qt::Key_Control: return XK_Control_L;
		case Qt::Key_Meta: return XK_Super_L;
		case Qt::Key_Alt: return XK_Alt_L;
		case Qt::Key_Escape: return XK_Escape;
		case Qt::Key_Tab: return XK_Tab;
		case Qt::Key_Backtab: return XK_Tab;
		case Qt::Key_Backspace: return XK_BackSpace;
		case Qt::Key_Return: return XK_Return;
		case Qt::Key_Insert: return XK_Insert;
		case Qt::Key_Delete: return XK_Delete;
		case Qt::Key_Pause: return XK_Pause;
		case Qt::Key_Print: return XK_Print;
		case Qt::Key_Home: return XK_Home;
		case Qt::Key_End: return XK_End;
		case Qt::Key_Left: return XK_Left;
		case Qt::Key_Up: return XK_Up;
		case Qt::Key_Right: return XK_Right;
		case Qt::Key_Down: return XK_Down;
		case Qt::Key_PageUp: return XK_Prior;
		case Qt::Key_PageDown: return XK_Next;
		case Qt::Key_CapsLock: return XK_Caps_Lock;
		case Qt::Key_NumLock: return XK_Num_Lock;
		case Qt::Key_ScrollLock: return XK_Scroll_Lock;
		case Qt::Key_Super_L: return XK_Super_L;
		case Qt::Key_Super_R: return XK_Super_R;
		case Qt::Key_Menu: return XK_Menu;
		case Qt::Key_Hyper_L: return XK_Hyper_L;
		case Qt::Key_Hyper_R: return XK_Hyper_R;
		case Qt::Key_Help: return XK_Help;
		case Qt::Key_Multi_key: return XK_Multi_key;
		case Qt::Key_SingleCandidate: return XK_SingleCandidate;
		case Qt::Key_MultipleCandidate: return XK_MultipleCandidate;
		case Qt::Key_PreviousCandidate: return XK_PreviousCandidate;
		case Qt::Key_Mode_switch: return XK_Mode_switch;
		case Qt::Key_Kanji: return XK_Kanji;
		case Qt::Key_Muhenkan: return XK_Muhenkan;
		case Qt::Key_Henkan: return XK_Henkan;
		case Qt::Key_Romaji: return XK_Romaji;
		case Qt::Key_Hiragana: return XK_Hiragana;
		case Qt::Key_Katakana: return XK_Katakana;
		case Qt::Key_Hiragana_Katakana: return XK_Hiragana_Katakana;
		case Qt::Key_Zenkaku: return XK_Zenkaku;
		case Qt::Key_Hankaku: return XK_Hankaku;
		case Qt::Key_Zenkaku_Hankaku: return XK_Zenkaku_Hankaku;
		case Qt::Key_Touroku: return XK_Touroku;
		case Qt::Key_Massyo: return XK_Massyo;
		case Qt::Key_Kana_Lock: return XK_Kana_Lock;
		case Qt::Key_Kana_Shift: return XK_Kana_Shift;
		case Qt::Key_Eisu_Shift: return XK_Eisu_Shift;
		case Qt::Key_Eisu_toggle: return XK_Eisu_toggle;
		case Qt::Key_Hangul: return XK_Hangul;
		case Qt::Key_Hangul_Start: return XK_Hangul_Start;
		case Qt::Key_Hangul_End: return XK_Hangul_End;
		case Qt::Key_Hangul_Hanja: return XK_Hangul_Hanja;
		case Qt::Key_Hangul_Jamo: return XK_Hangul_Jamo;
		case Qt::Key_Hangul_Romaja: return XK_Hangul_Romaja;
		case Qt::Key_Hangul_Jeonja: return XK_Hangul_Jeonja;
		case Qt::Key_Hangul_Banja: return XK_Hangul_Banja;
		case Qt::Key_Hangul_PreHanja: return XK_Hangul_PreHanja;
		case Qt::Key_Hangul_PostHanja: return XK_Hangul_PostHanja;
		case Qt::Key_Hangul_Special: return XK_Hangul_Special;
		case Qt::Key_Dead_Grave: return XK_dead_grave;
		case Qt::Key_Dead_Acute: return XK_dead_acute;
		case Qt::Key_Dead_Circumflex: return XK_dead_circumflex;
		case Qt::Key_Dead_Tilde: return XK_dead_tilde;
		case Qt::Key_Dead_Macron: return XK_dead_macron;
		case Qt::Key_Dead_Breve: return XK_dead_breve;
		case Qt::Key_Dead_Abovedot: return XK_dead_abovedot;
		case Qt::Key_Dead_Diaeresis: return XK_dead_diaeresis;
		case Qt::Key_Dead_Abovering: return XK_dead_abovering;
		case Qt::Key_Dead_Doubleacute: return XK_dead_doubleacute;
		case Qt::Key_Dead_Caron: return XK_dead_caron;
		case Qt::Key_Dead_Cedilla: return XK_dead_cedilla;
		case Qt::Key_Dead_Ogonek: return XK_dead_ogonek;
		case Qt::Key_Dead_Iota: return XK_dead_iota;
		case Qt::Key_Dead_Voiced_Sound: return XK_dead_voiced_sound;
		case Qt::Key_Dead_Semivoiced_Sound: return XK_dead_semivoiced_sound;
		case Qt::Key_Dead_Belowdot: return XK_dead_belowdot;
		default:
			break;
		}

		if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F35)
		{
			return XK_F1 + qtKey - Qt::Key_F1;
		}

		return 0;
	};

	auto key = qtKeyToXKey(event->key());

	if (key == 0)
	{
		const auto text = event->text();
		if (auto keySequence = QKeySequence(event->key()).toString();
			m_modifierKeys.size() == 1 && m_modifierKeys.contains(XK_Control_L) && keySequence.length() == 1)
		{
			if (!m_modifierKeys.contains(XK_Shift_L))
			{
				keySequence = keySequence.toLower();
			}
			key = keySequence.at(0).unicode();
		}
		else
		{
			const auto ch = text.isEmpty() ? QChar(0) : text.at(0);

			if (ch.unicode() > 0 && ch.unicode() <= 0x00FF)
			{
				key = ch.unicode();
			}
			else if (ch.unicode() >= 0x0100)
			{
				key = 0x01000000 | ch.unicode();
			}
		}

		const auto mods = event->modifiers();
		const auto isAltCtrl = (mods & Qt::GroupSwitchModifier)
							 || ((mods & Qt::AltModifier) && (mods & Qt::ControlModifier))
							 || (m_modifierKeys.contains(XK_Alt_L) && m_modifierKeys.contains(XK_Control_L));
		if (isAltCtrl && key > 0)
		{
			unpressModifiers({XK_Alt_L, XK_Control_L});
		}
	}
#endif

	// handle Ctrl+Alt+Del replacement (Meta/Super key+Del)
	if ((m_modifierKeys.contains(XK_Super_L) ||
		 m_modifierKeys.contains(XK_Super_R) ||
		 m_modifierKeys.contains(XK_Meta_L)) &&
		event->key() == Qt::Key_Delete)
	{
		if( pressed )
		{
			unpressAllModifiers();
			m_connection->keyEvent( XK_Control_L, true );
			m_connection->keyEvent( XK_Alt_L, true );
			m_connection->keyEvent( XK_Delete, true );
			m_connection->keyEvent( XK_Delete, false );
			m_connection->keyEvent( XK_Alt_L, false );
			m_connection->keyEvent( XK_Control_L, false );
			key = 0;
		}
	}

	// handle modifiers
	switch (key)
	{
	case XK_ISO_Level3_Shift:
		if (pressed)
		{
			m_modifierKeys[XK_Alt_L] = true;
			m_modifierKeys[XK_Control_L] = true;
		}
		else
		{
			m_modifierKeys.remove(XK_Alt_L);
			m_modifierKeys.remove(XK_Control_L);
		}
		key = 0;
		break;
	case XK_Caps_Lock:
	case XK_Num_Lock:
		key = 0;
		break;
	case XK_Shift_L:
	case XK_Control_L:
	case XK_Meta_L:
	case XK_Meta_R:
	case XK_Alt_L:
	case XK_Super_L:
	case XK_Super_R:
		if (pressed)
		{
			m_modifierKeys[key] = true;
		}
		else
		{
			m_modifierKeys.remove(key);
		}
		break;
	}

	if (key)
	{
		m_connection->keyEvent(key, pressed);
	}

	event->accept();
}



void VncView::mouseEventHandler( QMouseEvent* event )
{
	if( event == nullptr || m_viewOnly )
	{
		return;
	}

	struct ButtonTranslation
	{
		Qt::MouseButton qt;
		int rfb;
	};

	static constexpr std::array<ButtonTranslation, 3> buttonTranslationMap{ {
		{ Qt::LeftButton, rfbButton1Mask },
		{ Qt::MiddleButton, rfbButton2Mask },
		{ Qt::RightButton, rfbButton3Mask }
	} };

	if( event->type() != QEvent::MouseMove )
	{
		for( const auto& i : buttonTranslationMap )
		{
			if( event->button() == i.qt )
			{
				if( event->type() == QEvent::MouseButtonPress ||
					event->type() == QEvent::MouseButtonDblClick )
				{
					m_buttonMask |= uint(i.rfb);
				}
				else
				{
					m_buttonMask &= ~uint(i.rfb);
				}
			}
		}
	}

	const auto pos = mapToFramebuffer( event->pos() );
	m_connection->mouseEvent( pos.x(), pos.y(), m_buttonMask );
}



void VncView::wheelEventHandler( QWheelEvent* event )
{
	if( event == nullptr )
	{
		return;
	}

	const auto p = mapToFramebuffer( event->position().toPoint() );
	const uint scrollButtonMask = ( event->angleDelta().y() < 0 ) ? rfbButton5Mask : rfbButton4Mask;
	m_connection->mouseEvent( p.x(), p.y(), m_buttonMask | scrollButtonMask );
	m_connection->mouseEvent( p.x(), p.y(), m_buttonMask );
}



void VncView::updateLocalCursor()
{
	if( m_cursorShape.isNull() == false && viewOnly() == false )
	{
		setViewCursor( QCursor( m_cursorShape, m_cursorHot.x(), m_cursorHot.y() ) );
	}
	else
	{
		setViewCursor( Qt::ArrowCursor );
	}
}



void VncView::pressKey(KeyCode key)
{
	m_connection->keyEvent( key, true );
}



void VncView::unpressKey(KeyCode key)
{
	m_connection->keyEvent( key, false );
}
