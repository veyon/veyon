// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QRect>

#include "CryptoCore.h"
#include "rfb/rfbproto.h"

class QBuffer;
class QIODevice;

class VEYON_CORE_EXPORT VncClientProtocol
{
public:
	using Password = CryptoCore::SecureArray;

	enum State {
		Disconnected,
		Protocol,
		SecurityInit,
		SecurityChallenge,
		SecurityResult,
		FramebufferInit,
		Running,
		StateCount
	};

	VncClientProtocol(QIODevice* socket, const Password& vncPassword);

	State state() const
	{
		return m_state;
	}

	void reset();
	void start();

	// refuse servers which don't offer VNC authentication (no fallback to rfbSecTypeNone)
	void setVncAuthenticationRequired(bool required)
	{
		m_vncAuthenticationRequired = required;
	}
	bool read(); // Flawfinder: ignore

	const QByteArray& serverInitMessage() const
	{
		return m_serverInitMessage;
	}

	int framebufferWidth() const
	{
		return m_framebufferWidth;
	}

	int framebufferHeight() const
	{
		return m_framebufferHeight;
	}

	void setPixelFormat(rfbPixelFormat pixelFormat);
	void setEncodings(const QVector<uint32_t>& encodings);

	bool sendPixelFormat();
	bool sendEncodings();

	void requestFramebufferUpdate(bool incremental);

	bool receiveMessage();

	const QByteArray& lastMessage() const
	{
		return m_lastMessage;
	}

	uint8_t lastMessageType() const
	{
		return static_cast<uint8_t>(m_lastMessage.constData()[0]);
	}

protected:
	void setState(State state)
	{
		m_state = state;
	}

private:
	static constexpr auto MaxMessageSize = 64 * 1024 * 1024;
	static constexpr auto MaximumFramebufferDimension = 16384;
	static constexpr auto MaximumRectanglesPerUpdate = 16384;

	bool readProtocol();
	bool receiveSecurityTypes();
	bool receiveSecurityChallenge();
	bool receiveSecurityResult();
	bool receiveServerInitMessage();

	bool receiveFramebufferUpdateMessage();
	bool receiveColourMapEntriesMessage();
	bool receiveBellMessage();
	bool receiveCutTextMessage();
	bool receiveResizeFramebufferMessage();
	bool receiveXvpMessage();

	bool readMessage(int size);

	bool handleRect(QBuffer& buffer, rfbFramebufferUpdateRectHeader rectHeader);
	bool handleRectEncodingRRE(QBuffer& buffer, uint bytesPerPixel);
	bool handleRectEncodingCoRRE(QBuffer& buffer, uint bytesPerPixel);
	bool handleRectEncodingHextile(QBuffer& buffer,
								   const rfbFramebufferUpdateRectHeader rectHeader,
								   uint bytesPerPixel);
	bool handleRectEncodingZlib(QBuffer& buffer);
	bool handleRectEncodingZRLE(QBuffer& buffer);
	bool handleRectEncodingTight(QBuffer& buffer,
								 const rfbFramebufferUpdateRectHeader rectHeader);
	bool handleRectEncodingExtDesktopSize(QBuffer& buffer);

	static bool isPseudoEncoding(rfbFramebufferUpdateRectHeader header);

	static constexpr auto MaximumMessageSize = 4096 * 4096 * 4;

	QIODevice* m_socket;
	State m_state;

	Password m_vncPassword;
	bool m_vncAuthenticationRequired = false;

	QByteArray m_serverInitMessage;

	rfbPixelFormat m_pixelFormat{};
	QVector<uint32_t> m_encodings{};

	quint16 m_framebufferWidth;
	quint16 m_framebufferHeight;

	QByteArray m_lastMessage;
};
