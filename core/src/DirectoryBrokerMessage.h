// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QVariantMap>

#include "DirectoryBrokerProtocol.h"
#include "Feature.h"
#include "IpcMessage.h"
#include "NetworkObject.h"

#include <optional>

class FeatureMessage;

// Requests and replies of the directory and user-groups broker (docs/specs/04-directory-broker.md
// §4, §6) and their encodings as broker feature message (master ⇄ session server) and as IPC
// message (session server ⇄ veyon-service, 02-ipc.md §6). Argument maps use the names of
// DirectoryBrokerProtocol::Key and are JSON objects in both encodings (in feature messages as
// JSON-encoded argument); enums are sent by name, UIDs as strings.
class VEYON_CORE_EXPORT DirectoryBrokerMessage
{
	Q_GADGET
public:
	// argument indices of the broker feature message
	enum class Argument {
		RequestId,
		Command,
		Arguments,
	};
	Q_ENUM(Argument)

	struct Request
	{
		quint32 requestId = 0;
		QString command{};
		QVariantMap arguments{};
	};

	struct Reply
	{
		quint32 requestId = 0;
		QString command{};
		QVariantMap arguments{};

		bool isError() const
		{
			return command == DirectoryBrokerProtocol::Command::Error;
		}

		QString errorCode() const;
		QString errorMessage() const;
	};

	static Feature::Uid featureUid()
	{
		return Feature::Uid{QStringLiteral("f34c9499-dcbf-48b2-a067-ca23c65618c8")};
	}

	static bool isBrokerCommand(const QString& command);

	// forwarded by a session server for a master (see DirectoryBrokerProtocol::Key::Forwarded)
	static bool isForwarded(const Request& request)
	{
		return request.arguments.value(DirectoryBrokerProtocol::Key::Forwarded).toBool();
	}

	// checks command name, argument names, types and values; returns an error reply
	// (bad-request/too-large) for invalid requests
	[[nodiscard]] static std::optional<Reply> validateRequest(const Request& request);

	// UTF-8 of at most MaximumStringSize bytes, valid UTF-16, no control characters
	[[nodiscard]] static bool isValidString(const QString& string);

	static Reply success(const Request& request, const QVariantMap& arguments = {});
	static Reply error(quint32 requestId, QLatin1String code, const QString& message = {});

	// broker feature message; message has to be created with featureUid()
	static void toFeatureMessage(const Request& request, FeatureMessage& message);
	static void toFeatureMessage(const Reply& reply, FeatureMessage& message);
	[[nodiscard]] static std::optional<Request> requestFromFeatureMessage(const FeatureMessage& message);
	[[nodiscard]] static std::optional<Reply> replyFromFeatureMessage(const FeatureMessage& message);

	// IPC message (the IPC request ID takes the role of the RequestId)
	static Request fromIpcRequest(const IpcMessage::Request& request);
	static IpcMessage::Request toIpcRequest(int ipcRequestId, const QString& command, const QVariantMap& arguments);
	static IpcMessage::Response toIpcResponse(int ipcRequestId, const Reply& reply);
	static Reply fromIpcResponse(quint32 requestId, const QString& command, const IpcMessage::Response& response);

	// typed argument values
	static QVariant objectToVariant(const NetworkObject& object);
	// fails for anything that is not a well-formed directory object (client side reply validation)
	[[nodiscard]] static std::optional<NetworkObject> objectFromVariant(const QVariant& value);

	static QString typeName(NetworkObject::Type type);
	static QString attributeName(NetworkObject::Attribute attribute);
	[[nodiscard]] static std::optional<NetworkObject::Type> typeFromName(const QVariant& value);
	[[nodiscard]] static std::optional<NetworkObject::Attribute> attributeFromName(const QVariant& value);
	[[nodiscard]] static std::optional<NetworkObject::Uid> uidFromVariant(const QVariant& value);
	[[nodiscard]] static bool isValidCursor(const QVariant& value);

	static QLatin1String listKey(const QString& command);
	static int maximumItemsPerPage(const QString& command);
};
