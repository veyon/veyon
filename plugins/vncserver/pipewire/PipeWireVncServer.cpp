// SPDX-FileCopyrightText: 2024-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QThread>
#include <QTimer>

#include "PipeWireFramebuffer.h"
#include "PipeWireVncConfigurationWidget.h"
#include "PipeWireVncServer.h"
#include "PlatformCoreFunctions.h"
#include "PlatformInputDeviceFunctions.h"
#include "PortalSession.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"

#include <cstring>

// Initial framebuffer dimensions (used before the first PipeWire frame arrives)
static constexpr int DefaultWidth = 1920;
static constexpr int DefaultHeight = 1080;
// Qt event loop tick interval in milliseconds
static constexpr int EventLoopTickMs = 25;


PipeWireVncServer::PipeWireVncServer(QObject* parent) :
	QObject(parent), m_configuration(&VeyonCore::config())
{
}



QWidget* PipeWireVncServer::configurationWidget()
{
	return new PipeWireVncConfigurationWidget(m_configuration);
}

PipeWireVncServer::~PipeWireVncServer()
{
	cleanupVncServer();
}

void PipeWireVncServer::prepareServer()
{
	VeyonCore::platform().coreFunctions().prepareSessionBusAccess();
}

bool PipeWireVncServer::runServer(int serverPort, const Password& password)
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

	if (!initVncServer(serverPort, password))
	{
		return false;
	}

	m_serverRunning = true;

	// Portal session manages the XDG Desktop Portal RemoteDesktop DBus interaction.
	// Created with nullptr parent to avoid the cross-thread QObject parent warning:
	// runServer() is called from the VncServer thread while PipeWireVncServer itself
	// lives on a different thread. Ownership is transferred to cleanupVncServer().
	m_portalSession = new PortalSession(m_configuration.persistRestoreToken(), nullptr);
	connect(m_portalSession, &PortalSession::started, this, &PipeWireVncServer::onPortalStarted,
			Qt::QueuedConnection);
	connect(m_portalSession, &PortalSession::failed, this, &PipeWireVncServer::onPortalFailed,
			Qt::QueuedConnection);

	// PipeWireFramebuffer consumes the PipeWire stream and feeds the rfbScreen.
	// Created with nullptr parent for the same cross-thread reason as m_portalSession.
	m_framebuffer = new PipeWireFramebuffer(&m_screenMutex);
	connect(m_framebuffer, &PipeWireFramebuffer::streamEnded,
			this, &PipeWireVncServer::onStreamEnded, Qt::QueuedConnection);
	connect(m_framebuffer, &PipeWireFramebuffer::frameSizeChanged,
			this, &PipeWireVncServer::onFrameSizeChanged, Qt::QueuedConnection);

	// Start the portal flow asynchronously
	m_portalSession->start();

	// Drive the Qt event loop (portal DBus responses) and rfb clients
	while (m_serverRunning)
	{
		QCoreApplication::processEvents(QEventLoop::AllEvents, EventLoopTickMs);

		m_screenMutex.lock();
		if (m_rfbScreen)
		{
			rfbProcessEvents(m_rfbScreen, 0);
		}
		m_screenMutex.unlock();

		if (m_shouldRestart)
		{
			m_shouldRestart = false;
			vInfo() << "Restarting XDG Portal session";
			m_portalSession->start();
		}

		QThread::msleep(10);
	}

	cleanupVncServer();
	return true;
}

// ---------------------------------------------------------------------------
// Portal / stream callbacks
// ---------------------------------------------------------------------------

void PipeWireVncServer::onPortalStarted()
{
	vInfo() << "XDG Portal RemoteDesktop session started; opening PipeWire stream";

	const int fd = m_portalSession->pipewireFd();
	const auto nodeId = m_portalSession->pipeWireNodeId();

	if (!m_framebuffer->open(fd, nodeId, m_rfbScreen))
	{
		vCritical() << "Failed to open PipeWire framebuffer – will retry";
		QTimer::singleShot(5000, this, [this]() { m_shouldRestart = true; });
	}
}

void PipeWireVncServer::onPortalFailed()
{
	vWarning() << "XDG Portal session failed – will retry in 5 s";

	// the session may have failed while running (see PortalInputDispatcher), so never let the
	// restart open a second stream
	if (m_framebuffer)
	{
		m_framebuffer->close();
	}

	QTimer::singleShot(5000, this, [this]() { m_shouldRestart = true; });
}

void PipeWireVncServer::onStreamEnded()
{
	vInfo() << "PipeWire stream ended – closing session and reconnecting in 2 s";
	closeAndRestartSession();
}

void PipeWireVncServer::closeAndRestartSession()
{
	if (m_framebuffer)
	{
		m_framebuffer->close();
	}
	if (m_portalSession)
	{
		// the portal session belongs to the VncServer thread running runServer()
		QMetaObject::invokeMethod(m_portalSession, &PortalSession::close, Qt::QueuedConnection);
	}

	QTimer::singleShot(2000, this, [this]() { m_shouldRestart = true; });
}

void PipeWireVncServer::onFrameSizeChanged(QSize size)
{
	// the stream size from the portal's Start response, which xdg-desktop-portal checks pointer
	// positions against, is outdated now (e.g. after a change of the output's resolution), so
	// read it again; a change of the scale alone doesn't change the buffers and isn't noticed
	vInfo() << "PipeWire stream size changed to" << size << "– closing session and reconnecting in 2 s";
	closeAndRestartSession();
}

// ---------------------------------------------------------------------------
// LibVNCServer setup / teardown
// ---------------------------------------------------------------------------

bool PipeWireVncServer::initVncServer(int serverPort, const Password& password)
{
	const int fbSize = DefaultWidth * DefaultHeight * 4;
	auto framebufferData = new char[fbSize];
	std::memset(framebufferData, 0, static_cast<size_t>(fbSize));

	m_rfbScreen = rfbGetScreen(nullptr, nullptr, DefaultWidth, DefaultHeight, 8, 3, 4);
	if (!m_rfbScreen)
	{
		vCritical() << "rfbGetScreen failed";
		delete[] framebufferData;
		return false;
	}

	m_vncPassword = qstrdup(password.toByteArray().constData());
	m_vncPasswords[0] = m_vncPassword;
	m_vncPasswords[1] = nullptr;

	m_rfbScreen->desktopName = "VeyonVNC";
	m_rfbScreen->frameBuffer = framebufferData;
	m_rfbScreen->port = serverPort;
	m_rfbScreen->ipv6port = serverPort;
	m_rfbScreen->listenInterface = htonl(INADDR_LOOPBACK);
	m_rfbScreen->listen6Interface = const_cast<char*>("::1");

	m_rfbScreen->authPasswdData = m_vncPasswords;
	m_rfbScreen->passwordCheck = rfbCheckPasswordByList;

	// PipeWireFramebuffer::convertFrame() writes R,G,B,x bytes (rgbSwapped() of the
	// BGRx stream), and rfbNewFramebuffer() resets to exactly that (little-endian) on a resize
	m_rfbScreen->serverFormat.redShift = 0;
	m_rfbScreen->serverFormat.greenShift = 8;
	m_rfbScreen->serverFormat.blueShift = 16;
	m_rfbScreen->serverFormat.redMax = 255;
	m_rfbScreen->serverFormat.greenMax = 255;
	m_rfbScreen->serverFormat.blueMax = 255;
	m_rfbScreen->serverFormat.trueColour = true;
	m_rfbScreen->serverFormat.bitsPerPixel = 32;

	m_rfbScreen->alwaysShared = true;
	m_rfbScreen->handleEventsEagerly = true;
	m_rfbScreen->deferUpdateTime = 5;
	m_rfbScreen->cursor = nullptr;

	// Store a back-pointer so the static C callbacks can access this instance.
	m_rfbScreen->screenData = this;

	rfbInitServer(m_rfbScreen);

	// Install hooks AFTER rfbInitServer (resets them to defaults)
	m_rfbScreen->kbdAddEvent = &PipeWireVncServer::onKbdAddEvent;
	m_rfbScreen->ptrAddEvent = &PipeWireVncServer::onPtrAddEvent;

	rfbMarkRectAsModified(m_rfbScreen, 0, 0, DefaultWidth, DefaultHeight);

	return true;
}

void PipeWireVncServer::cleanupVncServer()
{
	m_serverRunning = false;

	if (m_framebuffer)
	{
		m_framebuffer->close();
		delete m_framebuffer;
		m_framebuffer = nullptr;
	}

	if (m_portalSession)
	{
		m_portalSession->close();
		delete m_portalSession;
		m_portalSession = nullptr;
	}

	if (m_rfbScreen)
	{
		rfbShutdownServer(m_rfbScreen, true);
		delete[] m_rfbScreen->frameBuffer;
		m_rfbScreen->frameBuffer = nullptr;
		rfbScreenCleanup(m_rfbScreen);
		m_rfbScreen = nullptr;
	}

	delete[] m_vncPassword;
	m_vncPassword = nullptr;
}

// ---------------------------------------------------------------------------
// LibVNCServer logging
// ---------------------------------------------------------------------------

void PipeWireVncServer::rfbLogDebug(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	static constexpr int MaxLen = 256;
	char msg[MaxLen];
	vsnprintf(msg, sizeof(msg), format, args);
	msg[MaxLen - 1] = 0;
	va_end(args);
	vDebug() << msg;
}

void PipeWireVncServer::rfbLogNone(const char* /*format*/, ...)
{
}

// ---------------------------------------------------------------------------
// LibVNCServer input event callbacks
// ---------------------------------------------------------------------------

void PipeWireVncServer::onKbdAddEvent(rfbBool down, rfbKeySym keySym, rfbClientRec* cl)
{
	auto* self = static_cast<PipeWireVncServer*>(cl->screen->screenData);
	if (self->m_portalSession)
	{
		self->m_portalSession->notifyKeyboardKeysym(static_cast<quint32>(keySym), down != 0);
	}
}

void PipeWireVncServer::onPtrAddEvent(int buttonMask, int x, int y, rfbClientRec* cl)
{
	auto* self = static_cast<PipeWireVncServer*>(cl->screen->screenData);
	if (self->m_portalSession)
	{
		// called from rfbProcessEvents() with the screen mutex held, so the size matches the
		// framebuffer the client's position refers to, also after a resize
		self->m_portalSession->notifyPointer(buttonMask, x, y, QSize(cl->screen->width, cl->screen->height));
	}
}


IMPLEMENT_CONFIG_PROXY(PipeWireVncConfiguration)
