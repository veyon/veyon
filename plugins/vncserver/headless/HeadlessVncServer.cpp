/*
 * HeadlessVncServer.cpp - implementation of HeadlessVncServer class
 *
 * Copyright (c) 2020-2021 Tobias Junghans <tobydox@veyon.io>
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
}

#include "HeadlessVncServer.h"
#include "VeyonConfiguration.h"


struct HeadlessVncScreen
{
	~HeadlessVncScreen()
	{
		delete[] passwords[0];
	}

	rfbScreenInfoPtr rfbScreen{nullptr};
	std::array<char *, 2> passwords{};
	QImage framebuffer;

};


HeadlessVncServer::HeadlessVncServer( QObject* parent ) :
	QObject( parent ),
	m_configuration( &VeyonCore::config() )
{
}



void HeadlessVncServer::prepareServer()
{
}



bool HeadlessVncServer::runServer( int serverPort, const Password& password )
{
	HeadlessVncScreen screen;

	if( initScreen( &screen ) == false ||
		initVncServer( serverPort, password, &screen ) == false )
	{
		return false;
	}

	while( true )
	{
		QThread::msleep( DefaultSleepTime );

		rfbProcessEvents( screen.rfbScreen, 0 );
	}

	rfbShutdownServer( screen.rfbScreen, true );
	rfbScreenCleanup( screen.rfbScreen );

	return true;
}



bool HeadlessVncServer::initScreen( HeadlessVncScreen* screen )
{
	screen->framebuffer = QImage( DefaultFramebufferWidth, DefaultFramebufferHeight, QImage::Format_RGB32 );
	screen->framebuffer.fill( m_configuration.backgroundColor() );

	return true;
}



bool HeadlessVncServer::initVncServer( int serverPort, const VncServerPluginInterface::Password& password,
									  HeadlessVncScreen* screen )
{
	auto rfbScreen = rfbGetScreen( nullptr, nullptr,
								   screen->framebuffer.width(), screen->framebuffer.height(),
								   8, 3, 4 );

	if( rfbScreen == nullptr )
	{
		return false;
	}

	screen->passwords[0] = qstrdup( password.toByteArray().constData() );

	rfbScreen->desktopName = "VeyonVNC";
	rfbScreen->frameBuffer = reinterpret_cast<char *>( screen->framebuffer.bits() );
	rfbScreen->port = serverPort;
	rfbScreen->ipv6port = serverPort;

	rfbScreen->authPasswdData = screen->passwords.data();
	rfbScreen->passwordCheck = rfbCheckPasswordByList;

	rfbScreen->serverFormat.redShift = 16;
	rfbScreen->serverFormat.greenShift = 8;
	rfbScreen->serverFormat.blueShift = 0;

	rfbScreen->serverFormat.redMax = 255;
	rfbScreen->serverFormat.greenMax = 255;
	rfbScreen->serverFormat.blueMax = 255;

	rfbScreen->serverFormat.trueColour = true;
	rfbScreen->serverFormat.bitsPerPixel = 32;

	rfbScreen->alwaysShared = true;
	rfbScreen->handleEventsEagerly = true;
	rfbScreen->deferUpdateTime = 5;

	rfbScreen->screenData = screen;

	rfbScreen->cursor = nullptr;

	rfbInitServer( rfbScreen );

	rfbMarkRectAsModified( rfbScreen, 0, 0, rfbScreen->width, rfbScreen->height );

	screen->rfbScreen = rfbScreen;

	return true;
}


IMPLEMENT_CONFIG_PROXY(HeadlessVncConfiguration)
