/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <QtTest>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtEndian>

#include "VncClientProtocol.h"
#include "VncProxyConnection.h"
#include "VncServerClient.h"
#include "VncServerProtocol.h"

// These tests start after external authentication and access control. Both
// production protocol parsers and the proxy's real TCP sockets remain in use.
class TestServerProtocol : public VncServerProtocol
{
public:
	using VncServerProtocol::VncServerProtocol;
private:
	QVector<RfbVeyonAuth::Type> supportedAuthTypes() const override { return {}; }
	void processAuthenticationMessage(VariantArrayMessage&) override {}
	void performAccessControl() override {}
};

class TestProxy : public VncProxyConnection
{
public:
	TestProxy(QTcpSocket* socket, int backendPort) : VncProxyConnection(socket, backendPort, nullptr),
		server(socket, &peer), backend(vncServerSocket(), {})
	{
		peer.setAuthState(VncServerClient::AuthState::Successful);
		peer.setAccessControlState(VncServerClient::AccessControlState::Successful);
		peer.setProtocolState(VncServerProtocol::State::FramebufferInit);
	}
	VncClientProtocol& clientProtocol() override { return backend; }
	VncServerProtocol& serverProtocol() override { return server; }
	VncServerClient peer;
	TestServerProtocol server;
	VncClientProtocol backend;
};

class VncProxyConnectionTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void initTestCase()
	{
		qRegisterMetaType<QAbstractSocket::SocketError>();
	}

	void backendStartingLateDoesNotRequireAnExternalReconnect()
	{
		QTcpServer backend;
		QVERIFY(backend.listen(QHostAddress::LocalHost));
		const auto port = backend.serverPort();
		backend.close(); // Model a capture child which has not opened its listener.
		QTcpServer listener;
		QVERIFY(listener.listen(QHostAddress::LocalHost));
		QTcpSocket master;
		master.connectToHost(QHostAddress::LocalHost, listener.serverPort());
		QTRY_VERIFY(listener.hasPendingConnections());
		TestProxy proxy(listener.nextPendingConnection(), port);
		QSignalSpy errors(proxy.vncServerSocket(), &QTcpSocket::errorOccurred);
		proxy.start();
		master.write(QByteArray(1, '\1')); // ClientInit, after authentication.
		QTRY_VERIFY(errors.size() >= 2);
		QCOMPARE(errors.first().first().value<QAbstractSocket::SocketError>(), QAbstractSocket::ConnectionRefusedError);
		QCOMPARE(master.state(), QAbstractSocket::ConnectedState);
		QVERIFY(backend.listen(QHostAddress::LocalHost, port));
		QTRY_VERIFY(backend.hasPendingConnections());
		auto* capture = backend.nextPendingConnection();
		capture->write(QByteArrayLiteral("RFB 003.008\n"));
		QTRY_VERIFY(capture->bytesAvailable() >= 12);
		QCOMPARE(capture->read(12), QByteArrayLiteral("RFB 003.008\n"));
		const char securityTypes[] = {1, rfbSecTypeNone};
		capture->write(securityTypes, sizeof(securityTypes));
		QTRY_VERIFY(capture->bytesAvailable() >= 1);
		QCOMPARE(capture->read(1), QByteArray(1, rfbSecTypeNone));
		const auto result = qToBigEndian<uint32_t>(rfbVncAuthOK);
		capture->write(reinterpret_cast<const char*>(&result), sizeof(result));
		QTRY_VERIFY(capture->bytesAvailable() >= 1);
		QCOMPARE(capture->read(1), QByteArray(1, '\1'));
		rfbServerInitMsg init{};
		init.framebufferWidth = qToBigEndian<uint16_t>(640);
		init.framebufferHeight = qToBigEndian<uint16_t>(480);
		init.format.bitsPerPixel = 32;
		init.format.depth = 24;
		init.format.trueColour = 1;
		init.nameLength = qToBigEndian<uint32_t>(4);
		const auto serverInit = QByteArray(reinterpret_cast<const char*>(&init), sz_rfbServerInitMsg) + QByteArrayLiteral("test");
		capture->write(serverInit);
		QTRY_COMPARE(proxy.server.state(), VncServerProtocol::State::Running);
		QTRY_COMPARE(master.bytesAvailable(), qint64(serverInit.size()));
		QCOMPARE(master.readAll(), serverInit);
		QCOMPARE(master.state(), QAbstractSocket::ConnectedState);

		// The same authenticated external socket can request and receive a frame.
		rfbFramebufferUpdateRequestMsg request{};
		request.type = rfbFramebufferUpdateRequest;
		request.w = qToBigEndian<uint16_t>(640);
		request.h = qToBigEndian<uint16_t>(480);
		const auto requestBytes = QByteArray(reinterpret_cast<const char*>(&request), sz_rfbFramebufferUpdateRequestMsg);
		master.write(requestBytes);
		QTRY_COMPARE(capture->bytesAvailable(), qint64(requestBytes.size()));
		QCOMPARE(capture->readAll(), requestBytes);
		rfbFramebufferUpdateMsg update{};
		update.type = rfbFramebufferUpdate;
		update.nRects = qToBigEndian<uint16_t>(1);
		rfbFramebufferUpdateRectHeader rect{};
		rect.r.w = qToBigEndian<uint16_t>(1);
		rect.r.h = qToBigEndian<uint16_t>(1);
		rect.encoding = qToBigEndian<uint32_t>(rfbEncodingRaw);
		const auto frame = QByteArray(reinterpret_cast<const char*>(&update), sz_rfbFramebufferUpdateMsg) +
			QByteArray(reinterpret_cast<const char*>(&rect), sz_rfbFramebufferUpdateRectHeader) + QByteArray::fromHex("12345600");
		capture->write(frame);
		QTRY_COMPARE(master.bytesAvailable(), qint64(frame.size()));
		QCOMPARE(master.readAll(), frame);
	}

	void refusedBackendDoesNotReopenAfterExternalDisconnect()
	{
		QTcpServer backend;
		QVERIFY(backend.listen(QHostAddress::LocalHost));
		const auto port = backend.serverPort();
		backend.close();
		QTcpServer listener;
		QVERIFY(listener.listen(QHostAddress::LocalHost));
		QTcpSocket master;
		master.connectToHost(QHostAddress::LocalHost, listener.serverPort());
		QTRY_VERIFY(listener.hasPendingConnections());
		TestProxy proxy(listener.nextPendingConnection(), port);
		QSignalSpy errors(proxy.vncServerSocket(), &QTcpSocket::errorOccurred);
		proxy.start();
		master.write(QByteArray(1, '\1'));
		QTRY_VERIFY(!errors.isEmpty());
		master.abort();
		QTRY_COMPARE(proxy.proxyClientSocket()->state(), QAbstractSocket::UnconnectedState);
		QVERIFY(backend.listen(QHostAddress::LocalHost, port));
		// Observe several scheduled retry intervals with the proxy still alive.
		QTest::qWait(800);
		QVERIFY(!backend.hasPendingConnections());
		QCOMPARE(proxy.vncServerSocket()->state(), QAbstractSocket::UnconnectedState);
	}
};

QTEST_GUILESS_MAIN(VncProxyConnectionTest)
#include "VncProxyConnectionTest.moc"
