// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QJsonObject>

#include "IpcProtocol.h"
#include "VeyonCore.h"

#include <optional>

// JSON application messages of the local IPC channel (docs/specs/02-ipc.md §6):
// request {"id": <int>, "cmd": "<Command>", "args": {...}},
// response {"id": <int>, "ok": true|false, "error": "<code>", "result": {...}}
class VEYON_CORE_EXPORT IpcMessage
{
public:
	struct Request
	{
		int id = 0;
		QString command;
		QJsonObject arguments;
	};

	struct Response
	{
		int id = 0;
		bool ok = false;
		QString error;
		QJsonObject result;
	};

	[[nodiscard]] static QByteArray encodeRequest(const Request& request);
	[[nodiscard]] static QByteArray encodeResponse(const Response& response);

	// fail on invalid JSON, a nesting depth > MaximumJsonDepth or missing/mistyped members
	[[nodiscard]] static std::optional<Request> decodeRequest(const QByteArray& data);
	[[nodiscard]] static std::optional<Response> decodeResponse(const QByteArray& data);

	static Response success(int id, const QJsonObject& result = {})
	{
		return {id, true, {}, result};
	}

	static Response failure(int id, QLatin1String error)
	{
		return {id, false, error, {}};
	}

private:
	static std::optional<QJsonObject> decodeObject(const QByteArray& data);
	static std::optional<int> decodeId(const QJsonObject& object);
};
