#undef  LIBVNCSERVER_HAVE_XSHM
#define LIBVNCSERVER_HAVE_XSHM 0
#undef  LIBVNCSERVER_HAVE_XTEST
#define LIBVNCSERVER_HAVE_XTEST 0
#undef  LIBVNCSERVER_HAVE_XTESTGRABCONTROL
#define LIBVNCSERVER_HAVE_XTESTGRABCONTROL 0
#undef  LIBVNCSERVER_HAVE_XKEYBOARD
#define LIBVNCSERVER_HAVE_XKEYBOARD 0
#undef  LIBVNCSERVER_HAVE_LIBXINERAMA
#define LIBVNCSERVER_HAVE_LIBXINERAMA 0
#undef  LIBVNCSERVER_HAVE_LIBXRANDR
#define LIBVNCSERVER_HAVE_LIBXRANDR 0
#undef  LIBVNCSERVER_HAVE_LIBXFIXES
#define LIBVNCSERVER_HAVE_LIBXFIXES 0
#undef  LIBVNCSERVER_HAVE_LIBXDAMAGE
#define LIBVNCSERVER_HAVE_LIBXDAMAGE 0
#undef  LIBVNCSERVER_HAVE_RECORD
#define LIBVNCSERVER_HAVE_RECORD 0
#undef  LIBVNCSERVER_HAVE_LIBXTRAP
#define LIBVNCSERVER_HAVE_LIBXTRAP 0
#undef  LIBVNCSERVER_HAVE_SOLARIS_XREADSCREEN
#define LIBVNCSERVER_HAVE_SOLARIS_XREADSCREEN 0
#undef  LIBVNCSERVER_HAVE_IRIX_XREADDISPLAY
#define LIBVNCSERVER_HAVE_IRIX_XREADDISPLAY 0
#undef  LIBVNCSERVER_HAVE_FBPM
#define LIBVNCSERVER_HAVE_FBPM 0

/* default keysyms */
#if 0
/* XXX go with the subset in rfb/keysym.h for now */
#define XK_MISCELLANY
#define XK_XKB_KEYS
#define XK_LATIN1
#define XK_LATIN2
#define XK_LATIN3
#define XK_LATIN4
#define XK_LATIN8
#define XK_LATIN9
#define XK_CAUCASUS
#define XK_GREEK
#define XK_KATAKANA
#define XK_ARABIC
#define XK_CYRILLIC
#define XK_HEBREW
#define XK_THAI
#define XK_KOREAN
#define XK_ARMENIAN
#define XK_GEORGIAN
#define XK_VIETNAMESE
#define XK_CURRENCY
#endif

/*
 *	$Xorg: X.h,v 1.4 2001/02/09 02:03:22 xorgcvs Exp $
 */

/* Definitions for the X window system likely to be used by applications */

#ifndef X_H
#define X_H

/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $XFree86: xc/include/X.h,v 1.5 2001/12/14 19:53:25 dawes Exp $ */

#define X_PROTOCOL	11		/* current protocol version */
#define X_PROTOCOL_REVISION 0		/* current minor version */

/* Resources */

/*
 * _XSERVER64 must ONLY be defined when compiling X server sources on
 * systems where unsigned long is not 32 bits, must NOT be used in
 * client or library code.
 */
#ifndef _XSERVER64
#  ifndef _XTYPEDEF_XID
#    define _XTYPEDEF_XID
typedef unsigned long XID;
#  endif
#  ifndef _XTYPEDEF_MASK
#    define _XTYPEDEF_MASK
typedef unsigned long Mask;
#  endif
#  ifndef _XTYPEDEF_ATOM
#    define _XTYPEDEF_ATOM
typedef unsigned long Atom;		/* Also in Xdefs.h */
#  endif
typedef unsigned long VisualID;
typedef unsigned long Time;
#else
#  include <X11/Xmd.h>
#  ifndef _XTYPEDEF_XID
#    define _XTYPEDEF_XID
typedef CARD32 XID;
#  endif
#  ifndef _XTYPEDEF_MASK
#    define _XTYPEDEF_MASK
typedef CARD32 Mask;
#  endif
#  ifndef _XTYPEDEF_ATOM
#    define _XTYPEDEF_ATOM
typedef CARD32 Atom;
#  endif
typedef CARD32 VisualID;
typedef CARD32 Time;
#endif

typedef XID Window;
typedef XID Drawable;
#ifndef _XTYPEDEF_FONT
#  define _XTYPEDEF_FONT
typedef XID Font;
#endif
typedef XID Pixmap;
typedef XID Cursor;
typedef XID Colormap;
typedef XID GContext;
typedef XID KeySym;

typedef unsigned char KeyCode;

/*****************************************************************
 * RESERVED RESOURCE AND CONSTANT DEFINITIONS
 *****************************************************************/

#ifndef None
#define None                 0L	/* universal null resource or null atom */
#endif

#define ParentRelative       1L	/* background pixmap in CreateWindow
				    and ChangeWindowAttributes */

#define CopyFromParent       0L	/* border pixmap in CreateWindow
				       and ChangeWindowAttributes
				   special VisualID and special window
				       class passed to CreateWindow */

#define PointerWindow        0L	/* destination window in SendEvent */
#define InputFocus           1L	/* destination window in SendEvent */

#define PointerRoot          1L	/* focus window in SetInputFocus */

#define AnyPropertyType      0L	/* special Atom, passed to GetProperty */

#define AnyKey		     0L	/* special Key Code, passed to GrabKey */

#define AnyButton            0L	/* special Button Code, passed to GrabButton */

#define AllTemporary         0L	/* special Resource ID passed to KillClient */

#define CurrentTime          0L	/* special Time */

#define NoSymbol	     0L	/* special KeySym */

/***************************************************************** 
 * EVENT DEFINITIONS 
 *****************************************************************/

/* Input Event Masks. Used as event-mask window attribute and as arguments
   to Grab requests.  Not to be confused with event names.  */

#define NoEventMask			0L
#define KeyPressMask			(1L<<0)  
#define KeyReleaseMask			(1L<<1)  
#define ButtonPressMask			(1L<<2)  
#define ButtonReleaseMask		(1L<<3)  
#define EnterWindowMask			(1L<<4)  
#define LeaveWindowMask			(1L<<5)  
#define PointerMotionMask		(1L<<6)  
#define PointerMotionHintMask		(1L<<7)  
#define Button1MotionMask		(1L<<8)  
#define Button2MotionMask		(1L<<9)  
#define Button3MotionMask		(1L<<10) 
#define Button4MotionMask		(1L<<11) 
#define Button5MotionMask		(1L<<12) 
#define ButtonMotionMask		(1L<<13) 
#define KeymapStateMask			(1L<<14)
#define ExposureMask			(1L<<15) 
#define VisibilityChangeMask		(1L<<16) 
#define StructureNotifyMask		(1L<<17) 
#define ResizeRedirectMask		(1L<<18) 
#define SubstructureNotifyMask		(1L<<19) 
#define SubstructureRedirectMask	(1L<<20) 
#define FocusChangeMask			(1L<<21) 
#define PropertyChangeMask		(1L<<22) 
#define ColormapChangeMask		(1L<<23) 
#define OwnerGrabButtonMask		(1L<<24) 

/* Event names.  Used in "type" field in XEvent structures.  Not to be
confused with event masks above.  They start from 2 because 0 and 1
are reserved in the protocol for errors and replies. */

#define KeyPress		2
#define KeyRelease		3
#define ButtonPress		4
#define ButtonRelease		5
#define MotionNotify		6
#define EnterNotify		7
#define LeaveNotify		8
#define FocusIn			9
#define FocusOut		10
#define KeymapNotify		11
#define Expose			12
#define GraphicsExpose		13
#define NoExpose		14
#define VisibilityNotify	15
#define CreateNotify		16
#define DestroyNotify		17
#define UnmapNotify		18
#define MapNotify		19
#define MapRequest		20
#define ReparentNotify		21
#define ConfigureNotify		22
#define ConfigureRequest	23
#define GravityNotify		24
#define ResizeRequest		25
#define CirculateNotify		26
#define CirculateRequest	27
#define PropertyNotify		28
#define SelectionClear		29
#define SelectionRequest	30
#define SelectionNotify		31
#define ColormapNotify		32
#define ClientMessage		33
#define MappingNotify		34
#define LASTEvent		35	/* must be bigger than any event # */


/* Key masks. Used as modifiers to GrabButton and GrabKey, results of QueryPointer,
   state in various key-, mouse-, and button-related events. */

#define ShiftMask		(1<<0)
#define LockMask		(1<<1)
#define ControlMask		(1<<2)
#define Mod1Mask		(1<<3)
#define Mod2Mask		(1<<4)
#define Mod3Mask		(1<<5)
#define Mod4Mask		(1<<6)
#define Mod5Mask		(1<<7)

/* modifier names.  Used to build a SetModifierMapping request or
   to read a GetModifierMapping request.  These correspond to the
   masks defined above. */
#define ShiftMapIndex		0
#define LockMapIndex		1
#define ControlMapIndex		2
#define Mod1MapIndex		3
#define Mod2MapIndex		4
#define Mod3MapIndex		5
#define Mod4MapIndex		6
#define Mod5MapIndex		7


/* button masks.  Used in same manner as Key masks above. Not to be confused
   with button names below. */

#define Button1Mask		(1<<8)
#define Button2Mask		(1<<9)
#define Button3Mask		(1<<10)
#define Button4Mask		(1<<11)
#define Button5Mask		(1<<12)

#define AnyModifier		(1<<15)  /* used in GrabButton, GrabKey */


/* button names. Used as arguments to GrabButton and as detail in ButtonPress
   and ButtonRelease events.  Not to be confused with button masks above.
   Note that 0 is already defined above as "AnyButton".  */

#define Button1			1
#define Button2			2
#define Button3			3
#define Button4			4
#define Button5			5

/* Notify modes */

#define NotifyNormal		0
#define NotifyGrab		1
#define NotifyUngrab		2
#define NotifyWhileGrabbed	3

#define NotifyHint		1	/* for MotionNotify events */
		       
/* Notify detail */

#define NotifyAncestor		0
#define NotifyVirtual		1
#define NotifyInferior		2
#define NotifyNonlinear		3
#define NotifyNonlinearVirtual	4
#define NotifyPointer		5
#define NotifyPointerRoot	6
#define NotifyDetailNone	7

/* Visibility notify */

#define VisibilityUnobscured		0
#define VisibilityPartiallyObscured	1
#define VisibilityFullyObscured		2

/* Circulation request */

#define PlaceOnTop		0
#define PlaceOnBottom		1

/* protocol families */

#define FamilyInternet		0
#define FamilyDECnet		1
#define FamilyChaos		2

/* Property notification */

#define PropertyNewValue	0
#define PropertyDelete		1

/* Color Map notification */

#define ColormapUninstalled	0
#define ColormapInstalled	1

/* GrabPointer, GrabButton, GrabKeyboard, GrabKey Modes */

#define GrabModeSync		0
#define GrabModeAsync		1

/* GrabPointer, GrabKeyboard reply status */

#define GrabSuccess		0
#define AlreadyGrabbed		1
#define GrabInvalidTime		2
#define GrabNotViewable		3
#define GrabFrozen		4

/* AllowEvents modes */

#define AsyncPointer		0
#define SyncPointer		1
#define ReplayPointer		2
#define AsyncKeyboard		3
#define SyncKeyboard		4
#define ReplayKeyboard		5
#define AsyncBoth		6
#define SyncBoth		7

/* Used in SetInputFocus, GetInputFocus */

#define RevertToNone		(int)None
#define RevertToPointerRoot	(int)PointerRoot
#define RevertToParent		2

/*****************************************************************
 * ERROR CODES 
 *****************************************************************/

#define Success		   0	/* everything's okay */
#define BadRequest	   1	/* bad request code */
#define BadValue	   2	/* int parameter out of range */
#define BadWindow	   3	/* parameter not a Window */
#define BadPixmap	   4	/* parameter not a Pixmap */
#define BadAtom		   5	/* parameter not an Atom */
#define BadCursor	   6	/* parameter not a Cursor */
#define BadFont		   7	/* parameter not a Font */
#define BadMatch	   8	/* parameter mismatch */
#define BadDrawable	   9	/* parameter not a Pixmap or Window */
#define BadAccess	  10	/* depending on context:
				 - key/button already grabbed
				 - attempt to free an illegal 
				   cmap entry 
				- attempt to store into a read-only 
				   color map entry.
 				- attempt to modify the access control
				   list from other than the local host.
				*/
#define BadAlloc	  11	/* insufficient resources */
#define BadColor	  12	/* no such colormap */
#define BadGC		  13	/* parameter not a GC */
#define BadIDChoice	  14	/* choice not in range or already used */
#define BadName		  15	/* font or color name doesn't exist */
#define BadLength	  16	/* Request length incorrect */
#define BadImplementation 17	/* server is defective */

#define FirstExtensionError	128
#define LastExtensionError	255

/*****************************************************************
 * WINDOW DEFINITIONS 
 *****************************************************************/

/* Window classes used by CreateWindow */
/* Note that CopyFromParent is already defined as 0 above */

#define InputOutput		1
#define InputOnly		2

/* Window attributes for CreateWindow and ChangeWindowAttributes */

#define CWBackPixmap		(1L<<0)
#define CWBackPixel		(1L<<1)
#define CWBorderPixmap		(1L<<2)
#define CWBorderPixel           (1L<<3)
#define CWBitGravity		(1L<<4)
#define CWWinGravity		(1L<<5)
#define CWBackingStore          (1L<<6)
#define CWBackingPlanes	        (1L<<7)
#define CWBackingPixel	        (1L<<8)
#define CWOverrideRedirect	(1L<<9)
#define CWSaveUnder		(1L<<10)
#define CWEventMask		(1L<<11)
#define CWDontPropagate	        (1L<<12)
#define CWColormap		(1L<<13)
#define CWCursor	        (1L<<14)

/* ConfigureWindow structure */

#define CWX			(1<<0)
#define CWY			(1<<1)
#define CWWidth			(1<<2)
#define CWHeight		(1<<3)
#define CWBorderWidth		(1<<4)
#define CWSibling		(1<<5)
#define CWStackMode		(1<<6)


/* Bit Gravity */

#define ForgetGravity		0
#define NorthWestGravity	1
#define NorthGravity		2
#define NorthEastGravity	3
#define WestGravity		4
#define CenterGravity		5
#define EastGravity		6
#define SouthWestGravity	7
#define SouthGravity		8
#define SouthEastGravity	9
#define StaticGravity		10

/* Window gravity + bit gravity above */

#define UnmapGravity		0

/* Used in CreateWindow for backing-store hint */

#define NotUseful               0
#define WhenMapped              1
#define Always                  2

/* Used in GetWindowAttributes reply */

#define IsUnmapped		0
#define IsUnviewable		1
#define IsViewable		2

/* Used in ChangeSaveSet */

#define SetModeInsert           0
#define SetModeDelete           1

/* Used in ChangeCloseDownMode */

#define DestroyAll              0
#define RetainPermanent         1
#define RetainTemporary         2

/* Window stacking method (in configureWindow) */

#define Above                   0
#define Below                   1
#define TopIf                   2
#define BottomIf                3
#define Opposite                4

/* Circulation direction */

#define RaiseLowest             0
#define LowerHighest            1

/* Property modes */

#define PropModeReplace         0
#define PropModePrepend         1
#define PropModeAppend          2

/*****************************************************************
 * GRAPHICS DEFINITIONS
 *****************************************************************/

/* graphics functions, as in GC.alu */

#define	GXclear			0x0		/* 0 */
#define GXand			0x1		/* src AND dst */
#define GXandReverse		0x2		/* src AND NOT dst */
#define GXcopy			0x3		/* src */
#define GXandInverted		0x4		/* NOT src AND dst */
#define	GXnoop			0x5		/* dst */
#define GXxor			0x6		/* src XOR dst */
#define GXor			0x7		/* src OR dst */
#define GXnor			0x8		/* NOT src AND NOT dst */
#define GXequiv			0x9		/* NOT src XOR dst */
#define GXinvert		0xa		/* NOT dst */
#define GXorReverse		0xb		/* src OR NOT dst */
#define GXcopyInverted		0xc		/* NOT src */
#define GXorInverted		0xd		/* NOT src OR dst */
#define GXnand			0xe		/* NOT src OR NOT dst */
#define GXset			0xf		/* 1 */

/* LineStyle */

#define LineSolid		0
#define LineOnOffDash		1
#define LineDoubleDash		2

/* capStyle */

#define CapNotLast		0
#define CapButt			1
#define CapRound		2
#define CapProjecting		3

/* joinStyle */

#define JoinMiter		0
#define JoinRound		1
#define JoinBevel		2

/* fillStyle */

#define FillSolid		0
#define FillTiled		1
#define FillStippled		2
#define FillOpaqueStippled	3

/* fillRule */

#define EvenOddRule		0
#define WindingRule		1

/* subwindow mode */

#define ClipByChildren		0
#define IncludeInferiors	1

/* SetClipRectangles ordering */

#define Unsorted		0
#define YSorted			1
#define YXSorted		2
#define YXBanded		3

/* CoordinateMode for drawing routines */

#define CoordModeOrigin		0	/* relative to the origin */
#define CoordModePrevious       1	/* relative to previous point */

/* Polygon shapes */

#define Complex			0	/* paths may intersect */
#define Nonconvex		1	/* no paths intersect, but not convex */
#define Convex			2	/* wholly convex */

/* Arc modes for PolyFillArc */

#define ArcChord		0	/* join endpoints of arc */
#define ArcPieSlice		1	/* join endpoints to center of arc */

/* GC components: masks used in CreateGC, CopyGC, ChangeGC, OR'ed into
   GC.stateChanges */

#define GCFunction              (1L<<0)
#define GCPlaneMask             (1L<<1)
#define GCForeground            (1L<<2)
#define GCBackground            (1L<<3)
#define GCLineWidth             (1L<<4)
#define GCLineStyle             (1L<<5)
#define GCCapStyle              (1L<<6)
#define GCJoinStyle		(1L<<7)
#define GCFillStyle		(1L<<8)
#define GCFillRule		(1L<<9) 
#define GCTile			(1L<<10)
#define GCStipple		(1L<<11)
#define GCTileStipXOrigin	(1L<<12)
#define GCTileStipYOrigin	(1L<<13)
#define GCFont 			(1L<<14)
#define GCSubwindowMode		(1L<<15)
#define GCGraphicsExposures     (1L<<16)
#define GCClipXOrigin		(1L<<17)
#define GCClipYOrigin		(1L<<18)
#define GCClipMask		(1L<<19)
#define GCDashOffset		(1L<<20)
#define GCDashList		(1L<<21)
#define GCArcMode		(1L<<22)

#define GCLastBit		22
/*****************************************************************
 * FONTS 
 *****************************************************************/

/* used in QueryFont -- draw direction */

#define FontLeftToRight		0
#define FontRightToLeft		1

#define FontChange		255

/*****************************************************************
 *  IMAGING 
 *****************************************************************/

/* ImageFormat -- PutImage, GetImage */

#define XYBitmap		0	/* depth 1, XYFormat */
#define XYPixmap		1	/* depth == drawable depth */
#define ZPixmap			2	/* depth == drawable depth */

/*****************************************************************
 *  COLOR MAP STUFF 
 *****************************************************************/

/* For CreateColormap */

#define AllocNone		0	/* create map with no entries */
#define AllocAll		1	/* allocate entire map writeable */


/* Flags used in StoreNamedColor, StoreColors */

#define DoRed			(1<<0)
#define DoGreen			(1<<1)
#define DoBlue			(1<<2)

/*****************************************************************
 * CURSOR STUFF
 *****************************************************************/

/* QueryBestSize Class */

#define CursorShape		0	/* largest size that can be displayed */
#define TileShape		1	/* size tiled fastest */
#define StippleShape		2	/* size stippled fastest */

/***************************************************************** 
 * KEYBOARD/POINTER STUFF
 *****************************************************************/

#define AutoRepeatModeOff	0
#define AutoRepeatModeOn	1
#define AutoRepeatModeDefault	2

#define LedModeOff		0
#define LedModeOn		1

/* masks for ChangeKeyboardControl */

#define KBKeyClickPercent	(1L<<0)
#define KBBellPercent		(1L<<1)
#define KBBellPitch		(1L<<2)
#define KBBellDuration		(1L<<3)
#define KBLed			(1L<<4)
#define KBLedMode		(1L<<5)
#define KBKey			(1L<<6)
#define KBAutoRepeatMode	(1L<<7)

#define MappingSuccess     	0
#define MappingBusy        	1
#define MappingFailed		2

#define MappingModifier		0
#define MappingKeyboard		1
#define MappingPointer		2

/*****************************************************************
 * SCREEN SAVER STUFF 
 *****************************************************************/

#define DontPreferBlanking	0
#define PreferBlanking		1
#define DefaultBlanking		2

#define DisableScreenSaver	0
#define DisableScreenInterval	0

#define DontAllowExposures	0
#define AllowExposures		1
#define DefaultExposures	2

/* for ForceScreenSaver */

#define ScreenSaverReset 0
#define ScreenSaverActive 1

/*****************************************************************
 * HOSTS AND CONNECTIONS
 *****************************************************************/

/* for ChangeHosts */

#define HostInsert		0
#define HostDelete		1

/* for ChangeAccessControl */

#define EnableAccess		1      
#define DisableAccess		0

/* Display classes  used in opening the connection 
 * Note that the statically allocated ones are even numbered and the
 * dynamically changeable ones are odd numbered */

#define StaticGray		0
#define GrayScale		1
#define StaticColor		2
#define PseudoColor		3
#define TrueColor		4
#define DirectColor		5


/* Byte order  used in imageByteOrder and bitmapBitOrder */

#define LSBFirst		0
#define MSBFirst		1

#endif /* X_H */


/* $Xorg: Xlib.h,v 1.6 2001/02/09 02:03:38 xorgcvs Exp $ */
/* 

Copyright 1985, 1986, 1987, 1991, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/
/* $XFree86: xc/lib/X11/Xlib.h,v 3.23 2002/05/31 18:45:42 dawes Exp $ */


/*
 *	Xlib.h - Header definition and support file for the C subroutine
 *	interface library (Xlib) to the X Window System Protocol (V11).
 *	Structures and symbols starting with "_" are private to the library.
 */
#ifndef _XLIB_H_
#define _XLIB_H_

#define XlibSpecificationRelease 6

#ifdef USG
#ifndef __TYPES__
#include <sys/types.h>			/* forgot to protect it... */
#define __TYPES__
#endif /* __TYPES__ */
#else
#if defined(_POSIX_SOURCE) && defined(MOTOROLA)
#undef _POSIX_SOURCE
#include <sys/types.h>
#define _POSIX_SOURCE
#else
#include <sys/types.h>
#endif
#endif /* USG */

#if 0
#include <X11/X.h>

/* applications should not depend on these two headers being included! */
#include <X11/Xfuncproto.h>
#include <X11/Xosdefs.h>

#endif /* if 0 */

#ifndef X_WCHAR
#ifdef X_NOT_STDC_ENV
#ifndef SCO324
#ifndef ISC
#define X_WCHAR
#endif
#endif
#endif
#endif

#ifndef X_WCHAR
#include <stddef.h>
#else
#ifdef __UNIXOS2__
#include <stdlib.h>
#else
/* replace this with #include or typedef appropriate for your system */
typedef unsigned long wchar_t;
#endif
#endif

#if defined(ISC) && defined(USE_XMBTOWC)
#define wctomb(a,b)	_Xwctomb(a,b)
#define mblen(a,b)	_Xmblen(a,b) 
#ifndef USE_XWCHAR_STRING
#define mbtowc(a,b,c)	_Xmbtowc(a,b,c)
#endif
#endif

/* API mentioning "UTF8" or "utf8" is an XFree86 extension, introduced in
   November 2000. Its presence is indicated through the following macro. */
#define X_HAVE_UTF8_STRING 1

typedef char *XPointer;

#define Bool int
#define Status int
#define True 1
#define False 0

#define QueuedAlready 0
#define QueuedAfterReading 1
#define QueuedAfterFlush 2

#define ConnectionNumber(dpy) 	(((_XPrivDisplay)dpy)->fd)
#define RootWindow(dpy, scr) 	(ScreenOfDisplay(dpy,scr)->root)
#define DefaultScreen(dpy) 	(((_XPrivDisplay)dpy)->default_screen)
#define DefaultRootWindow(dpy) 	(ScreenOfDisplay(dpy,DefaultScreen(dpy))->root)
#define DefaultVisual(dpy, scr) (ScreenOfDisplay(dpy,scr)->root_visual)
#define DefaultGC(dpy, scr) 	(ScreenOfDisplay(dpy,scr)->default_gc)
#define BlackPixel(dpy, scr) 	(ScreenOfDisplay(dpy,scr)->black_pixel)
#define WhitePixel(dpy, scr) 	(ScreenOfDisplay(dpy,scr)->white_pixel)
#define AllPlanes 		((unsigned long)~0L)
#define QLength(dpy) 		(((_XPrivDisplay)dpy)->qlen)
#define DisplayWidth(dpy, scr) 	(ScreenOfDisplay(dpy,scr)->width)
#define DisplayHeight(dpy, scr) (ScreenOfDisplay(dpy,scr)->height)
#define DisplayWidthMM(dpy, scr)(ScreenOfDisplay(dpy,scr)->mwidth)
#define DisplayHeightMM(dpy, scr)(ScreenOfDisplay(dpy,scr)->mheight)
#define DisplayPlanes(dpy, scr) (ScreenOfDisplay(dpy,scr)->root_depth)
#define DisplayCells(dpy, scr) 	(DefaultVisual(dpy,scr)->map_entries)
#define ScreenCount(dpy) 	(((_XPrivDisplay)dpy)->nscreens)
#define ServerVendor(dpy) 	(((_XPrivDisplay)dpy)->vendor)
#define ProtocolVersion(dpy) 	(((_XPrivDisplay)dpy)->proto_major_version)
#define ProtocolRevision(dpy) 	(((_XPrivDisplay)dpy)->proto_minor_version)
#define VendorRelease(dpy) 	(((_XPrivDisplay)dpy)->release)
#define DisplayString(dpy) 	(((_XPrivDisplay)dpy)->display_name)
#define DefaultDepth(dpy, scr) 	(ScreenOfDisplay(dpy,scr)->root_depth)
#define DefaultColormap(dpy, scr)(ScreenOfDisplay(dpy,scr)->cmap)
#define BitmapUnit(dpy) 	(((_XPrivDisplay)dpy)->bitmap_unit)
#define BitmapBitOrder(dpy) 	(((_XPrivDisplay)dpy)->bitmap_bit_order)
#define BitmapPad(dpy) 		(((_XPrivDisplay)dpy)->bitmap_pad)
#define ImageByteOrder(dpy) 	(((_XPrivDisplay)dpy)->byte_order)
#ifdef CRAY /* unable to get WORD64 without pulling in other symbols */
#define NextRequest(dpy)	XNextRequest(dpy)
#else
#define NextRequest(dpy)	(((_XPrivDisplay)dpy)->request + 1)
#endif
#define LastKnownRequestProcessed(dpy)	(((_XPrivDisplay)dpy)->last_request_read)

/* macros for screen oriented applications (toolkit) */
#define ScreenOfDisplay(dpy, scr)(&((_XPrivDisplay)dpy)->screens[scr])
#define DefaultScreenOfDisplay(dpy) ScreenOfDisplay(dpy,DefaultScreen(dpy))
#define DisplayOfScreen(s)	((s)->display)
#define RootWindowOfScreen(s)	((s)->root)
#define BlackPixelOfScreen(s)	((s)->black_pixel)
#define WhitePixelOfScreen(s)	((s)->white_pixel)
#define DefaultColormapOfScreen(s)((s)->cmap)
#define DefaultDepthOfScreen(s)	((s)->root_depth)
#define DefaultGCOfScreen(s)	((s)->default_gc)
#define DefaultVisualOfScreen(s)((s)->root_visual)
#define WidthOfScreen(s)	((s)->width)
#define HeightOfScreen(s)	((s)->height)
#define WidthMMOfScreen(s)	((s)->mwidth)
#define HeightMMOfScreen(s)	((s)->mheight)
#define PlanesOfScreen(s)	((s)->root_depth)
#define CellsOfScreen(s)	(DefaultVisualOfScreen((s))->map_entries)
#define MinCmapsOfScreen(s)	((s)->min_maps)
#define MaxCmapsOfScreen(s)	((s)->max_maps)
#define DoesSaveUnders(s)	((s)->save_unders)
#define DoesBackingStore(s)	((s)->backing_store)
#define EventMaskOfScreen(s)	((s)->root_input_mask)

/*
 * Extensions need a way to hang private data on some structures.
 */
typedef struct _XExtData {
	int number;		/* number returned by XRegisterExtension */
	struct _XExtData *next;	/* next item on list of data for structure */
	int (*free_private)(	/* called to free private storage */
#if NeedFunctionPrototypes
	struct _XExtData *extension
#endif
	);
	XPointer private_data;	/* data private to this extension. */
} XExtData;

/*
 * This file contains structures used by the extension mechanism.
 */
typedef struct {		/* public to extension, cannot be changed */
	int extension;		/* extension number */
	int major_opcode;	/* major op-code assigned by server */
	int first_event;	/* first event number for the extension */
	int first_error;	/* first error number for the extension */
} XExtCodes;

/*
 * Data structure for retrieving info about pixmap formats.
 */

typedef struct {
    int depth;
    int bits_per_pixel;
    int scanline_pad;
} XPixmapFormatValues;


/*
 * Data structure for setting graphics context.
 */
typedef struct {
	int function;		/* logical operation */
	unsigned long plane_mask;/* plane mask */
	unsigned long foreground;/* foreground pixel */
	unsigned long background;/* background pixel */
	int line_width;		/* line width */
	int line_style;	 	/* LineSolid, LineOnOffDash, LineDoubleDash */
	int cap_style;	  	/* CapNotLast, CapButt, 
				   CapRound, CapProjecting */
	int join_style;	 	/* JoinMiter, JoinRound, JoinBevel */
	int fill_style;	 	/* FillSolid, FillTiled, 
				   FillStippled, FillOpaeueStippled */
	int fill_rule;	  	/* EvenOddRule, WindingRule */
	int arc_mode;		/* ArcChord, ArcPieSlice */
	Pixmap tile;		/* tile pixmap for tiling operations */
	Pixmap stipple;		/* stipple 1 plane pixmap for stipping */
	int ts_x_origin;	/* offset for tile or stipple operations */
	int ts_y_origin;
        Font font;	        /* default text font for text operations */
	int subwindow_mode;     /* ClipByChildren, IncludeInferiors */
	Bool graphics_exposures;/* boolean, should exposures be generated */
	int clip_x_origin;	/* origin for clipping */
	int clip_y_origin;
	Pixmap clip_mask;	/* bitmap clipping; other calls for rects */
	int dash_offset;	/* patterned/dashed line information */
	char dashes;
} XGCValues;

/*
 * Graphics context.  The contents of this structure are implementation
 * dependent.  A GC should be treated as opaque by application code.
 */

typedef struct _XGC
#ifdef XLIB_ILLEGAL_ACCESS
{
    XExtData *ext_data;	/* hook for extension to hang data */
    GContext gid;	/* protocol ID for graphics context */
    /* there is more to this structure, but it is private to Xlib */
}
#endif
*GC;

/*
 * Visual structure; contains information about colormapping possible.
 */
typedef struct {
	XExtData *ext_data;	/* hook for extension to hang data */
	VisualID visualid;	/* visual id of this visual */
#if defined(__cplusplus) || defined(c_plusplus)
	int c_class;		/* C++ class of screen (monochrome, etc.) */
#else
	int class;		/* class of screen (monochrome, etc.) */
#endif
	unsigned long red_mask, green_mask, blue_mask;	/* mask values */
	int bits_per_rgb;	/* log base 2 of distinct color values */
	int map_entries;	/* color map entries */
} Visual;

/*
 * Depth structure; contains information for each possible depth.
 */	
typedef struct {
	int depth;		/* this depth (Z) of the depth */
	int nvisuals;		/* number of Visual types at this depth */
	Visual *visuals;	/* list of visuals possible at this depth */
} Depth;

/*
 * Information about the screen.  The contents of this structure are
 * implementation dependent.  A Screen should be treated as opaque
 * by application code.
 */

struct _XDisplay;		/* Forward declare before use for C++ */

typedef struct {
	XExtData *ext_data;	/* hook for extension to hang data */
	struct _XDisplay *display;/* back pointer to display structure */
	Window root;		/* Root window id. */
	int width, height;	/* width and height of screen */
	int mwidth, mheight;	/* width and height of  in millimeters */
	int ndepths;		/* number of depths possible */
	Depth *depths;		/* list of allowable depths on the screen */
	int root_depth;		/* bits per pixel */
	Visual *root_visual;	/* root visual */
	GC default_gc;		/* GC for the root root visual */
	Colormap cmap;		/* default color map */
	unsigned long white_pixel;
	unsigned long black_pixel;	/* White and Black pixel values */
	int max_maps, min_maps;	/* max and min color maps */
	int backing_store;	/* Never, WhenMapped, Always */
	Bool save_unders;	
	long root_input_mask;	/* initial root input mask */
} Screen;

/*
 * Format structure; describes ZFormat data the screen will understand.
 */
typedef struct {
	XExtData *ext_data;	/* hook for extension to hang data */
	int depth;		/* depth of this image format */
	int bits_per_pixel;	/* bits/pixel at this depth */
	int scanline_pad;	/* scanline must padded to this multiple */
} ScreenFormat;

/*
 * Data structure for setting window attributes.
 */
typedef struct {
    Pixmap background_pixmap;	/* background or None or ParentRelative */
    unsigned long background_pixel;	/* background pixel */
    Pixmap border_pixmap;	/* border of the window */
    unsigned long border_pixel;	/* border pixel value */
    int bit_gravity;		/* one of bit gravity values */
    int win_gravity;		/* one of the window gravity values */
    int backing_store;		/* NotUseful, WhenMapped, Always */
    unsigned long backing_planes;/* planes to be preseved if possible */
    unsigned long backing_pixel;/* value to use in restoring planes */
    Bool save_under;		/* should bits under be saved? (popups) */
    long event_mask;		/* set of events that should be saved */
    long do_not_propagate_mask;	/* set of events that should not propagate */
    Bool override_redirect;	/* boolean value for override-redirect */
    Colormap colormap;		/* color map to be associated with window */
    Cursor cursor;		/* cursor to be displayed (or None) */
} XSetWindowAttributes;

typedef struct {
    int x, y;			/* location of window */
    int width, height;		/* width and height of window */
    int border_width;		/* border width of window */
    int depth;          	/* depth of window */
    Visual *visual;		/* the associated visual structure */
    Window root;        	/* root of screen containing window */
#if defined(__cplusplus) || defined(c_plusplus)
    int c_class;		/* C++ InputOutput, InputOnly*/
#else
    int class;			/* InputOutput, InputOnly*/
#endif
    int bit_gravity;		/* one of bit gravity values */
    int win_gravity;		/* one of the window gravity values */
    int backing_store;		/* NotUseful, WhenMapped, Always */
    unsigned long backing_planes;/* planes to be preserved if possible */
    unsigned long backing_pixel;/* value to be used when restoring planes */
    Bool save_under;		/* boolean, should bits under be saved? */
    Colormap colormap;		/* color map to be associated with window */
    Bool map_installed;		/* boolean, is color map currently installed*/
    int map_state;		/* IsUnmapped, IsUnviewable, IsViewable */
    long all_event_masks;	/* set of events all people have interest in*/
    long your_event_mask;	/* my event mask */
    long do_not_propagate_mask; /* set of events that should not propagate */
    Bool override_redirect;	/* boolean value for override-redirect */
    Screen *screen;		/* back pointer to correct screen */
} XWindowAttributes;

/*
 * Data structure for host setting; getting routines.
 *
 */

typedef struct {
	int family;		/* for example FamilyInternet */
	int length;		/* length of address, in bytes */
	char *address;		/* pointer to where to find the bytes */
} XHostAddress;

/*
 * Data structure for "image" data, used by image manipulation routines.
 */
typedef struct _XImage {
    int width, height;		/* size of image */
    int xoffset;		/* number of pixels offset in X direction */
    int format;			/* XYBitmap, XYPixmap, ZPixmap */
    char *data;			/* pointer to image data */
    int byte_order;		/* data byte order, LSBFirst, MSBFirst */
    int bitmap_unit;		/* quant. of scanline 8, 16, 32 */
    int bitmap_bit_order;	/* LSBFirst, MSBFirst */
    int bitmap_pad;		/* 8, 16, 32 either XY or ZPixmap */
    int depth;			/* depth of image */
    int bytes_per_line;		/* accelarator to next line */
    int bits_per_pixel;		/* bits per pixel (ZPixmap) */
    unsigned long red_mask;	/* bits in z arrangment */
    unsigned long green_mask;
    unsigned long blue_mask;
    XPointer obdata;		/* hook for the object routines to hang on */
    struct funcs {		/* image manipulation routines */
#if NeedFunctionPrototypes
	struct _XImage *(*create_image)(
		struct _XDisplay* /* display */,
		Visual*		/* visual */,
		unsigned int	/* depth */,
		int		/* format */,
		int		/* offset */,
		char*		/* data */,
		unsigned int	/* width */,
		unsigned int	/* height */,
		int		/* bitmap_pad */,
		int		/* bytes_per_line */);
	int (*destroy_image)        (struct _XImage *);
	unsigned long (*get_pixel)  (struct _XImage *, int, int);
	int (*put_pixel)            (struct _XImage *, int, int, unsigned long);
	struct _XImage *(*sub_image)(struct _XImage *, int, int, unsigned int, unsigned int);
	int (*add_pixel)            (struct _XImage *, long);
#else
	struct _XImage *(*create_image)();
	int (*destroy_image)();
	unsigned long (*get_pixel)();
	int (*put_pixel)();
	struct _XImage *(*sub_image)();
	int (*add_pixel)();
#endif
	} f;
} XImage;

/* 
 * Data structure for XReconfigureWindow
 */
typedef struct {
    int x, y;
    int width, height;
    int border_width;
    Window sibling;
    int stack_mode;
} XWindowChanges;

/*
 * Data structure used by color operations
 */
typedef struct {
	unsigned long pixel;
	unsigned short red, green, blue;
	char flags;  /* do_red, do_green, do_blue */
	char pad;
} XColor;

/* 
 * Data structures for graphics operations.  On most machines, these are
 * congruent with the wire protocol structures, so reformatting the data
 * can be avoided on these architectures.
 */
typedef struct {
    short x1, y1, x2, y2;
} XSegment;

typedef struct {
    short x, y;
} XPoint;
    
typedef struct {
    short x, y;
    unsigned short width, height;
} XRectangle;
    
typedef struct {
    short x, y;
    unsigned short width, height;
    short angle1, angle2;
} XArc;


/* Data structure for XChangeKeyboardControl */

typedef struct {
        int key_click_percent;
        int bell_percent;
        int bell_pitch;
        int bell_duration;
        int led;
        int led_mode;
        int key;
        int auto_repeat_mode;   /* On, Off, Default */
} XKeyboardControl;

/* Data structure for XGetKeyboardControl */

typedef struct {
        int key_click_percent;
	int bell_percent;
	unsigned int bell_pitch, bell_duration;
	unsigned long led_mask;
	int global_auto_repeat;
	char auto_repeats[32];
} XKeyboardState;

/* Data structure for XGetMotionEvents.  */

typedef struct {
        Time time;
	short x, y;
} XTimeCoord;

/* Data structure for X{Set,Get}ModifierMapping */

typedef struct {
 	int max_keypermod;	/* The server's max # of keys per modifier */
 	KeyCode *modifiermap;	/* An 8 by max_keypermod array of modifiers */
} XModifierKeymap;


/*
 * Display datatype maintaining display specific data.
 * The contents of this structure are implementation dependent.
 * A Display should be treated as opaque by application code.
 */
#ifndef XLIB_ILLEGAL_ACCESS
typedef struct _XDisplay Display;
#endif

struct _XPrivate;		/* Forward declare before use for C++ */
struct _XrmHashBucketRec;

typedef struct 
#ifdef XLIB_ILLEGAL_ACCESS
_XDisplay
#endif
{
	XExtData *ext_data;	/* hook for extension to hang data */
	struct _XPrivate *private1;
	int fd;			/* Network socket. */
	int private2;
	int proto_major_version;/* major version of server's X protocol */
	int proto_minor_version;/* minor version of servers X protocol */
	char *vendor;		/* vendor of the server hardware */
        XID private3;
	XID private4;
	XID private5;
	int private6;
	XID (*resource_alloc)(	/* allocator function */
#if NeedFunctionPrototypes
		struct _XDisplay*
#endif
	);
	int byte_order;		/* screen byte order, LSBFirst, MSBFirst */
	int bitmap_unit;	/* padding and data requirements */
	int bitmap_pad;		/* padding requirements on bitmaps */
	int bitmap_bit_order;	/* LeastSignificant or MostSignificant */
	int nformats;		/* number of pixmap formats in list */
	ScreenFormat *pixmap_format;	/* pixmap format list */
	int private8;
	int release;		/* release of the server */
	struct _XPrivate *private9, *private10;
	int qlen;		/* Length of input event queue */
	unsigned long last_request_read; /* seq number of last event read */
	unsigned long request;	/* sequence number of last request. */
	XPointer private11;
	XPointer private12;
	XPointer private13;
	XPointer private14;
	unsigned max_request_size; /* maximum number 32 bit words in request*/
	struct _XrmHashBucketRec *db;
	int (*private15)(
#if NeedFunctionPrototypes
		struct _XDisplay*
#endif
		);
	char *display_name;	/* "host:display" string used on this connect*/
	int default_screen;	/* default screen for operations */
	int nscreens;		/* number of screens on this server*/
	Screen *screens;	/* pointer to list of screens */
	unsigned long motion_buffer;	/* size of motion buffer */
	unsigned long private16;
	int min_keycode;	/* minimum defined keycode */
	int max_keycode;	/* maximum defined keycode */
	XPointer private17;
	XPointer private18;
	int private19;
	char *xdefaults;	/* contents of defaults from server */
	/* there is more to this structure, but it is private to Xlib */
}
#ifdef XLIB_ILLEGAL_ACCESS
Display, 
#endif
*_XPrivDisplay;

#if NeedFunctionPrototypes	/* prototypes require event type definitions */
#undef _XEVENT_
#endif
#ifndef _XEVENT_
/*
 * Definitions of specific events.
 */
typedef struct {
	int type;		/* of event */
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;	        /* "event" window it is reported relative to */
	Window root;	        /* root window that the event occurred on */
	Window subwindow;	/* child window */
	Time time;		/* milliseconds */
	int x, y;		/* pointer x, y coordinates in event window */
	int x_root, y_root;	/* coordinates relative to root */
	unsigned int state;	/* key or button mask */
	unsigned int keycode;	/* detail */
	Bool same_screen;	/* same screen flag */
} XKeyEvent;
typedef XKeyEvent XKeyPressedEvent;
typedef XKeyEvent XKeyReleasedEvent;

typedef struct {
	int type;		/* of event */
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;	        /* "event" window it is reported relative to */
	Window root;	        /* root window that the event occurred on */
	Window subwindow;	/* child window */
	Time time;		/* milliseconds */
	int x, y;		/* pointer x, y coordinates in event window */
	int x_root, y_root;	/* coordinates relative to root */
	unsigned int state;	/* key or button mask */
	unsigned int button;	/* detail */
	Bool same_screen;	/* same screen flag */
} XButtonEvent;
typedef XButtonEvent XButtonPressedEvent;
typedef XButtonEvent XButtonReleasedEvent;

typedef struct {
	int type;		/* of event */
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;	        /* "event" window reported relative to */
	Window root;	        /* root window that the event occurred on */
	Window subwindow;	/* child window */
	Time time;		/* milliseconds */
	int x, y;		/* pointer x, y coordinates in event window */
	int x_root, y_root;	/* coordinates relative to root */
	unsigned int state;	/* key or button mask */
	char is_hint;		/* detail */
	Bool same_screen;	/* same screen flag */
} XMotionEvent;
typedef XMotionEvent XPointerMovedEvent;

typedef struct {
	int type;		/* of event */
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;	        /* "event" window reported relative to */
	Window root;	        /* root window that the event occurred on */
	Window subwindow;	/* child window */
	Time time;		/* milliseconds */
	int x, y;		/* pointer x, y coordinates in event window */
	int x_root, y_root;	/* coordinates relative to root */
	int mode;		/* NotifyNormal, NotifyGrab, NotifyUngrab */
	int detail;
	/*
	 * NotifyAncestor, NotifyVirtual, NotifyInferior, 
	 * NotifyNonlinear,NotifyNonlinearVirtual
	 */
	Bool same_screen;	/* same screen flag */
	Bool focus;		/* boolean focus */
	unsigned int state;	/* key or button mask */
} XCrossingEvent;
typedef XCrossingEvent XEnterWindowEvent;
typedef XCrossingEvent XLeaveWindowEvent;

typedef struct {
	int type;		/* FocusIn or FocusOut */
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;		/* window of event */
	int mode;		/* NotifyNormal, NotifyGrab, NotifyUngrab */
	int detail;
	/*
	 * NotifyAncestor, NotifyVirtual, NotifyInferior, 
	 * NotifyNonlinear,NotifyNonlinearVirtual, NotifyPointer,
	 * NotifyPointerRoot, NotifyDetailNone 
	 */
} XFocusChangeEvent;
typedef XFocusChangeEvent XFocusInEvent;
typedef XFocusChangeEvent XFocusOutEvent;

/* generated on EnterWindow and FocusIn  when KeyMapState selected */
typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;
	char key_vector[32];
} XKeymapEvent;	

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;
	int x, y;
	int width, height;
	int count;		/* if non-zero, at least this many more */
} XExposeEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Drawable drawable;
	int x, y;
	int width, height;
	int count;		/* if non-zero, at least this many more */
	int major_code;		/* core is CopyArea or CopyPlane */
	int minor_code;		/* not defined in the core */
} XGraphicsExposeEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Drawable drawable;
	int major_code;		/* core is CopyArea or CopyPlane */
	int minor_code;		/* not defined in the core */
} XNoExposeEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;
	int state;		/* Visibility state */
} XVisibilityEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window parent;		/* parent of the window */
	Window window;		/* window id of window created */
	int x, y;		/* window location */
	int width, height;	/* size of window */
	int border_width;	/* border width */
	Bool override_redirect;	/* creation should be overridden */
} XCreateWindowEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window event;
	Window window;
} XDestroyWindowEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window event;
	Window window;
	Bool from_configure;
} XUnmapEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window event;
	Window window;
	Bool override_redirect;	/* boolean, is override set... */
} XMapEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window parent;
	Window window;
} XMapRequestEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window event;
	Window window;
	Window parent;
	int x, y;
	Bool override_redirect;
} XReparentEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window event;
	Window window;
	int x, y;
	int width, height;
	int border_width;
	Window above;
	Bool override_redirect;
} XConfigureEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window event;
	Window window;
	int x, y;
} XGravityEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;
	int width, height;
} XResizeRequestEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window parent;
	Window window;
	int x, y;
	int width, height;
	int border_width;
	Window above;
	int detail;		/* Above, Below, TopIf, BottomIf, Opposite */
	unsigned long value_mask;
} XConfigureRequestEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window event;
	Window window;
	int place;		/* PlaceOnTop, PlaceOnBottom */
} XCirculateEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window parent;
	Window window;
	int place;		/* PlaceOnTop, PlaceOnBottom */
} XCirculateRequestEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;
	Atom atom;
	Time time;
	int state;		/* NewValue, Deleted */
} XPropertyEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;
	Atom selection;
	Time time;
} XSelectionClearEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window owner;
	Window requestor;
	Atom selection;
	Atom target;
	Atom property;
	Time time;
} XSelectionRequestEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window requestor;
	Atom selection;
	Atom target;
	Atom property;		/* ATOM or None */
	Time time;
} XSelectionEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;
	Colormap colormap;	/* COLORMAP or None */
#if defined(__cplusplus) || defined(c_plusplus)
	Bool c_new;		/* C++ */
#else
	Bool new;
#endif
	int state;		/* ColormapInstalled, ColormapUninstalled */
} XColormapEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;
	Atom message_type;
	int format;
	union {
		char b[20];
		short s[10];
		long l[5];
		} data;
} XClientMessageEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;		/* unused */
	int request;		/* one of MappingModifier, MappingKeyboard,
				   MappingPointer */
	int first_keycode;	/* first keycode */
	int count;		/* defines range of change w. first_keycode*/
} XMappingEvent;

typedef struct {
	int type;
	Display *display;	/* Display the event was read from */
	XID resourceid;		/* resource id */
	unsigned long serial;	/* serial number of failed request */
	unsigned char error_code;	/* error code of failed request */
	unsigned char request_code;	/* Major op-code of failed request */
	unsigned char minor_code;	/* Minor op-code of failed request */
} XErrorEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;/* Display the event was read from */
	Window window;	/* window on which event was requested in event mask */
} XAnyEvent;

/*
 * this union is defined so Xlib can always use the same sized
 * event structure internally, to avoid memory fragmentation.
 */
typedef union _XEvent {
        int type;		/* must not be changed; first element */
	XAnyEvent xany;
	XKeyEvent xkey;
	XButtonEvent xbutton;
	XMotionEvent xmotion;
	XCrossingEvent xcrossing;
	XFocusChangeEvent xfocus;
	XExposeEvent xexpose;
	XGraphicsExposeEvent xgraphicsexpose;
	XNoExposeEvent xnoexpose;
	XVisibilityEvent xvisibility;
	XCreateWindowEvent xcreatewindow;
	XDestroyWindowEvent xdestroywindow;
	XUnmapEvent xunmap;
	XMapEvent xmap;
	XMapRequestEvent xmaprequest;
	XReparentEvent xreparent;
	XConfigureEvent xconfigure;
	XGravityEvent xgravity;
	XResizeRequestEvent xresizerequest;
	XConfigureRequestEvent xconfigurerequest;
	XCirculateEvent xcirculate;
	XCirculateRequestEvent xcirculaterequest;
	XPropertyEvent xproperty;
	XSelectionClearEvent xselectionclear;
	XSelectionRequestEvent xselectionrequest;
	XSelectionEvent xselection;
	XColormapEvent xcolormap;
	XClientMessageEvent xclient;
	XMappingEvent xmapping;
	XErrorEvent xerror;
	XKeymapEvent xkeymap;
	long pad[24];
} XEvent;
#endif

#define XAllocID(dpy) ((*((_XPrivDisplay)dpy)->resource_alloc)((dpy)))

/*
 * per character font metric information.
 */
typedef struct {
    short	lbearing;	/* origin to left edge of raster */
    short	rbearing;	/* origin to right edge of raster */
    short	width;		/* advance to next char's origin */
    short	ascent;		/* baseline to top edge of raster */
    short	descent;	/* baseline to bottom edge of raster */
    unsigned short attributes;	/* per char flags (not predefined) */
} XCharStruct;

/*
 * To allow arbitrary information with fonts, there are additional properties
 * returned.
 */
typedef struct {
    Atom name;
    unsigned long card32;
} XFontProp;

typedef struct {
    XExtData	*ext_data;	/* hook for extension to hang data */
    Font        fid;            /* Font id for this font */
    unsigned	direction;	/* hint about direction the font is painted */
    unsigned	min_char_or_byte2;/* first character */
    unsigned	max_char_or_byte2;/* last character */
    unsigned	min_byte1;	/* first row that exists */
    unsigned	max_byte1;	/* last row that exists */
    Bool	all_chars_exist;/* flag if all characters have non-zero size*/
    unsigned	default_char;	/* char to print for undefined character */
    int         n_properties;   /* how many properties there are */
    XFontProp	*properties;	/* pointer to array of additional properties*/
    XCharStruct	min_bounds;	/* minimum bounds over all existing char*/
    XCharStruct	max_bounds;	/* maximum bounds over all existing char*/
    XCharStruct	*per_char;	/* first_char to last_char information */
    int		ascent;		/* log. extent above baseline for spacing */
    int		descent;	/* log. descent below baseline for spacing */
} XFontStruct;

/*
 * PolyText routines take these as arguments.
 */
typedef struct {
    char *chars;		/* pointer to string */
    int nchars;			/* number of characters */
    int delta;			/* delta between strings */
    Font font;			/* font to print it in, None don't change */
} XTextItem;

typedef struct {		/* normal 16 bit characters are two bytes */
    unsigned char byte1;
    unsigned char byte2;
} XChar2b;

typedef struct {
    XChar2b *chars;		/* two byte characters */
    int nchars;			/* number of characters */
    int delta;			/* delta between strings */
    Font font;			/* font to print it in, None don't change */
} XTextItem16;


typedef union { Display *display;
		GC gc;
		Visual *visual;
		Screen *screen;
		ScreenFormat *pixmap_format;
		XFontStruct *font; } XEDataObject;

typedef struct {
    XRectangle      max_ink_extent;
    XRectangle      max_logical_extent;
} XFontSetExtents;

/* unused:
typedef void (*XOMProc)();
 */

typedef struct _XOM *XOM;
typedef struct _XOC *XOC, *XFontSet;

typedef struct {
    char           *chars;
    int             nchars;
    int             delta;
    XFontSet        font_set;
} XmbTextItem;

typedef struct {
    wchar_t        *chars;
    int             nchars;
    int             delta;
    XFontSet        font_set;
} XwcTextItem;

#define XNRequiredCharSet "requiredCharSet"
#define XNQueryOrientation "queryOrientation"
#define XNBaseFontName "baseFontName"
#define XNOMAutomatic "omAutomatic"
#define XNMissingCharSet "missingCharSet"
#define XNDefaultString "defaultString"
#define XNOrientation "orientation"
#define XNDirectionalDependentDrawing "directionalDependentDrawing"
#define XNContextualDrawing "contextualDrawing"
#define XNFontInfo "fontInfo"

typedef struct {
    int charset_count;
    char **charset_list;
} XOMCharSetList;

typedef enum {
    XOMOrientation_LTR_TTB,
    XOMOrientation_RTL_TTB,
    XOMOrientation_TTB_LTR,
    XOMOrientation_TTB_RTL,
    XOMOrientation_Context
} XOrientation;

typedef struct {
    int num_orientation;
    XOrientation *orientation;	/* Input Text description */
} XOMOrientation;

typedef struct {
    int num_font;
    XFontStruct **font_struct_list;
    char **font_name_list;
} XOMFontInfo;

typedef struct _XIM *XIM;
typedef struct _XIC *XIC;

typedef void (*XIMProc)(
#if NeedFunctionPrototypes
    XIM,
    XPointer,
    XPointer
#endif
);

typedef Bool (*XICProc)(
#if NeedFunctionPrototypes
    XIC,
    XPointer,
    XPointer
#endif
);

typedef void (*XIDProc)(
#if NeedFunctionPrototypes
    Display*,
    XPointer,
    XPointer
#endif
);

typedef unsigned long XIMStyle;

typedef struct {
    unsigned short count_styles;
    XIMStyle *supported_styles;
} XIMStyles;

#define XIMPreeditArea		0x0001L
#define XIMPreeditCallbacks	0x0002L
#define XIMPreeditPosition	0x0004L
#define XIMPreeditNothing	0x0008L
#define XIMPreeditNone		0x0010L
#define XIMStatusArea		0x0100L
#define XIMStatusCallbacks	0x0200L
#define XIMStatusNothing	0x0400L
#define XIMStatusNone		0x0800L

#define XNVaNestedList "XNVaNestedList"
#define XNQueryInputStyle "queryInputStyle"
#define XNClientWindow "clientWindow"
#define XNInputStyle "inputStyle"
#define XNFocusWindow "focusWindow"
#define XNResourceName "resourceName"
#define XNResourceClass "resourceClass"
#define XNGeometryCallback "geometryCallback"
#define XNDestroyCallback "destroyCallback"
#define XNFilterEvents "filterEvents"
#define XNPreeditStartCallback "preeditStartCallback"
#define XNPreeditDoneCallback "preeditDoneCallback"
#define XNPreeditDrawCallback "preeditDrawCallback"
#define XNPreeditCaretCallback "preeditCaretCallback"
#define XNPreeditStateNotifyCallback "preeditStateNotifyCallback"
#define XNPreeditAttributes "preeditAttributes"
#define XNStatusStartCallback "statusStartCallback"
#define XNStatusDoneCallback "statusDoneCallback"
#define XNStatusDrawCallback "statusDrawCallback"
#define XNStatusAttributes "statusAttributes"
#define XNArea "area"
#define XNAreaNeeded "areaNeeded"
#define XNSpotLocation "spotLocation"
#define XNColormap "colorMap"
#define XNStdColormap "stdColorMap"
#define XNForeground "foreground"
#define XNBackground "background"
#define XNBackgroundPixmap "backgroundPixmap"
#define XNFontSet "fontSet"
#define XNLineSpace "lineSpace"
#define XNCursor "cursor"

#define XNQueryIMValuesList "queryIMValuesList"
#define XNQueryICValuesList "queryICValuesList"
#define XNVisiblePosition "visiblePosition"
#define XNR6PreeditCallback "r6PreeditCallback"
#define XNStringConversionCallback "stringConversionCallback"
#define XNStringConversion "stringConversion"
#define XNResetState "resetState"
#define XNHotKey "hotKey"
#define XNHotKeyState "hotKeyState"
#define XNPreeditState "preeditState"
#define XNSeparatorofNestedList "separatorofNestedList"

#define XBufferOverflow		-1
#define XLookupNone		1
#define XLookupChars		2
#define XLookupKeySym		3
#define XLookupBoth		4

#if NeedFunctionPrototypes
typedef void *XVaNestedList;
#else
typedef XPointer XVaNestedList;
#endif

typedef struct {
    XPointer client_data;
    XIMProc callback;
} XIMCallback;

typedef struct {
    XPointer client_data;
    XICProc callback;
} XICCallback;

typedef unsigned long XIMFeedback;

#define XIMReverse		1L
#define XIMUnderline		(1L<<1) 
#define XIMHighlight		(1L<<2)
#define XIMPrimary	 	(1L<<5)
#define XIMSecondary		(1L<<6)
#define XIMTertiary	 	(1L<<7)
#define XIMVisibleToForward 	(1L<<8)
#define XIMVisibleToBackword 	(1L<<9)
#define XIMVisibleToCenter 	(1L<<10)

typedef struct _XIMText {
    unsigned short length;
    XIMFeedback *feedback;
    Bool encoding_is_wchar; 
    union {
	char *multi_byte;
	wchar_t *wide_char;
    } string; 
} XIMText;

typedef	unsigned long	 XIMPreeditState;

#define	XIMPreeditUnKnown	0L
#define	XIMPreeditEnable	1L
#define	XIMPreeditDisable	(1L<<1)

typedef	struct	_XIMPreeditStateNotifyCallbackStruct {
    XIMPreeditState state;
} XIMPreeditStateNotifyCallbackStruct;

typedef	unsigned long	 XIMResetState;

#define	XIMInitialState		1L
#define	XIMPreserveState	(1L<<1)

typedef unsigned long XIMStringConversionFeedback;

#define	XIMStringConversionLeftEdge	(0x00000001)
#define	XIMStringConversionRightEdge	(0x00000002)
#define	XIMStringConversionTopEdge	(0x00000004)
#define	XIMStringConversionBottomEdge	(0x00000008)
#define	XIMStringConversionConcealed	(0x00000010)
#define	XIMStringConversionWrapped	(0x00000020)

typedef struct _XIMStringConversionText {
    unsigned short length;
    XIMStringConversionFeedback *feedback;
    Bool encoding_is_wchar; 
    union {
	char *mbs;
	wchar_t *wcs;
    } string; 
} XIMStringConversionText;

typedef	unsigned short	XIMStringConversionPosition;

typedef	unsigned short	XIMStringConversionType;

#define	XIMStringConversionBuffer	(0x0001)
#define	XIMStringConversionLine		(0x0002)
#define	XIMStringConversionWord		(0x0003)
#define	XIMStringConversionChar		(0x0004)

typedef	unsigned short	XIMStringConversionOperation;

#define	XIMStringConversionSubstitution	(0x0001)
#define	XIMStringConversionRetrieval	(0x0002)

typedef enum {
    XIMForwardChar, XIMBackwardChar,
    XIMForwardWord, XIMBackwardWord,
    XIMCaretUp, XIMCaretDown,
    XIMNextLine, XIMPreviousLine,
    XIMLineStart, XIMLineEnd, 
    XIMAbsolutePosition,
    XIMDontChange
} XIMCaretDirection;

typedef struct _XIMStringConversionCallbackStruct {
    XIMStringConversionPosition position;
    XIMCaretDirection direction;
    XIMStringConversionOperation operation;
    unsigned short factor;
    XIMStringConversionText *text;
} XIMStringConversionCallbackStruct;

typedef struct _XIMPreeditDrawCallbackStruct {
    int caret;		/* Cursor offset within pre-edit string */
    int chg_first;	/* Starting change position */
    int chg_length;	/* Length of the change in character count */
    XIMText *text;
} XIMPreeditDrawCallbackStruct;

typedef enum {
    XIMIsInvisible,	/* Disable caret feedback */ 
    XIMIsPrimary,	/* UI defined caret feedback */
    XIMIsSecondary	/* UI defined caret feedback */
} XIMCaretStyle;

typedef struct _XIMPreeditCaretCallbackStruct {
    int position;		 /* Caret offset within pre-edit string */
    XIMCaretDirection direction; /* Caret moves direction */
    XIMCaretStyle style;	 /* Feedback of the caret */
} XIMPreeditCaretCallbackStruct;

typedef enum {
    XIMTextType,
    XIMBitmapType
} XIMStatusDataType;
	
typedef struct _XIMStatusDrawCallbackStruct {
    XIMStatusDataType type;
    union {
	XIMText *text;
	Pixmap  bitmap;
    } data;
} XIMStatusDrawCallbackStruct;

typedef struct _XIMHotKeyTrigger {
    KeySym	 keysym;
    int		 modifier;
    int		 modifier_mask;
} XIMHotKeyTrigger;

typedef struct _XIMHotKeyTriggers {
    int			 num_hot_key;
    XIMHotKeyTrigger	*key;
} XIMHotKeyTriggers;

typedef	unsigned long	 XIMHotKeyState;

#define	XIMHotKeyStateON	(0x0001L)
#define	XIMHotKeyStateOFF	(0x0002L)

typedef struct {
    unsigned short count_values;
    char **supported_values;
} XIMValuesList;

#if 0
_XFUNCPROTOBEGIN

#if defined(WIN32) && !defined(_XLIBINT_)
#define _Xdebug (*_Xdebug_p)
#endif

extern int _Xdebug;

extern XFontStruct *XLoadQueryFont(
#if NeedFunctionPrototypes
    Display*		/* display */,
    _Xconst char*	/* name */
#endif
);

extern XFontStruct *XQueryFont(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XID			/* font_ID */
#endif
);


extern XTimeCoord *XGetMotionEvents(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Time		/* start */,
    Time		/* stop */,
    int*		/* nevents_return */
#endif
);

extern XModifierKeymap *XDeleteModifiermapEntry(
#if NeedFunctionPrototypes
    XModifierKeymap*	/* modmap */,
#if NeedWidePrototypes
    unsigned int	/* keycode_entry */,
#else
    KeyCode		/* keycode_entry */,
#endif
    int			/* modifier */
#endif
);

extern XModifierKeymap	*XGetModifierMapping(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern XModifierKeymap	*XInsertModifiermapEntry(
#if NeedFunctionPrototypes
    XModifierKeymap*	/* modmap */,
#if NeedWidePrototypes
    unsigned int	/* keycode_entry */,
#else
    KeyCode		/* keycode_entry */,
#endif
    int			/* modifier */    
#endif
);

extern XModifierKeymap *XNewModifiermap(
#if NeedFunctionPrototypes
    int			/* max_keys_per_mod */
#endif
);

extern XImage *XCreateImage(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Visual*		/* visual */,
    unsigned int	/* depth */,
    int			/* format */,
    int			/* offset */,
    char*		/* data */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    int			/* bitmap_pad */,
    int			/* bytes_per_line */
#endif
);
extern Status XInitImage(
#if NeedFunctionPrototypes
    XImage*		/* image */
#endif
);
extern XImage *XGetImage(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    int			/* x */,
    int			/* y */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    unsigned long	/* plane_mask */,
    int			/* format */
#endif
);
extern XImage *XGetSubImage(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    int			/* x */,
    int			/* y */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    unsigned long	/* plane_mask */,
    int			/* format */,
    XImage*		/* dest_image */,
    int			/* dest_x */,
    int			/* dest_y */
#endif
);

/* 
 * X function declarations.
 */
extern Display *XOpenDisplay(
#if NeedFunctionPrototypes
    _Xconst char*	/* display_name */
#endif
);

extern void XrmInitialize(
#if NeedFunctionPrototypes
    void
#endif
);

extern char *XFetchBytes(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int*		/* nbytes_return */
#endif
);
extern char *XFetchBuffer(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int*		/* nbytes_return */,
    int			/* buffer */
#endif
);
extern char *XGetAtomName(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Atom		/* atom */
#endif
);
extern Status XGetAtomNames(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    Atom*		/* atoms */,
    int			/* count */,
    char**		/* names_return */
#endif
);
extern char *XGetDefault(
#if NeedFunctionPrototypes
    Display*		/* display */,
    _Xconst char*	/* program */,
    _Xconst char*	/* option */		  
#endif
);
extern char *XDisplayName(
#if NeedFunctionPrototypes
    _Xconst char*	/* string */
#endif
);
extern char *XKeysymToString(
#if NeedFunctionPrototypes
    KeySym		/* keysym */
#endif
);

extern int (*XSynchronize(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Bool		/* onoff */
#endif
))(
#if NeedNestedPrototypes
    Display*		/* display */
#endif
);
extern int (*XSetAfterFunction(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int (*) (
#if NeedNestedPrototypes
	     Display*	/* display */
#endif
            )		/* procedure */
#endif
))(
#if NeedNestedPrototypes
    Display*		/* display */
#endif
);
extern Atom XInternAtom(
#if NeedFunctionPrototypes
    Display*		/* display */,
    _Xconst char*	/* atom_name */,
    Bool		/* only_if_exists */		 
#endif
);
extern Status XInternAtoms(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    char**		/* names */,
    int			/* count */,
    Bool		/* onlyIfExists */,
    Atom*		/* atoms_return */
#endif
);
extern Colormap XCopyColormapAndFree(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Colormap		/* colormap */
#endif
);
extern Colormap XCreateColormap(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Visual*		/* visual */,
    int			/* alloc */			 
#endif
);
extern Cursor XCreatePixmapCursor(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Pixmap		/* source */,
    Pixmap		/* mask */,
    XColor*		/* foreground_color */,
    XColor*		/* background_color */,
    unsigned int	/* x */,
    unsigned int	/* y */			   
#endif
);
extern Cursor XCreateGlyphCursor(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Font		/* source_font */,
    Font		/* mask_font */,
    unsigned int	/* source_char */,
    unsigned int	/* mask_char */,
    XColor _Xconst *	/* foreground_color */,
    XColor _Xconst *	/* background_color */
#endif
);
extern Cursor XCreateFontCursor(
#if NeedFunctionPrototypes
    Display*		/* display */,
    unsigned int	/* shape */
#endif
);
extern Font XLoadFont(
#if NeedFunctionPrototypes
    Display*		/* display */,
    _Xconst char*	/* name */
#endif
);
extern GC XCreateGC(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    unsigned long	/* valuemask */,
    XGCValues*		/* values */
#endif
);
extern GContext XGContextFromGC(
#if NeedFunctionPrototypes
    GC			/* gc */
#endif
);
extern void XFlushGC(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */
#endif
);
extern Pixmap XCreatePixmap(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    unsigned int	/* depth */		        
#endif
);
extern Pixmap XCreateBitmapFromData(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    _Xconst char*	/* data */,
    unsigned int	/* width */,
    unsigned int	/* height */
#endif
);
extern Pixmap XCreatePixmapFromBitmapData(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    char*		/* data */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    unsigned long	/* fg */,
    unsigned long	/* bg */,
    unsigned int	/* depth */
#endif
);
extern Window XCreateSimpleWindow(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* parent */,
    int			/* x */,
    int			/* y */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    unsigned int	/* border_width */,
    unsigned long	/* border */,
    unsigned long	/* background */
#endif
);
extern Window XGetSelectionOwner(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Atom		/* selection */
#endif
);
extern Window XCreateWindow(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* parent */,
    int			/* x */,
    int			/* y */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    unsigned int	/* border_width */,
    int			/* depth */,
    unsigned int	/* class */,
    Visual*		/* visual */,
    unsigned long	/* valuemask */,
    XSetWindowAttributes*	/* attributes */
#endif
); 
extern Colormap *XListInstalledColormaps(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    int*		/* num_return */
#endif
);
extern char **XListFonts(
#if NeedFunctionPrototypes
    Display*		/* display */,
    _Xconst char*	/* pattern */,
    int			/* maxnames */,
    int*		/* actual_count_return */
#endif
);
extern char **XListFontsWithInfo(
#if NeedFunctionPrototypes
    Display*		/* display */,
    _Xconst char*	/* pattern */,
    int			/* maxnames */,
    int*		/* count_return */,
    XFontStruct**	/* info_return */
#endif
);
extern char **XGetFontPath(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int*		/* npaths_return */
#endif
);
extern char **XListExtensions(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int*		/* nextensions_return */
#endif
);
extern Atom *XListProperties(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    int*		/* num_prop_return */
#endif
);
extern XHostAddress *XListHosts(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int*		/* nhosts_return */,
    Bool*		/* state_return */
#endif
);
extern KeySym XKeycodeToKeysym(
#if NeedFunctionPrototypes
    Display*		/* display */,
#if NeedWidePrototypes
    unsigned int	/* keycode */,
#else
    KeyCode		/* keycode */,
#endif
    int			/* index */
#endif
);
extern KeySym XLookupKeysym(
#if NeedFunctionPrototypes
    XKeyEvent*		/* key_event */,
    int			/* index */
#endif
);
extern KeySym *XGetKeyboardMapping(
#if NeedFunctionPrototypes
    Display*		/* display */,
#if NeedWidePrototypes
    unsigned int	/* first_keycode */,
#else
    KeyCode		/* first_keycode */,
#endif
    int			/* keycode_count */,
    int*		/* keysyms_per_keycode_return */
#endif
);
extern KeySym XStringToKeysym(
#if NeedFunctionPrototypes
    _Xconst char*	/* string */
#endif
);
extern long XMaxRequestSize(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);
extern long XExtendedMaxRequestSize(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);
extern char *XResourceManagerString(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);
extern char *XScreenResourceString(
#if NeedFunctionPrototypes
	Screen*		/* screen */
#endif
);
extern unsigned long XDisplayMotionBufferSize(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);
extern VisualID XVisualIDFromVisual(
#if NeedFunctionPrototypes
    Visual*		/* visual */
#endif
);

/* multithread routines */

extern Status XInitThreads(
#if NeedFunctionPrototypes
    void
#endif
);

extern void XLockDisplay(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern void XUnlockDisplay(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

/* routines for dealing with extensions */

extern XExtCodes *XInitExtension(
#if NeedFunctionPrototypes
    Display*		/* display */,
    _Xconst char*	/* name */
#endif
);

extern XExtCodes *XAddExtension(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);
extern XExtData *XFindOnExtensionList(
#if NeedFunctionPrototypes
    XExtData**		/* structure */,
    int			/* number */
#endif
);
extern XExtData **XEHeadOfExtensionList(
#if NeedFunctionPrototypes
    XEDataObject	/* object */
#endif
);

/* these are routines for which there are also macros */
extern Window XRootWindow(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen_number */
#endif
);
extern Window XDefaultRootWindow(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);
extern Window XRootWindowOfScreen(
#if NeedFunctionPrototypes
    Screen*		/* screen */
#endif
);
extern Visual *XDefaultVisual(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen_number */
#endif
);
extern Visual *XDefaultVisualOfScreen(
#if NeedFunctionPrototypes
    Screen*		/* screen */
#endif
);
extern GC XDefaultGC(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen_number */
#endif
);
extern GC XDefaultGCOfScreen(
#if NeedFunctionPrototypes
    Screen*		/* screen */
#endif
);
extern unsigned long XBlackPixel(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen_number */
#endif
);
extern unsigned long XWhitePixel(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen_number */
#endif
);
extern unsigned long XAllPlanes(
#if NeedFunctionPrototypes
    void
#endif
);
extern unsigned long XBlackPixelOfScreen(
#if NeedFunctionPrototypes
    Screen*		/* screen */
#endif
);
extern unsigned long XWhitePixelOfScreen(
#if NeedFunctionPrototypes
    Screen*		/* screen */
#endif
);
extern unsigned long XNextRequest(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);
extern unsigned long XLastKnownRequestProcessed(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);
extern char *XServerVendor(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);
extern char *XDisplayString(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);
extern Colormap XDefaultColormap(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen_number */
#endif
);
extern Colormap XDefaultColormapOfScreen(
#if NeedFunctionPrototypes
    Screen*		/* screen */
#endif
);
extern Display *XDisplayOfScreen(
#if NeedFunctionPrototypes
    Screen*		/* screen */
#endif
);
extern Screen *XScreenOfDisplay(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen_number */
#endif
);
extern Screen *XDefaultScreenOfDisplay(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);
extern long XEventMaskOfScreen(
#if NeedFunctionPrototypes
    Screen*		/* screen */
#endif
);

extern int XScreenNumberOfScreen(
#if NeedFunctionPrototypes
    Screen*		/* screen */
#endif
);

typedef int (*XErrorHandler) (	    /* WARNING, this type not in Xlib spec */
#if NeedFunctionPrototypes
    Display*		/* display */,
    XErrorEvent*	/* error_event */
#endif
);

extern XErrorHandler XSetErrorHandler (
#if NeedFunctionPrototypes
    XErrorHandler	/* handler */
#endif
);


typedef int (*XIOErrorHandler) (    /* WARNING, this type not in Xlib spec */
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern XIOErrorHandler XSetIOErrorHandler (
#if NeedFunctionPrototypes
    XIOErrorHandler	/* handler */
#endif
);

extern XPixmapFormatValues *XListPixmapFormats(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int*		/* count_return */
#endif
);
extern int *XListDepths(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen_number */,
    int*		/* count_return */
#endif
);

/* ICCCM routines for things that don't require special include files; */
/* other declarations are given in Xutil.h                             */
extern Status XReconfigureWMWindow(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    int			/* screen_number */,
    unsigned int	/* mask */,
    XWindowChanges*	/* changes */
#endif
);

extern Status XGetWMProtocols(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Atom**		/* protocols_return */,
    int*		/* count_return */
#endif
);
extern Status XSetWMProtocols(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Atom*		/* protocols */,
    int			/* count */
#endif
);
extern Status XIconifyWindow(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    int			/* screen_number */
#endif
);
extern Status XWithdrawWindow(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    int			/* screen_number */
#endif
);
extern Status XGetCommand(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    char***		/* argv_return */,
    int*		/* argc_return */
#endif
);
extern Status XGetWMColormapWindows(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Window**		/* windows_return */,
    int*		/* count_return */
#endif
);
extern Status XSetWMColormapWindows(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Window*		/* colormap_windows */,
    int			/* count */
#endif
);
extern void XFreeStringList(
#if NeedFunctionPrototypes
    char**		/* list */
#endif
);
extern int XSetTransientForHint(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Window		/* prop_window */
#endif
);

/* The following are given in alphabetical order */

extern int XActivateScreenSaver(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern int XAddHost(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XHostAddress*	/* host */
#endif
);

extern int XAddHosts(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XHostAddress*	/* hosts */,
    int			/* num_hosts */    
#endif
);

extern int XAddToExtensionList(
#if NeedFunctionPrototypes
    struct _XExtData**	/* structure */,
    XExtData*		/* ext_data */
#endif
);

extern int XAddToSaveSet(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */
#endif
);

extern Status XAllocColor(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Colormap		/* colormap */,
    XColor*		/* screen_in_out */
#endif
);

extern Status XAllocColorCells(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Colormap		/* colormap */,
    Bool	        /* contig */,
    unsigned long*	/* plane_masks_return */,
    unsigned int	/* nplanes */,
    unsigned long*	/* pixels_return */,
    unsigned int 	/* npixels */
#endif
);

extern Status XAllocColorPlanes(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Colormap		/* colormap */,
    Bool		/* contig */,
    unsigned long*	/* pixels_return */,
    int			/* ncolors */,
    int			/* nreds */,
    int			/* ngreens */,
    int			/* nblues */,
    unsigned long*	/* rmask_return */,
    unsigned long*	/* gmask_return */,
    unsigned long*	/* bmask_return */
#endif
);

extern Status XAllocNamedColor(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Colormap		/* colormap */,
    _Xconst char*	/* color_name */,
    XColor*		/* screen_def_return */,
    XColor*		/* exact_def_return */
#endif
);

extern int XAllowEvents(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* event_mode */,
    Time		/* time */
#endif
);

extern int XAutoRepeatOff(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern int XAutoRepeatOn(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern int XBell(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* percent */
#endif
);

extern int XBitmapBitOrder(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern int XBitmapPad(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern int XBitmapUnit(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern int XCellsOfScreen(
#if NeedFunctionPrototypes
    Screen*		/* screen */
#endif
);

extern int XChangeActivePointerGrab(
#if NeedFunctionPrototypes
    Display*		/* display */,
    unsigned int	/* event_mask */,
    Cursor		/* cursor */,
    Time		/* time */
#endif
);

extern int XChangeGC(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    unsigned long	/* valuemask */,
    XGCValues*		/* values */
#endif
);

extern int XChangeKeyboardControl(
#if NeedFunctionPrototypes
    Display*		/* display */,
    unsigned long	/* value_mask */,
    XKeyboardControl*	/* values */
#endif
);

extern int XChangeKeyboardMapping(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* first_keycode */,
    int			/* keysyms_per_keycode */,
    KeySym*		/* keysyms */,
    int			/* num_codes */
#endif
);

extern int XChangePointerControl(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Bool		/* do_accel */,
    Bool		/* do_threshold */,
    int			/* accel_numerator */,
    int			/* accel_denominator */,
    int			/* threshold */
#endif
);

extern int XChangeProperty(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Atom		/* property */,
    Atom		/* type */,
    int			/* format */,
    int			/* mode */,
    _Xconst unsigned char*	/* data */,
    int			/* nelements */
#endif
);

extern int XChangeSaveSet(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    int			/* change_mode */
#endif
);

extern int XChangeWindowAttributes(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    unsigned long	/* valuemask */,
    XSetWindowAttributes* /* attributes */
#endif
);

extern Bool XCheckIfEvent(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XEvent*		/* event_return */,
    Bool (*) (
#if NeedNestedPrototypes
	       Display*			/* display */,
               XEvent*			/* event */,
               XPointer			/* arg */
#endif
             )		/* predicate */,
    XPointer		/* arg */
#endif
);

extern Bool XCheckMaskEvent(
#if NeedFunctionPrototypes
    Display*		/* display */,
    long		/* event_mask */,
    XEvent*		/* event_return */
#endif
);

extern Bool XCheckTypedEvent(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* event_type */,
    XEvent*		/* event_return */
#endif
);

extern Bool XCheckTypedWindowEvent(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    int			/* event_type */,
    XEvent*		/* event_return */
#endif
);

extern Bool XCheckWindowEvent(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    long		/* event_mask */,
    XEvent*		/* event_return */
#endif
);

extern int XCirculateSubwindows(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    int			/* direction */
#endif
);

extern int XCirculateSubwindowsDown(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */
#endif
);

extern int XCirculateSubwindowsUp(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */
#endif
);

extern int XClearArea(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    int			/* x */,
    int			/* y */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    Bool		/* exposures */
#endif
);

extern int XClearWindow(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */
#endif
);

extern int XCloseDisplay(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern int XConfigureWindow(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    unsigned int	/* value_mask */,
    XWindowChanges*	/* values */		 
#endif
);

extern int XConnectionNumber(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern int XConvertSelection(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Atom		/* selection */,
    Atom 		/* target */,
    Atom		/* property */,
    Window		/* requestor */,
    Time		/* time */
#endif
);

extern int XCopyArea(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* src */,
    Drawable		/* dest */,
    GC			/* gc */,
    int			/* src_x */,
    int			/* src_y */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    int			/* dest_x */,
    int			/* dest_y */
#endif
);

extern int XCopyGC(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* src */,
    unsigned long	/* valuemask */,
    GC			/* dest */
#endif
);

extern int XCopyPlane(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* src */,
    Drawable		/* dest */,
    GC			/* gc */,
    int			/* src_x */,
    int			/* src_y */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    int			/* dest_x */,
    int			/* dest_y */,
    unsigned long	/* plane */
#endif
);

extern int XDefaultDepth(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen_number */
#endif
);

extern int XDefaultDepthOfScreen(
#if NeedFunctionPrototypes
    Screen*		/* screen */
#endif
);

extern int XDefaultScreen(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern int XDefineCursor(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Cursor		/* cursor */
#endif
);

extern int XDeleteProperty(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Atom		/* property */
#endif
);

extern int XDestroyWindow(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */
#endif
);

extern int XDestroySubwindows(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */
#endif
);

extern int XDoesBackingStore(
#if NeedFunctionPrototypes
    Screen*		/* screen */    
#endif
);

extern Bool XDoesSaveUnders(
#if NeedFunctionPrototypes
    Screen*		/* screen */
#endif
);

extern int XDisableAccessControl(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);


extern int XDisplayCells(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen_number */
#endif
);

extern int XDisplayHeight(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen_number */
#endif
);

extern int XDisplayHeightMM(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen_number */
#endif
);

extern int XDisplayKeycodes(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int*		/* min_keycodes_return */,
    int*		/* max_keycodes_return */
#endif
);

extern int XDisplayPlanes(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen_number */
#endif
);

extern int XDisplayWidth(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen_number */
#endif
);

extern int XDisplayWidthMM(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen_number */
#endif
);

extern int XDrawArc(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    int			/* angle1 */,
    int			/* angle2 */
#endif
);

extern int XDrawArcs(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    XArc*		/* arcs */,
    int			/* narcs */
#endif
);

extern int XDrawImageString(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    _Xconst char*	/* string */,
    int			/* length */
#endif
);

extern int XDrawImageString16(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    _Xconst XChar2b*	/* string */,
    int			/* length */
#endif
);

extern int XDrawLine(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    int			/* x1 */,
    int			/* y1 */,
    int			/* x2 */,
    int			/* y2 */
#endif
);

extern int XDrawLines(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    XPoint*		/* points */,
    int			/* npoints */,
    int			/* mode */
#endif
);

extern int XDrawPoint(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */
#endif
);

extern int XDrawPoints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    XPoint*		/* points */,
    int			/* npoints */,
    int			/* mode */
#endif
);

extern int XDrawRectangle(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    unsigned int	/* width */,
    unsigned int	/* height */
#endif
);

extern int XDrawRectangles(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    XRectangle*		/* rectangles */,
    int			/* nrectangles */
#endif
);

extern int XDrawSegments(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    XSegment*		/* segments */,
    int			/* nsegments */
#endif
);

extern int XDrawString(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    _Xconst char*	/* string */,
    int			/* length */
#endif
);

extern int XDrawString16(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    _Xconst XChar2b*	/* string */,
    int			/* length */
#endif
);

extern int XDrawText(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    XTextItem*		/* items */,
    int			/* nitems */
#endif
);

extern int XDrawText16(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    XTextItem16*	/* items */,
    int			/* nitems */
#endif
);

extern int XEnableAccessControl(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern int XEventsQueued(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* mode */
#endif
);

extern Status XFetchName(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    char**		/* window_name_return */
#endif
);

extern int XFillArc(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    int			/* angle1 */,
    int			/* angle2 */
#endif
);

extern int XFillArcs(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    XArc*		/* arcs */,
    int			/* narcs */
#endif
);

extern int XFillPolygon(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    XPoint*		/* points */,
    int			/* npoints */,
    int			/* shape */,
    int			/* mode */
#endif
);

extern int XFillRectangle(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    unsigned int	/* width */,
    unsigned int	/* height */
#endif
);

extern int XFillRectangles(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    XRectangle*		/* rectangles */,
    int			/* nrectangles */
#endif
);

extern int XFlush(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern int XForceScreenSaver(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* mode */
#endif
);

extern int XFree(
#if NeedFunctionPrototypes
    void*		/* data */
#endif
);

extern int XFreeColormap(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Colormap		/* colormap */
#endif
);

extern int XFreeColors(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Colormap		/* colormap */,
    unsigned long*	/* pixels */,
    int			/* npixels */,
    unsigned long	/* planes */
#endif
);

extern int XFreeCursor(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Cursor		/* cursor */
#endif
);

extern int XFreeExtensionList(
#if NeedFunctionPrototypes
    char**		/* list */    
#endif
);

extern int XFreeFont(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XFontStruct*	/* font_struct */
#endif
);

extern int XFreeFontInfo(
#if NeedFunctionPrototypes
    char**		/* names */,
    XFontStruct*	/* free_info */,
    int			/* actual_count */
#endif
);

extern int XFreeFontNames(
#if NeedFunctionPrototypes
    char**		/* list */
#endif
);

extern int XFreeFontPath(
#if NeedFunctionPrototypes
    char**		/* list */
#endif
);

extern int XFreeGC(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */
#endif
);

extern int XFreeModifiermap(
#if NeedFunctionPrototypes
    XModifierKeymap*	/* modmap */
#endif
);

extern int XFreePixmap(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Pixmap		/* pixmap */
#endif
);

extern int XGeometry(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen */,
    _Xconst char*	/* position */,
    _Xconst char*	/* default_position */,
    unsigned int	/* bwidth */,
    unsigned int	/* fwidth */,
    unsigned int	/* fheight */,
    int			/* xadder */,
    int			/* yadder */,
    int*		/* x_return */,
    int*		/* y_return */,
    int*		/* width_return */,
    int*		/* height_return */
#endif
);

extern int XGetErrorDatabaseText(
#if NeedFunctionPrototypes
    Display*		/* display */,
    _Xconst char*	/* name */,
    _Xconst char*	/* message */,
    _Xconst char*	/* default_string */,
    char*		/* buffer_return */,
    int			/* length */
#endif
);

extern int XGetErrorText(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* code */,
    char*		/* buffer_return */,
    int			/* length */
#endif
);

extern Bool XGetFontProperty(
#if NeedFunctionPrototypes
    XFontStruct*	/* font_struct */,
    Atom		/* atom */,
    unsigned long*	/* value_return */
#endif
);

extern Status XGetGCValues(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    unsigned long	/* valuemask */,
    XGCValues*		/* values_return */
#endif
);

extern Status XGetGeometry(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    Window*		/* root_return */,
    int*		/* x_return */,
    int*		/* y_return */,
    unsigned int*	/* width_return */,
    unsigned int*	/* height_return */,
    unsigned int*	/* border_width_return */,
    unsigned int*	/* depth_return */
#endif
);

extern Status XGetIconName(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    char**		/* icon_name_return */
#endif
);

extern int XGetInputFocus(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window*		/* focus_return */,
    int*		/* revert_to_return */
#endif
);

extern int XGetKeyboardControl(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XKeyboardState*	/* values_return */
#endif
);

extern int XGetPointerControl(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int*		/* accel_numerator_return */,
    int*		/* accel_denominator_return */,
    int*		/* threshold_return */
#endif
);

extern int XGetPointerMapping(
#if NeedFunctionPrototypes
    Display*		/* display */,
    unsigned char*	/* map_return */,
    int			/* nmap */
#endif
);

extern int XGetScreenSaver(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int*		/* timeout_return */,
    int*		/* interval_return */,
    int*		/* prefer_blanking_return */,
    int*		/* allow_exposures_return */
#endif
);

extern Status XGetTransientForHint(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Window*		/* prop_window_return */
#endif
);

extern int XGetWindowProperty(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Atom		/* property */,
    long		/* long_offset */,
    long		/* long_length */,
    Bool		/* delete */,
    Atom		/* req_type */,
    Atom*		/* actual_type_return */,
    int*		/* actual_format_return */,
    unsigned long*	/* nitems_return */,
    unsigned long*	/* bytes_after_return */,
    unsigned char**	/* prop_return */
#endif
);

extern Status XGetWindowAttributes(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XWindowAttributes*	/* window_attributes_return */
#endif
);

extern int XGrabButton(
#if NeedFunctionPrototypes
    Display*		/* display */,
    unsigned int	/* button */,
    unsigned int	/* modifiers */,
    Window		/* grab_window */,
    Bool		/* owner_events */,
    unsigned int	/* event_mask */,
    int			/* pointer_mode */,
    int			/* keyboard_mode */,
    Window		/* confine_to */,
    Cursor		/* cursor */
#endif
);

extern int XGrabKey(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* keycode */,
    unsigned int	/* modifiers */,
    Window		/* grab_window */,
    Bool		/* owner_events */,
    int			/* pointer_mode */,
    int			/* keyboard_mode */
#endif
);

extern int XGrabKeyboard(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* grab_window */,
    Bool		/* owner_events */,
    int			/* pointer_mode */,
    int			/* keyboard_mode */,
    Time		/* time */
#endif
);

extern int XGrabPointer(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* grab_window */,
    Bool		/* owner_events */,
    unsigned int	/* event_mask */,
    int			/* pointer_mode */,
    int			/* keyboard_mode */,
    Window		/* confine_to */,
    Cursor		/* cursor */,
    Time		/* time */
#endif
);

extern int XGrabServer(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern int XHeightMMOfScreen(
#if NeedFunctionPrototypes
    Screen*		/* screen */
#endif
);

extern int XHeightOfScreen(
#if NeedFunctionPrototypes
    Screen*		/* screen */
#endif
);

extern int XIfEvent(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XEvent*		/* event_return */,
    Bool (*) (
#if NeedNestedPrototypes
	       Display*			/* display */,
               XEvent*			/* event */,
               XPointer			/* arg */
#endif
             )		/* predicate */,
    XPointer		/* arg */
#endif
);

extern int XImageByteOrder(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern int XInstallColormap(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Colormap		/* colormap */
#endif
);

extern KeyCode XKeysymToKeycode(
#if NeedFunctionPrototypes
    Display*		/* display */,
    KeySym		/* keysym */
#endif
);

extern int XKillClient(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XID			/* resource */
#endif
);

extern Status XLookupColor(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Colormap		/* colormap */,
    _Xconst char*	/* color_name */,
    XColor*		/* exact_def_return */,
    XColor*		/* screen_def_return */
#endif
);

extern int XLowerWindow(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */
#endif
);

extern int XMapRaised(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */
#endif
);

extern int XMapSubwindows(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */
#endif
);

extern int XMapWindow(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */
#endif
);

extern int XMaskEvent(
#if NeedFunctionPrototypes
    Display*		/* display */,
    long		/* event_mask */,
    XEvent*		/* event_return */
#endif
);

extern int XMaxCmapsOfScreen(
#if NeedFunctionPrototypes
    Screen*		/* screen */
#endif
);

extern int XMinCmapsOfScreen(
#if NeedFunctionPrototypes
    Screen*		/* screen */
#endif
);

extern int XMoveResizeWindow(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    int			/* x */,
    int			/* y */,
    unsigned int	/* width */,
    unsigned int	/* height */
#endif
);

extern int XMoveWindow(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    int			/* x */,
    int			/* y */
#endif
);

extern int XNextEvent(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XEvent*		/* event_return */
#endif
);

extern int XNoOp(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern Status XParseColor(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Colormap		/* colormap */,
    _Xconst char*	/* spec */,
    XColor*		/* exact_def_return */
#endif
);

extern int XParseGeometry(
#if NeedFunctionPrototypes
    _Xconst char*	/* parsestring */,
    int*		/* x_return */,
    int*		/* y_return */,
    unsigned int*	/* width_return */,
    unsigned int*	/* height_return */
#endif
);

extern int XPeekEvent(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XEvent*		/* event_return */
#endif
);

extern int XPeekIfEvent(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XEvent*		/* event_return */,
    Bool (*) (
#if NeedNestedPrototypes
	       Display*		/* display */,
               XEvent*		/* event */,
               XPointer		/* arg */
#endif
             )		/* predicate */,
    XPointer		/* arg */
#endif
);

extern int XPending(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern int XPlanesOfScreen(
#if NeedFunctionPrototypes
    Screen*		/* screen */
    
#endif
);

extern int XProtocolRevision(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern int XProtocolVersion(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);


extern int XPutBackEvent(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XEvent*		/* event */
#endif
);

extern int XPutImage(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    XImage*		/* image */,
    int			/* src_x */,
    int			/* src_y */,
    int			/* dest_x */,
    int			/* dest_y */,
    unsigned int	/* width */,
    unsigned int	/* height */	  
#endif
);

extern int XQLength(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern Status XQueryBestCursor(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    unsigned int        /* width */,
    unsigned int	/* height */,
    unsigned int*	/* width_return */,
    unsigned int*	/* height_return */
#endif
);

extern Status XQueryBestSize(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* class */,
    Drawable		/* which_screen */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    unsigned int*	/* width_return */,
    unsigned int*	/* height_return */
#endif
);

extern Status XQueryBestStipple(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* which_screen */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    unsigned int*	/* width_return */,
    unsigned int*	/* height_return */
#endif
);

extern Status XQueryBestTile(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* which_screen */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    unsigned int*	/* width_return */,
    unsigned int*	/* height_return */
#endif
);

extern int XQueryColor(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Colormap		/* colormap */,
    XColor*		/* def_in_out */
#endif
);

extern int XQueryColors(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Colormap		/* colormap */,
    XColor*		/* defs_in_out */,
    int			/* ncolors */
#endif
);

extern Bool XQueryExtension(
#if NeedFunctionPrototypes
    Display*		/* display */,
    _Xconst char*	/* name */,
    int*		/* major_opcode_return */,
    int*		/* first_event_return */,
    int*		/* first_error_return */
#endif
);

extern int XQueryKeymap(
#if NeedFunctionPrototypes
    Display*		/* display */,
    char [32]		/* keys_return */
#endif
);

extern Bool XQueryPointer(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Window*		/* root_return */,
    Window*		/* child_return */,
    int*		/* root_x_return */,
    int*		/* root_y_return */,
    int*		/* win_x_return */,
    int*		/* win_y_return */,
    unsigned int*       /* mask_return */
#endif
);

extern int XQueryTextExtents(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XID			/* font_ID */,
    _Xconst char*	/* string */,
    int			/* nchars */,
    int*		/* direction_return */,
    int*		/* font_ascent_return */,
    int*		/* font_descent_return */,
    XCharStruct*	/* overall_return */    
#endif
);

extern int XQueryTextExtents16(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XID			/* font_ID */,
    _Xconst XChar2b*	/* string */,
    int			/* nchars */,
    int*		/* direction_return */,
    int*		/* font_ascent_return */,
    int*		/* font_descent_return */,
    XCharStruct*	/* overall_return */
#endif
);

extern Status XQueryTree(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Window*		/* root_return */,
    Window*		/* parent_return */,
    Window**		/* children_return */,
    unsigned int*	/* nchildren_return */
#endif
);

extern int XRaiseWindow(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */
#endif
);

extern int XReadBitmapFile(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable 		/* d */,
    _Xconst char*	/* filename */,
    unsigned int*	/* width_return */,
    unsigned int*	/* height_return */,
    Pixmap*		/* bitmap_return */,
    int*		/* x_hot_return */,
    int*		/* y_hot_return */
#endif
);

extern int XReadBitmapFileData(
#if NeedFunctionPrototypes
    _Xconst char*	/* filename */,
    unsigned int*	/* width_return */,
    unsigned int*	/* height_return */,
    unsigned char**	/* data_return */,
    int*		/* x_hot_return */,
    int*		/* y_hot_return */
#endif
);

extern int XRebindKeysym(
#if NeedFunctionPrototypes
    Display*		/* display */,
    KeySym		/* keysym */,
    KeySym*		/* list */,
    int			/* mod_count */,
    _Xconst unsigned char*	/* string */,
    int			/* bytes_string */
#endif
);

extern int XRecolorCursor(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Cursor		/* cursor */,
    XColor*		/* foreground_color */,
    XColor*		/* background_color */
#endif
);

extern int XRefreshKeyboardMapping(
#if NeedFunctionPrototypes
    XMappingEvent*	/* event_map */    
#endif
);

extern int XRemoveFromSaveSet(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */
#endif
);

extern int XRemoveHost(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XHostAddress*	/* host */
#endif
);

extern int XRemoveHosts(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XHostAddress*	/* hosts */,
    int			/* num_hosts */
#endif
);

extern int XReparentWindow(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Window		/* parent */,
    int			/* x */,
    int			/* y */
#endif
);

extern int XResetScreenSaver(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern int XResizeWindow(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    unsigned int	/* width */,
    unsigned int	/* height */
#endif
);

extern int XRestackWindows(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window*		/* windows */,
    int			/* nwindows */
#endif
);

extern int XRotateBuffers(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* rotate */
#endif
);

extern int XRotateWindowProperties(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Atom*		/* properties */,
    int			/* num_prop */,
    int			/* npositions */
#endif
);

extern int XScreenCount(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern int XSelectInput(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    long		/* event_mask */
#endif
);

extern Status XSendEvent(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Bool		/* propagate */,
    long		/* event_mask */,
    XEvent*		/* event_send */
#endif
);

extern int XSetAccessControl(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* mode */
#endif
);

extern int XSetArcMode(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    int			/* arc_mode */
#endif
);

extern int XSetBackground(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    unsigned long	/* background */
#endif
);

extern int XSetClipMask(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    Pixmap		/* pixmap */
#endif
);

extern int XSetClipOrigin(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    int			/* clip_x_origin */,
    int			/* clip_y_origin */
#endif
);

extern int XSetClipRectangles(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    int			/* clip_x_origin */,
    int			/* clip_y_origin */,
    XRectangle*		/* rectangles */,
    int			/* n */,
    int			/* ordering */
#endif
);

extern int XSetCloseDownMode(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* close_mode */
#endif
);

extern int XSetCommand(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    char**		/* argv */,
    int			/* argc */
#endif
);

extern int XSetDashes(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    int			/* dash_offset */,
    _Xconst char*	/* dash_list */,
    int			/* n */
#endif
);

extern int XSetFillRule(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    int			/* fill_rule */
#endif
);

extern int XSetFillStyle(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    int			/* fill_style */
#endif
);

extern int XSetFont(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    Font		/* font */
#endif
);

extern int XSetFontPath(
#if NeedFunctionPrototypes
    Display*		/* display */,
    char**		/* directories */,
    int			/* ndirs */	     
#endif
);

extern int XSetForeground(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    unsigned long	/* foreground */
#endif
);

extern int XSetFunction(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    int			/* function */
#endif
);

extern int XSetGraphicsExposures(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    Bool		/* graphics_exposures */
#endif
);

extern int XSetIconName(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    _Xconst char*	/* icon_name */
#endif
);

extern int XSetInputFocus(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* focus */,
    int			/* revert_to */,
    Time		/* time */
#endif
);

extern int XSetLineAttributes(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    unsigned int	/* line_width */,
    int			/* line_style */,
    int			/* cap_style */,
    int			/* join_style */
#endif
);

extern int XSetModifierMapping(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XModifierKeymap*	/* modmap */
#endif
);

extern int XSetPlaneMask(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    unsigned long	/* plane_mask */
#endif
);

extern int XSetPointerMapping(
#if NeedFunctionPrototypes
    Display*		/* display */,
    _Xconst unsigned char*	/* map */,
    int			/* nmap */
#endif
);

extern int XSetScreenSaver(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* timeout */,
    int			/* interval */,
    int			/* prefer_blanking */,
    int			/* allow_exposures */
#endif
);

extern int XSetSelectionOwner(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Atom	        /* selection */,
    Window		/* owner */,
    Time		/* time */
#endif
);

extern int XSetState(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    unsigned long 	/* foreground */,
    unsigned long	/* background */,
    int			/* function */,
    unsigned long	/* plane_mask */
#endif
);

extern int XSetStipple(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    Pixmap		/* stipple */
#endif
);

extern int XSetSubwindowMode(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    int			/* subwindow_mode */
#endif
);

extern int XSetTSOrigin(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    int			/* ts_x_origin */,
    int			/* ts_y_origin */
#endif
);

extern int XSetTile(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    Pixmap		/* tile */
#endif
);

extern int XSetWindowBackground(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    unsigned long	/* background_pixel */
#endif
);

extern int XSetWindowBackgroundPixmap(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Pixmap		/* background_pixmap */
#endif
);

extern int XSetWindowBorder(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    unsigned long	/* border_pixel */
#endif
);

extern int XSetWindowBorderPixmap(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Pixmap		/* border_pixmap */
#endif
);

extern int XSetWindowBorderWidth(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    unsigned int	/* width */
#endif
);

extern int XSetWindowColormap(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    Colormap		/* colormap */
#endif
);

extern int XStoreBuffer(
#if NeedFunctionPrototypes
    Display*		/* display */,
    _Xconst char*	/* bytes */,
    int			/* nbytes */,
    int			/* buffer */
#endif
);

extern int XStoreBytes(
#if NeedFunctionPrototypes
    Display*		/* display */,
    _Xconst char*	/* bytes */,
    int			/* nbytes */
#endif
);

extern int XStoreColor(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Colormap		/* colormap */,
    XColor*		/* color */
#endif
);

extern int XStoreColors(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Colormap		/* colormap */,
    XColor*		/* color */,
    int			/* ncolors */
#endif
);

extern int XStoreName(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    _Xconst char*	/* window_name */
#endif
);

extern int XStoreNamedColor(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Colormap		/* colormap */,
    _Xconst char*	/* color */,
    unsigned long	/* pixel */,
    int			/* flags */
#endif
);

extern int XSync(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Bool		/* discard */
#endif
);

extern int XTextExtents(
#if NeedFunctionPrototypes
    XFontStruct*	/* font_struct */,
    _Xconst char*	/* string */,
    int			/* nchars */,
    int*		/* direction_return */,
    int*		/* font_ascent_return */,
    int*		/* font_descent_return */,
    XCharStruct*	/* overall_return */
#endif
);

extern int XTextExtents16(
#if NeedFunctionPrototypes
    XFontStruct*	/* font_struct */,
    _Xconst XChar2b*	/* string */,
    int			/* nchars */,
    int*		/* direction_return */,
    int*		/* font_ascent_return */,
    int*		/* font_descent_return */,
    XCharStruct*	/* overall_return */
#endif
);

extern int XTextWidth(
#if NeedFunctionPrototypes
    XFontStruct*	/* font_struct */,
    _Xconst char*	/* string */,
    int			/* count */
#endif
);

extern int XTextWidth16(
#if NeedFunctionPrototypes
    XFontStruct*	/* font_struct */,
    _Xconst XChar2b*	/* string */,
    int			/* count */
#endif
);

extern Bool XTranslateCoordinates(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* src_w */,
    Window		/* dest_w */,
    int			/* src_x */,
    int			/* src_y */,
    int*		/* dest_x_return */,
    int*		/* dest_y_return */,
    Window*		/* child_return */
#endif
);

extern int XUndefineCursor(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */
#endif
);

extern int XUngrabButton(
#if NeedFunctionPrototypes
    Display*		/* display */,
    unsigned int	/* button */,
    unsigned int	/* modifiers */,
    Window		/* grab_window */
#endif
);

extern int XUngrabKey(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* keycode */,
    unsigned int	/* modifiers */,
    Window		/* grab_window */
#endif
);

extern int XUngrabKeyboard(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Time		/* time */
#endif
);

extern int XUngrabPointer(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Time		/* time */
#endif
);

extern int XUngrabServer(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern int XUninstallColormap(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Colormap		/* colormap */
#endif
);

extern int XUnloadFont(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Font		/* font */
#endif
);

extern int XUnmapSubwindows(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */
#endif
);

extern int XUnmapWindow(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */
#endif
);

extern int XVendorRelease(
#if NeedFunctionPrototypes
    Display*		/* display */
#endif
);

extern int XWarpPointer(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* src_w */,
    Window		/* dest_w */,
    int			/* src_x */,
    int			/* src_y */,
    unsigned int	/* src_width */,
    unsigned int	/* src_height */,
    int			/* dest_x */,
    int			/* dest_y */	     
#endif
);

extern int XWidthMMOfScreen(
#if NeedFunctionPrototypes
    Screen*		/* screen */
#endif
);

extern int XWidthOfScreen(
#if NeedFunctionPrototypes
    Screen*		/* screen */
#endif
);

extern int XWindowEvent(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    long		/* event_mask */,
    XEvent*		/* event_return */
#endif
);

extern int XWriteBitmapFile(
#if NeedFunctionPrototypes
    Display*		/* display */,
    _Xconst char*	/* filename */,
    Pixmap		/* bitmap */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    int			/* x_hot */,
    int			/* y_hot */		     
#endif
);

extern Bool XSupportsLocale (void);

extern char *XSetLocaleModifiers(
    const char*		/* modifier_list */
);

extern XOM XOpenOM(
#if NeedFunctionPrototypes
    Display*			/* display */,
    struct _XrmHashBucketRec*	/* rdb */,
    _Xconst char*		/* res_name */,
    _Xconst char*		/* res_class */
#endif
);

extern Status XCloseOM(
#if NeedFunctionPrototypes
    XOM			/* om */
#endif
);

extern char *XSetOMValues(
#if NeedVarargsPrototypes
    XOM			/* om */,
    ...
#endif
);

extern char *XGetOMValues(
#if NeedVarargsPrototypes
    XOM			/* om */,
    ...
#endif
);

extern Display *XDisplayOfOM(
#if NeedFunctionPrototypes
    XOM			/* om */
#endif
);

extern char *XLocaleOfOM(
#if NeedFunctionPrototypes
    XOM			/* om */
#endif
);

extern XOC XCreateOC(
#if NeedVarargsPrototypes
    XOM			/* om */,
    ...
#endif
);

extern void XDestroyOC(
#if NeedFunctionPrototypes
    XOC			/* oc */
#endif
);

extern XOM XOMOfOC(
#if NeedFunctionPrototypes
    XOC			/* oc */
#endif
);

extern char *XSetOCValues(
#if NeedVarargsPrototypes
    XOC			/* oc */,
    ...
#endif
);

extern char *XGetOCValues(
#if NeedVarargsPrototypes
    XOC			/* oc */,
    ...
#endif
);

extern XFontSet XCreateFontSet(
#if NeedFunctionPrototypes
    Display*		/* display */,
    _Xconst char*	/* base_font_name_list */,
    char***		/* missing_charset_list */,
    int*		/* missing_charset_count */,
    char**		/* def_string */
#endif
);

extern void XFreeFontSet(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XFontSet		/* font_set */
#endif
);

extern int XFontsOfFontSet(
#if NeedFunctionPrototypes
    XFontSet		/* font_set */,
    XFontStruct***	/* font_struct_list */,
    char***		/* font_name_list */
#endif
);

extern char *XBaseFontNameListOfFontSet(
#if NeedFunctionPrototypes
    XFontSet		/* font_set */
#endif
);

extern char *XLocaleOfFontSet(
#if NeedFunctionPrototypes
    XFontSet		/* font_set */
#endif
);

extern Bool XContextDependentDrawing(
#if NeedFunctionPrototypes
    XFontSet		/* font_set */
#endif
);

extern Bool XDirectionalDependentDrawing(
#if NeedFunctionPrototypes
    XFontSet		/* font_set */
#endif
);

extern Bool XContextualDrawing(
#if NeedFunctionPrototypes
    XFontSet		/* font_set */
#endif
);

extern XFontSetExtents *XExtentsOfFontSet(
#if NeedFunctionPrototypes
    XFontSet		/* font_set */
#endif
);

extern int XmbTextEscapement(
#if NeedFunctionPrototypes
    XFontSet		/* font_set */,
    _Xconst char*	/* text */,
    int			/* bytes_text */
#endif
);

extern int XwcTextEscapement(
#if NeedFunctionPrototypes
    XFontSet		/* font_set */,
    _Xconst wchar_t*	/* text */,
    int			/* num_wchars */
#endif
);

extern int Xutf8TextEscapement(
#if NeedFunctionPrototypes
    XFontSet		/* font_set */,
    _Xconst char*	/* text */,
    int			/* bytes_text */
#endif
);

extern int XmbTextExtents(
#if NeedFunctionPrototypes
    XFontSet		/* font_set */,
    _Xconst char*	/* text */,
    int			/* bytes_text */,
    XRectangle*		/* overall_ink_return */,
    XRectangle*		/* overall_logical_return */
#endif
);

extern int XwcTextExtents(
#if NeedFunctionPrototypes
    XFontSet		/* font_set */,
    _Xconst wchar_t*	/* text */,
    int			/* num_wchars */,
    XRectangle*		/* overall_ink_return */,
    XRectangle*		/* overall_logical_return */
#endif
);

extern int Xutf8TextExtents(
#if NeedFunctionPrototypes
    XFontSet		/* font_set */,
    _Xconst char*	/* text */,
    int			/* bytes_text */,
    XRectangle*		/* overall_ink_return */,
    XRectangle*		/* overall_logical_return */
#endif
);

extern Status XmbTextPerCharExtents(
#if NeedFunctionPrototypes
    XFontSet		/* font_set */,
    _Xconst char*	/* text */,
    int			/* bytes_text */,
    XRectangle*		/* ink_extents_buffer */,
    XRectangle*		/* logical_extents_buffer */,
    int			/* buffer_size */,
    int*		/* num_chars */,
    XRectangle*		/* overall_ink_return */,
    XRectangle*		/* overall_logical_return */
#endif
);

extern Status XwcTextPerCharExtents(
#if NeedFunctionPrototypes
    XFontSet		/* font_set */,
    _Xconst wchar_t*	/* text */,
    int			/* num_wchars */,
    XRectangle*		/* ink_extents_buffer */,
    XRectangle*		/* logical_extents_buffer */,
    int			/* buffer_size */,
    int*		/* num_chars */,
    XRectangle*		/* overall_ink_return */,
    XRectangle*		/* overall_logical_return */
#endif
);

extern Status Xutf8TextPerCharExtents(
#if NeedFunctionPrototypes
    XFontSet		/* font_set */,
    _Xconst char*	/* text */,
    int			/* bytes_text */,
    XRectangle*		/* ink_extents_buffer */,
    XRectangle*		/* logical_extents_buffer */,
    int			/* buffer_size */,
    int*		/* num_chars */,
    XRectangle*		/* overall_ink_return */,
    XRectangle*		/* overall_logical_return */
#endif
);

extern void XmbDrawText(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    XmbTextItem*	/* text_items */,
    int			/* nitems */
#endif
);

extern void XwcDrawText(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    XwcTextItem*	/* text_items */,
    int			/* nitems */
#endif
);

extern void Xutf8DrawText(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    XmbTextItem*	/* text_items */,
    int			/* nitems */
#endif
);

extern void XmbDrawString(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    XFontSet		/* font_set */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    _Xconst char*	/* text */,
    int			/* bytes_text */
#endif
);

extern void XwcDrawString(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    XFontSet		/* font_set */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    _Xconst wchar_t*	/* text */,
    int			/* num_wchars */
#endif
);

extern void Xutf8DrawString(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    XFontSet		/* font_set */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    _Xconst char*	/* text */,
    int			/* bytes_text */
#endif
);

extern void XmbDrawImageString(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    XFontSet		/* font_set */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    _Xconst char*	/* text */,
    int			/* bytes_text */
#endif
);

extern void XwcDrawImageString(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    XFontSet		/* font_set */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    _Xconst wchar_t*	/* text */,
    int			/* num_wchars */
#endif
);

extern void Xutf8DrawImageString(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Drawable		/* d */,
    XFontSet		/* font_set */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    _Xconst char*	/* text */,
    int			/* bytes_text */
#endif
);

extern XIM XOpenIM(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    struct _XrmHashBucketRec*	/* rdb */,
    char*			/* res_name */,
    char*			/* res_class */
#endif
);

extern Status XCloseIM(
#if NeedFunctionPrototypes
    XIM /* im */
#endif
);

extern char *XGetIMValues(
#if NeedVarargsPrototypes
    XIM /* im */, ...
#endif
);

extern char *XSetIMValues(
#if NeedVarargsPrototypes
    XIM /* im */, ...
#endif
);

extern Display *XDisplayOfIM(
#if NeedFunctionPrototypes
    XIM /* im */
#endif
);

extern char *XLocaleOfIM(
#if NeedFunctionPrototypes
    XIM /* im*/
#endif
);

extern XIC XCreateIC(
#if NeedVarargsPrototypes
    XIM /* im */, ...
#endif
);

extern void XDestroyIC(
#if NeedFunctionPrototypes
    XIC /* ic */
#endif
);

extern void XSetICFocus(
#if NeedFunctionPrototypes
    XIC /* ic */
#endif
);

extern void XUnsetICFocus(
#if NeedFunctionPrototypes
    XIC /* ic */
#endif
);

extern wchar_t *XwcResetIC(
#if NeedFunctionPrototypes
    XIC /* ic */
#endif
);

extern char *XmbResetIC(
#if NeedFunctionPrototypes
    XIC /* ic */
#endif
);

extern char *Xutf8ResetIC(
#if NeedFunctionPrototypes
    XIC /* ic */
#endif
);

extern char *XSetICValues(
#if NeedVarargsPrototypes
    XIC /* ic */, ...
#endif
);

extern char *XGetICValues(
#if NeedVarargsPrototypes
    XIC /* ic */, ...
#endif
);

extern XIM XIMOfIC(
#if NeedFunctionPrototypes
    XIC /* ic */
#endif
);

extern Bool XFilterEvent(
#if NeedFunctionPrototypes
    XEvent*	/* event */,
    Window	/* window */
#endif
);

extern int XmbLookupString(
#if NeedFunctionPrototypes
    XIC			/* ic */,
    XKeyPressedEvent*	/* event */,
    char*		/* buffer_return */,
    int			/* bytes_buffer */,
    KeySym*		/* keysym_return */,
    Status*		/* status_return */
#endif
);

extern int XwcLookupString(
#if NeedFunctionPrototypes
    XIC			/* ic */,
    XKeyPressedEvent*	/* event */,
    wchar_t*		/* buffer_return */,
    int			/* wchars_buffer */,
    KeySym*		/* keysym_return */,
    Status*		/* status_return */
#endif
);

extern int Xutf8LookupString(
#if NeedFunctionPrototypes
    XIC			/* ic */,
    XKeyPressedEvent*	/* event */,
    char*		/* buffer_return */,
    int			/* bytes_buffer */,
    KeySym*		/* keysym_return */,
    Status*		/* status_return */
#endif
);

extern XVaNestedList XVaCreateNestedList(
#if NeedVarargsPrototypes
    int /*unused*/, ...
#endif
);

/* internal connections for IMs */

extern Bool XRegisterIMInstantiateCallback(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    struct _XrmHashBucketRec*	/* rdb */,
    char*			/* res_name */,
    char*			/* res_class */,
    XIDProc			/* callback */,
    XPointer			/* client_data */
#endif
);

extern Bool XUnregisterIMInstantiateCallback(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    struct _XrmHashBucketRec*	/* rdb */,
    char*			/* res_name */,
    char*			/* res_class */,
    XIDProc			/* callback */,
    XPointer			/* client_data */
#endif
);

typedef void (*XConnectionWatchProc)(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    XPointer			/* client_data */,
    int				/* fd */,
    Bool			/* opening */,	 /* open or close flag */
    XPointer*			/* watch_data */ /* open sets, close uses */
#endif
);
    

extern Status XInternalConnectionNumbers(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    int**			/* fd_return */,
    int*			/* count_return */
#endif
);

extern void XProcessInternalConnection(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    int				/* fd */
#endif
);

extern Status XAddConnectionWatch(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    XConnectionWatchProc	/* callback */,
    XPointer			/* client_data */
#endif
);

extern void XRemoveConnectionWatch(
#if NeedFunctionPrototypes
    Display*			/* dpy */,
    XConnectionWatchProc	/* callback */,
    XPointer			/* client_data */
#endif
);

extern void XSetAuthorization(
#if NeedFunctionPrototypes
    char *			/* name */,
    int				/* namelen */, 
    char *			/* data */,
    int				/* datalen */
#endif
);

extern int _Xmbtowc(
#if NeedFunctionPrototypes
    wchar_t *			/* wstr */,
#ifdef ISC
    char const *		/* str */,
    size_t			/* len */
#else
    char *			/* str */,
    int				/* len */
#endif
#endif
);

extern int _Xwctomb(
#if NeedFunctionPrototypes
    char *			/* str */,
    wchar_t			/* wc */
#endif
);

_XFUNCPROTOEND
#endif		/* if 0 */

#endif /* _XLIB_H_ */


/* $Xorg: Xutil.h,v 1.8 2001/02/09 02:03:39 xorgcvs Exp $ */

/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $XFree86: xc/lib/X11/Xutil.h,v 3.5 2003/01/26 02:40:10 dawes Exp $ */

#ifndef _XUTIL_H_
#define _XUTIL_H_

/* You must include <X11/Xlib.h> before including this file */
#if 0
#include <X11/Xlib.h>
#endif

/* 
 * Bitmask returned by XParseGeometry().  Each bit tells if the corresponding
 * value (x, y, width, height) was found in the parsed string.
 */
#define NoValue		0x0000
#define XValue  	0x0001
#define YValue		0x0002
#define WidthValue  	0x0004
#define HeightValue  	0x0008
#define AllValues 	0x000F
#define XNegative 	0x0010
#define YNegative 	0x0020

/*
 * new version containing base_width, base_height, and win_gravity fields;
 * used with WM_NORMAL_HINTS.
 */
typedef struct {
    	long flags;	/* marks which fields in this structure are defined */
	int x, y;		/* obsolete for new window mgrs, but clients */
	int width, height;	/* should set so old wm's don't mess up */
	int min_width, min_height;
	int max_width, max_height;
    	int width_inc, height_inc;
	struct {
		int x;	/* numerator */
		int y;	/* denominator */
	} min_aspect, max_aspect;
	int base_width, base_height;		/* added by ICCCM version 1 */
	int win_gravity;			/* added by ICCCM version 1 */
} XSizeHints;

/*
 * The next block of definitions are for window manager properties that
 * clients and applications use for communication.
 */

/* flags argument in size hints */
#define USPosition	(1L << 0) /* user specified x, y */
#define USSize		(1L << 1) /* user specified width, height */

#define PPosition	(1L << 2) /* program specified position */
#define PSize		(1L << 3) /* program specified size */
#define PMinSize	(1L << 4) /* program specified minimum size */
#define PMaxSize	(1L << 5) /* program specified maximum size */
#define PResizeInc	(1L << 6) /* program specified resize increments */
#define PAspect		(1L << 7) /* program specified min and max aspect ratios */
#define PBaseSize	(1L << 8) /* program specified base for incrementing */
#define PWinGravity	(1L << 9) /* program specified window gravity */

/* obsolete */
#define PAllHints (PPosition|PSize|PMinSize|PMaxSize|PResizeInc|PAspect)



typedef struct {
	long flags;	/* marks which fields in this structure are defined */
	Bool input;	/* does this application rely on the window manager to
			get keyboard input? */
	int initial_state;	/* see below */
	Pixmap icon_pixmap;	/* pixmap to be used as icon */
	Window icon_window; 	/* window to be used as icon */
	int icon_x, icon_y; 	/* initial position of icon */
	Pixmap icon_mask;	/* icon mask bitmap */
	XID window_group;	/* id of related window group */
	/* this structure may be extended in the future */
} XWMHints;

/* definition for flags of XWMHints */

#define InputHint 		(1L << 0)
#define StateHint 		(1L << 1)
#define IconPixmapHint		(1L << 2)
#define IconWindowHint		(1L << 3)
#define IconPositionHint 	(1L << 4)
#define IconMaskHint		(1L << 5)
#define WindowGroupHint		(1L << 6)
#define AllHints (InputHint|StateHint|IconPixmapHint|IconWindowHint| \
IconPositionHint|IconMaskHint|WindowGroupHint)
#define XUrgencyHint		(1L << 8)

/* definitions for initial window state */
#define WithdrawnState 0	/* for windows that are not mapped */
#define NormalState 1	/* most applications want to start this way */
#define IconicState 3	/* application wants to start as an icon */

/*
 * Obsolete states no longer defined by ICCCM
 */
#define DontCareState 0	/* don't know or care */
#define ZoomState 2	/* application wants to start zoomed */
#define InactiveState 4	/* application believes it is seldom used; */
			/* some wm's may put it on inactive menu */


/*
 * new structure for manipulating TEXT properties; used with WM_NAME, 
 * WM_ICON_NAME, WM_CLIENT_MACHINE, and WM_COMMAND.
 */
typedef struct {
    unsigned char *value;		/* same as Property routines */
    Atom encoding;			/* prop type */
    int format;				/* prop data format: 8, 16, or 32 */
    unsigned long nitems;		/* number of data items in value */
} XTextProperty;

#define XNoMemory -1
#define XLocaleNotSupported -2
#define XConverterNotFound -3

typedef enum {
    XStringStyle,		/* STRING */
    XCompoundTextStyle,		/* COMPOUND_TEXT */
    XTextStyle,			/* text in owner's encoding (current locale)*/
    XStdICCTextStyle,		/* STRING, else COMPOUND_TEXT */
    /* The following is an XFree86 extension, introduced in November 2000 */
    XUTF8StringStyle		/* UTF8_STRING */
} XICCEncodingStyle;

typedef struct {
	int min_width, min_height;
	int max_width, max_height;
	int width_inc, height_inc;
} XIconSize;

typedef struct {
	char *res_name;
	char *res_class;
} XClassHint;

/*
 * These macros are used to give some sugar to the image routines so that
 * naive people are more comfortable with them.
 */
#define XDestroyImage(ximage) \
	((*((ximage)->f.destroy_image))((ximage)))
#define XGetPixel(ximage, x, y) \
	((*((ximage)->f.get_pixel))((ximage), (x), (y)))
#define XPutPixel(ximage, x, y, pixel) \
	((*((ximage)->f.put_pixel))((ximage), (x), (y), (pixel)))
#define XSubImage(ximage, x, y, width, height)  \
	((*((ximage)->f.sub_image))((ximage), (x), (y), (width), (height)))
#define XAddPixel(ximage, value) \
	((*((ximage)->f.add_pixel))((ximage), (value)))

/*
 * Compose sequence status structure, used in calling XLookupString.
 */
typedef struct _XComposeStatus {
    XPointer compose_ptr;	/* state table pointer */
    int chars_matched;		/* match state */
} XComposeStatus;

/*
 * Keysym macros, used on Keysyms to test for classes of symbols
 */
#define IsKeypadKey(keysym) \
  (((KeySym)(keysym) >= XK_KP_Space) && ((KeySym)(keysym) <= XK_KP_Equal))

#define IsPrivateKeypadKey(keysym) \
  (((KeySym)(keysym) >= 0x11000000) && ((KeySym)(keysym) <= 0x1100FFFF))

#define IsCursorKey(keysym) \
  (((KeySym)(keysym) >= XK_Home)     && ((KeySym)(keysym) <  XK_Select))

#define IsPFKey(keysym) \
  (((KeySym)(keysym) >= XK_KP_F1)     && ((KeySym)(keysym) <= XK_KP_F4))

#define IsFunctionKey(keysym) \
  (((KeySym)(keysym) >= XK_F1)       && ((KeySym)(keysym) <= XK_F35))

#define IsMiscFunctionKey(keysym) \
  (((KeySym)(keysym) >= XK_Select)   && ((KeySym)(keysym) <= XK_Break))

#define IsModifierKey(keysym) \
  ((((KeySym)(keysym) >= XK_Shift_L) && ((KeySym)(keysym) <= XK_Hyper_R)) \
   || (((KeySym)(keysym) >= XK_ISO_Lock) && \
       ((KeySym)(keysym) <= XK_ISO_Last_Group_Lock)) \
   || ((KeySym)(keysym) == XK_Mode_switch) \
   || ((KeySym)(keysym) == XK_Num_Lock))
/*
 * opaque reference to Region data type 
 */
typedef struct _XRegion *Region; 

/* Return values from XRectInRegion() */
 
#define RectangleOut 0
#define RectangleIn  1
#define RectanglePart 2
 

/*
 * Information used by the visual utility routines to find desired visual
 * type from the many visuals a display may support.
 */

typedef struct {
  Visual *visual;
  VisualID visualid;
  int screen;
  int depth;
#if defined(__cplusplus) || defined(c_plusplus)
  int c_class;					/* C++ */
#else
  int class;
#endif
  unsigned long red_mask;
  unsigned long green_mask;
  unsigned long blue_mask;
  int colormap_size;
  int bits_per_rgb;
} XVisualInfo;

#define VisualNoMask		0x0
#define VisualIDMask 		0x1
#define VisualScreenMask	0x2
#define VisualDepthMask		0x4
#define VisualClassMask		0x8
#define VisualRedMaskMask	0x10
#define VisualGreenMaskMask	0x20
#define VisualBlueMaskMask	0x40
#define VisualColormapSizeMask	0x80
#define VisualBitsPerRGBMask	0x100
#define VisualAllMask		0x1FF

/*
 * This defines a window manager property that clients may use to
 * share standard color maps of type RGB_COLOR_MAP:
 */
typedef struct {
	Colormap colormap;
	unsigned long red_max;
	unsigned long red_mult;
	unsigned long green_max;
	unsigned long green_mult;
	unsigned long blue_max;
	unsigned long blue_mult;
	unsigned long base_pixel;
	VisualID visualid;		/* added by ICCCM version 1 */
	XID killid;			/* added by ICCCM version 1 */
} XStandardColormap;

#define ReleaseByFreeingColormap ((XID) 1L)  /* for killid field above */


/*
 * return codes for XReadBitmapFile and XWriteBitmapFile
 */
#define BitmapSuccess		0
#define BitmapOpenFailed 	1
#define BitmapFileInvalid 	2
#define BitmapNoMemory		3

/****************************************************************
 *
 * Context Management
 *
 ****************************************************************/


/* Associative lookup table return codes */

#define XCSUCCESS 0	/* No error. */
#define XCNOMEM   1    /* Out of memory */
#define XCNOENT   2    /* No entry in table */

typedef int XContext;

#define XUniqueContext()       ((XContext) XrmUniqueQuark())
#define XStringToContext(string)   ((XContext) XrmStringToQuark(string))

#if 0
_XFUNCPROTOBEGIN

/* The following declarations are alphabetized. */

extern XClassHint *XAllocClassHint (
#if NeedFunctionPrototypes
    void
#endif
);

extern XIconSize *XAllocIconSize (
#if NeedFunctionPrototypes
    void
#endif
);

extern XSizeHints *XAllocSizeHints (
#if NeedFunctionPrototypes
    void
#endif
);

extern XStandardColormap *XAllocStandardColormap (
#if NeedFunctionPrototypes
    void
#endif
);

extern XWMHints *XAllocWMHints (
#if NeedFunctionPrototypes
    void
#endif
);

extern int XClipBox(
#if NeedFunctionPrototypes
    Region		/* r */,
    XRectangle*		/* rect_return */
#endif
);

extern Region XCreateRegion(
#if NeedFunctionPrototypes
    void
#endif
);

extern const char *XDefaultString (void);

extern int XDeleteContext(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XID			/* rid */,
    XContext		/* context */
#endif
);

extern int XDestroyRegion(
#if NeedFunctionPrototypes
    Region		/* r */
#endif
);

extern int XEmptyRegion(
#if NeedFunctionPrototypes
    Region		/* r */
#endif
);

extern int XEqualRegion(
#if NeedFunctionPrototypes
    Region		/* r1 */,
    Region		/* r2 */
#endif
);

extern int XFindContext(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XID			/* rid */,
    XContext		/* context */,
    XPointer*		/* data_return */
#endif
);

extern Status XGetClassHint(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XClassHint*		/* class_hints_return */
#endif
);

extern Status XGetIconSizes(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XIconSize**		/* size_list_return */,
    int*		/* count_return */
#endif
);

extern Status XGetNormalHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* hints_return */
#endif
);

extern Status XGetRGBColormaps(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XStandardColormap** /* stdcmap_return */,
    int*		/* count_return */,
    Atom		/* property */
#endif
);

extern Status XGetSizeHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* hints_return */,
    Atom		/* property */
#endif
);

extern Status XGetStandardColormap(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XStandardColormap*	/* colormap_return */,
    Atom		/* property */			    
#endif
);

extern Status XGetTextProperty(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* window */,
    XTextProperty*	/* text_prop_return */,
    Atom		/* property */
#endif
);

extern XVisualInfo *XGetVisualInfo(
#if NeedFunctionPrototypes
    Display*		/* display */,
    long		/* vinfo_mask */,
    XVisualInfo*	/* vinfo_template */,
    int*		/* nitems_return */
#endif
);

extern Status XGetWMClientMachine(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XTextProperty*	/* text_prop_return */
#endif
);

extern XWMHints *XGetWMHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */		      
#endif
);

extern Status XGetWMIconName(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XTextProperty*	/* text_prop_return */
#endif
);

extern Status XGetWMName(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XTextProperty*	/* text_prop_return */
#endif
);

extern Status XGetWMNormalHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* hints_return */,
    long*		/* supplied_return */ 
#endif
);

extern Status XGetWMSizeHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* hints_return */,
    long*		/* supplied_return */,
    Atom		/* property */
#endif
);

extern Status XGetZoomHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* zhints_return */
#endif
);

extern int XIntersectRegion(
#if NeedFunctionPrototypes
    Region		/* sra */,
    Region		/* srb */,
    Region		/* dr_return */
#endif
);

extern void XConvertCase(
#if NeedFunctionPrototypes
    KeySym		/* sym */,
    KeySym*		/* lower */,
    KeySym*		/* upper */
#endif
);

extern int XLookupString(
#if NeedFunctionPrototypes
    XKeyEvent*		/* event_struct */,
    char*		/* buffer_return */,
    int			/* bytes_buffer */,
    KeySym*		/* keysym_return */,
    XComposeStatus*	/* status_in_out */
#endif
);

extern Status XMatchVisualInfo(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen */,
    int			/* depth */,
    int			/* class */,
    XVisualInfo*	/* vinfo_return */
#endif
);

extern int XOffsetRegion(
#if NeedFunctionPrototypes
    Region		/* r */,
    int			/* dx */,
    int			/* dy */
#endif
);

extern Bool XPointInRegion(
#if NeedFunctionPrototypes
    Region		/* r */,
    int			/* x */,
    int			/* y */
#endif
);

extern Region XPolygonRegion(
#if NeedFunctionPrototypes
    XPoint*		/* points */,
    int			/* n */,
    int			/* fill_rule */
#endif
);

extern int XRectInRegion(
#if NeedFunctionPrototypes
    Region		/* r */,
    int			/* x */,
    int			/* y */,
    unsigned int	/* width */,
    unsigned int	/* height */
#endif
);

extern int XSaveContext(
#if NeedFunctionPrototypes
    Display*		/* display */,
    XID			/* rid */,
    XContext		/* context */,
    _Xconst char*	/* data */
#endif
);

extern int XSetClassHint(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XClassHint*		/* class_hints */
#endif
);

extern int XSetIconSizes(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XIconSize*		/* size_list */,
    int			/* count */    
#endif
);

extern int XSetNormalHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* hints */
#endif
);

extern void XSetRGBColormaps(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XStandardColormap*	/* stdcmaps */,
    int			/* count */,
    Atom		/* property */
#endif
);

extern int XSetSizeHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* hints */,
    Atom		/* property */
#endif
);

extern int XSetStandardProperties(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    _Xconst char*	/* window_name */,
    _Xconst char*	/* icon_name */,
    Pixmap		/* icon_pixmap */,
    char**		/* argv */,
    int			/* argc */,
    XSizeHints*		/* hints */
#endif
);

extern void XSetTextProperty(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XTextProperty*	/* text_prop */,
    Atom		/* property */
#endif
);

extern void XSetWMClientMachine(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XTextProperty*	/* text_prop */
#endif
);

extern int XSetWMHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XWMHints*		/* wm_hints */
#endif
);

extern void XSetWMIconName(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XTextProperty*	/* text_prop */
#endif
);

extern void XSetWMName(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XTextProperty*	/* text_prop */
#endif
);

extern void XSetWMNormalHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* hints */
#endif
);

extern void XSetWMProperties(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XTextProperty*	/* window_name */,
    XTextProperty*	/* icon_name */,
    char**		/* argv */,
    int			/* argc */,
    XSizeHints*		/* normal_hints */,
    XWMHints*		/* wm_hints */,
    XClassHint*		/* class_hints */
#endif
);

extern void XmbSetWMProperties(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    _Xconst char*	/* window_name */,
    _Xconst char*	/* icon_name */,
    char**		/* argv */,
    int			/* argc */,
    XSizeHints*		/* normal_hints */,
    XWMHints*		/* wm_hints */,
    XClassHint*		/* class_hints */
#endif
);

extern void Xutf8SetWMProperties(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    _Xconst char*	/* window_name */,
    _Xconst char*	/* icon_name */,
    char**		/* argv */,
    int			/* argc */,
    XSizeHints*		/* normal_hints */,
    XWMHints*		/* wm_hints */,
    XClassHint*		/* class_hints */
#endif
);

extern void XSetWMSizeHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* hints */,
    Atom		/* property */
#endif
);

extern int XSetRegion(
#if NeedFunctionPrototypes
    Display*		/* display */,
    GC			/* gc */,
    Region		/* r */
#endif
);

extern void XSetStandardColormap(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XStandardColormap*	/* colormap */,
    Atom		/* property */
#endif
);

extern int XSetZoomHints(
#if NeedFunctionPrototypes
    Display*		/* display */,
    Window		/* w */,
    XSizeHints*		/* zhints */
#endif
);

extern int XShrinkRegion(
#if NeedFunctionPrototypes
    Region		/* r */,
    int			/* dx */,
    int			/* dy */
#endif
);

extern Status XStringListToTextProperty(
#if NeedFunctionPrototypes
    char**		/* list */,
    int			/* count */,
    XTextProperty*	/* text_prop_return */
#endif
);

extern int XSubtractRegion(
#if NeedFunctionPrototypes
    Region		/* sra */,
    Region		/* srb */,
    Region		/* dr_return */
#endif
);

extern int XmbTextListToTextProperty(
    Display*		display,
    char**		list,
    int			count,
    XICCEncodingStyle	style,
    XTextProperty*	text_prop_return
);

extern int XwcTextListToTextProperty(
    Display*		display,
    wchar_t**		list,
    int			count,
    XICCEncodingStyle	style,
    XTextProperty*	text_prop_return
);

extern int Xutf8TextListToTextProperty(
    Display*		display,
    char**		list,
    int			count,
    XICCEncodingStyle	style,
    XTextProperty*	text_prop_return
);

extern void XwcFreeStringList(
    wchar_t**		list
);

extern Status XTextPropertyToStringList(
#if NeedFunctionPrototypes
    XTextProperty*	/* text_prop */,
    char***		/* list_return */,
    int*		/* count_return */
#endif
);

extern int XmbTextPropertyToTextList(
    Display*		display,
    const XTextProperty* text_prop,
    char***		list_return,
    int*		count_return
);

extern int XwcTextPropertyToTextList(
    Display*		display,
    const XTextProperty* text_prop,
    wchar_t***		list_return,
    int*		count_return
);

extern int Xutf8TextPropertyToTextList(
    Display*		display,
    const XTextProperty* text_prop,
    char***		list_return,
    int*		count_return
);

extern int XUnionRectWithRegion(
#if NeedFunctionPrototypes
    XRectangle*		/* rectangle */,
    Region		/* src_region */,
    Region		/* dest_region_return */
#endif
);

extern int XUnionRegion(
#if NeedFunctionPrototypes
    Region		/* sra */,
    Region		/* srb */,
    Region		/* dr_return */
#endif
);

extern int XWMGeometry(
#if NeedFunctionPrototypes
    Display*		/* display */,
    int			/* screen_number */,
    _Xconst char*	/* user_geometry */,
    _Xconst char*	/* default_geometry */,
    unsigned int	/* border_width */,
    XSizeHints*		/* hints */,
    int*		/* x_return */,
    int*		/* y_return */,
    int*		/* width_return */,
    int*		/* height_return */,
    int*		/* gravity_return */
#endif
);

extern int XXorRegion(
#if NeedFunctionPrototypes
    Region		/* sra */,
    Region		/* srb */,
    Region		/* dr_return */
#endif
);

_XFUNCPROTOEND
#endif /* if 0 */

#ifndef XATOM_H
#define XATOM_H 1

/* THIS IS A GENERATED FILE
 *
 * Do not change!  Changing this file implies a protocol change!
 */

#define XA_PRIMARY ((Atom) 1)
#define XA_SECONDARY ((Atom) 2)
#define XA_ARC ((Atom) 3)
#define XA_ATOM ((Atom) 4)
#define XA_BITMAP ((Atom) 5)
#define XA_CARDINAL ((Atom) 6)
#define XA_COLORMAP ((Atom) 7)
#define XA_CURSOR ((Atom) 8)
#define XA_CUT_BUFFER0 ((Atom) 9)
#define XA_CUT_BUFFER1 ((Atom) 10)
#define XA_CUT_BUFFER2 ((Atom) 11)
#define XA_CUT_BUFFER3 ((Atom) 12)
#define XA_CUT_BUFFER4 ((Atom) 13)
#define XA_CUT_BUFFER5 ((Atom) 14)
#define XA_CUT_BUFFER6 ((Atom) 15)
#define XA_CUT_BUFFER7 ((Atom) 16)
#define XA_DRAWABLE ((Atom) 17)
#define XA_FONT ((Atom) 18)
#define XA_INTEGER ((Atom) 19)
#define XA_PIXMAP ((Atom) 20)
#define XA_POINT ((Atom) 21)
#define XA_RECTANGLE ((Atom) 22)
#define XA_RESOURCE_MANAGER ((Atom) 23)
#define XA_RGB_COLOR_MAP ((Atom) 24)
#define XA_RGB_BEST_MAP ((Atom) 25)
#define XA_RGB_BLUE_MAP ((Atom) 26)
#define XA_RGB_DEFAULT_MAP ((Atom) 27)
#define XA_RGB_GRAY_MAP ((Atom) 28)
#define XA_RGB_GREEN_MAP ((Atom) 29)
#define XA_RGB_RED_MAP ((Atom) 30)
#define XA_STRING ((Atom) 31)
#define XA_VISUALID ((Atom) 32)
#define XA_WINDOW ((Atom) 33)
#define XA_WM_COMMAND ((Atom) 34)
#define XA_WM_HINTS ((Atom) 35)
#define XA_WM_CLIENT_MACHINE ((Atom) 36)
#define XA_WM_ICON_NAME ((Atom) 37)
#define XA_WM_ICON_SIZE ((Atom) 38)
#define XA_WM_NAME ((Atom) 39)
#define XA_WM_NORMAL_HINTS ((Atom) 40)
#define XA_WM_SIZE_HINTS ((Atom) 41)
#define XA_WM_ZOOM_HINTS ((Atom) 42)
#define XA_MIN_SPACE ((Atom) 43)
#define XA_NORM_SPACE ((Atom) 44)
#define XA_MAX_SPACE ((Atom) 45)
#define XA_END_SPACE ((Atom) 46)
#define XA_SUPERSCRIPT_X ((Atom) 47)
#define XA_SUPERSCRIPT_Y ((Atom) 48)
#define XA_SUBSCRIPT_X ((Atom) 49)
#define XA_SUBSCRIPT_Y ((Atom) 50)
#define XA_UNDERLINE_POSITION ((Atom) 51)
#define XA_UNDERLINE_THICKNESS ((Atom) 52)
#define XA_STRIKEOUT_ASCENT ((Atom) 53)
#define XA_STRIKEOUT_DESCENT ((Atom) 54)
#define XA_ITALIC_ANGLE ((Atom) 55)
#define XA_X_HEIGHT ((Atom) 56)
#define XA_QUAD_WIDTH ((Atom) 57)
#define XA_WEIGHT ((Atom) 58)
#define XA_POINT_SIZE ((Atom) 59)
#define XA_RESOLUTION ((Atom) 60)
#define XA_COPYRIGHT ((Atom) 61)
#define XA_NOTICE ((Atom) 62)
#define XA_FONT_NAME ((Atom) 63)
#define XA_FAMILY_NAME ((Atom) 64)
#define XA_FULL_NAME ((Atom) 65)
#define XA_CAP_HEIGHT ((Atom) 66)
#define XA_WM_CLASS ((Atom) 67)
#define XA_WM_TRANSIENT_FOR ((Atom) 68)

#define XA_LAST_PREDEFINED ((Atom) 68)
#endif /* XATOM_H */

#endif /* _XUTIL_H_ */

extern KeyCode XKeysymToKeycode(
    Display*		/* display */,
    KeySym		/* keysym */
);
extern KeySym XKeycodeToKeysym(
    Display*		/* display */,
    KeyCode		/* keycode */,
    int			/* index */
);
extern char *XKeysymToString(
    KeySym		/* keysym */
);
extern KeySym XStringToKeysym(
    char*	/* string */
);

typedef int (*XErrorHandler) (	    /* WARNING, this type not in Xlib spec */
    Display*		/* display */,
    XErrorEvent*	/* error_event */
);

extern XErrorHandler XSetErrorHandler (
    XErrorHandler	/* handler */
);

typedef int (*XIOErrorHandler) (    /* WARNING, this type not in Xlib spec */
    Display*		/* display */
);

extern XIOErrorHandler XSetIOErrorHandler (
    XIOErrorHandler	/* handler */
);

#define X_ShmQueryVersion               0
#define X_ShmAttach                     1
#define X_ShmDetach                     2
#define X_ShmPutImage                   3
#define X_ShmGetImage                   4
#define X_ShmCreatePixmap               5
