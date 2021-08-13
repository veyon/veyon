#define SHOW_NO_PASSWORD_WARNING 0
#define main x11vnc_main

#include <X11/Xfuncproto.h>

#include "x11vnc.c"
#include "pm.c"

#ifdef HAVE_XSHM

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>

static int xshmOpCode = 0;
static int shmErrorCount = 0;
static XErrorHandler defaultXErrorHandler = NULL;

static int handleXError( Display* display, XErrorEvent* error )
{
	if( xshmOpCode > 0 && error->request_code == xshmOpCode )
	{
		shmErrorCount++;
		return 0;
	}

	return defaultXErrorHandler(display, error);
}



static XImage* createXShmTestImage( Display* display, XShmSegmentInfo* shm )
{
	shm->shmid = -1;
	shm->shmaddr = (char *) -1;

	const int screen = DefaultScreen(display);

	XImage* xim = XShmCreateImage(display, DefaultVisual(display, screen),
								   DefaultDepth(display, screen),
								   ZPixmap, NULL, shm, 1, 1);
	if( xim == NULL )
	{
		shmErrorCount++;
		return NULL;
	}

	shm->shmid = shmget(IPC_PRIVATE, xim->bytes_per_line * xim->height, IPC_CREAT | 0600);

	if( shm->shmid == -1 )
	{
		shmErrorCount++;
		return xim;
	}

	shm->shmaddr = xim->data = (char *) shmat(shm->shmid, NULL, 0);
	shm->readOnly = False;

	if( shm->shmaddr == (char *)-1 )
	{
		shmErrorCount++;
		return xim;
	}

	if( XShmAttach(display, shm) )
	{
		XSync(display, False);
		XShmDetach(display, shm);
	}
	else
	{
		shmErrorCount++;
	}

	XSync(display, False);

	return xim;
}



int hasWorkingXShm()
{
	shmErrorCount = 0;

	Display* display = XOpenDisplay(NULL);
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

	if( shm.shmaddr != (char *) -1 )
	{
		shmdt(shm.shmaddr);
	}

	if( shm.shmid != -1 )
	{
		shmctl(shm.shmid, IPC_RMID, 0);
	}

	XSync(display, False);

	XCloseDisplay(display);

	return shmErrorCount == 0;
}

#else

int hasWorkingXShm()
{
	return 0;
}

#endif
