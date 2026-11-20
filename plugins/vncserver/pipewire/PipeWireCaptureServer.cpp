// SPDX-FileCopyrightText: 2024-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QFile>
#include <QSocketNotifier>
#include <QThread>
#include <QTimer>

#include "CaptureHelperCredentials.h"
#include "ConstantTime.h"
#include "PipeWireCaptureServer.h"
#include "PipeWireFramebuffer.h"
#include "PortalSession.h"
#include "VeyonCore.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <openssl/crypto.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

// Initial framebuffer dimensions (used before the first PipeWire frame arrives)
static constexpr int DefaultWidth = 1920;
static constexpr int DefaultHeight = 1080;
// Qt event loop tick interval in milliseconds
static constexpr int EventLoopTickMs = 25;


PipeWireCaptureServer::PipeWireCaptureServer(bool persistRestoreToken, QObject* parent) :
	QObject(parent),
	m_persistRestoreToken(persistRestoreToken)
{
}



PipeWireCaptureServer::~PipeWireCaptureServer()
{
	cleanupVncServer();
}



bool PipeWireCaptureServer::run(const QString& socketPath, const Veyon::Crypto::SecureBuffer& vncPassword)
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

	if (!initVncServer(vncPassword) || !listenOnSocket(socketPath))
	{
		return false;
	}

	m_serverRunning = true;

	// Portal session manages the XDG Desktop Portal RemoteDesktop DBus interaction.
	// Ownership is transferred to cleanupVncServer().
	m_portalSession = new PortalSession(m_persistRestoreToken, nullptr);
	connect(m_portalSession, &PortalSession::started, this, &PipeWireCaptureServer::onPortalStarted,
			Qt::QueuedConnection);
	connect(m_portalSession, &PortalSession::failed, this, &PipeWireCaptureServer::onPortalFailed,
			Qt::QueuedConnection);

	// PipeWireFramebuffer consumes the PipeWire stream and feeds the rfbScreen.
	m_framebuffer = new PipeWireFramebuffer(&m_screenMutex);
	connect(m_framebuffer, &PipeWireFramebuffer::streamEnded,
			this, &PipeWireCaptureServer::onStreamEnded, Qt::QueuedConnection);
	connect(m_framebuffer, &PipeWireFramebuffer::frameSizeChanged,
			this, &PipeWireCaptureServer::onFrameSizeChanged, Qt::QueuedConnection);

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

void PipeWireCaptureServer::onPortalStarted()
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

void PipeWireCaptureServer::onPortalFailed()
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

void PipeWireCaptureServer::onStreamEnded()
{
	vInfo() << "PipeWire stream ended – closing session and reconnecting in 2 s";
	closeAndRestartSession();
}

void PipeWireCaptureServer::onFrameSizeChanged(QSize size)
{
	// the stream size from the portal's Start response, which xdg-desktop-portal checks pointer
	// positions against, is outdated now (e.g. after a change of the output's resolution), so
	// read it again; a change of the scale alone doesn't change the buffers and isn't noticed
	vInfo() << "PipeWire stream size changed to" << size << "– closing session and reconnecting in 2 s";
	closeAndRestartSession();
}

void PipeWireCaptureServer::closeAndRestartSession()
{
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

bool PipeWireCaptureServer::initVncServer(const Veyon::Crypto::SecureBuffer& vncPassword)
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

	m_vncPassword = vncPassword.clone();

	m_rfbScreen->desktopName = "VeyonVNC";
	m_rfbScreen->frameBuffer = framebufferData;
	// no TCP/IPv6/UDP listener (and thus no WebSockets): listenOnSocket() accepts the clients
	m_rfbScreen->port = 0;
	m_rfbScreen->ipv6port = 0;
	m_rfbScreen->udpPort = 0;
	m_rfbScreen->autoPort = FALSE;
	m_rfbScreen->httpPort = 0;
	m_rfbScreen->httpDir = nullptr;
	m_rfbScreen->maxClientWait = MaximumClientWait;

	// LibVNCServer offers VNC authentication whenever authPasswdData is set; checkVncPassword()
	// uses m_vncPassword instead
	m_rfbScreen->authPasswdData = this;
	m_rfbScreen->passwordCheck = &PipeWireCaptureServer::checkVncPassword;

	// PipeWireFrame::copy() writes R,G,B,x bytes (red and blue of the
	// BGRx stream swapped), and rfbNewFramebuffer() resets to exactly that (little-endian) on a resize
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

	if (m_rfbScreen->listenSock != RFB_INVALID_SOCKET || m_rfbScreen->listen6Sock != RFB_INVALID_SOCKET)
	{
		vCritical() << "LibVNCServer unexpectedly listens on a TCP port";
		return false;
	}

	m_rfbScreen->newClientHook = &PipeWireCaptureServer::onNewClient;

	// Install hooks AFTER rfbInitServer (resets them to defaults)
	m_rfbScreen->kbdAddEvent = &PipeWireCaptureServer::onKbdAddEvent;
	m_rfbScreen->ptrAddEvent = &PipeWireCaptureServer::onPtrAddEvent;

	rfbMarkRectAsModified(m_rfbScreen, 0, 0, DefaultWidth, DefaultHeight);

	return true;
}

void PipeWireCaptureServer::cleanupVncServer()
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

	delete m_listenNotifier;
	m_listenNotifier = nullptr;

	if (m_listenSocket >= 0)
	{
		close(m_listenSocket);
		m_listenSocket = -1;
		unlink(QFile::encodeName(m_socketPath).constData());
	}

	if (m_rfbScreen)
	{
		rfbShutdownServer(m_rfbScreen, true);
		delete[] m_rfbScreen->frameBuffer;
		m_rfbScreen->frameBuffer = nullptr;
		rfbScreenCleanup(m_rfbScreen);
		m_rfbScreen = nullptr;
	}

	m_vncPassword.clear();
}

// ---------------------------------------------------------------------------
// LibVNCServer logging
// ---------------------------------------------------------------------------

void PipeWireCaptureServer::rfbLogDebug(const char* format, ...)
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

void PipeWireCaptureServer::rfbLogNone(const char* /*format*/, ...)
{
}

bool PipeWireCaptureServer::listenOnSocket(const QString& socketPath)
{
	const auto encodedPath = QFile::encodeName(socketPath);

	sockaddr_un address{};
	address.sun_family = AF_UNIX;
	if (encodedPath.isEmpty() || size_t(encodedPath.size()) >= sizeof(address.sun_path))
	{
		vCritical() << "invalid socket path" << socketPath;
		return false;
	}
	std::memcpy(address.sun_path, encodedPath.constData(), size_t(encodedPath.size()));

	m_listenSocket = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
	if (m_listenSocket < 0)
	{
		vCritical() << "could not create socket";
		return false;
	}

	// the directory is private (0700) already, the socket is not accessible for others either;
	// bind() fails if the path exists, e.g. when another process (possibly of the same user,
	// R-19) created it - exit so that veyon-server restarts the helper
	const auto previousUmask = umask(0077);
	const auto bound = bind(m_listenSocket, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0;
	umask(previousUmask);
	if (bound == false)
	{
		vCritical() << "could not bind socket" << socketPath << strerror(errno);
		close(m_listenSocket);
		m_listenSocket = -1;
		return false;
	}
	m_socketPath = socketPath;

	if (chmod(encodedPath.constData(), 0600) != 0 || listen(m_listenSocket, MaximumClients) != 0)
	{
		vCritical() << "could not listen on socket" << socketPath << strerror(errno);
		return false;
	}

	m_listenNotifier = new QSocketNotifier(m_listenSocket, QSocketNotifier::Read);
	connect(m_listenNotifier, &QSocketNotifier::activated, this, &PipeWireCaptureServer::acceptConnections);

	return true;
}



void PipeWireCaptureServer::acceptConnections()
{
	while (true)
	{
		const auto clientSocket = accept4(m_listenSocket, nullptr, nullptr, SOCK_CLOEXEC);
		if (clientSocket < 0)
		{
			return;
		}

		// LibVNCServer uses select(): a descriptor beyond FD_SETSIZE would make FD_SET() abort
		if (clientSocket >= FD_SETSIZE)
		{
			close(clientSocket);
			continue;
		}

		// only processes of the own user may connect, i.e. veyon-server and its demo server; the
		// peer is checked before LibVNCServer sees a single byte
		ucred credentials{};
		socklen_t credentialsSize = sizeof(credentials);
		if (getsockopt(clientSocket, SOL_SOCKET, SO_PEERCRED, &credentials, &credentialsSize) != 0 ||
			credentialsSize != sizeof(credentials) || credentials.uid != getuid())
		{
			vWarning() << "rejecting connection of uid" << credentials.uid;
			close(clientSocket);
			continue;
		}

		if (m_clientCount >= MaximumClients)
		{
			vWarning() << "rejecting connection: too many clients";
			close(clientSocket);
			continue;
		}

		QMutexLocker screenLocker(&m_screenMutex);
		// takes ownership of the descriptor, also on failure
		rfbNewClient(m_rfbScreen, clientSocket);
	}
}



rfbNewClientAction PipeWireCaptureServer::onNewClient(rfbClientPtr client)
{
	auto* self = static_cast<PipeWireCaptureServer*>(client->screen->screenData);
	if (self->m_clientCount >= MaximumClients)
	{
		return RFB_CLIENT_REFUSE;
	}

	++self->m_clientCount;
	client->clientGoneHook = &PipeWireCaptureServer::onClientGone;

	return RFB_CLIENT_ACCEPT;
}



void PipeWireCaptureServer::onClientGone(rfbClientPtr client)
{
	auto* self = static_cast<PipeWireCaptureServer*>(client->screen->screenData);
	--self->m_clientCount;
}



rfbBool PipeWireCaptureServer::checkVncPassword(rfbClientPtr client, const char* response, int length)
{
	static constexpr auto KeySize = CaptureHelperCredentials::VncAuthenticationKeySize;

	const auto* self = static_cast<PipeWireCaptureServer*>(client->screen->screenData);
	if (length != CHALLENGESIZE || self->m_vncPassword.size() < KeySize)
	{
		return FALSE;
	}

	// same key as VncClientProtocol: the first 8 password bytes, which are never NUL
	// (CaptureHelperCredentials::fromPassword()), so the exported rfbEncryptBytes() can be used
	char key[KeySize + 1]{};
	std::memcpy(key, self->m_vncPassword.constData(), size_t(KeySize));

	unsigned char expectedResponse[CHALLENGESIZE];
	std::memcpy(expectedResponse, client->authChallenge, CHALLENGESIZE);
	rfbEncryptBytes(expectedResponse, key);
	OPENSSL_cleanse(key, sizeof(key));

	const auto valid = Veyon::Crypto::constantTimeEquals(
		{reinterpret_cast<const char*>(expectedResponse), CHALLENGESIZE}, {response, length});
	OPENSSL_cleanse(expectedResponse, sizeof(expectedResponse));

	if (valid == false)
	{
		vWarning() << "VNC authentication failed";
	}

	return valid ? TRUE : FALSE;
}

// ---------------------------------------------------------------------------
// LibVNCServer input event callbacks
// ---------------------------------------------------------------------------

void PipeWireCaptureServer::onKbdAddEvent(rfbBool down, rfbKeySym keySym, rfbClientRec* cl)
{
	auto* self = static_cast<PipeWireCaptureServer*>(cl->screen->screenData);
	if (self->m_portalSession)
	{
		self->m_portalSession->notifyKeyboardKeysym(static_cast<quint32>(keySym), down != 0);
	}
}

void PipeWireCaptureServer::onPtrAddEvent(int buttonMask, int x, int y, rfbClientRec* cl)
{
	auto* self = static_cast<PipeWireCaptureServer*>(cl->screen->screenData);
	if (self->m_portalSession)
	{
		// called from rfbProcessEvents() with the screen mutex held, so the size matches the
		// framebuffer the client's position refers to, also after a resize
		self->m_portalSession->notifyPointer(buttonMask, x, y, QSize(cl->screen->width, cl->screen->height));
	}
}
