// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtEndian>
#include <QtTest>

#include "ConstantTime.h"
#include "IpcChannel.h"
#include "IpcEndpointCredentials.h"
#include "IpcMessage.h"
#include "Random.h"
#include "StandardInputReader.h"

#include <cstring>
#include <memory>

#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#endif

using VeyonCrypto::SecureBuffer;
using Role = IpcProtocol::Role;
using Type = IpcChannel::Output::Type;

// docs/specs/02-ipc.md §8 cases 1–8 and 10
class IpcChannelTest : public QObject
{
	Q_OBJECT
private:
	static constexpr quint32 SessionId = 3;

	static QString endpointName()
	{
		return QStringLiteral("veyon-ipc-3-0123abcd");
	}

	static SecureBuffer tokenA()
	{
		return SecureBuffer(QByteArray(IpcProtocol::TokenSize, 'A').constData(), IpcProtocol::TokenSize);
	}

	static SecureBuffer tokenB()
	{
		return SecureBuffer(QByteArray(IpcProtocol::TokenSize, 'B').constData(), IpcProtocol::TokenSize);
	}

	struct Pair
	{
		qint64 now = 1000;
		IpcChannel::NonceHistory history;
		std::unique_ptr<IpcChannel> server;
		std::unique_ptr<IpcChannel> client;

		Pair(SecureBuffer&& serverToken, SecureBuffer&& clientToken,
			 Role serverRole = Role::SessionServer, Role clientRole = Role::SessionServer,
			 quint32 serverSessionId = SessionId, quint32 clientSessionId = SessionId,
			 const QString& serverName = endpointName(), const QString& clientName = endpointName())
		{
			const auto clock = [this]() { return now; };
			server = std::make_unique<IpcChannel>(IpcChannel::Side::Server, serverRole, serverSessionId, serverName,
												  std::move(serverToken), &history, clock);
			client = std::make_unique<IpcChannel>(IpcChannel::Side::Client, clientRole, clientSessionId, clientName,
												  std::move(clientToken), nullptr, clock);
		}
	};

	static QByteArray sent(const IpcChannel::Outputs& outputs)
	{
		QByteArray data;
		for (const auto& output : outputs)
		{
			if (output.type == Type::Send)
			{
				data.append(output.data);
			}
		}
		return data;
	}

	static bool contains(const IpcChannel::Outputs& outputs, Type type)
	{
		for (const auto& output : outputs)
		{
			if (output.type == type)
			{
				return true;
			}
		}
		return false;
	}

	static QList<QByteArray> messages(const IpcChannel::Outputs& outputs)
	{
		QList<QByteArray> result;
		for (const auto& output : outputs)
		{
			if (output.type == Type::Message)
			{
				result.append(output.data);
			}
		}
		return result;
	}

	// runs the handshake and returns message 1 as sent by the client
	static QByteArray handshake(Pair& pair)
	{
		(void)pair.server->start();
		const auto hello = sent(pair.client->start());
		const auto challenge = sent(pair.server->feed(hello));
		const auto clientProof = sent(pair.client->feed(challenge));
		const auto confirmation = sent(pair.server->feed(clientProof));
		(void)pair.client->feed(confirmation);
		return hello;
	}

	static QByteArray framePayload(const QByteArray& frame)
	{
		return frame.mid(IpcProtocol::FrameHeaderSize);
	}

	static QByteArray frame(const QByteArray& payload)
	{
		QByteArray data(IpcProtocol::FrameHeaderSize, 0);
		qToBigEndian(quint32(payload.size()), data.data());
		return data + payload;
	}

	static int pipeFds(int fds[2])
	{
#ifdef Q_OS_WIN
		return _pipe(fds, 4096, _O_BINARY);
#else
		return pipe(fds);
#endif
	}

	static void writeFd(int fd, const QByteArray& data)
	{
#ifdef Q_OS_WIN
		QCOMPARE(_write(fd, data.constData(), unsigned(data.size())), int(data.size()));
#else
		QCOMPARE(write(fd, data.constData(), size_t(data.size())), ssize_t(data.size()));
#endif
	}

	static void closeFd(int fd)
	{
#ifdef Q_OS_WIN
		_close(fd);
#else
		close(fd);
#endif
	}

	using SecretLineResult = StandardInputReader::SecretLineResult;

	static SecretLineResult readLineResult(const QByteArray& input, SecureBuffer& out, int maxLength = 128)
	{
		int fds[2];
		if (pipeFds(fds) != 0)
		{
			return SecretLineResult::Invalid;
		}
		writeFd(fds[1], input);
		closeFd(fds[1]);
		const auto result = StandardInputReader::readSecretLine(fds[0], out, maxLength, 1000);
		closeFd(fds[0]);
		return result;
	}

	static bool readLine(const QByteArray& input, SecureBuffer& out, int maxLength = 128)
	{
		return readLineResult(input, out, maxLength) == SecretLineResult::Read;
	}

private Q_SLOTS:
	// 1. happy path both ways, keys differ per direction
	void happyPath()
	{
		Pair pair(tokenA(), tokenA());
		handshake(pair);
		QVERIFY(pair.server->isEstablished());
		QVERIFY(pair.client->isEstablished());

		const QByteArray request("{\"id\":1,\"cmd\":\"GetSessionData\"}");
		const auto clientRecord = pair.client->encrypt(request);
		QVERIFY(clientRecord.has_value());
		QCOMPARE(messages(pair.server->feed(*clientRecord)), QList<QByteArray>{request});

		const auto serverRecord = pair.server->encrypt(request);
		QVERIFY(serverRecord.has_value());
		QCOMPARE(messages(pair.client->feed(*serverRecord)), QList<QByteArray>{request});

		// same plaintext and sequence number 0 in both directions: different keys ⇒ different records
		QVERIFY(*clientRecord != *serverRecord);

		// several records in one chunk and a record split into single bytes
		const auto second = pair.client->encrypt(QByteArray("second"));
		const auto third = pair.client->encrypt(QByteArray("third"));
		const QByteArray combined = *second + *third;
		QCOMPARE(messages(pair.server->feed(combined)), (QList<QByteArray>{"second", "third"}));
		const auto fourth = pair.client->encrypt(QByteArray("fourth"));
		QList<QByteArray> received;
		for (const auto byte : *fourth)
		{
			received.append(messages(pair.server->feed(QByteArray(1, byte))));
		}
		QCOMPARE(received, QList<QByteArray>{"fourth"});

		// empty and maximum-size messages
		const auto empty = pair.client->encrypt(QByteArray());
		QCOMPARE(messages(pair.server->feed(*empty)), QList<QByteArray>{QByteArray()});
		const QByteArray maximum(IpcProtocol::MaximumRecordPlaintextSize, 'x');
		const auto maximumRecord = pair.client->encrypt(maximum);
		QVERIFY(maximumRecord.has_value());
		QCOMPARE(messages(pair.server->feed(*maximumRecord)), QList<QByteArray>{maximum});
		const QByteArray tooLarge = maximum + 'x';
		QVERIFY(pair.client->encrypt(tooLarge).has_value() == false);
	}

	void workerRole()
	{
		Pair pair(tokenA(), tokenA(), Role::Worker, Role::Worker, SessionId, SessionId,
				  QStringLiteral("veyon-worker-3-0123abcd"), QStringLiteral("veyon-worker-3-0123abcd"));
		handshake(pair);
		QVERIFY(pair.server->isEstablished());
		QVERIFY(pair.client->isEstablished());
	}

	// 2. wrong token: both sides fail, no proof that validates under the wrong token
	void wrongToken()
	{
		Pair pair(tokenA(), tokenB());
		(void)pair.server->start();
		const auto hello = sent(pair.client->start());
		const auto challenge = sent(pair.server->feed(hello));
		QCOMPARE(qsizetype(challenge.size()), IpcProtocol::FrameHeaderSize + IpcProtocol::ServerChallengeSize);

		const auto clientOutputs = pair.client->feed(challenge);
		QVERIFY(pair.client->hasFailed());
		QVERIFY(sent(clientOutputs).isEmpty());
		QVERIFY(contains(clientOutputs, Type::Failed));

		// a client proof computed with the wrong token is rejected by the server
		Pair other(tokenA(), tokenB());
		(void)other.server->start();
		(void)other.server->feed(sent(other.client->start()));
		const auto forgedProof = frame(VeyonCrypto::Random::bytes(IpcProtocol::ProofSize)->toUnprotectedByteArray());
		const auto serverOutputs = other.server->feed(forgedProof);
		QVERIFY(other.server->hasFailed());
		QVERIFY(sent(serverOutputs).isEmpty());
	}

	// 3. the client never sends its proof after an invalid server proof
	void serverProofCheckedFirst()
	{
		Pair pair(tokenA(), tokenA());
		(void)pair.server->start();
		const auto hello = sent(pair.client->start());
		auto challenge = sent(pair.server->feed(hello));
		challenge[challenge.size() - 1] = char(challenge.at(challenge.size() - 1) ^ 0x01);
		const auto outputs = pair.client->feed(challenge);
		QVERIFY(pair.client->hasFailed());
		QVERIFY(sent(outputs).isEmpty());

		// a squatting "server" answering with random data does not get a proof either
		Pair squatted(tokenB(), tokenA());
		(void)squatted.server->start();
		const auto squatterChallenge = sent(squatted.server->feed(sent(squatted.client->start())));
		QVERIFY(sent(squatted.client->feed(squatterChallenge)).isEmpty());
		QVERIFY(squatted.client->hasFailed());
	}

	// 4. replay of a recorded handshake fails
	void replay()
	{
		Pair pair(tokenA(), tokenA());
		(void)pair.server->start();
		const auto hello = sent(pair.client->start());
		const auto challenge = sent(pair.server->feed(hello));
		const auto clientProof = sent(pair.client->feed(challenge));
		(void)pair.server->feed(clientProof);
		QVERIFY(pair.server->isEstablished());

		// same endpoint (nonce history), new connection: reused nonceC
		IpcChannel replayServer(IpcChannel::Side::Server, Role::SessionServer, SessionId, endpointName(), tokenA(),
								&pair.history, [&pair]() { return pair.now; });
		(void)replayServer.start();
		(void)replayServer.feed(hello);
		QVERIFY(replayServer.hasFailed());

		// without nonce history the server picks a new nonceS, so the recorded proofC does not match
		IpcChannel freshServer(IpcChannel::Side::Server, Role::SessionServer, SessionId, endpointName(), tokenA(),
							   nullptr, [&pair]() { return pair.now; });
		(void)freshServer.start();
		const auto newChallenge = sent(freshServer.feed(hello));
		QVERIFY(newChallenge != challenge);
		const auto outputs = freshServer.feed(clientProof);
		QVERIFY(freshServer.hasFailed());
		QVERIFY(sent(outputs).isEmpty());
	}

	// 5. wrong session ID, version, role; tampered message 1 or different endpoint name
	void wrongParameters()
	{
		{
			Pair pair(tokenA(), tokenA(), Role::SessionServer, Role::SessionServer, SessionId, SessionId + 1);
			(void)pair.server->start();
			QVERIFY(sent(pair.server->feed(sent(pair.client->start()))).isEmpty());
			QVERIFY(pair.server->hasFailed());
		}
		{
			Pair pair(tokenA(), tokenA(), Role::SessionServer, Role::Worker);
			(void)pair.server->start();
			QVERIFY(sent(pair.server->feed(sent(pair.client->start()))).isEmpty());
			QVERIFY(pair.server->hasFailed());
		}
		{
			Pair pair(tokenA(), tokenA());
			(void)pair.server->start();
			auto hello = sent(pair.client->start());
			hello[int(IpcProtocol::FrameHeaderSize + IpcProtocol::MagicSize)] = 0x02;
			QVERIFY(sent(pair.server->feed(hello)).isEmpty());
			QVERIFY(pair.server->hasFailed());
		}
		{
			Pair pair(tokenA(), tokenA());
			(void)pair.server->start();
			auto hello = sent(pair.client->start());
			hello[int(IpcProtocol::FrameHeaderSize)] = 'X';
			QVERIFY(sent(pair.server->feed(hello)).isEmpty());
			QVERIFY(pair.server->hasFailed());
		}
		{
			// a man in the middle flips the role byte towards a server expecting the other role:
			// the transcript differs, so the client rejects the server proof
			Pair pair(tokenA(), tokenA(), Role::Worker, Role::SessionServer);
			(void)pair.server->start();
			auto hello = sent(pair.client->start());
			hello[int(IpcProtocol::FrameHeaderSize + IpcProtocol::MagicSize + 1)] = char(Role::Worker);
			const auto challenge = sent(pair.server->feed(hello));
			QVERIFY(challenge.isEmpty() == false);
			QVERIFY(sent(pair.client->feed(challenge)).isEmpty());
			QVERIFY(pair.client->hasFailed());
		}
		{
			// different endpoint name on both sides ⇒ proof fails
			Pair pair(tokenA(), tokenA(), Role::SessionServer, Role::SessionServer, SessionId, SessionId,
					  endpointName(), QStringLiteral("veyon-ipc-3-ffffffff"));
			(void)pair.server->start();
			const auto challenge = sent(pair.server->feed(sent(pair.client->start())));
			QVERIFY(sent(pair.client->feed(challenge)).isEmpty());
			QVERIFY(pair.client->hasFailed());
		}
		{
			// wrong message sizes
			Pair pair(tokenA(), tokenA());
			(void)pair.server->start();
			auto hello = sent(pair.client->start());
			QVERIFY(sent(pair.server->feed(frame(framePayload(hello) + 'x'))).isEmpty());
			QVERIFY(pair.server->hasFailed());
		}
	}

	// 6. oversized length pre-auth (257) and post-auth (65537): closed without allocation
	void oversizedFrames()
	{
		{
			Pair pair(tokenA(), tokenA());
			(void)pair.server->start();
			QByteArray header(IpcProtocol::FrameHeaderSize, 0);
			qToBigEndian(quint32(IpcProtocol::MaximumPreAuthFrameSize + 1), header.data());
			const QByteArray data = header + QByteArray(64, 'x');
			const auto outputs = pair.server->feed(data);
			QVERIFY(contains(outputs, Type::Failed));
			QVERIFY(pair.server->hasFailed());
			QVERIFY(pair.server->receiveBufferCapacity() < IpcProtocol::MaximumPreAuthFrameSize);
		}
		{
			Pair pair(tokenA(), tokenA());
			(void)pair.server->start();
			QByteArray header(IpcProtocol::FrameHeaderSize, 0);
			qToBigEndian(quint32(0xffffffff), header.data());
			(void)pair.server->feed(header);
			QVERIFY(pair.server->hasFailed());
			QVERIFY(pair.server->receiveBufferCapacity() < IpcProtocol::MaximumPreAuthFrameSize);
		}
		{
			// a valid pre-auth frame of 256 bytes is accepted by the framing (and rejected as hello)
			Pair pair(tokenA(), tokenA());
			(void)pair.server->start();
			(void)pair.server->feed(frame(QByteArray(IpcProtocol::MaximumPreAuthFrameSize, 'x')));
			QCOMPARE(QByteArray(pair.server->failureReason()), QByteArray("malformed hello"));
		}
		{
			Pair pair(tokenA(), tokenA());
			handshake(pair);
			QByteArray header(IpcProtocol::FrameHeaderSize, 0);
			qToBigEndian(quint32(IpcProtocol::MaximumFrameSize + 1), header.data());
			const QByteArray data = header + QByteArray(1024, 'x');
			(void)pair.server->feed(data);
			QVERIFY(pair.server->hasFailed());
			QVERIFY(pair.server->receiveBufferCapacity() < IpcProtocol::MaximumPreAuthFrameSize);
		}
		{
			// post-auth frames shorter than a tag
			Pair pair(tokenA(), tokenA());
			handshake(pair);
			(void)pair.server->feed(frame(QByteArray(IpcProtocol::RecordTagSize - 1, 'x')));
			QVERIFY(pair.server->hasFailed());
		}
		{
			// zero length pre-auth
			Pair pair(tokenA(), tokenA());
			(void)pair.server->start();
			(void)pair.server->feed(QByteArray(IpcProtocol::FrameHeaderSize, 0));
			QVERIFY(pair.server->hasFailed());
		}
	}

	// Q1: the worker channel (role 2) allows frames of up to 1 MiB, the service channel 64 KiB
	void workerFrameLimit()
	{
		const auto workerName = QStringLiteral("veyon-worker-3-0123abcd");
		{
			Pair pair(tokenA(), tokenA(), Role::Worker, Role::Worker, SessionId, SessionId, workerName, workerName);
			handshake(pair);
			const QByteArray maximum(int(IpcProtocol::maximumRecordPlaintextSize(Role::Worker)), 'x');
			const auto record = pair.client->encrypt(maximum);
			QVERIFY(record.has_value());
			QCOMPARE(qsizetype(record->size()), IpcProtocol::FrameHeaderSize + IpcProtocol::MaximumWorkerFrameSize);
			QCOMPARE(messages(pair.server->feed(*record)), QList<QByteArray>{maximum});
			const QByteArray tooLarge = maximum + 'x';
			QVERIFY(pair.client->encrypt(tooLarge).has_value() == false);

			// a 256 KiB feature message as used for file transfers
			const QByteArray chunk(256 * 1024, 'c');
			QCOMPARE(messages(pair.client->feed(*pair.server->encrypt(chunk))), QList<QByteArray>{chunk});
		}
		{
			// 1 MiB + 1 is rejected before any allocation
			Pair pair(tokenA(), tokenA(), Role::Worker, Role::Worker, SessionId, SessionId, workerName, workerName);
			handshake(pair);
			QByteArray header(IpcProtocol::FrameHeaderSize, 0);
			qToBigEndian(quint32(IpcProtocol::MaximumWorkerFrameSize + 1), header.data());
			const QByteArray data = header + QByteArray(1024, 'x');
			(void)pair.server->feed(data);
			QVERIFY(pair.server->hasFailed());
			QVERIFY(pair.server->receiveBufferCapacity() < IpcProtocol::MaximumPreAuthFrameSize);
		}
		{
			// the larger limit does not apply to the service channel nor before authentication
			Pair pair(tokenA(), tokenA());
			handshake(pair);
			QVERIFY(pair.client->encrypt(QByteArray(int(IpcProtocol::MaximumRecordPlaintextSize) + 1, 'x')).has_value() == false);
			QByteArray header(IpcProtocol::FrameHeaderSize, 0);
			qToBigEndian(quint32(IpcProtocol::MaximumFrameSize + 1), header.data());
			(void)pair.server->feed(header);
			QVERIFY(pair.server->hasFailed());

			Pair workerPair(tokenA(), tokenA(), Role::Worker, Role::Worker, SessionId, SessionId, workerName, workerName);
			(void)workerPair.server->start();
			qToBigEndian(quint32(IpcProtocol::MaximumPreAuthFrameSize + 1), header.data());
			(void)workerPair.server->feed(header);
			QVERIFY(workerPair.server->hasFailed());
		}
	}

	// 7. record tampering, reordering, duplicates, dropped records
	void recordTampering_data()
	{
		QTest::addColumn<int>("tamperedByte");
		QTest::newRow("length") << 3;
		QTest::newRow("ciphertext") << int(IpcProtocol::FrameHeaderSize);
		QTest::newRow("tag") << -1;
	}

	void recordTampering()
	{
		QFETCH(int, tamperedByte);

		Pair pair(tokenA(), tokenA());
		handshake(pair);
		auto record = *pair.client->encrypt(QByteArray("payload"));
		const auto index = tamperedByte >= 0 ? tamperedByte : int(record.size()) - 1;
		record[index] = char(record.at(index) ^ 0x01);
		const auto outputs = pair.server->feed(record);
		QVERIFY(messages(outputs).isEmpty());
		// a modified length either fails immediately or once the deadline for the frame expires
		pair.now += IpcProtocol::FrameTimeout + 1;
		(void)pair.server->checkTimeout();
		QVERIFY(pair.server->hasFailed());
	}

	void recordOrdering()
	{
		{
			Pair pair(tokenA(), tokenA());
			handshake(pair);
			const auto first = *pair.client->encrypt(QByteArray("1"));
			const auto second = *pair.client->encrypt(QByteArray("2"));
			QVERIFY(messages(pair.server->feed(second)).isEmpty());
			QVERIFY(pair.server->hasFailed());
			Q_UNUSED(first)
		}
		{
			Pair pair(tokenA(), tokenA());
			handshake(pair);
			const auto first = *pair.client->encrypt(QByteArray("1"));
			QCOMPARE(int(messages(pair.server->feed(first)).size()), 1);
			QVERIFY(messages(pair.server->feed(first)).isEmpty());
			QVERIFY(pair.server->hasFailed());
		}
		{
			// dropped record (sequence gap)
			Pair pair(tokenA(), tokenA());
			handshake(pair);
			(void)pair.client->encrypt(QByteArray("1"));
			const auto second = *pair.client->encrypt(QByteArray("2"));
			QVERIFY(messages(pair.server->feed(second)).isEmpty());
			QVERIFY(pair.server->hasFailed());
		}
		{
			// a record from the other direction (reflection) is rejected
			Pair pair(tokenA(), tokenA());
			handshake(pair);
			const auto serverRecord = *pair.server->encrypt(QByteArray("1"));
			QVERIFY(messages(pair.server->feed(serverRecord)).isEmpty());
			QVERIFY(pair.server->hasFailed());
		}
		{
			// nothing can be sent before the handshake completed
			Pair pair(tokenA(), tokenA());
			(void)pair.server->start();
			(void)pair.client->start();
			QVERIFY(pair.client->encrypt(QByteArray("x")).has_value() == false);
			QVERIFY(pair.server->encrypt(QByteArray("x")).has_value() == false);
		}
	}

	// 8. handshake timeout with fake clock
	void handshakeTimeout()
	{
		{
			Pair pair(tokenA(), tokenA());
			(void)pair.server->start();
			const auto hello = sent(pair.client->start());
			pair.now += IpcProtocol::HandshakeTimeout;
			const auto challenge = sent(pair.server->feed(hello));
			QVERIFY(challenge.isEmpty() == false);
			pair.now += 1;
			QVERIFY(contains(pair.server->checkTimeout(), Type::Failed));
			QVERIFY(sent(pair.client->feed(challenge)).isEmpty());
			QVERIFY(pair.client->hasFailed());
		}
		{
			// slowloris: one byte at a time, the deadline is for the whole handshake
			Pair pair(tokenA(), tokenA());
			(void)pair.server->start();
			const auto hello = sent(pair.client->start());
			for (int i = 0; i < hello.size() - 1; ++i)
			{
				(void)pair.server->feed(hello.mid(i, 1));
				pair.now += 100;
			}
			QVERIFY(pair.server->hasFailed());
		}
		{
			// established channels have no idle limit, but a started frame must complete in 10 s
			Pair pair(tokenA(), tokenA());
			handshake(pair);
			pair.now += 24 * 3600 * 1000;
			QVERIFY(pair.server->checkTimeout().isEmpty());
			const auto record = *pair.client->encrypt(QByteArray("late"));
			QCOMPARE(int(messages(pair.server->feed(record)).size()), 1);
			const auto partial = *pair.client->encrypt(QByteArray("partial"));
			(void)pair.server->feed(partial.left(6));
			pair.now += IpcProtocol::FrameTimeout;
			QVERIFY(pair.server->checkTimeout().isEmpty());
			pair.now += 1;
			QVERIFY(contains(pair.server->checkTimeout(), Type::Failed));
		}
	}

	void invalidParameters()
	{
		IpcChannel shortToken(IpcChannel::Side::Server, Role::SessionServer, SessionId, endpointName(),
							  SecureBuffer(16));
		QVERIFY(shortToken.hasFailed());

		IpcChannel longName(IpcChannel::Side::Client, Role::SessionServer, SessionId,
							QString(IpcProtocol::MaximumEndpointNameSize + 1, QLatin1Char('a')), tokenA());
		QVERIFY(longName.hasFailed());
		QVERIFY(longName.start().isEmpty());

		// data before start() and data after failure
		IpcChannel server(IpcChannel::Side::Server, Role::SessionServer, SessionId, endpointName(), tokenA());
		QVERIFY(contains(server.feed(QByteArray("x")), Type::Failed));
		QVERIFY(server.feed(QByteArray("x")).isEmpty());
	}

	// §6 application messages
	void jsonMessages()
	{
		const auto request = IpcMessage::decodeRequest(
			IpcMessage::encodeRequest({7, QStringLiteral("SetSessionData"), QJsonObject{{QStringLiteral("data"), QStringLiteral("AA==")}}}));
		QVERIFY(request.has_value());
		QCOMPARE(request->id, 7);
		QCOMPARE(request->command, QStringLiteral("SetSessionData"));
		QCOMPARE(request->arguments.value(QStringLiteral("data")).toString(), QStringLiteral("AA=="));

		const auto response = IpcMessage::decodeResponse(IpcMessage::encodeResponse(IpcMessage::failure(7, IpcProtocol::Error::DeniedError)));
		QVERIFY(response.has_value());
		QCOMPARE(response->ok, false);
		QCOMPARE(response->error, QStringLiteral("denied"));

		QVERIFY(IpcMessage::decodeRequest(QByteArray("{\"id\":1,\"cmd\":\"X\"}")).has_value());
		QVERIFY(IpcMessage::decodeRequest(QByteArray("{\"id\":1,\"cmd\":\"X\",\"args\":{\"a\":[[[[[[1]]]]]]}}")).has_value());

		const QList<QByteArray> invalidRequests{
			"",
			"[]",
			"{\"id\":1}",
			"{\"cmd\":\"X\"}",
			"{\"id\":\"1\",\"cmd\":\"X\"}",
			"{\"id\":1.5,\"cmd\":\"X\"}",
			"{\"id\":-1,\"cmd\":\"X\"}",
			"{\"id\":1e300,\"cmd\":\"X\"}",
			"{\"id\":1,\"cmd\":1}",
			"{\"id\":1,\"cmd\":\"X\",\"args\":[]}",
			// depth 9
			"{\"id\":1,\"cmd\":\"X\",\"args\":{\"a\":[[[[[[[1]]]]]]]}}",
			"{\"id\":1,\"cmd\":\"X\"",
		};
		for (const auto& invalidRequest : invalidRequests)
		{
			QVERIFY2(IpcMessage::decodeRequest(invalidRequest).has_value() == false, invalidRequest.constData());
		}

		QVERIFY(IpcMessage::decodeResponse(QByteArray("{\"id\":1}")).has_value() == false);
		QVERIFY(IpcMessage::decodeResponse(QByteArray("{\"id\":1,\"ok\":true,\"result\":1}")).has_value() == false);
		QVERIFY(IpcMessage::decodeRequest(QByteArray(IpcProtocol::MaximumRecordPlaintextSize + 1, ' ')).has_value() == false);
	}

	// 10. readSecretLine and the credentials line
	void readSecretLine()
	{
		SecureBuffer line;
		QVERIFY(readLine(QByteArray("abc\nrest"), line));
		QCOMPARE(line.toUnprotectedByteArray(), QByteArray("abc"));

		QVERIFY(readLine(QByteArray(128, 'x') + '\n', line));
		QCOMPARE(line.size(), qsizetype(128));

		// oversize line, missing newline, NUL byte: nothing is returned
		QVERIFY(readLine(QByteArray(129, 'x') + '\n', line) == false);
		QVERIFY(line.isEmpty());
		QVERIFY(readLine(QByteArray("abc"), line) == false);
		QVERIFY(line.isEmpty());
		QVERIFY(readLine(QByteArray("ab\0c\n", 5), line) == false);
		QVERIFY(line.isEmpty());
		QVERIFY(line.isEmpty());

		// F2: EOF without any data means "no credentials" (standalone), everything else is invalid
		QCOMPARE(readLineResult(QByteArray(), line), SecretLineResult::NoInput);
		QCOMPARE(readLineResult(QByteArray("abc"), line), SecretLineResult::Invalid);
		QCOMPARE(readLineResult(QByteArray(129, 'x') + '\n', line), SecretLineResult::Invalid);
		QCOMPARE(readLineResult(QByteArray("\n"), line), SecretLineResult::Read);
		QVERIFY(line.isEmpty());

		// no data within the timeout while the writer keeps the pipe open (Windows: PeekNamedPipe loop)
		int fds[2];
		QCOMPARE(pipeFds(fds), 0);
		QCOMPARE(StandardInputReader::readSecretLine(fds[0], line, 128, 50), SecretLineResult::Invalid);
		writeFd(fds[1], QByteArray("partial"));
		QCOMPARE(StandardInputReader::readSecretLine(fds[0], line, 128, 50), SecretLineResult::Invalid);
		QVERIFY(line.isEmpty());
		closeFd(fds[0]);
		closeFd(fds[1]);
	}

#ifndef Q_OS_WIN
	// F2: tri-state result for stdin (absent / valid / invalid)
	void credentialsFromStandardInput()
	{
		const auto readWithStandardInput = [](const QByteArray& input) {
			int fds[2];
			if (pipe(fds) != 0)
			{
				return IpcStandardInputCredentials{IpcStandardInputCredentials::Result::Invalid};
			}
			if (write(fds[1], input.constData(), size_t(input.size())) != ssize_t(input.size()))
			{
				return IpcStandardInputCredentials{IpcStandardInputCredentials::Result::Invalid};
			}
			close(fds[1]);
			const auto savedStandardInput = dup(STDIN_FILENO);
			dup2(fds[0], STDIN_FILENO);
			close(fds[0]);
			auto result = IpcEndpointCredentials::readFromStandardInput(1000);
			dup2(savedStandardInput, STDIN_FILENO);
			close(savedStandardInput);
			return result;
		};

		const auto credentials = IpcEndpointCredentials::generate(IpcProtocol::ServerEndpointPrefix, 1);
		auto valid = readWithStandardInput(credentials->toLine().toUnprotectedByteArray());
		QCOMPARE(valid.result, IpcStandardInputCredentials::Result::Valid);
		QCOMPARE(valid.credentials->endpointName(), credentials->endpointName());

		QCOMPARE(readWithStandardInput({}).result, IpcStandardInputCredentials::Result::Absent);
		QCOMPARE(readWithStandardInput("VEYONIPC1 garbage\n").result, IpcStandardInputCredentials::Result::Invalid);
		const auto truncated = credentials->toLine().toUnprotectedByteArray().left(40);
		QCOMPARE(readWithStandardInput(truncated).result, IpcStandardInputCredentials::Result::Invalid);
	}
#endif

	void credentialsLine()
	{
		auto credentials = IpcEndpointCredentials::generate(IpcProtocol::ServerEndpointPrefix, 12);
		QVERIFY(credentials.has_value());
		QVERIFY(credentials->endpointName().startsWith(QStringLiteral("veyon-ipc-12-")));
		QCOMPARE(credentials->token().size(), IpcProtocol::TokenSize);

		auto line = credentials->toLine();
		QVERIFY(line.size() < IpcProtocol::MaximumCredentialsLineSize);
		QCOMPARE(line.constData()[line.size() - 1], '\n');

		SecureBuffer readBack;
		QVERIFY(readLine(line.toUnprotectedByteArray(), readBack));
		auto parsed = IpcEndpointCredentials::parse(readBack);
		// the parsed line buffer is wiped
		QVERIFY(readBack.isEmpty());
		QVERIFY(parsed.has_value());
		QCOMPARE(parsed->endpointName(), credentials->endpointName());
		QVERIFY(VeyonCrypto::constantTimeEquals(parsed->token(), credentials->token()));

		const auto worker = IpcEndpointCredentials::generate(IpcProtocol::WorkerEndpointPrefix, 0);
		QVERIFY(worker->endpointName().startsWith(QStringLiteral("veyon-worker-0-")));
		QVERIFY(IpcEndpointCredentials::generate(IpcProtocol::ServerEndpointPrefix, -1).has_value() == false);

		const auto tokenHex = QByteArray(64, 'a');
		const QList<QByteArray> invalidLines{
			QByteArray("VEYONIPC1 veyon-ipc-1-0123abcd ") + QByteArray(63, 'a'),
			QByteArray("VEYONIPC1 veyon-ipc-1-0123abcd ") + QByteArray(65, 'a'),
			QByteArray("VEYONIPC1 veyon-ipc-1-0123abcd ") + QByteArray(62, 'a') + "zz",
			QByteArray("VEYONIPC2 veyon-ipc-1-0123abcd ") + tokenHex,
			QByteArray("VEYONIPC1 veyon-ipc-1-0123ABCD ") + tokenHex,
			QByteArray("VEYONIPC1 ../../etc/passwd-1-0123abcd ") + tokenHex,
			QByteArray("VEYONIPC1 veyon-ipc-1-0123abcd  ") + tokenHex,
			QByteArray("VEYONIPC1 veyon-other-1-0123abcd ") + tokenHex,
			QByteArray("VEYONIPC1  ") + tokenHex,
			QByteArray("VEYONIPC1"),
		};
		for (const auto& invalidLine : invalidLines)
		{
			auto buffer = SecureBuffer::fromByteArray(invalidLine);
			QVERIFY2(IpcEndpointCredentials::parse(buffer).has_value() == false, invalidLine.constData());
			QVERIFY(buffer.isEmpty());
		}

		// "$" must not match before a trailing newline
		QVERIFY(IpcEndpointCredentials::isValidEndpointName(QStringLiteral("veyon-ipc-1-0123abcd")));
		QVERIFY(IpcEndpointCredentials::isValidEndpointName(QStringLiteral("veyon-ipc-1-0123abcd\n")) == false);

		auto valid = SecureBuffer::fromByteArray(QByteArray("VEYONIPC1 veyon-worker-1-0123abcd ") + tokenHex);
		QVERIFY(IpcEndpointCredentials::parse(valid).has_value());
	}
};


QTEST_GUILESS_MAIN(IpcChannelTest)
#include "IpcChannelTest.moc"
