/*
 * VncProtocolRobustnessTest.cpp - protocol fragmentation and size-limit tests
 *
 * Copyright (c) 2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 */

#include <QtTest>

#include <cstring>
#include <limits>

#include "rfb/rfbproto.h"

#include "VariantArrayMessage.h"
#include "VncClientProtocol.h"

class DuplexDevice : public QIODevice
{
public:
	DuplexDevice()
	{
		open(QIODevice::ReadWrite);
	}

	bool isSequential() const override
	{
		return true;
	}

	qint64 bytesAvailable() const override
	{
		return m_input.size() + QIODevice::bytesAvailable();
	}

	void appendInput(const QByteArray& data)
	{
		m_input.append(data);
	}

	const QByteArray& output() const
	{
		return m_output;
	}

protected:
	qint64 readData(char* data, qint64 maxSize) override
	{
		const auto size = qMin(maxSize, qint64(m_input.size()));
		if( size <= 0 )
		{
			return 0;
		}
		std::memcpy(data, m_input.constData(), static_cast<size_t>(size));
		m_input.remove(0, static_cast<int>(size));
		return size;
	}

	qint64 writeData(const char* data, qint64 size) override
	{
		m_output.append(data, static_cast<int>(size));
		return size;
	}

private:
	QByteArray m_input{};
	QByteArray m_output{};
};


class VncProtocolRobustnessTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void handlesPipelinedAndFragmentedHandshake();
	void rejectsOversizedCutText();
	void rejectsOversizedVariantMessageEarly();

private:
	static void completeHandshake(DuplexDevice& device, VncClientProtocol& protocol);
};


void VncProtocolRobustnessTest::completeHandshake(DuplexDevice& device, VncClientProtocol& protocol)
{
	protocol.start();
	device.appendInput(QByteArrayLiteral("RFB 003.008\n"));
	QVERIFY(protocol.read());

	const char securityTypes[] = { 1, rfbSecTypeNone };
	device.appendInput(QByteArray(securityTypes, sizeof(securityTypes)));
	QVERIFY(protocol.read());

	const auto authResult = qToBigEndian<uint32_t>(rfbVncAuthOK);
	device.appendInput(QByteArray(reinterpret_cast<const char*>(&authResult), sizeof(authResult)));
	QVERIFY(protocol.read());

	rfbServerInitMsg init{};
	init.framebufferWidth = qToBigEndian<uint16_t>(640);
	init.framebufferHeight = qToBigEndian<uint16_t>(480);
	init.format.bitsPerPixel = 32;
	init.format.depth = 24;
	init.format.trueColour = 1;
	init.nameLength = qToBigEndian<uint32_t>(4);
	device.appendInput(QByteArray(reinterpret_cast<const char*>(&init), sz_rfbServerInitMsg));
	device.appendInput(QByteArrayLiteral("test"));
	QVERIFY(protocol.read());
	QCOMPARE(protocol.state(), VncClientProtocol::Running);
}


void VncProtocolRobustnessTest::handlesPipelinedAndFragmentedHandshake()
{
	DuplexDevice device;
	VncClientProtocol protocol(&device, {});
	protocol.start();

	const char firstFragment[] = { 2, rfbSecTypeVncAuth };
	device.appendInput(QByteArrayLiteral("RFB 003.008\n") + QByteArray(firstFragment, sizeof(firstFragment)));
	QVERIFY(protocol.read());
	QCOMPARE(protocol.state(), VncClientProtocol::SecurityInit);

	// The count and first entry must not be consumed until the full list arrives.
	QVERIFY(protocol.read() == false);
	QCOMPARE(protocol.state(), VncClientProtocol::SecurityInit);

	const char secondFragment = rfbSecTypeNone;
	device.appendInput(QByteArray(&secondFragment, 1));
	QVERIFY(protocol.read());
	QCOMPARE(protocol.state(), VncClientProtocol::SecurityChallenge);
}


void VncProtocolRobustnessTest::rejectsOversizedCutText()
{
	DuplexDevice device;
	VncClientProtocol protocol(&device, {});
	completeHandshake(device, protocol);

	rfbServerCutTextMsg message{};
	message.type = rfbServerCutText;
	message.length = qToBigEndian<uint32_t>(std::numeric_limits<uint32_t>::max());
	device.appendInput(QByteArray(reinterpret_cast<const char*>(&message), sz_rfbServerCutTextMsg));

	QVERIFY(protocol.receiveMessage() == false);
	QVERIFY(device.isOpen() == false);
}


void VncProtocolRobustnessTest::rejectsOversizedVariantMessageEarly()
{
	DuplexDevice device;
	const auto size = qToBigEndian<VariantArrayMessage::MessageSize>(64 * 1024 * 1024);
	device.appendInput(QByteArray(reinterpret_cast<const char*>(&size), sizeof(size)));

	VariantArrayMessage message(&device);
	QVERIFY(message.isReadyForReceive() == false);
	QVERIFY(device.isOpen() == false);
}


QTEST_GUILESS_MAIN(VncProtocolRobustnessTest)

#include "VncProtocolRobustnessTest.moc"
