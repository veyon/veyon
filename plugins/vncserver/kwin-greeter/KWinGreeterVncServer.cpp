/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "KWinGreeterVncServer.h"
#include "GreeterPipeWire.h"
#include "KWinScreencast.h"
#include "PipeWireFramebuffer.h"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QMutex>
#include <QScopeGuard>
#include <QTimer>
#include <unistd.h>

bool KWinGreeterVncServer::runServer(int serverPort, const Password& password)
{
	const auto arguments = QCoreApplication::arguments();
	if (arguments.value(1) != QLatin1String("--greeter-vnc") || arguments.value(2) != QLatin1String("kwin") || ::getuid() == 0)
	{
		vCritical() << "KWin greeter capture requires the isolated backend under the greeter account";
		return false;
	}
	const auto displayName = qEnvironmentVariable("WAYLAND_DISPLAY");
	if (displayName.isEmpty())
	{
		vCritical() << "KWin greeter capture needs the greeter's WAYLAND_DISPLAY";
		return false;
	}
	const auto displayPath = QDir::isAbsolutePath(displayName) ? displayName :
		QDir(qEnvironmentVariable("XDG_RUNTIME_DIR")).filePath(displayName);

	// The listener is initialized only after the first real PipeWire frame.
	auto* screen = rfbGetScreen(nullptr, nullptr, 1, 1, 8, 3, 4);
	if (!screen) { vCritical() << "Could not allocate the KWin greeter RFB screen"; return false; }
	screen->frameBuffer = new char[4]{};
	screen->desktopName = "VeyonGreeter";
	screen->port = serverPort;
	screen->ipv6port = serverPort;
	screen->listenInterface = htonl(INADDR_LOOPBACK);
	screen->listen6Interface = const_cast<char*>("::1");
	auto passwordBytes = password.toByteArray();
	char* passwords[] = {passwordBytes.data(), nullptr};
	screen->authPasswdData = passwords;
	screen->passwordCheck = rfbCheckPasswordByList;
	screen->alwaysShared = true;
	screen->cursor = nullptr;
	screen->serverFormat.redShift = 0;
	screen->serverFormat.greenShift = 8;
	screen->serverFormat.blueShift = 16;
	screen->serverFormat.redMax = screen->serverFormat.greenMax = screen->serverFormat.blueMax = 255;
	screen->serverFormat.trueColour = true;
	screen->serverFormat.bitsPerPixel = 32;

	QMutex screenMutex;
	PipeWireFramebuffer framebuffer(&screenMutex);
	GreeterPipeWire pipewire;
	KWinScreencast capture;
	QEventLoop loop;
	QTimer startupDeadline;
	startupDeadline.setSingleShot(true);
	QTimer events;
	bool listening = false;
	bool success = false;
	int pipewireFd = -1;
	const auto closeFd = qScopeGuard([&]() { if (pipewireFd >= 0) { ::close(pipewireFd); } });
	const auto fail = [&](const QString& error) {
		vCritical() << error;
		success = false;
		loop.quit();
	};
	connect(&pipewire, &GreeterPipeWire::failed, &loop, fail);
	connect(&capture, &KWinScreencast::failed, &loop, fail);
	connect(&startupDeadline, &QTimer::timeout, &loop, [&]() { fail(QStringLiteral("KWin greeter stream did not deliver a first frame before the deadline")); });
	connect(&framebuffer, &PipeWireFramebuffer::streamEnded, &loop, [&]() {
		fail(QStringLiteral("KWin greeter PipeWire stream ended"));
	}, Qt::QueuedConnection);
	connect(&framebuffer, &PipeWireFramebuffer::firstFrameReady, &loop, [&]() {
		QMutexLocker lock(&screenMutex);
		rfbInitServer(screen);
		screen->kbdAddEvent = [](rfbBool, rfbKeySym, rfbClientPtr) {};
		screen->ptrAddEvent = [](int, int, int, rfbClientPtr) {};
		if (screen->listenSock < 0 && screen->listen6Sock < 0)
		{
			fail(QStringLiteral("Could not listen on the internal KWin greeter VNC port"));
			return;
		}
		listening = true;
		success = true;
		startupDeadline.stop();
		rfbMarkRectAsModified(screen, 0, 0, screen->width, screen->height);
		vInfo() << "KWin greeter PipeWire stream ready with actual frame" << screen->width << screen->height;
		events.start(10);
	}, Qt::QueuedConnection);
	connect(&events, &QTimer::timeout, &loop, [&]() {
		QMutexLocker lock(&screenMutex);
		rfbProcessEvents(screen, 0);
	});
	connect(&capture, &KWinScreencast::streamCreated, &loop, [&](quint32 nodeId) {
		const auto fd = pipewireFd;
		pipewireFd = -1;
		if (fd < 0 || !framebuffer.open(fd, nodeId, screen, true))
		{
			fail(QStringLiteral("Could not open the KWin greeter PipeWire consumer"));
		}
	});
	connect(&pipewire, &GreeterPipeWire::ready, &loop, [&](int fd) {
		pipewireFd = fd;
		const auto waylandFd = GreeterPipeWire::connectSocket(displayPath, uint(::getuid()));
		if (waylandFd < 0) { fail(QStringLiteral("Cannot connect to the greeter-owned Wayland socket")); return; }
		capture.start(waylandFd);
	});
	// Leave room within the supervisor's ten-second authenticated RFB probe.
	startupDeadline.start(8500);
	QTimer::singleShot(0, &pipewire, &GreeterPipeWire::start);
	loop.exec();
	events.stop();
	framebuffer.close(); // joins its thread before releasing the shared RFB screen
	capture.stop();
	if (listening) { rfbShutdownServer(screen, true); }
	delete[] screen->frameBuffer;
	screen->frameBuffer = nullptr;
	rfbScreenCleanup(screen);
	return success;
}
