/*
 * PipeWireVncServer.cpp - implementation of PipeWireVncServer class
 *
 * Copyright (c) 2017-2020 Tobias Junghans <tobydox@veyon.io>
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

extern "C" {
#include "rfb/rfb.h"
#include "rfb/rfbregion.h"
}

#include "PipeWireVncServer.h"
#include "krfb_fb_pipewire_debug.h"
#include "pw_framebuffer.h"
#include "xdpevents.h"

struct PipeWireScreen
{
	~PipeWireScreen()
	{
		delete[] passwords[0];
		delete framebuffer;
	}

	rfbScreenInfoPtr rfbScreen{nullptr};
	PWFrameBuffer* framebuffer{nullptr};
	XdpEventHandler* eventHandler{nullptr};
	std::array<char *, 2> passwords{};
};


static void handleClientGone( rfbClientPtr cl )
{
	vInfo() << cl->host;
}


static rfbNewClientAction handleNewClient( rfbClientPtr cl )
{
	cl->clientGoneHook = handleClientGone;

	vInfo() << "New client connection from host" << cl->host;

	return RFB_CLIENT_ACCEPT;
}


static void handleClipboardText( char* str, int len, rfbClientPtr cl )
{
	Q_UNUSED(cl)

	str[len] = '\0';
	qInfo() << "Got clipboard:" << str;
}


static void handleKeyEvent( rfbBool down, rfbKeySym keySym, rfbClientPtr cl )
{
	const auto screen = reinterpret_cast<PipeWireScreen *>( cl->screen->screenData );

	screen->eventHandler->handleKeyboard( down, keySym );
}



static void handlePointerEvent( int buttonMask, int x, int y, rfbClientPtr cl )
{
	const auto screen = reinterpret_cast<PipeWireScreen *>( cl->screen->screenData );

	screen->eventHandler->handlePointer( buttonMask, x, y );
}



PipeWireVncServer::PipeWireVncServer( QObject* parent ) :
	QObject( parent )
{
}



QWidget* PipeWireVncServer::configurationWidget()
{
	return nullptr;
}



void PipeWireVncServer::prepareServer()
{
}



bool PipeWireVncServer::runServer( int serverPort, const Password& password )
{
	while( true )
	{
		PipeWireScreen screen;

		if( initScreen( &screen ) == false ||
			initVncServer( serverPort, password, &screen ) == false )
		{
			break;
		}

		while( screen.framebuffer->isValid() )
		{
			// any clients connected?
			if( screen.rfbScreen->clientHead )
			{
				if( clientUpdateRequestsPending( &screen ) == false )
				{
					QThread::msleep( UpdateTime );
				}
				handleScreenChanges( &screen );
			}
			else
			{
				QThread::msleep( IdleSleepTime );
			}

			rfbProcessEvents( screen.rfbScreen, 0 );
		}

		vInfo() << "Screen configuration changed";

		rfbShutdownServer( screen.rfbScreen, true );
		rfbScreenCleanup( screen.rfbScreen );
	}

	return true;
}


bool PipeWireVncServer::initVncServer( int serverPort, const VncServerPluginInterface::Password& password, PipeWireScreen* screen )
{
	vDebug();

	auto rfbScreen = rfbGetScreen( nullptr, nullptr,
								   screen->framebuffer->width(), screen->framebuffer->height(),
								   8, 3, 4 );

	if( rfbScreen == nullptr )
	{
		return false;
	}

	screen->passwords[0] = qstrdup( password.toByteArray().constData() );

	rfbScreen->desktopName = "PipeWire";
	rfbScreen->frameBuffer = screen->framebuffer->data();
	rfbScreen->port = serverPort;
	rfbScreen->kbdAddEvent = handleKeyEvent;
	rfbScreen->ptrAddEvent = handlePointerEvent;
	rfbScreen->newClientHook = handleNewClient;
	rfbScreen->setXCutText = handleClipboardText;

	rfbScreen->authPasswdData = screen->passwords.data();
	rfbScreen->passwordCheck = rfbCheckPasswordByList;

	rfbScreen->serverFormat.redShift = 16;
	rfbScreen->serverFormat.greenShift = 8;
	rfbScreen->serverFormat.blueShift = 0;

	rfbScreen->serverFormat.redMax = 255;
	rfbScreen->serverFormat.greenMax = 255;
	rfbScreen->serverFormat.blueMax = 255;

	rfbScreen->serverFormat.trueColour = true;

	screen->framebuffer->getServerFormat( rfbScreen->serverFormat );

	rfbScreen->alwaysShared = true;
	rfbScreen->handleEventsEagerly = true;
	rfbScreen->deferUpdateTime = 5;

	rfbScreen->screenData = screen;

	rfbInitServer( rfbScreen );

	rfbMarkRectAsModified( rfbScreen, 0, 0, rfbScreen->width, rfbScreen->height );

	screen->rfbScreen = rfbScreen;

	return true;
}



bool PipeWireVncServer::initScreen( PipeWireScreen* screen )
{
	screen->framebuffer = new PWFrameBuffer( 0 );

	return false;
}



void PipeWireVncServer::handleScreenChanges( PipeWireScreen* screen )
{
	const auto rects = screen->framebuffer->modifiedTiles();
	for( const auto& rect : rects )
	{
		rfbMarkRectAsModified( screen->rfbScreen, rect.left(), rect.top(), rect.right(), rect.bottom() );
	}

	// TODO: cursor movements
}



bool PipeWireVncServer::clientUpdateRequestsPending( const PipeWireScreen* screen ) const
{
	for( auto clientPtr = screen->rfbScreen->clientHead; clientPtr != nullptr; clientPtr = clientPtr->next )
	{
		if( sraRgnEmpty( clientPtr->requestedRegion ) == false )
		{
			return true;
		}
	}

	return false;
}


Q_LOGGING_CATEGORY(KRFB_FB_PIPEWIRE, "PipeWire.KRFB");
