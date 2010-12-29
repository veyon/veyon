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

/* -- macosx_opengl.c -- */

#if (defined(__MACH__) && defined(__APPLE__))

#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>

#include <rfb/rfb.h>
#if LIBVNCSERVER_HAVE_MACOSX_OPENGL_H
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#endif

extern int macosx_no_opengl, macosx_read_opengl;
extern CGDirectDisplayID displayID;

static CGLContextObj glContextObj;

int macosx_opengl_width = 0;
int macosx_opengl_height = 0;
int macosx_opengl_bpp = 0;

int macosx_opengl_get_width(void) {
	GLint viewport[4];

	glGetIntegerv(GL_VIEWPORT, viewport);
	return (int) viewport[2];
}

int macosx_opengl_get_height(void) {
	GLint viewport[4];

	glGetIntegerv(GL_VIEWPORT, viewport);
	return (int) viewport[3];
}

int macosx_opengl_get_bpp(void) {
	return 32;
}

int macosx_opengl_get_bps(void) {
	return 8;
}

int macosx_opengl_get_spp(void) {
	return 3;
}

void macosx_opengl_init(void) {
	CGLPixelFormatObj pixelFormatObj;
	GLint numPixelFormats;
	CGLPixelFormatAttribute attribs[] = {
		kCGLPFAFullScreen,
		kCGLPFADisplayMask,
		0,
		0
	};

	if (macosx_no_opengl) {
		return;
	}

	attribs[2] = CGDisplayIDToOpenGLDisplayMask(displayID);

	CGLChoosePixelFormat(attribs, &pixelFormatObj, &numPixelFormats);
	if (pixelFormatObj == NULL) {
		rfbLog("macosx_opengl_init: CGLChoosePixelFormat failed. Not using OpenGL.\n");
		return;
	}

	CGLCreateContext(pixelFormatObj, NULL, &glContextObj);
	CGLDestroyPixelFormat(pixelFormatObj);

	if (glContextObj == NULL) {
		rfbLog("macosx_opengl_init: CGLCreateContext failed. Not using OpenGL.\n");
		return;
	}

	CGLSetCurrentContext(glContextObj);
	CGLSetFullScreen(glContextObj);

	macosx_opengl_width  = macosx_opengl_get_width();
	macosx_opengl_height = macosx_opengl_get_height();

	macosx_opengl_bpp = macosx_opengl_get_bpp();

	glFinish();

	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_SKIP_ROWS, 0);
	glPixelStorei(GL_PACK_SKIP_PIXELS, 0);

	rfbLog("macosx_opengl_init: Using OpenGL for screen capture.\n");
	macosx_read_opengl = 1;
}

void macosx_opengl_fini(void) {
	if (!macosx_read_opengl) {
		return;
	}
	CGLSetCurrentContext(NULL);
	CGLClearDrawable(glContextObj);
	CGLDestroyContext(glContextObj);
}

void macosx_copy_opengl(char *dest, int x, int y, unsigned int w, unsigned int h) {
	int yflip = macosx_opengl_height - y - h; 

	CGLSetCurrentContext(glContextObj);

	glReadPixels((GLint) x, (GLint) yflip, (int) w, (int) h,
	    GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, dest);

	if (h > 1) {
		static char *pbuf = NULL;
		static int buflen = 0;
		int top = 0, bot = h - 1, rowlen = w * macosx_opengl_bpp/8;
		char *ptop, *pbot;

		if (rowlen > buflen || buflen == 0)  {
			buflen = rowlen + 128;
			if (pbuf) {
				free(pbuf);
			}
			pbuf = (char *) malloc(buflen);
		}
		while (top < bot) {
			ptop = dest + top * rowlen;
			pbot = dest + bot * rowlen;
			memcpy(pbuf, ptop, rowlen);
			memcpy(ptop, pbot, rowlen);
			memcpy(pbot, pbuf, rowlen);
			top++;
			bot--;
		}
	}
}


#else

#endif	/* __APPLE__ */

