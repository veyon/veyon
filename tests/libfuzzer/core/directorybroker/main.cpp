// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QIODevice>

#include "DirectoryBrokerMessage.h"
#include "FeatureMessage.h"

// Request parser of the directory and user-groups broker (docs/specs/04-directory-broker.md §7):
// untrusted broker requests of masters (feature messages, parsed by the session server) and of
// session servers (IPC messages, parsed by veyon-service), plus reply objects parsed by masters.
extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	const auto input = QByteArray::fromRawData(data, int(size));

	FeatureMessage featureMessage;
	if (featureMessage.fromByteArray(input))
	{
		const auto request = DirectoryBrokerMessage::requestFromFeatureMessage(featureMessage);
		if (request.has_value())
		{
			(void)DirectoryBrokerMessage::validateRequest(*request);
		}

		const auto reply = DirectoryBrokerMessage::replyFromFeatureMessage(featureMessage);
		if (reply.has_value())
		{
			for (const auto& object : reply->arguments.value(DirectoryBrokerProtocol::Key::Objects).toList())
			{
				(void)DirectoryBrokerMessage::objectFromVariant(object);
			}
		}
	}

	const auto ipcRequest = IpcMessage::decodeRequest(input);
	if (ipcRequest.has_value())
	{
		const auto request = DirectoryBrokerMessage::fromIpcRequest(*ipcRequest);
		const auto validationError = DirectoryBrokerMessage::validateRequest(request);

		// a valid request has an encoding that is accepted again
		if (validationError.has_value() == false &&
			DirectoryBrokerMessage::validateRequest(
				DirectoryBrokerMessage::fromIpcRequest(*IpcMessage::decodeRequest(IpcMessage::encodeRequest(
					DirectoryBrokerMessage::toIpcRequest(1, request.command, request.arguments)))))
				.has_value())
		{
			__builtin_trap();
		}
	}

	return 0;
}
