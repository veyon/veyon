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

#include <cstring>

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
	(void)!::write( m_uinputFd, &ev, sizeof( ev ) );
	ev.type = EV_SYN; ev.code = SYN_REPORT; ev.value = 0;
	(void)!::write( m_uinputFd, &ev, sizeof( ev ) );

	ev.type = EV_KEY; ev.code = linuxKeycode; ev.value = 0;
	(void)!::write( m_uinputFd, &ev, sizeof( ev ) );
	ev.type = EV_SYN; ev.code = SYN_REPORT; ev.value = 0;
	(void)!::write( m_uinputFd, &ev, sizeof( ev ) );
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
	static constexpr struct { uint32_t keysym; int keycode; } table[] = {
		{ XK_Escape,       KEY_ESC },
		{ XK_1,            KEY_1 },
		{ XK_2,            KEY_2 },
		{ XK_3,            KEY_3 },
		{ XK_4,            KEY_4 },
		{ XK_5,            KEY_5 },
		{ XK_6,            KEY_6 },
		{ XK_7,            KEY_7 },
		{ XK_8,            KEY_8 },
		{ XK_9,            KEY_9 },
		{ XK_0,            KEY_0 },
		{ XK_minus,        KEY_MINUS },
		{ XK_equal,        KEY_EQUAL },
		{ XK_BackSpace,    KEY_BACKSPACE },
		{ XK_Tab,          KEY_TAB },
		{ XK_q,            KEY_Q },
		{ XK_w,            KEY_W },
		{ XK_e,            KEY_E },
		{ XK_r,            KEY_R },
		{ XK_t,            KEY_T },
		{ XK_y,            KEY_Y },
		{ XK_u,            KEY_U },
		{ XK_i,            KEY_I },
		{ XK_o,            KEY_O },
		{ XK_p,            KEY_P },
		{ XK_bracketleft,  KEY_LEFTBRACE },
		{ XK_bracketright, KEY_RIGHTBRACE },
		{ XK_Return,       KEY_ENTER },
		{ XK_Control_L,    KEY_LEFTCTRL },
		{ XK_a,            KEY_A },
		{ XK_s,            KEY_S },
		{ XK_d,            KEY_D },
		{ XK_f,            KEY_F },
		{ XK_g,            KEY_G },
		{ XK_h,            KEY_H },
		{ XK_j,            KEY_J },
		{ XK_k,            KEY_K },
		{ XK_l,            KEY_L },
		{ XK_semicolon,    KEY_SEMICOLON },
		{ XK_apostrophe,   KEY_APOSTROPHE },
		{ XK_grave,        KEY_GRAVE },
		{ XK_Shift_L,      KEY_LEFTSHIFT },
		{ XK_backslash,    KEY_BACKSLASH },
		{ XK_z,            KEY_Z },
		{ XK_x,            KEY_X },
		{ XK_c,            KEY_C },
		{ XK_v,            KEY_V },
		{ XK_b,            KEY_B },
		{ XK_n,            KEY_N },
		{ XK_m,            KEY_M },
		{ XK_comma,        KEY_COMMA },
		{ XK_period,       KEY_DOT },
		{ XK_slash,        KEY_SLASH },
		{ XK_Shift_R,      KEY_RIGHTSHIFT },
		{ XK_KP_Multiply,  KEY_KPASTERISK },
		{ XK_Alt_L,        KEY_LEFTALT },
		{ XK_space,        KEY_SPACE },
		{ XK_Caps_Lock,    KEY_CAPSLOCK },
		{ XK_F1,           KEY_F1 },
		{ XK_F2,           KEY_F2 },
		{ XK_F3,           KEY_F3 },
		{ XK_F4,           KEY_F4 },
		{ XK_F5,           KEY_F5 },
		{ XK_F6,           KEY_F6 },
		{ XK_F7,           KEY_F7 },
		{ XK_F8,           KEY_F8 },
		{ XK_F9,           KEY_F9 },
		{ XK_F10,          KEY_F10 },
		{ XK_Num_Lock,     KEY_NUMLOCK },
		{ XK_Scroll_Lock,  KEY_SCROLLLOCK },
		{ XK_KP_7,         KEY_KP7 },
		{ XK_KP_8,         KEY_KP8 },
		{ XK_KP_9,         KEY_KP9 },
		{ XK_KP_Subtract,  KEY_KPMINUS },
		{ XK_KP_4,         KEY_KP4 },
		{ XK_KP_5,         KEY_KP5 },
		{ XK_KP_6,         KEY_KP6 },
		{ XK_KP_Add,       KEY_KPPLUS },
		{ XK_KP_1,         KEY_KP1 },
		{ XK_KP_2,         KEY_KP2 },
		{ XK_KP_3,         KEY_KP3 },
		{ XK_KP_0,         KEY_KP0 },
		{ XK_KP_Decimal,   KEY_KPDOT },
		{ XK_F11,          KEY_F11 },
		{ XK_F12,          KEY_F12 },
		{ XK_KP_Enter,     KEY_KPENTER },
		{ XK_Control_R,    KEY_RIGHTCTRL },
		{ XK_KP_Divide,    KEY_KPSLASH },
		{ XK_Print,        KEY_SYSRQ },
		{ XK_Alt_R,        KEY_RIGHTALT },
		{ XK_Home,         KEY_HOME },
		{ XK_Up,           KEY_UP },
		{ XK_Prior,        KEY_PAGEUP },
		{ XK_Left,         KEY_LEFT },
		{ XK_Right,        KEY_RIGHT },
		{ XK_End,          KEY_END },
		{ XK_Down,         KEY_DOWN },
		{ XK_Next,         KEY_PAGEDOWN },
		{ XK_Insert,       KEY_INSERT },
		{ XK_Delete,       KEY_DELETE },
		{ XK_Super_L,      KEY_LEFTMETA },
		{ XK_Super_R,      KEY_RIGHTMETA },
		{ XK_Menu,         KEY_MENU },
		{ XK_A,            KEY_A },
		{ XK_B,            KEY_B },
		{ XK_C,            KEY_C },
		{ XK_D,            KEY_D },
		{ XK_E,            KEY_E },
		{ XK_F,            KEY_F },
		{ XK_G,            KEY_G },
		{ XK_H,            KEY_H },
		{ XK_I,            KEY_I },
		{ XK_J,            KEY_J },
		{ XK_K,            KEY_K },
		{ XK_L,            KEY_L },
		{ XK_M,            KEY_M },
		{ XK_N,            KEY_N },
		{ XK_O,            KEY_O },
		{ XK_P,            KEY_P },
		{ XK_Q,            KEY_Q },
		{ XK_R,            KEY_R },
		{ XK_S,            KEY_S },
		{ XK_T,            KEY_T },
		{ XK_U,            KEY_U },
		{ XK_V,            KEY_V },
		{ XK_W,            KEY_W },
		{ XK_X,            KEY_X },
		{ XK_Y,            KEY_Y },
		{ XK_Z,            KEY_Z },
	};

	for( const auto& entry : table )
	{
		if( entry.keysym == keysym )
		{
			return entry.keycode;
		}
	}

	return -1;
}





int LinuxKeyboardInput::utf8ToLinuxKeycode( const QByteArray& utf8Data )
{
	if( utf8Data.size() != 1 )
	{
		return -1;
	}

	const auto c = utf8Data.at( 0 );

	// Fast path for alphanumeric ASCII
	if( c >= 'a' && c <= 'z' )
	{
		static constexpr int letterKeycodes[] = {
		// a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  p  q  r  s  t  u  v  w  x  y  z
			30,48,46,32,18,33,34,35,23,36,37,38,50,49,24,25,16,19,31,20,22,47,17,45,21,44
		};
		return letterKeycodes[ c - 'a' ];
	}
	if( c >= 'A' && c <= 'Z' )
	{
		static constexpr int letterKeycodes[] = {
		// A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z
			30,48,46,32,18,33,34,35,23,36,37,38,50,49,24,25,16,19,31,20,22,47,17,45,21,44
		};
		return letterKeycodes[ c - 'A' ];
	}

	static constexpr struct { char ch; int keycode; } table[] = {
		{ '0', KEY_0 },
		{ '1', KEY_1 },
		{ '2', KEY_2 },
		{ '3', KEY_3 },
		{ '4', KEY_4 },
		{ '5', KEY_5 },
		{ '6', KEY_6 },
		{ '7', KEY_7 },
		{ '8', KEY_8 },
		{ '9', KEY_9 },
		{ '!', KEY_1 },
		{ '@', KEY_2 },
		{ '#', KEY_3 },
		{ '$', KEY_4 },
		{ '%', KEY_5 },
		{ '^', KEY_6 },
		{ '&', KEY_7 },
		{ '*', KEY_8 },
		{ '(', KEY_9 },
		{ ')', KEY_0 },
		{ '-', KEY_MINUS },
		{ '_', KEY_MINUS },
		{ '=', KEY_EQUAL },
		{ '+', KEY_EQUAL },
		{ '[', KEY_LEFTBRACE },
		{ '{', KEY_LEFTBRACE },
		{ ']', KEY_RIGHTBRACE },
		{ '}', KEY_RIGHTBRACE },
		{ '\\', KEY_BACKSLASH },
		{ '|', KEY_BACKSLASH },
		{ ';', KEY_SEMICOLON },
		{ ':', KEY_SEMICOLON },
		{ '\'', KEY_APOSTROPHE },
		{ '"', KEY_APOSTROPHE },
		{ '`', KEY_GRAVE },
		{ '~', KEY_GRAVE },
		{ ',', KEY_COMMA },
		{ '<', KEY_COMMA },
		{ '.', KEY_DOT },
		{ '>', KEY_DOT },
		{ '/', KEY_SLASH },
		{ '?', KEY_SLASH },
		{ ' ', KEY_SPACE },
		{ '\n', KEY_ENTER },
		{ '\t', KEY_TAB },
	};

	for( const auto& entry : table )
	{
		if( entry.ch == c )
		{
			return entry.keycode;
		}
	}

	return -1;
}
