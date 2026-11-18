// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QEventLoop>
#include <QHostAddress>
#include <QTcpSocket>
#include <QThread>
#include <QtEndian>

#include "LegacyRsaPublicKey.h"
#include "LocalCredentialsVerifier.h"
#include "RfbVeyonAuth.h"
#include "VariantArrayMessage.h"
#include "VeyonConfiguration.h"

#include <memory>


namespace {

// authentication messages of the server are small (auth types, public key)
constexpr qint64 MaximumMessageSize = 64 * 1024;
constexpr qsizetype ProtocolVersionSize = 12;
constexpr auto ProtocolVersion = "RFB 003.008\n";

bool waitForBytes(QTcpSocket& socket, qint64 count, const QDeadlineTimer& deadline)
{
	while (socket.bytesAvailable() < count)
	{
		if (deadline.hasExpired() || socket.state() != QAbstractSocket::ConnectedState ||
			socket.waitForReadyRead(int(qMax<qint64>(1, deadline.remainingTime()))) == false)
		{
			return socket.bytesAvailable() >= count;
		}
	}

	return true;
}



bool receiveMessage(QTcpSocket& socket, VariantArrayMessage& message, const QDeadlineTimer& deadline)
{
	VariantArrayMessage::MessageSize size = 0;
	if (waitForBytes(socket, sizeof(size), deadline) == false ||
		socket.peek(reinterpret_cast<char*>(&size), sizeof(size)) != sizeof(size))
	{
		return false;
	}

	size = qFromBigEndian(size);
	return size <= MaximumMessageSize && waitForBytes(socket, qint64(sizeof(size)) + size, deadline) &&
		message.receive();
}

} // namespace



LocalCredentialsVerifier::Result LocalCredentialsVerifier::verify(const QString& username,
																  const Veyon::Crypto::SecureBuffer& password)
{
	const auto port = quint16(VeyonCore::config().veyonServerPort() + VeyonCore::sessionId());

	auto result = Result::Unavailable;
	std::unique_ptr<QThread> thread(QThread::create([&]() {
		result = verify(username, password, QHostAddress(QHostAddress::LocalHost).toString(), port);
	}));

	QEventLoop eventLoop;
	QObject::connect(thread.get(), &QThread::finished, &eventLoop, &QEventLoop::quit);
	thread->start();
	eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
	thread->wait();

	return result;
}



LocalCredentialsVerifier::Result LocalCredentialsVerifier::verify(const QString& username,
																  const Veyon::Crypto::SecureBuffer& password,
																  const QString& host, quint16 port, int timeout)
{
	const QDeadlineTimer deadline(timeout);

	QTcpSocket socket;
	socket.connectToHost(host, port);
	if (socket.waitForConnected(int(qMax<qint64>(1, deadline.remainingTime()))) == false)
	{
		vDebug() << "no Veyon Server at" << host << port;
		return Result::Unavailable;
	}

	// protocol version and security types: only rfbSecTypeVeyon
	if (waitForBytes(socket, ProtocolVersionSize, deadline) == false ||
		socket.read(ProtocolVersionSize) != QByteArray(ProtocolVersion))
	{
		vWarning() << "unexpected protocol at" << host << port;
		return Result::Unavailable;
	}
	socket.write(ProtocolVersion, ProtocolVersionSize);

	quint8 securityTypeCount = 0;
	if (waitForBytes(socket, 1, deadline) == false ||
		socket.read(reinterpret_cast<char*>(&securityTypeCount), 1) != 1 || securityTypeCount == 0 ||
		waitForBytes(socket, securityTypeCount, deadline) == false ||
		socket.read(securityTypeCount).contains(char(rfbSecTypeVeyon)) == false)
	{
		vWarning() << "Veyon security type not offered";
		return Result::Unavailable;
	}
	const char securityType = rfbSecTypeVeyon;
	socket.write(&securityType, 1);

	// Veyon authentication types
	VariantArrayMessage authTypesMessage(&socket);
	if (receiveMessage(socket, authTypesMessage, deadline) == false)
	{
		return Result::Unavailable;
	}

	const auto authTypeCount = authTypesMessage.read().toInt();
	bool logonOffered = false;
	for (int i = 0; i < authTypeCount && i < 32; ++i)
	{
		logonOffered |= authTypesMessage.read().value<RfbVeyonAuth::Type>() == RfbVeyonAuth::Logon;
	}
	if (logonOffered == false)
	{
		vWarning() << "server does not offer logon authentication";
		return Result::Unavailable;
	}

	VariantArrayMessage authTypeResponse(&socket);
	authTypeResponse.write(int(RfbVeyonAuth::Logon)).write(username);
	if (authTypeResponse.send() == false)
	{
		return Result::Unavailable;
	}

	// acknowledge, then the server's per-connection public key
	VariantArrayMessage ackMessage(&socket);
	VariantArrayMessage publicKeyMessage(&socket);
	if (receiveMessage(socket, ackMessage, deadline) == false ||
		receiveMessage(socket, publicKeyMessage, deadline) == false)
	{
		return Result::Unavailable;
	}

	// 4.x Logon scheme: RSAES-OAEP (SHA-1) with the server's per-connection key (LegacyRsa.h)
	const auto publicKey = Veyon::Crypto::LegacyRsaPublicKey::fromPem(publicKeyMessage.read().toString().toUtf8());
	if (publicKey.has_value() == false)
	{
		vWarning() << "invalid public key";
		return Result::Unavailable;
	}

	const auto encryptedPassword =
		publicKey->encryptOaepSha1(Veyon::Crypto::ByteView(password.constData(), password.size()));
	VariantArrayMessage passwordMessage(&socket);
	if (encryptedPassword.has_value() == false || passwordMessage.write(*encryptedPassword).send() == false)
	{
		return Result::Unavailable;
	}

	// security result: rfbVncAuthOK, or the server closes the connection; access control only
	// follows and is not part of the check
	quint32 securityResult = 0;
	if (waitForBytes(socket, sizeof(securityResult), deadline) == false)
	{
		if (socket.state() == QAbstractSocket::ConnectedState)
		{
			vWarning() << "no authentication result in time";
			return Result::Unavailable;
		}
		return Result::Invalid;
	}

	socket.read(reinterpret_cast<char*>(&securityResult), sizeof(securityResult));
	return qFromBigEndian(securityResult) == 0 ? Result::Valid : Result::Invalid;
}
