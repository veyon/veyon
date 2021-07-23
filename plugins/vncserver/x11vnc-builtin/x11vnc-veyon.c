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
	}

	return defaultXErrorHandler(display, error);
}



static XImage* createXShmTestImage( Display* display, XShmSegmentInfo* shm )
{
	shm->shmid = -1;
	shm->shmaddr = (char *) -1;

	XImage* xim = XShmCreateImage(display, NULL, 32, ZPixmap, NULL, shm, 1, 1);
	if( xim == NULL )
	{
		return NULL;
	}

	shm->shmid = shmget(IPC_PRIVATE, xim->bytes_per_line * xim->height, IPC_CREAT | 0600);

	if( shm->shmid == -1 )
	{
		return xim;
	}

	shm->shmaddr = xim->data = (char *) shmat(shm->shmid, 0, 0);

	if( shm->shmaddr == (char *)-1 )
	{
		return xim;
	}

	if( !XShmAttach(display, shm) )
	{
		xshmAttachErrorCount++;
	}

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

	XShmDetach(display, &shm);
	XDestroyImage(xim);

	shm_delete(&shm);

	XCloseDisplay(display);

	XSetErrorHandler(defaultXErrorHandler);

	return xshmAttachErrorCount == 0;
}

#else

int hasWorkingXShm()
{
	return 0;
}

#endif
