// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

extern "C" {
#include "rfb/rfb.h"
}

#include <QImage>

#include "HeadlessVncServer.h"
#include "VeyonConfiguration.h"

#include <array>
#include <cstdio>


struct HeadlessVncScreen
{
	~HeadlessVncScreen()
	{
		delete[] passwords[0];
	}

	rfbScreenInfoPtr rfbScreen{nullptr};
	std::array<char*, 2> passwords{};
	QImage framebuffer;
};


HeadlessVncServer::HeadlessVncServer(QObject* parent) :
	QObject(parent),
	m_configuration(&VeyonCore::config())
{
}



void HeadlessVncServer::prepareServer()
{
}



bool HeadlessVncServer::runServer(int serverPort, const Password& password)
{
	if (VeyonCore::isDebugging())
	{
		rfbLog = rfbLogDebug;
		rfbErr = rfbLogDebug;
	}
	else
	{
		rfbLog = rfbLogNone;
		rfbErr = rfbLogNone;
	}

	HeadlessVncScreen screen;

	if (initScreen(&screen) == false ||
		initVncServer(serverPort, password, &screen) == false)
	{
		return false;
	}

	while (true)
	{
		QThread::msleep(DefaultSleepTime);

		rfbProcessEvents(screen.rfbScreen, 0);
	}

	rfbShutdownServer(screen.rfbScreen, true);
	rfbScreenCleanup(screen.rfbScreen);

	return true;
}



bool HeadlessVncServer::initScreen(HeadlessVncScreen* screen)
{
	screen->framebuffer = QImage(DefaultFramebufferWidth, DefaultFramebufferHeight, QImage::Format_RGB32);
	screen->framebuffer.fill(m_configuration.backgroundColor());

	return true;
}



bool HeadlessVncServer::initVncServer(int serverPort, const VncServerPluginInterface::Password& password,
									  HeadlessVncScreen* screen)
{
	auto rfbScreen = rfbGetScreen(nullptr, nullptr,
								  screen->framebuffer.width(), screen->framebuffer.height(),
								  8, 3, 4);

	if (rfbScreen == nullptr)
	{
		return false;
	}

	screen->passwords[0] = qstrdup(password.toByteArray().constData());

	rfbScreen->desktopName = "VeyonVNC";
	rfbScreen->frameBuffer = reinterpret_cast<char*>(screen->framebuffer.bits());
	rfbScreen->port = serverPort;
	rfbScreen->ipv6port = serverPort;

	rfbScreen->listenInterface = htonl(INADDR_LOOPBACK);
	rfbScreen->listen6Interface = const_cast<char*>("::1");

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

	rfbInitServer(rfbScreen);

	rfbMarkRectAsModified(rfbScreen, 0, 0, rfbScreen->width, rfbScreen->height);

	screen->rfbScreen = rfbScreen;

	return true;
}



void HeadlessVncServer::rfbLogDebug(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	static constexpr int MaxMessageLength = 256;
	std::array<char, MaxMessageLength> message;

	std::vsnprintf(message.data(), message.size(), format, args); // Flawfinder: ignore

	va_end(args);

	vDebug() << message.data();
}



void HeadlessVncServer::rfbLogNone(const char* format, ...)
{
	Q_UNUSED(format);
}


IMPLEMENT_CONFIG_PROXY(HeadlessVncConfiguration)
