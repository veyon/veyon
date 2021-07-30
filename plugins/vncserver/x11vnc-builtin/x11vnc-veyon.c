#define SHOW_NO_PASSWORD_WARNING 0
#define main x11vnc_main

#include <X11/Xfuncproto.h>

#include "x11vnc.c"
#include "pm.c"

#ifdef HAVE_XSHM

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>

static int xshmOpCode = 0;
static int xshmAttachErrorCount = 0;
static XErrorHandler defaultXErrorHandler = NULL;

static int handleXError( Display* display, XErrorEvent* error )
{
	if( xshmOpCode > 0 && error->request_code == xshmOpCode )
	{
		xshmAttachErrorCount++;
		return 0;
	}

	return defaultXErrorHandler(display, error);
}



static XImage* createXShmTestImage( Display* display, XShmSegmentInfo* shm )
{
	shm->shmid = -1;
	shm->shmaddr = (char *) -1;

	int screen = DefaultScreen(display);

	XImage* xim = XShmCreateImage(display, DefaultVisual(display, screen),
								   DefaultDepth(display, screen),
								   ZPixmap, NULL, shm, 1, 1);
	if( xim == NULL )
	{
		return NULL;
	}

	shm->shmid = shmget(IPC_PRIVATE, xim->bytes_per_line * xim->height, IPC_CREAT | 0600);

	if( shm->shmid == -1 )
	{
		return xim;
	}

	shm->shmaddr = xim->data = (char *) shmat(shm->shmid, NULL, 0);
	shm->readOnly = 1;

	if( shm->shmaddr == (char *)-1 )
	{
		return xim;
	}

	if( XShmAttach(display, shm) )
	{
		XSync(display, False);
		XShmDetach(display, shm);
	}
	else
	{
		xshmAttachErrorCount++;
	}

	XSync(display, False);

	return xim;
}



int hasWorkingXShm()
{
	xshmAttachErrorCount = 0;

	char* displayName = NULL;
	if( getenv("DISPLAY") )
	{
		displayName = getenv("DISPLAY");
	}

	Display* display = XOpenDisplay(displayName);
	if( display == NULL )
	{
		return 0;
	}

	int op, ev, er;
	if( XQueryExtension(display, "MIT-SHM", &op, &ev, &er) )
	{
		xshmOpCode = op;
	}
	else
	{
		XCloseDisplay(display);
		return 0;
	}

	defaultXErrorHandler = XSetErrorHandler(handleXError);

	XShmSegmentInfo shm;
	XImage* xim = createXShmTestImage(display, &shm);

	XSetErrorHandler(defaultXErrorHandler);

	if( xim )
	{
		XDestroyImage(xim);
	}

	shm_delete(&shm);

	XCloseDisplay(display);

	return xshmAttachErrorCount == 0;
}

#else

int hasWorkingXShm()
{
	return 0;
}

#endif
