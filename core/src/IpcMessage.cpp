// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QJsonArray>
#include <QJsonDocument>

#include "IpcMessage.h"

#include <cmath>
#include <limits>


static bool isWithinDepth(const QJsonValue& value, int remainingDepth)
{
	if (value.isObject() || value.isArray())
	{
		if (remainingDepth <= 0)
		{
			return false;
		}

		if (value.isObject())
		{
			const auto object = value.toObject();
			for (auto it = object.constBegin(); it != object.constEnd(); ++it)
			{
				if (isWithinDepth(it.value(), remainingDepth - 1) == false)
				{
					return false;
				}
			}
		}
		else
		{
			const auto array = value.toArray();
			for (const auto& element : array)
			{
				if (isWithinDepth(element, remainingDepth - 1) == false)
				{
					return false;
				}
			}
		}
	}

	return true;
}



QByteArray IpcMessage::encodeRequest(const Request& request)
{
	QJsonObject object{
		{IpcProtocol::Key::Id, request.id},
		{IpcProtocol::Key::Command, request.command},
	};
	if (request.arguments.isEmpty() == false)
	{
		object[IpcProtocol::Key::Arguments] = request.arguments;
	}

	return QJsonDocument(object).toJson(QJsonDocument::Compact);
}



QByteArray IpcMessage::encodeResponse(const Response& response)
{
	QJsonObject object{
		{IpcProtocol::Key::Id, response.id},
		{IpcProtocol::Key::Ok, response.ok},
	};
	if (response.ok)
	{
		object[IpcProtocol::Key::Result] = response.result;
	}
	else
	{
		object[IpcProtocol::Key::Error] = response.error;
	}

	return QJsonDocument(object).toJson(QJsonDocument::Compact);
}



std::optional<IpcMessage::Request> IpcMessage::decodeRequest(const QByteArray& data)
{
	const auto object = decodeObject(data);
	if (object.has_value() == false)
	{
		return {};
	}

	const auto id = decodeId(*object);
	const auto command = object->value(IpcProtocol::Key::Command);
	const auto arguments = object->value(IpcProtocol::Key::Arguments);
	if (id.has_value() == false ||
		command.isString() == false ||
		(arguments.isUndefined() == false && arguments.isObject() == false))
	{
		return {};
	}

	return Request{*id, command.toString(), arguments.toObject()};
}



std::optional<IpcMessage::Response> IpcMessage::decodeResponse(const QByteArray& data)
{
	const auto object = decodeObject(data);
	if (object.has_value() == false)
	{
		return {};
	}

	const auto id = decodeId(*object);
	const auto ok = object->value(IpcProtocol::Key::Ok);
	const auto error = object->value(IpcProtocol::Key::Error);
	const auto result = object->value(IpcProtocol::Key::Result);
	if (id.has_value() == false ||
		ok.isBool() == false ||
		(error.isUndefined() == false && error.isString() == false) ||
		(result.isUndefined() == false && result.isObject() == false))
	{
		return {};
	}

	return Response{*id, ok.toBool(), error.toString(), result.toObject()};
}



std::optional<QJsonObject> IpcMessage::decodeObject(const QByteArray& data)
{
	if (data.size() > IpcProtocol::MaximumRecordPlaintextSize)
	{
		return {};
	}

	QJsonParseError parseError{};
	const auto document = QJsonDocument::fromJson(data, &parseError);
	if (parseError.error != QJsonParseError::NoError ||
		document.isObject() == false ||
		isWithinDepth(document.object(), IpcProtocol::MaximumJsonDepth) == false)
	{
		return {};
	}

	return document.object();
}



std::optional<int> IpcMessage::decodeId(const QJsonObject& object)
{
	const auto id = object.value(IpcProtocol::Key::Id);
	if (id.isDouble() == false)
	{
		return {};
	}

	const auto value = id.toDouble();
	if ((value >= 0 && value <= double(std::numeric_limits<int>::max())) == false ||
		value != std::floor(value))
	{
		return {};
	}

	return int(value);
}
