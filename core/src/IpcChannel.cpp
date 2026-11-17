// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtEndian>

#include "Aead.h"
#include "Hash.h"
#include "Hkdf.h"
#include "Hmac.h"
#include "IpcChannel.h"
#include "Random.h"

#include <chrono>
#include <cstring>
#include <limits>

using VeyonCrypto::ByteView;
using VeyonCrypto::SecureBuffer;


void IpcChannel::NonceHistory::add(const QByteArray& nonce)
{
	m_nonces.append(nonce);
	while (m_nonces.size() > IpcProtocol::NonceHistorySize)
	{
		m_nonces.removeFirst();
	}
}



IpcChannel::IpcChannel(Side side, IpcProtocol::Role role, quint32 sessionId, const QString& endpointName,
					   SecureBuffer&& token, NonceHistory* nonceHistory, const Clock& clock) :
	m_side(side),
	m_role(role),
	m_sessionId(sessionId),
	m_endpointName(endpointName.toUtf8()),
	m_token(std::move(token)),
	m_nonceHistory(nonceHistory),
	m_clock(clock),
	m_state(side == Side::Server ? State::AwaitHello : State::AwaitServerProof)
{
	// the name is encoded with a one-byte length in the transcript
	if (m_token.size() != IpcProtocol::TokenSize ||
		m_endpointName.isEmpty() ||
		m_endpointName.size() > IpcProtocol::MaximumEndpointNameSize)
	{
		m_token.clear();
		m_state = State::Failed;
		m_failureReason = "invalid channel parameters";
	}
}



IpcChannel::~IpcChannel() = default;



qint64 IpcChannel::steadyNow()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(
			   std::chrono::steady_clock::now().time_since_epoch())
		.count();
}



IpcChannel::Outputs IpcChannel::start()
{
	Outputs outputs;

	if (m_state == State::Failed || m_handshakeStartTime >= 0)
	{
		return outputs;
	}

	m_handshakeStartTime = m_clock();

	if (m_side == Side::Server)
	{
		return outputs;
	}

	const auto clientNonce = VeyonCrypto::Random::bytes(IpcProtocol::NonceSize);
	if (clientNonce.has_value() == false)
	{
		fail(outputs, "no random data for nonce");
		return outputs;
	}

	QByteArray hello;
	hello.reserve(IpcProtocol::HelloSize);
	hello.append(IpcProtocol::Magic, IpcProtocol::MagicSize);
	hello.append(char(IpcProtocol::Version));
	hello.append(char(m_role));
	char sessionIdBytes[4];
	qToBigEndian(m_sessionId, sessionIdBytes);
	hello.append(sessionIdBytes, sizeof(sessionIdBytes));
	hello.append(clientNonce->constData(), clientNonce->size());

	const auto helloHash = VeyonCrypto::Hash::sha256(hello);
	if (helloHash.has_value() == false)
	{
		fail(outputs, "hashing failed");
		return outputs;
	}
	m_helloHash = *helloHash;

	outputs.append({Output::Type::Send, frame(hello)});

	return outputs;
}



IpcChannel::Outputs IpcChannel::feed(ByteView data)
{
	auto outputs = checkTimeout();
	if (m_state == State::Failed)
	{
		return outputs;
	}

	if (m_handshakeStartTime < 0)
	{
		fail(outputs, "data before start");
		return outputs;
	}

	qsizetype offset = 0;
	while (offset < data.size() && m_state != State::Failed)
	{
		if (m_header.isEmpty())
		{
			m_frameStartTime = m_clock();
		}

		if (m_header.size() < IpcProtocol::FrameHeaderSize)
		{
			const auto count = qMin(IpcProtocol::FrameHeaderSize - m_header.size(), data.size() - offset);
			m_header.append(data.data() + offset, count);
			offset += count;

			if (m_header.size() < IpcProtocol::FrameHeaderSize)
			{
				break;
			}

			// validate the announced length before allocating anything for the payload
			const auto announcedSize = qFromBigEndian<quint32>(m_header.constData());
			const auto minimumSize = m_state == State::Established ? IpcProtocol::RecordTagSize : 1;
			if (announcedSize < quint32(minimumSize) || announcedSize > quint32(maximumFrameSize()))
			{
				fail(outputs, "invalid frame length");
				break;
			}
			m_payloadSize = qsizetype(announcedSize);
		}

		const auto count = qMin(m_payloadSize - m_payload.size(), data.size() - offset);
		m_payload.append(data.data() + offset, count);
		offset += count;

		if (m_payload.size() == m_payloadSize)
		{
			processFrame(outputs);
			m_header.clear();
			m_payload.clear();
			m_payloadSize = -1;
			m_frameStartTime = -1;
		}
	}

	return outputs;
}



IpcChannel::Outputs IpcChannel::checkTimeout()
{
	Outputs outputs;

	if (m_state == State::Failed || m_handshakeStartTime < 0)
	{
		return outputs;
	}

	const auto now = m_clock();

	if (m_state != State::Established && now - m_handshakeStartTime > IpcProtocol::HandshakeTimeout)
	{
		fail(outputs, "handshake timed out");
	}
	else if (m_state == State::Established && m_frameStartTime >= 0 &&
			 now - m_frameStartTime > IpcProtocol::FrameTimeout)
	{
		fail(outputs, "frame timed out");
	}

	return outputs;
}



std::optional<QByteArray> IpcChannel::encrypt(ByteView plaintext)
{
	if (m_state != State::Established ||
		plaintext.size() > IpcProtocol::maximumRecordPlaintextSize(m_role) ||
		m_sendSequenceNumber == std::numeric_limits<quint64>::max())
	{
		return {};
	}

	char header[IpcProtocol::FrameHeaderSize];
	qToBigEndian(quint32(plaintext.size() + IpcProtocol::RecordTagSize), header);

	const auto record = VeyonCrypto::Aead::seal(VeyonCrypto::Aead::Algorithm::ChaCha20Poly1305,
												m_sendKey, recordNonce(m_sendSequenceNumber), plaintext,
												ByteView(header, sizeof(header)));
	if (record.has_value() == false)
	{
		return {};
	}

	++m_sendSequenceNumber;

	QByteArray frameData;
	frameData.reserve(IpcProtocol::FrameHeaderSize + record->size());
	frameData.append(header, sizeof(header));
	frameData.append(*record);
	return frameData;
}



void IpcChannel::fail(Outputs& outputs, const char* reason)
{
	m_state = State::Failed;
	m_failureReason = reason;

	m_token.clear();
	m_sendKey.clear();
	m_receiveKey.clear();
	m_header.clear();
	m_payload.clear();
	m_payloadSize = -1;

	outputs.append({Output::Type::Failed});
}



void IpcChannel::processFrame(Outputs& outputs)
{
	switch (m_state)
	{
	case State::AwaitHello: processHello(outputs); break;
	case State::AwaitProof: processClientProof(outputs); break;
	case State::AwaitServerProof: processServerChallenge(outputs); break;
	case State::AwaitConfirmation: processConfirmation(outputs); break;
	case State::Established: processRecord(outputs); break;
	case State::Failed: break;
	}
}



void IpcChannel::processHello(Outputs& outputs)
{
	const auto& hello = m_payload;
	if (hello.size() != IpcProtocol::HelloSize ||
		std::memcmp(hello.constData(), IpcProtocol::Magic, IpcProtocol::MagicSize) != 0)
	{
		fail(outputs, "malformed hello");
		return;
	}

	qsizetype offset = IpcProtocol::MagicSize;
	if (quint8(hello.at(offset++)) != IpcProtocol::Version)
	{
		fail(outputs, "unsupported protocol version");
		return;
	}

	if (quint8(hello.at(offset++)) != quint8(m_role))
	{
		fail(outputs, "unexpected role");
		return;
	}

	if (qFromBigEndian<quint32>(hello.constData() + offset) != m_sessionId)
	{
		fail(outputs, "unexpected session ID");
		return;
	}
	offset += 4;

	const auto clientNonce = hello.mid(offset, IpcProtocol::NonceSize);
	if (m_nonceHistory)
	{
		if (m_nonceHistory->contains(clientNonce))
		{
			fail(outputs, "client nonce reused");
			return;
		}
		m_nonceHistory->add(clientNonce);
	}

	const auto helloHash = VeyonCrypto::Hash::sha256(hello);
	const auto serverNonce = VeyonCrypto::Random::bytes(IpcProtocol::NonceSize);
	if (helloHash.has_value() == false || serverNonce.has_value() == false)
	{
		fail(outputs, "crypto failure");
		return;
	}
	m_helloHash = *helloHash;

	const auto serverNonceData = serverNonce->toUnprotectedByteArray();
	m_transcript = transcript(serverNonceData);

	const auto serverProof = proof(IpcProtocol::ServerProofLabel, m_transcript);
	if (serverProof.has_value() == false)
	{
		fail(outputs, "crypto failure");
		return;
	}

	m_state = State::AwaitProof;
	const QByteArray serverChallenge = serverNonceData + *serverProof;
	outputs.append({Output::Type::Send, frame(serverChallenge)});
}



void IpcChannel::processClientProof(Outputs& outputs)
{
	const QByteArray message = QByteArray(IpcProtocol::ClientProofLabel) + m_transcript;
	if (m_payload.size() != IpcProtocol::ProofSize ||
		VeyonCrypto::Hmac::verifySha256(m_token, message, m_payload) == false)
	{
		fail(outputs, "client proof invalid");
		return;
	}

	if (deriveKeys(m_transcript) == false)
	{
		fail(outputs, "key derivation failed");
		return;
	}

	m_state = State::Established;
	outputs.append({Output::Type::Send, frame(QByteArray(1, char(IpcProtocol::HandshakeConfirmation)))});
	outputs.append({Output::Type::Established});
}



void IpcChannel::processServerChallenge(Outputs& outputs)
{
	if (m_payload.size() != IpcProtocol::ServerChallengeSize)
	{
		fail(outputs, "malformed server challenge");
		return;
	}

	const auto serverNonce = m_payload.left(IpcProtocol::NonceSize);
	const auto serverProof = m_payload.mid(IpcProtocol::NonceSize);

	m_transcript = transcript(serverNonce);

	// verify the server before proving anything ourselves: a process squatting the endpoint must
	// not be able to harvest a client proof
	const QByteArray message = QByteArray(IpcProtocol::ServerProofLabel) + m_transcript;
	if (VeyonCrypto::Hmac::verifySha256(m_token, message, serverProof) == false)
	{
		fail(outputs, "server proof invalid");
		return;
	}

	const auto clientProof = proof(IpcProtocol::ClientProofLabel, m_transcript);
	if (clientProof.has_value() == false)
	{
		fail(outputs, "crypto failure");
		return;
	}

	m_state = State::AwaitConfirmation;
	outputs.append({Output::Type::Send, frame(*clientProof)});
}



void IpcChannel::processConfirmation(Outputs& outputs)
{
	if (m_payload.size() != IpcProtocol::HandshakeConfirmationSize ||
		quint8(m_payload.at(0)) != IpcProtocol::HandshakeConfirmation)
	{
		fail(outputs, "handshake not confirmed");
		return;
	}

	if (deriveKeys(m_transcript) == false)
	{
		fail(outputs, "key derivation failed");
		return;
	}

	m_state = State::Established;
	outputs.append({Output::Type::Established});
}



void IpcChannel::processRecord(Outputs& outputs)
{
	if (m_receiveSequenceNumber == std::numeric_limits<quint64>::max())
	{
		fail(outputs, "sequence number exhausted");
		return;
	}

	// the nonce is derived from the expected sequence number, so reordered, duplicated and
	// dropped records fail authentication just like modified ones
	const auto plaintext = VeyonCrypto::Aead::open(VeyonCrypto::Aead::Algorithm::ChaCha20Poly1305,
												   m_receiveKey, recordNonce(m_receiveSequenceNumber),
												   m_payload, m_header);
	if (plaintext.has_value() == false)
	{
		fail(outputs, "record authentication failed");
		return;
	}

	++m_receiveSequenceNumber;

	outputs.append({Output::Type::Message, plaintext->toUnprotectedByteArray()});
}



qsizetype IpcChannel::maximumFrameSize() const
{
	return m_state == State::Established ? IpcProtocol::maximumFrameSize(m_role) : IpcProtocol::MaximumPreAuthFrameSize;
}



QByteArray IpcChannel::transcript(const QByteArray& serverNonce) const
{
	// SHA-256(message 1) || nonceS || u8(len(name)) || name
	QByteArray data;
	data.reserve(m_helloHash.size() + serverNonce.size() + 1 + m_endpointName.size());
	data.append(m_helloHash);
	data.append(serverNonce);
	data.append(char(quint8(m_endpointName.size())));
	data.append(m_endpointName);
	return data;
}



std::optional<QByteArray> IpcChannel::proof(const char* label, const QByteArray& transcript) const
{
	const QByteArray message = QByteArray(label) + transcript;
	return VeyonCrypto::Hmac::sha256(m_token, message);
}



bool IpcChannel::deriveKeys(const QByteArray& transcript)
{
	const QByteArray clientToServerInfo = QByteArray(IpcProtocol::KeyLabel) + IpcProtocol::ClientToServerKeyInfo;
	const QByteArray serverToClientInfo = QByteArray(IpcProtocol::KeyLabel) + IpcProtocol::ServerToClientKeyInfo;
	auto clientToServerKey = VeyonCrypto::Hkdf::sha256(m_token, transcript, clientToServerInfo, IpcProtocol::KeySize);
	auto serverToClientKey = VeyonCrypto::Hkdf::sha256(m_token, transcript, serverToClientInfo, IpcProtocol::KeySize);

	// the token is not needed anymore: the connection only uses the derived keys
	m_token.clear();

	if (clientToServerKey.has_value() == false || serverToClientKey.has_value() == false)
	{
		return false;
	}

	if (m_side == Side::Server)
	{
		m_receiveKey = std::move(*clientToServerKey);
		m_sendKey = std::move(*serverToClientKey);
	}
	else
	{
		m_sendKey = std::move(*clientToServerKey);
		m_receiveKey = std::move(*serverToClientKey);
	}

	return true;
}



QByteArray IpcChannel::frame(const QByteArray& payload)
{
	char header[IpcProtocol::FrameHeaderSize];
	qToBigEndian(quint32(payload.size()), header);

	QByteArray frameData;
	frameData.reserve(IpcProtocol::FrameHeaderSize + payload.size());
	frameData.append(header, sizeof(header));
	frameData.append(payload);
	return frameData;
}



QByteArray IpcChannel::recordNonce(quint64 sequenceNumber)
{
	// 0x00000000 || u64be(seq)
	QByteArray nonce(IpcProtocol::RecordNonceSize, 0);
	qToBigEndian(sequenceNumber, nonce.data() + 4);
	return nonce;
}
