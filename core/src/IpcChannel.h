// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QByteArray>
#include <QList>

#include "IpcProtocol.h"
#include "SecureBuffer.h"
#include "VeyonCore.h"

#include <functional>
#include <optional>

// Transport-independent local IPC channel (docs/specs/02-ipc.md §3–§5): framing, mutual
// HMAC handshake bound to the per-start token, the role, the session ID and the endpoint name,
// followed by ChaCha20-Poly1305 records with per-direction keys and sequence numbers.
//
// The caller feeds received bytes and writes the returned Send outputs to the transport. The
// channel fails closed: any framing, handshake or record error, and an exceeded deadline, moves
// it to State::Failed permanently, after which the caller closes the transport.
class VEYON_CORE_EXPORT IpcChannel
{
public:
	enum class Side {
		Server,
		Client,
	};

	enum class State {
		AwaitHello,		   // server: waiting for message 1
		AwaitProof,		   // server: waiting for message 3
		AwaitServerProof,  // client: waiting for message 2
		AwaitConfirmation, // client: waiting for message 4
		Established,
		Failed,
	};

	struct Output
	{
		enum class Type {
			Send,		 // write data to the transport
			Message,	 // decrypted application message
			Established, // handshake completed
			Failed,		 // channel failed, close the transport
		};
		Type type;
		QByteArray data{};
	};
	using Outputs = QList<Output>;

	using Clock = std::function<qint64()>;

	// last nonceC values accepted by one endpoint (replay protection across connections)
	class NonceHistory
	{
	public:
		bool contains(const QByteArray& nonce) const
		{
			return m_nonces.contains(nonce);
		}

		void add(const QByteArray& nonce);

	private:
		QList<QByteArray> m_nonces;
	};

	// the token is consumed and wiped as soon as the handshake has completed or failed;
	// nonceHistory (server side, optional for tests) must outlive the channel
	IpcChannel(Side side, IpcProtocol::Role role, quint32 sessionId, const QString& endpointName,
			   VeyonCrypto::SecureBuffer&& token,
			   NonceHistory* nonceHistory = nullptr,
			   const Clock& clock = steadyNow);
	~IpcChannel();

	static qint64 steadyNow();

	// starts the handshake deadline; on the client side additionally returns message 1
	[[nodiscard]] Outputs start();

	[[nodiscard]] Outputs feed(VeyonCrypto::ByteView data);

	// to be called periodically (and by feed()): fails the channel when the handshake deadline
	// (§3: 3 s total) or the deadline of a partially received post-auth frame (10 s) is exceeded
	[[nodiscard]] Outputs checkTimeout();

	// returns the complete frame for the encrypted record, or nothing if the channel is not
	// established or the message exceeds maximumRecordPlaintextSize() of the role
	[[nodiscard]] std::optional<QByteArray> encrypt(VeyonCrypto::ByteView plaintext);

	State state() const
	{
		return m_state;
	}

	bool isEstablished() const
	{
		return m_state == State::Established;
	}

	bool hasFailed() const
	{
		return m_state == State::Failed;
	}

	// static description of the failure for log lines, never contains peer data or secrets
	const char* failureReason() const
	{
		return m_failureReason;
	}

	// receive buffer capacity (tests: no allocation proportional to an announced frame length)
	qsizetype receiveBufferCapacity() const
	{
		return m_header.capacity() + m_payload.capacity();
	}

private:
	void fail(Outputs& outputs, const char* reason);
	void processFrame(Outputs& outputs);
	void processHello(Outputs& outputs);
	void processClientProof(Outputs& outputs);
	void processServerChallenge(Outputs& outputs);
	void processConfirmation(Outputs& outputs);
	void processRecord(Outputs& outputs);

	qsizetype maximumFrameSize() const;
	QByteArray transcript(const QByteArray& serverNonce) const;
	std::optional<QByteArray> proof(const char* label, const QByteArray& transcript) const;
	bool deriveKeys(const QByteArray& transcript);
	static QByteArray frame(const QByteArray& payload);
	static QByteArray recordNonce(quint64 sequenceNumber);

	const Side m_side;
	const IpcProtocol::Role m_role;
	const quint32 m_sessionId;
	const QByteArray m_endpointName;
	VeyonCrypto::SecureBuffer m_token;
	NonceHistory* const m_nonceHistory;
	const Clock m_clock;

	State m_state;
	const char* m_failureReason = nullptr;

	qint64 m_handshakeStartTime = -1;
	qint64 m_frameStartTime = -1;

	QByteArray m_header;
	QByteArray m_payload;
	qsizetype m_payloadSize = -1;

	QByteArray m_helloHash;
	QByteArray m_transcript;

	VeyonCrypto::SecureBuffer m_sendKey;
	VeyonCrypto::SecureBuffer m_receiveKey;
	quint64 m_sendSequenceNumber = 0;
	quint64 m_receiveSequenceNumber = 0;
};
