// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "IpcChannel.h"

// Local IPC channel in its pre-authentication states (docs/specs/02-ipc.md §8.11): untrusted
// bytes from any local process that can connect to an endpoint. The first input byte selects the
// side and how the remaining bytes are split into chunks.
extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	if (size < 1)
	{
		return 0;
	}

	const auto mode = quint8(data[0]);
	const auto side = (mode & 1) ? IpcChannel::Side::Client : IpcChannel::Side::Server;
	const auto role = (mode & 2) ? IpcProtocol::Role::Worker : IpcProtocol::Role::SessionServer;
	const auto chunkSize = qsizetype((mode >> 2) + 1);

	qint64 now = 0;
	IpcChannel::NonceHistory history;
	IpcChannel channel(side, role, 1, QStringLiteral("veyon-ipc-1-0123abcd"),
					   Veyon::Crypto::SecureBuffer(QByteArray(IpcProtocol::TokenSize, 't').constData(),
												   IpcProtocol::TokenSize),
					   &history, [&now]() { return now; });
	(void)channel.start();

	const auto* input = data + 1;
	auto remaining = qsizetype(size - 1);
	while (remaining > 0 && channel.hasFailed() == false)
	{
		const auto count = qMin(chunkSize, remaining);
		(void)channel.feed(Veyon::Crypto::ByteView(input, count));
		input += count;
		remaining -= count;
		now += 10;
		(void)channel.checkTimeout();
	}

	// unauthenticated input must never complete the handshake
	if (channel.isEstablished())
	{
		__builtin_trap();
	}

	return 0;
}
