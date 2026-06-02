/*
 * LinuxKeyboardInput.cpp - implementation of LinuxKeyboardInput class
 *
 * Copyright (c) 2019-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QThread>
#include <QProcessEnvironment>

#include "VeyonCore.h"
#include "LinuxKeyboardInput.h"

#include <X11/Xlib.h>

#include <fakekey/fakekey.h>

#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <unistd.h>


LinuxKeyboardInput::LinuxKeyboardInput() :
	m_isWaylandSession(qEnvironmentVariableIsSet("WAYLAND_DISPLAY"))
{
	if (m_isWaylandSession)
	{
		initUinput();
	}
	else
	{
		m_display = XOpenDisplay( nullptr );
		m_fakeKeyHandle = fakekey_init( m_display );
	}
}



LinuxKeyboardInput::~LinuxKeyboardInput()
{
	if( m_uinputAvailable )
	{
		cleanupUinput();
	}
	else
	{
		free( m_fakeKeyHandle );
		XCloseDisplay( m_display );
	}
}



void LinuxKeyboardInput::pressAndReleaseKey( uint32_t keysym )
{
	if( m_uinputAvailable )
	{
		const auto linuxKeycode = keysymToLinuxKeycode( keysym );
		if( linuxKeycode >= 0 )
		{
			pressAndReleaseKeyUinput( static_cast<uint32_t>( linuxKeycode ) );
		}
		return;
	}

	fakekey_press_keysym( m_fakeKeyHandle, keysym, 0 );
	fakekey_release( m_fakeKeyHandle );
}



void LinuxKeyboardInput::pressAndReleaseKey( const QByteArray& utf8Data )
{
	if( m_uinputAvailable )
	{
		pressAndReleaseKeyUinputUtf8( utf8Data );
		return;
	}

	fakekey_press( m_fakeKeyHandle, reinterpret_cast<const unsigned char *>( utf8Data.constData() ), utf8Data.size(), 0 );
	fakekey_release( m_fakeKeyHandle );
}



void LinuxKeyboardInput::sendString(const QString& string, int keyPressInterval)
{
	for( int i = 0; i < string.size(); ++i )
	{
		pressAndReleaseKey( string.mid( i, 1 ).toUtf8() );
		QThread::msleep(keyPressInterval);
	}
}



void LinuxKeyboardInput::initUinput()
{
	m_uinputFd = ::open( "/dev/uinput", O_WRONLY | O_NONBLOCK );
	if( m_uinputFd < 0 )
	{
		m_uinputFd = ::open( "/dev/input/uinput", O_WRONLY | O_NONBLOCK );
	}

	if( m_uinputFd < 0 )
	{
		vWarning() << "uinput not available";
		m_uinputAvailable = false;
		return;
	}

	::ioctl( m_uinputFd, UI_SET_EVBIT, EV_KEY );
	::ioctl( m_uinputFd, UI_SET_EVBIT, EV_SYN );
	for( int key = 0; key < KEY_MAX; ++key )
	{
		::ioctl( m_uinputFd, UI_SET_KEYBIT, key );
	}

	struct uinput_setup usetup;
	std::memset( &usetup, 0, sizeof( usetup ) );
	usetup.id.bustype = BUS_USB;
	usetup.id.vendor  = 0x1d6b;
	usetup.id.product = 0x0001;
	std::strncpy( usetup.name, "Veyon Virtual Keyboard", sizeof( usetup.name ) - 1 );

	if( ::ioctl( m_uinputFd, UI_DEV_SETUP, &usetup ) < 0 ||
		::ioctl( m_uinputFd, UI_DEV_CREATE ) < 0 )
	{
		vWarning() << "uinput create failed";
		::close( m_uinputFd );
		m_uinputFd = -1;
		m_uinputAvailable = false;
		return;
	}

	m_uinputAvailable = true;
}



void LinuxKeyboardInput::cleanupUinput()
{
	if( m_uinputFd >= 0 )
	{
		::ioctl( m_uinputFd, UI_DEV_DESTROY );
		::close( m_uinputFd );
		m_uinputFd = -1;
	}
	m_uinputAvailable = false;
}



void LinuxKeyboardInput::pressAndReleaseKeyUinput( uint32_t linuxKeycode )
{
	if( m_uinputFd < 0 )
	{
		return;
	}

	struct input_event ev;
	std::memset( &ev, 0, sizeof( ev ) );

	ev.type = EV_KEY; ev.code = linuxKeycode; ev.value = 1;
	::write( m_uinputFd, &ev, sizeof( ev ) );
	ev.type = EV_SYN; ev.code = SYN_REPORT; ev.value = 0;
	::write( m_uinputFd, &ev, sizeof( ev ) );

	ev.type = EV_KEY; ev.code = linuxKeycode; ev.value = 0;
	::write( m_uinputFd, &ev, sizeof( ev ) );
	ev.type = EV_SYN; ev.code = SYN_REPORT; ev.value = 0;
	::write( m_uinputFd, &ev, sizeof( ev ) );
}



void LinuxKeyboardInput::pressAndReleaseKeyUinputUtf8( const QByteArray& utf8Data )
{
	const auto linuxKeycode = utf8ToLinuxKeycode( utf8Data );
	if( linuxKeycode >= 0 )
	{
		pressAndReleaseKeyUinput( static_cast<uint32_t>( linuxKeycode ) );
	}
}


#define XK_LATIN1
#define XK_MISCELLANY
#define XK_XKB_KEYS
#include <X11/keysymdef.h>


int LinuxKeyboardInput::keysymToLinuxKeycode( uint32_t keysym )
{
	switch( keysym )
	{
	case XK_Escape:       return KEY_ESC;
	case XK_1:            return KEY_1;
	case XK_2:            return KEY_2;
	case XK_3:            return KEY_3;
	case XK_4:            return KEY_4;
	case XK_5:            return KEY_5;
	case XK_6:            return KEY_6;
	case XK_7:            return KEY_7;
	case XK_8:            return KEY_8;
	case XK_9:            return KEY_9;
	case XK_0:            return KEY_0;
	case XK_minus:        return KEY_MINUS;
	case XK_equal:        return KEY_EQUAL;
	case XK_BackSpace:    return KEY_BACKSPACE;
	case XK_Tab:          return KEY_TAB;
	case XK_q:            return KEY_Q;
	case XK_w:            return KEY_W;
	case XK_e:            return KEY_E;
	case XK_r:            return KEY_R;
	case XK_t:            return KEY_T;
	case XK_y:            return KEY_Y;
	case XK_u:            return KEY_U;
	case XK_i:            return KEY_I;
	case XK_o:            return KEY_O;
	case XK_p:            return KEY_P;
	case XK_bracketleft:  return KEY_LEFTBRACE;
	case XK_bracketright: return KEY_RIGHTBRACE;
	case XK_Return:       return KEY_ENTER;
	case XK_Control_L:    return KEY_LEFTCTRL;
	case XK_a:            return KEY_A;
	case XK_s:            return KEY_S;
	case XK_d:            return KEY_D;
	case XK_f:            return KEY_F;
	case XK_g:            return KEY_G;
	case XK_h:            return KEY_H;
	case XK_j:            return KEY_J;
	case XK_k:            return KEY_K;
	case XK_l:            return KEY_L;
	case XK_semicolon:    return KEY_SEMICOLON;
	case XK_apostrophe:   return KEY_APOSTROPHE;
	case XK_grave:        return KEY_GRAVE;
	case XK_Shift_L:      return KEY_LEFTSHIFT;
	case XK_backslash:    return KEY_BACKSLASH;
	case XK_z:            return KEY_Z;
	case XK_x:            return KEY_X;
	case XK_c:            return KEY_C;
	case XK_v:            return KEY_V;
	case XK_b:            return KEY_B;
	case XK_n:            return KEY_N;
	case XK_m:            return KEY_M;
	case XK_comma:        return KEY_COMMA;
	case XK_period:       return KEY_DOT;
	case XK_slash:        return KEY_SLASH;
	case XK_Shift_R:      return KEY_RIGHTSHIFT;
	case XK_KP_Multiply:  return KEY_KPASTERISK;
	case XK_Alt_L:        return KEY_LEFTALT;
	case XK_space:        return KEY_SPACE;
	case XK_Caps_Lock:    return KEY_CAPSLOCK;
	case XK_F1:           return KEY_F1;
	case XK_F2:           return KEY_F2;
	case XK_F3:           return KEY_F3;
	case XK_F4:           return KEY_F4;
	case XK_F5:           return KEY_F5;
	case XK_F6:           return KEY_F6;
	case XK_F7:           return KEY_F7;
	case XK_F8:           return KEY_F8;
	case XK_F9:           return KEY_F9;
	case XK_F10:          return KEY_F10;
	case XK_Num_Lock:     return KEY_NUMLOCK;
	case XK_Scroll_Lock:  return KEY_SCROLLLOCK;
	case XK_KP_7:         return KEY_KP7;
	case XK_KP_8:         return KEY_KP8;
	case XK_KP_9:         return KEY_KP9;
	case XK_KP_Subtract:  return KEY_KPMINUS;
	case XK_KP_4:         return KEY_KP4;
	case XK_KP_5:         return KEY_KP5;
	case XK_KP_6:         return KEY_KP6;
	case XK_KP_Add:       return KEY_KPPLUS;
	case XK_KP_1:         return KEY_KP1;
	case XK_KP_2:         return KEY_KP2;
	case XK_KP_3:         return KEY_KP3;
	case XK_KP_0:         return KEY_KP0;
	case XK_KP_Decimal:   return KEY_KPDOT;
	case XK_F11:          return KEY_F11;
	case XK_F12:          return KEY_F12;
	case XK_KP_Enter:     return KEY_KPENTER;
	case XK_Control_R:    return KEY_RIGHTCTRL;
	case XK_KP_Divide:    return KEY_KPSLASH;
	case XK_Print:        return KEY_SYSRQ;
	case XK_Alt_R:        return KEY_RIGHTALT;
	case XK_Home:         return KEY_HOME;
	case XK_Up:           return KEY_UP;
	case XK_Prior:        return KEY_PAGEUP;
	case XK_Left:         return KEY_LEFT;
	case XK_Right:        return KEY_RIGHT;
	case XK_End:          return KEY_END;
	case XK_Down:         return KEY_DOWN;
	case XK_Next:         return KEY_PAGEDOWN;
	case XK_Insert:       return KEY_INSERT;
	case XK_Delete:       return KEY_DELETE;
	case XK_Super_L:      return KEY_LEFTMETA;
	case XK_Super_R:      return KEY_RIGHTMETA;
	case XK_Menu:         return KEY_MENU;
	case XK_A:            return KEY_A;
	case XK_B:            return KEY_B;
	case XK_C:            return KEY_C;
	case XK_D:            return KEY_D;
	case XK_E:            return KEY_E;
	case XK_F:            return KEY_F;
	case XK_G:            return KEY_G;
	case XK_H:            return KEY_H;
	case XK_I:            return KEY_I;
	case XK_J:            return KEY_J;
	case XK_K:            return KEY_K;
	case XK_L:            return KEY_L;
	case XK_M:            return KEY_M;
	case XK_N:            return KEY_N;
	case XK_O:            return KEY_O;
	case XK_P:            return KEY_P;
	case XK_Q:            return KEY_Q;
	case XK_R:            return KEY_R;
	case XK_S:            return KEY_S;
	case XK_T:            return KEY_T;
	case XK_U:            return KEY_U;
	case XK_V:            return KEY_V;
	case XK_W:            return KEY_W;
	case XK_X:            return KEY_X;
	case XK_Y:            return KEY_Y;
	case XK_Z:            return KEY_Z;
	default:
		return -1;
	}
}





int LinuxKeyboardInput::utf8ToLinuxKeycode( const QByteArray& utf8Data )
{
	if( utf8Data.size() != 1 )
	{
		return -1;
	}

	const char c = utf8Data.at( 0 );

	// digit_0 maps to KEY_0, not KEY_1 + (-1)
	if( c >= '0' && c <= '9' )
	{
		constexpr int digitKeycodes[] = { 11, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
		return digitKeycodes[ c - '0' ];
	}

	// Linux keycodes for letters are NOT sequential (KEY_A=30, KEY_B=48, etc.)
	// Must use explicit lookup instead of KEY_A + offset
	constexpr int letterKeycodes[] = {
		// a b c d e f g h i j k l m n o p q r s t u v w x y z
		30,48,46,32,18,33,34,35,23,36,37,38,50,49,24,25,16,19,31,20,22,47,17,45,21,44
		// A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
		// same values as lowercase
	};

	if( c >= 'a' && c <= 'z' )
	{
		return letterKeycodes[ c - 'a' ];
	}
	if( c >= 'A' && c <= 'Z' )
	{
		return letterKeycodes[ c - 'A' ];
	}
	if( c >= '0' && c <= '9' )
	{
		return KEY_1 + ( c - '1' );
	}

	switch( c )
	{
	case '!': return KEY_1;
	case '@': return KEY_2;
	case '#': return KEY_3;
	case '$': return KEY_4;
	case '%': return KEY_5;
	case '^': return KEY_6;
	case '&': return KEY_7;
	case '*': return KEY_8;
	case '(': return KEY_9;
	case ')': return KEY_0;
	case '-': return KEY_MINUS;
	case '_': return KEY_MINUS;
	case '=': return KEY_EQUAL;
	case '+': return KEY_EQUAL;
	case '[': return KEY_LEFTBRACE;
	case '{': return KEY_LEFTBRACE;
	case ']': return KEY_RIGHTBRACE;
	case '}': return KEY_RIGHTBRACE;
	case '\\': return KEY_BACKSLASH;
	case '|': return KEY_BACKSLASH;
	case ';': return KEY_SEMICOLON;
	case ':': return KEY_SEMICOLON;
	case '\'': return KEY_APOSTROPHE;
	case '"': return KEY_APOSTROPHE;
	case '`': return KEY_GRAVE;
	case '~': return KEY_GRAVE;
	case ',': return KEY_COMMA;
	case '<': return KEY_COMMA;
	case '.': return KEY_DOT;
	case '>': return KEY_DOT;
	case '/': return KEY_SLASH;
	case '?': return KEY_SLASH;
	case ' ': return KEY_SPACE;
	case '\n': return KEY_ENTER;
	case '\t': return KEY_TAB;
	default: return -1;
	}
}
