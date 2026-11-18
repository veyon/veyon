// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AuthenticationLimiter.h"
#include "IpcAuthenticationRequest.h"
#include "IpcMessage.h"
#include "ServiceIpcCommandHandler.h"

// IPC command Authenticate as received by veyon-service (docs/specs/02-ipc.md §6b): the decrypted
// application message of a session server, parsed, validated and limited before any password
// check. The first input byte selects how the fake password check answers.
extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	if (size < 1)
	{
		return 0;
	}

	const auto mode = quint8(data[0]);
	const auto message = QByteArray::fromRawData(data + 1, qsizetype(size - 1));

	const auto request = IpcMessage::decodeRequest(message);
	if (request.has_value() == false)
	{
		return 0;
	}

	// whatever passes the parser satisfies the limits
	const auto arguments = IpcAuthenticationRequest::decode(request->arguments);
	if (arguments.has_value() &&
		AuthenticationLimiter::isValidRequest(arguments->username, arguments->password.constData(),
											  arguments->password.size()) == false)
	{
		__builtin_trap();
	}

	qint64 now = 0;
	ServiceIpcCommandHandler handler([&now]() { return now; });
	const auto limiter = AuthenticationLimiter::create([&now]() { return now; });

	int checks = 0;
	ServiceIpcCommandHandler::AuthenticationResultHandler pendingResult;
	handler.setAuthenticator(
		[&](const QString& username, Veyon::Crypto::SecureBuffer&& password, QObject*,
			const ServiceIpcCommandHandler::AuthenticationResultHandler& resultHandler) {
		++checks;
		if (AuthenticationLimiter::isValidRequest(username, password.constData(), password.size()) == false)
		{
			__builtin_trap();
		}
		if (mode & 1)
		{
			resultHandler((mode & 2) ? ServiceIpcCommandHandler::AuthenticationResult::Authenticated : ServiceIpcCommandHandler::AuthenticationResult::Failed);
		}
		else
		{
			pendingResult = resultHandler;
		}
	},
		limiter);

	ServiceIpcCommandHandler::Client client;
	client.sessionId = 1;
	client.platformSessionId = QStringLiteral("seat0");
	client.hasLoggedOnUser = []() { return true; };

	const auto encodeResponse = [](const IpcMessage::Response& response) {
		(void)IpcMessage::encodeResponse(response);
	};

	// the same request twice: the second one must never reach the check while the first is
	// outstanding or backing off
	handler.handleRequest(client, *request, nullptr, encodeResponse);
	handler.handleRequest(client, *request, nullptr, encodeResponse);

	// only a successful first check admits the second request right away
	const auto isAuthenticateRequest = request->command == IpcProtocol::Command::Authenticate && arguments.has_value();
	const auto expectedChecks = isAuthenticateRequest ? ((mode & 3) == 3 ? 2 : 1) : 0;
	if (checks != expectedChecks || limiter->runningCount() > 1)
	{
		__builtin_trap();
	}

	pendingResult = nullptr;
	if (limiter->runningCount() != 0)
	{
		__builtin_trap();
	}

	return 0;
}
