/*
   Copyright (C) 2002-2010 Karl J. Runge <runge@karlrunge.com> 
   All rights reserved.

This file is part of x11vnc.

x11vnc is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

x11vnc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with x11vnc; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
or see <http://www.gnu.org/licenses/>.

In addition, as a special exception, Karl J. Runge
gives permission to link the code of its release of x11vnc with the
OpenSSL project's "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.
*/

#ifndef _X11VNC_PARAMS_H
#define _X11VNC_PARAMS_H

/* -- params.h -- */

#define ICON_MODE_SOCKS 16

/* had lw=3 for a long time */
#ifndef WIREFRAME_PARMS
#define WIREFRAME_PARMS "0xff,2,0,32+8+8+8,all,0.15+0.30+5.0+0.125"
#endif

#ifndef SCROLL_COPYRECT_PARMS
#define SCROLL_COPYRECT_PARMS "0+64+32+32,0.02+0.10+0.9,0.03+0.06+0.5+0.1+5.0"
#endif

#ifndef POLL_8TO24_DELAY
#define POLL_8TO24_DELAY 0.05
#endif

#define LATENCY0 20     /* 20ms */
#define NETRATE0 20     /* 20KB/sec */

#define POINTER_MODE_NOFB 2

/* scan pattern jitter from x0rfbserver */
#define NSCAN 32

#define FB_COPY 0x1
#define FB_MOD  0x2
#define FB_REQ  0x4

#define VNC_CONNECT_MAX 16384
#define X11VNC_REMOTE_MAX 65536
#define PROP_MAX (262144L)

#define MAXN 256

#define PIPEINPUT_NONE		0x0
#define PIPEINPUT_VID		0x1
#define PIPEINPUT_CONSOLE	0x2
#define PIPEINPUT_UINPUT	0x3
#define PIPEINPUT_MACOSX	0x4
#define PIPEINPUT_VNC		0x5

#define MAX_BUTTONS 5

#define ROTATE_NONE		0
#define ROTATE_X		1
#define ROTATE_Y		2
#define ROTATE_XY		3
#define ROTATE_90		4
#define ROTATE_90X		5
#define ROTATE_90Y		6
#define ROTATE_270		7

#define VENCRYPT_NONE		0
#define VENCRYPT_SUPPORT	1
#define VENCRYPT_SOLE		2
#define VENCRYPT_FORCE		3

#define VENCRYPT_BOTH		0
#define VENCRYPT_NODH		1
#define VENCRYPT_NOX509		2

#define ANONTLS_NONE		0
#define ANONTLS_SUPPORT		1
#define ANONTLS_SOLE		2
#define ANONTLS_FORCE		3

#endif /* _X11VNC_PARAMS_H */
