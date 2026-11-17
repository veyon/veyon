// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DirectoryBrokerMessage.h"
#include "VeyonCore.h"

#include <functional>
#include <memory>
#include <optional>

// Client side transport of the directory and user-groups broker (docs/specs/04-directory-broker.md
// §3): local IPC to veyon-service in the session server (DirectoryBrokerIpcClient), broker feature
// messages over an authenticated Veyon connection to the session server on DirectoryBroker/Host in
// the master, CLI and configurator (DirectoryBrokerConnectionClient). Every failure is returned as
// an error reply, never as an empty result.
class VEYON_CORE_EXPORT DirectoryBrokerClient
{
public:
	using Reply = DirectoryBrokerMessage::Reply;
	using ReplyHandler = std::function<void(const Reply&)>;
	// items is std::nullopt if any page failed; reply is the last (or failed) reply
	using ListHandler = std::function<void(const std::optional<QVariantList>& items, const Reply& reply)>;

	enum class Placement {
		InProcess,
		Broker,
	};

	virtual ~DirectoryBrokerClient() = default;

	// synchronous request with the broker's request timeout
	virtual Reply request(const QString& command, const QVariantMap& arguments) = 0;

	// asynchronous request; has to be called in the thread of the client object, the handler is
	// called exactly once in that thread, never before this function has returned
	virtual void requestAsync(const QString& command, const QVariantMap& arguments, const ReplyHandler& handler) = 0;

	// all pages of a list result (Objects or Groups), following NextCursor
	std::optional<QVariantList> requestList(const QString& command, const QVariantMap& arguments, Reply* lastReply = nullptr);
	void requestListAsync(const QString& command, const QVariantMap& arguments, const ListHandler& handler);

	// where a directory or user-groups backend is run in this process (04 §2)
	static Placement placement(VeyonCore::Component component, VeyonCore::DirectoryBrokerMode mode,
							   bool requiresSecrets, bool isSecretStoreAvailable, bool isLocalBrokerHost);
	static Placement configuredPlacement(bool requiresSecrets);

	// DirectoryBroker/Mode, invalid values are read as Required
	static VeyonCore::DirectoryBrokerMode configuredMode();

	// loopback names and addresses only (no name resolution)
	static bool isLocalHost(const QString& host);

	// the shared client of this process for broker-backed directories and user-groups backends
	static std::shared_ptr<DirectoryBrokerClient> configuredClient();

private:
	void requestNextPage(const QString& command, const QVariantMap& arguments, const QVariantList& items,
						 const ListHandler& handler);

	static bool appendPage(const Reply& reply, QLatin1String listKey, QVariantList& items, QString& nextCursor);
};
