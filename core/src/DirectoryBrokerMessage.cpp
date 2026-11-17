// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QJsonArray>
#include <QJsonDocument>
#include <QMetaEnum>

#include "DirectoryBrokerLimits.h"
#include "DirectoryBrokerMessage.h"
#include "FeatureMessage.h"

#include <array>
#include <cmath>
#include <limits>


namespace {

using namespace DirectoryBrokerProtocol;

enum class ArgumentType {
	Uid,
	Cursor,
	TypeName,
	AttributeName,
	String,
	Name,
	Bool,
};

struct ArgumentSpecification
{
	QLatin1String key;
	ArgumentType type;
	bool required;
};

struct CommandSpecification
{
	QLatin1String command;
	std::array<ArgumentSpecification, 4> arguments;
	int argumentCount;
};

const std::array<CommandSpecification, 8>& commandSpecifications()
{
	static const std::array<CommandSpecification, 8> specifications{{
		{Command::DirectoryUpdate, {{{Key::Cursor, ArgumentType::Cursor, false}}}, 1},
		{Command::DirectoryFetchObjects, {{{Key::ObjectUid, ArgumentType::Uid, true}, {Key::Cursor, ArgumentType::Cursor, false}}}, 2},
		{Command::DirectoryQueryObjects, {{{Key::Type, ArgumentType::TypeName, true}, {Key::Attribute, ArgumentType::AttributeName, true}, {Key::Value, ArgumentType::String, false}, {Key::Cursor, ArgumentType::Cursor, false}}}, 4},
		{Command::DirectoryQueryParents, {{{Key::ObjectUid, ArgumentType::Uid, true}, {Key::Cursor, ArgumentType::Cursor, false}}}, 2},
		{Command::DirectoryQueryObjectAttribute, {{{Key::ObjectUid, ArgumentType::Uid, true}, {Key::Attribute, ArgumentType::AttributeName, true}}}, 2},
		{Command::UserGroupsList, {{{Key::QueryDomainGroups, ArgumentType::Bool, false}, {Key::Cursor, ArgumentType::Cursor, false}}}, 2},
		{Command::UserGroupsGroupsOfUser, {{{Key::Username, ArgumentType::Name, true}, {Key::QueryDomainGroups, ArgumentType::Bool, false}, {Key::Cursor, ArgumentType::Cursor, false}}}, 3},
		{Command::UserGroupsGroupSecurityIdentifier, {{{Key::GroupName, ArgumentType::Name, true}}}, 1},
	}};

	return specifications;
}



const CommandSpecification* commandSpecification(const QString& command)
{
	for (const auto& specification : commandSpecifications())
	{
		if (command == specification.command)
		{
			return &specification;
		}
	}

	return nullptr;
}



bool isString(const QVariant& value)
{
	return value.userType() == QMetaType::QString;
}



bool isValidArgument(const QVariant& value, ArgumentType type)
{
	switch (type)
	{
	case ArgumentType::Uid: return DirectoryBrokerMessage::uidFromVariant(value).has_value();
	case ArgumentType::Cursor: return DirectoryBrokerMessage::isValidCursor(value);
	case ArgumentType::TypeName: return DirectoryBrokerMessage::typeFromName(value).has_value();
	case ArgumentType::AttributeName: return DirectoryBrokerMessage::attributeFromName(value).has_value();
	case ArgumentType::String: return isString(value) && DirectoryBrokerMessage::isValidString(value.toString());
	case ArgumentType::Name:
		return isString(value) && value.toString().isEmpty() == false &&
			DirectoryBrokerMessage::isValidString(value.toString());
	case ArgumentType::Bool: return value.userType() == QMetaType::Bool;
	}

	return false;
}



template<typename T>
std::optional<T> enumFromName(const QVariant& value)
{
	if (isString(value) == false)
	{
		return {};
	}

	const auto name = value.toString().toLatin1();
	if (name.isEmpty() || name.size() > DirectoryBrokerLimits::MaximumStringSize)
	{
		return {};
	}

	// exact enumerator names only, never numbers
	const auto metaEnum = QMetaEnum::fromType<T>();
	for (int index = 0; index < metaEnum.keyCount(); ++index)
	{
		if (name == metaEnum.key(index))
		{
			return T(metaEnum.value(index));
		}
	}

	return {};
}

bool isWithinDepth(const QJsonValue& value, int remainingDepth)
{
	if (value.isObject() == false && value.isArray() == false)
	{
		return true;
	}

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
		return true;
	}

	const auto array = value.toArray();
	for (const auto& element : array)
	{
		if (isWithinDepth(element, remainingDepth - 1) == false)
		{
			return false;
		}
	}

	return true;
}



// the argument map is carried as JSON as over IPC: one encoding, and feature messages
// (VariantStream) neither allow deeper nesting nor floating point numbers
QByteArray encodeArguments(const QVariantMap& arguments)
{
	return QJsonDocument(QJsonObject::fromVariantMap(arguments)).toJson(QJsonDocument::Compact);
}



std::optional<QVariantMap> decodeArguments(const QByteArray& data)
{
	QJsonParseError parseError{};
	const auto document = QJsonDocument::fromJson(data, &parseError);
	if (parseError.error != QJsonParseError::NoError ||
		document.isObject() == false ||
		isWithinDepth(document.object(), IpcProtocol::MaximumJsonDepth) == false)
	{
		return {};
	}

	return document.object().toVariantMap();
}

} // namespace



QString DirectoryBrokerMessage::Reply::errorCode() const
{
	return isError() ? arguments.value(DirectoryBrokerProtocol::Key::Code).toString() : QString{};
}



QString DirectoryBrokerMessage::Reply::errorMessage() const
{
	return isError() ? arguments.value(DirectoryBrokerProtocol::Key::Message).toString() : QString{};
}



bool DirectoryBrokerMessage::isBrokerCommand(const QString& command)
{
	return commandSpecification(command) != nullptr;
}



std::optional<DirectoryBrokerMessage::Reply> DirectoryBrokerMessage::validateRequest(const Request& request)
{
	const auto specification = commandSpecification(request.command);
	if (specification == nullptr)
	{
		return error(request.requestId, Error::BadRequest, QStringLiteral("unknown command"));
	}

	if (request.arguments.size() > DirectoryBrokerLimits::MaximumArgumentCount)
	{
		return error(request.requestId, Error::TooLarge, QStringLiteral("too many arguments"));
	}

	int knownArguments = 0;

	// accepted for every command (set by the session server)
	const auto forwarded = request.arguments.constFind(Key::Forwarded);
	if (forwarded != request.arguments.constEnd())
	{
		if (forwarded.value().userType() != QMetaType::Bool)
		{
			return error(request.requestId, Error::BadRequest, QStringLiteral("invalid argument %1").arg(Key::Forwarded));
		}
		++knownArguments;
	}

	for (int index = 0; index < specification->argumentCount; ++index)
	{
		const auto& argument = specification->arguments.at(size_t(index));
		const auto it = request.arguments.constFind(argument.key);
		if (it == request.arguments.constEnd())
		{
			if (argument.required)
			{
				return error(request.requestId, Error::BadRequest, QStringLiteral("missing argument %1").arg(argument.key));
			}
			continue;
		}

		if (isValidArgument(it.value(), argument.type) == false)
		{
			return error(request.requestId, Error::BadRequest, QStringLiteral("invalid argument %1").arg(argument.key));
		}

		++knownArguments;
	}

	if (knownArguments != request.arguments.size())
	{
		return error(request.requestId, Error::BadRequest, QStringLiteral("unknown argument"));
	}

	// all values are bounded above, this protects against future argument types
	const auto encodedSize = QJsonDocument(QJsonObject::fromVariantMap(request.arguments)).toJson(QJsonDocument::Compact).size() +
		request.command.size();
	if (encodedSize > DirectoryBrokerLimits::MaximumRequestSize)
	{
		return error(request.requestId, Error::TooLarge, QStringLiteral("request too large"));
	}

	return {};
}



bool DirectoryBrokerMessage::isValidString(const QString& string)
{
	if (string.size() > DirectoryBrokerLimits::MaximumStringSize ||
		string.isValidUtf16() == false ||
		string.toUtf8().size() > DirectoryBrokerLimits::MaximumStringSize)
	{
		return false;
	}

	for (const auto character : string)
	{
		if (character.category() == QChar::Other_Control)
		{
			return false;
		}
	}

	return true;
}



DirectoryBrokerMessage::Reply DirectoryBrokerMessage::success(const Request& request, const QVariantMap& arguments)
{
	return {request.requestId, request.command, arguments};
}



DirectoryBrokerMessage::Reply DirectoryBrokerMessage::error(quint32 requestId, QLatin1String code, const QString& message)
{
	return {requestId,
			DirectoryBrokerProtocol::Command::Error,
			{{DirectoryBrokerProtocol::Key::Code, QString(code)}, {DirectoryBrokerProtocol::Key::Message, message}}};
}



void DirectoryBrokerMessage::toFeatureMessage(const Request& request, FeatureMessage& message)
{
	// VariantStream does not accept unsigned integers
	message.addArgument(Argument::RequestId, qint64(request.requestId))
		.addArgument(Argument::Command, request.command)
		.addArgument(Argument::Arguments, encodeArguments(request.arguments));
}



void DirectoryBrokerMessage::toFeatureMessage(const Reply& reply, FeatureMessage& message)
{
	message.addArgument(Argument::RequestId, qint64(reply.requestId))
		.addArgument(Argument::Command, reply.command)
		.addArgument(Argument::Arguments, encodeArguments(reply.arguments));
}



static bool decodeFeatureMessage(const FeatureMessage& message, int maximumArgumentsSize,
								 quint32& requestId, QString& command, QVariantMap& arguments)
{
	if (message.featureUid() != DirectoryBrokerMessage::featureUid() ||
		message.arguments().size() != 3)
	{
		return false;
	}

	const auto requestIdValue = message.argument(DirectoryBrokerMessage::Argument::RequestId);
	const auto commandValue = message.argument(DirectoryBrokerMessage::Argument::Command);
	const auto argumentsValue = message.argument(DirectoryBrokerMessage::Argument::Arguments);

	if (requestIdValue.userType() != QMetaType::LongLong ||
		requestIdValue.toLongLong() < 0 || requestIdValue.toLongLong() > std::numeric_limits<quint32>::max() ||
		commandValue.userType() != QMetaType::QString ||
		commandValue.toString().size() > DirectoryBrokerLimits::MaximumStringSize ||
		argumentsValue.userType() != QMetaType::QByteArray ||
		argumentsValue.toByteArray().size() > maximumArgumentsSize)
	{
		return false;
	}

	const auto decodedArguments = decodeArguments(argumentsValue.toByteArray());
	if (decodedArguments.has_value() == false)
	{
		return false;
	}

	requestId = quint32(requestIdValue.toLongLong());
	command = commandValue.toString();
	arguments = *decodedArguments;

	return true;
}



std::optional<DirectoryBrokerMessage::Request> DirectoryBrokerMessage::requestFromFeatureMessage(const FeatureMessage& message)
{
	Request request;
	if (decodeFeatureMessage(message, DirectoryBrokerLimits::MaximumRequestSize,
							 request.requestId, request.command, request.arguments) == false)
	{
		return {};
	}

	return request;
}



std::optional<DirectoryBrokerMessage::Reply> DirectoryBrokerMessage::replyFromFeatureMessage(const FeatureMessage& message)
{
	Reply reply;
	if (decodeFeatureMessage(message, DirectoryBrokerLimits::MaximumReplySize,
							 reply.requestId, reply.command, reply.arguments) == false)
	{
		return {};
	}

	return reply;
}



DirectoryBrokerMessage::Request DirectoryBrokerMessage::fromIpcRequest(const IpcMessage::Request& request)
{
	return {quint32(request.id), request.command, request.arguments.toVariantMap()};
}



IpcMessage::Request DirectoryBrokerMessage::toIpcRequest(int ipcRequestId, const QString& command, const QVariantMap& arguments)
{
	return {ipcRequestId, command, QJsonObject::fromVariantMap(arguments)};
}



IpcMessage::Response DirectoryBrokerMessage::toIpcResponse(int ipcRequestId, const Reply& reply)
{
	if (reply.isError())
	{
		return {ipcRequestId, false, reply.errorCode(), {}};
	}

	return {ipcRequestId, true, {}, QJsonObject::fromVariantMap(reply.arguments)};
}



DirectoryBrokerMessage::Reply DirectoryBrokerMessage::fromIpcResponse(quint32 requestId, const QString& command,
																	  const IpcMessage::Response& response)
{
	if (response.ok)
	{
		return {requestId, command, response.result.toVariantMap()};
	}

	// failed calls (no connection, timeout) have no error code; codes of other IPC commands
	// (e.g. unknown-command of an incompatible service) are not passed on
	static const std::array<QLatin1String, 6> brokerErrors{
		DirectoryBrokerProtocol::Error::Unavailable, DirectoryBrokerProtocol::Error::Denied,
		DirectoryBrokerProtocol::Error::TooLarge, DirectoryBrokerProtocol::Error::BadRequest,
		DirectoryBrokerProtocol::Error::Timeout, DirectoryBrokerProtocol::Error::RateLimited};

	for (const auto& brokerError : brokerErrors)
	{
		if (response.error == brokerError)
		{
			return error(requestId, brokerError);
		}
	}

	return error(requestId, DirectoryBrokerProtocol::Error::Unavailable, QStringLiteral("directory broker not reachable"));
}



QVariant DirectoryBrokerMessage::objectToVariant(const NetworkObject& object)
{
	return object.toJson().toVariantMap();
}



std::optional<NetworkObject> DirectoryBrokerMessage::objectFromVariant(const QVariant& value)
{
	if (value.userType() != QMetaType::QVariantMap)
	{
		return {};
	}

	const auto map = value.toMap();
	if (map.size() > 7)
	{
		return {};
	}

	for (auto it = map.constBegin(); it != map.constEnd(); ++it)
	{
		if (it.key() != QLatin1String("Type") &&
			(isString(it.value()) == false || it.value().toString().size() > DirectoryBrokerLimits::MaximumReplyStringSize))
		{
			return {};
		}
	}

	// NetworkObject::toJson() encodes the type as number (JSON double over IPC, int otherwise)
	const auto typeValue = map.value(QStringLiteral("Type"));
	const auto typeValueType = typeValue.userType();
	if (typeValueType != QMetaType::Int && typeValueType != QMetaType::UInt &&
		typeValueType != QMetaType::LongLong && typeValueType != QMetaType::ULongLong &&
		typeValueType != QMetaType::Double)
	{
		return {};
	}

	const auto typeNumber = typeValue.toDouble();
	if ((typeNumber >= 0 && typeNumber <= 255) == false || typeNumber != std::floor(typeNumber))
	{
		return {};
	}

	const auto type = NetworkObject::Type(int(typeNumber));
	if ((type != NetworkObject::Type::Location && type != NetworkObject::Type::Host &&
		 type != NetworkObject::Type::Label && type != NetworkObject::Type::DesktopGroup))
	{
		return {};
	}

	auto json = QJsonObject::fromVariantMap(map);
	json[QStringLiteral("Type")] = int(type);

	NetworkObject object(json);
	if (object.uid().isNull() ||
		(map.contains(QStringLiteral("ParentUid")) && object.parentUid().isNull()))
	{
		return {};
	}

	return object;
}



QString DirectoryBrokerMessage::typeName(NetworkObject::Type type)
{
	return QString::fromLatin1(QMetaEnum::fromType<NetworkObject::Type>().valueToKey(int(type)));
}



QString DirectoryBrokerMessage::attributeName(NetworkObject::Attribute attribute)
{
	return QString::fromLatin1(QMetaEnum::fromType<NetworkObject::Attribute>().valueToKey(int(attribute)));
}



std::optional<NetworkObject::Type> DirectoryBrokerMessage::typeFromName(const QVariant& value)
{
	return enumFromName<NetworkObject::Type>(value);
}



std::optional<NetworkObject::Attribute> DirectoryBrokerMessage::attributeFromName(const QVariant& value)
{
	return enumFromName<NetworkObject::Attribute>(value);
}



std::optional<NetworkObject::Uid> DirectoryBrokerMessage::uidFromVariant(const QVariant& value)
{
	if (isString(value) == false || value.toString().size() > 38)
	{
		return {};
	}

	const auto uid = NetworkObject::Uid(value.toString());
	if (uid.isNull())
	{
		return {};
	}

	return uid;
}



bool DirectoryBrokerMessage::isValidCursor(const QVariant& value)
{
	if (isString(value) == false)
	{
		return false;
	}

	const auto cursor = value.toString();
	if (cursor.isEmpty())
	{
		return true;
	}

	if (cursor.size() != DirectoryBrokerLimits::CursorSize * 2)
	{
		return false;
	}

	for (const auto character : cursor)
	{
		if ((character >= QLatin1Char('0') && character <= QLatin1Char('9')) == false &&
			(character >= QLatin1Char('a') && character <= QLatin1Char('f')) == false)
		{
			return false;
		}
	}

	return true;
}



QLatin1String DirectoryBrokerMessage::listKey(const QString& command)
{
	if (command.startsWith(QLatin1String("UserGroups.")))
	{
		return DirectoryBrokerProtocol::Key::Groups;
	}

	return DirectoryBrokerProtocol::Key::Objects;
}



int DirectoryBrokerMessage::maximumItemsPerPage(const QString& command)
{
	return listKey(command) == DirectoryBrokerProtocol::Key::Groups ? DirectoryBrokerLimits::MaximumGroupsPerPage : DirectoryBrokerLimits::MaximumObjectsPerPage;
}
