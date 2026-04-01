/*
 * PipeWireVncServer.cpp - Wayland VNC server backend using PipeWire + XDG Desktop Portal
 *
 * Copyright (c) 2024-2026 Tobias Junghans <tobydox@veyon.io>
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

#include "PipeWireVncServer.h"
#include "PipeWireFramebuffer.h"
#include "PortalSession.h"

#include <VeyonCore.h>

#include <QCoreApplication>
#include <QTimer>

#include <cstring>

// Initial framebuffer dimensions (used before the first PipeWire frame arrives)
static constexpr int DefaultWidth  = 1920;
static constexpr int DefaultHeight = 1080;
// Qt event loop tick interval in milliseconds
static constexpr int EventLoopTickMs = 25;


PipeWireVncServer::PipeWireVncServer(QObject* parent)
	: QObject(parent)
{
}

PipeWireVncServer::~PipeWireVncServer()
{
	cleanupVncServer();
}

void PipeWireVncServer::prepareServer()
{
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

	// Portal session manages the XDG Desktop Portal RemoteDesktop DBus interaction
	m_portalSession = new PortalSession(this);
	connect(m_portalSession, &PortalSession::started, this, &PipeWireVncServer::onPortalStarted,
	        Qt::QueuedConnection);
	connect(m_portalSession, &PortalSession::failed,  this, &PipeWireVncServer::onPortalFailed,
	        Qt::QueuedConnection);

	// PipeWireFramebuffer consumes the PipeWire stream and feeds the rfbScreen
	m_framebuffer = new PipeWireFramebuffer(this);
	connect(m_framebuffer, &PipeWireFramebuffer::streamEnded,
	        this, &PipeWireVncServer::onStreamEnded, Qt::QueuedConnection);

	// Start the portal flow asynchronously
	m_portalSession->start();

	// Drive the Qt event loop (portal DBus responses) and rfb clients
	while (m_serverRunning)
	{
		QCoreApplication::processEvents(QEventLoop::AllEvents, EventLoopTickMs);

		if (m_rfbScreen)
		{
			rfbProcessEvents(m_rfbScreen, 0);
		}

		if (m_shouldRestart)
		{
			m_shouldRestart = false;
			vInfo() << "Restarting XDG Portal session";
			m_portalSession->start();
		}
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

	const int  fd     = m_portalSession->pipewireFd();
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
	QTimer::singleShot(5000, this, [this]() { m_shouldRestart = true; });
}

void PipeWireVncServer::onStreamEnded()
{
	vInfo() << "PipeWire stream ended – closing session and reconnecting in 2 s";

	if (m_framebuffer)
	{
		m_framebuffer->close();
	}
	if (m_portalSession)
	{
		m_portalSession->close();
	}

	QTimer::singleShot(2000, this, [this]() { m_shouldRestart = true; });
}

// ---------------------------------------------------------------------------
// LibVNCServer setup / teardown
// ---------------------------------------------------------------------------

bool PipeWireVncServer::initVncServer(int serverPort, const Password& password)
{
	const int fbSize = DefaultWidth * DefaultHeight * 4;
	m_framebufferData = new char[fbSize];
	std::memset(m_framebufferData, 0, static_cast<size_t>(fbSize));

	m_rfbScreen = rfbGetScreen(nullptr, nullptr, DefaultWidth, DefaultHeight, 8, 3, 4);
	if (!m_rfbScreen)
	{
		vCritical() << "rfbGetScreen failed";
		delete[] m_framebufferData;
		m_framebufferData = nullptr;
		return false;
	}

	m_vncPassword = qstrdup(password.toByteArray().constData());
	m_vncPasswords[0] = m_vncPassword;
	m_vncPasswords[1] = nullptr;

	m_rfbScreen->desktopName     = "VeyonVNC";
	m_rfbScreen->frameBuffer     = m_framebufferData;
	m_rfbScreen->port            = serverPort;
	m_rfbScreen->ipv6port        = serverPort;
	m_rfbScreen->authPasswdData  = m_vncPasswords;
	m_rfbScreen->passwordCheck   = rfbCheckPasswordByList;

	m_rfbScreen->serverFormat.redShift     = 16;
	m_rfbScreen->serverFormat.greenShift   = 8;
	m_rfbScreen->serverFormat.blueShift    = 0;
	m_rfbScreen->serverFormat.redMax       = 255;
	m_rfbScreen->serverFormat.greenMax     = 255;
	m_rfbScreen->serverFormat.blueMax      = 255;
	m_rfbScreen->serverFormat.trueColour   = true;
	m_rfbScreen->serverFormat.bitsPerPixel = 32;

	m_rfbScreen->alwaysShared        = true;
	m_rfbScreen->handleEventsEagerly = true;
	m_rfbScreen->deferUpdateTime     = 5;
	m_rfbScreen->cursor              = nullptr;

	rfbInitServer(m_rfbScreen);
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
		rfbScreenCleanup(m_rfbScreen);
		m_rfbScreen = nullptr;
	}

	delete[] m_framebufferData;
	m_framebufferData = nullptr;

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
