// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QHostAddress>

#include "DirectoryBrokerClient.h"
#include "DirectoryBrokerConnectionClient.h"
#include "DirectoryBrokerIpcClient.h"
#include "DirectoryBrokerLimits.h"
#include "SecretStore.h"
#include "VeyonConfiguration.h"
#include "pki/TlsConfiguration.h"


using namespace DirectoryBrokerProtocol;


std::optional<QVariantList> DirectoryBrokerClient::requestList(const QString& command, const QVariantMap& arguments, Reply* lastReply)
{
	const auto listKey = DirectoryBrokerMessage::listKey(command);

	QVariantList items;
	auto pageArguments = arguments;

	for (;;)
	{
		const auto reply = request(command, pageArguments);
		if (lastReply)
		{
			*lastReply = reply;
		}

		QString nextCursor;
		if (appendPage(reply, listKey, items, nextCursor) == false)
		{
			return {};
		}

		if (nextCursor.isEmpty())
		{
			return items;
		}

		pageArguments[Key::Cursor] = nextCursor;
	}
}



void DirectoryBrokerClient::requestListAsync(const QString& command, const QVariantMap& arguments, const ListHandler& handler)
{
	requestNextPage(command, arguments, {}, handler);
}



DirectoryBrokerClient::Placement DirectoryBrokerClient::placement(VeyonCore::Component component,
																  VeyonCore::DirectoryBrokerMode mode,
																  bool requiresSecrets, bool isSecretStoreAvailable,
																  bool isLocalBrokerHost)
{
	// backends without secrets (builtin directory, local system user groups) are unchanged
	if (requiresSecrets == false)
	{
		return Placement::InProcess;
	}

	switch (component)
	{
	case VeyonCore::Component::Server:
	case VeyonCore::Component::Worker:
		// never, regardless of mode and uid (root on X11, SYSTEM on Windows)
		return Placement::Broker;
	case VeyonCore::Component::Service:
		// the broker itself; in central mode only the central host holds the credential (04 §3a)
		return isLocalBrokerHost ? Placement::InProcess : Placement::Broker;
	default:
		break;
	}

	if (mode == VeyonCore::DirectoryBrokerMode::Disabled)
	{
		// legacy behaviour: works only where the secret is readable, i.e. in elevated processes
		return Placement::InProcess;
	}

	// elevated configurator/CLI; in all other components the store is not available, so the
	// Auto fallback "in-process if the secret is readable" never applies there
	if (isSecretStoreAvailable)
	{
		return Placement::InProcess;
	}

	return Placement::Broker;
}



DirectoryBrokerClient::Placement DirectoryBrokerClient::configuredPlacement(bool requiresSecrets)
{
	return placement(VeyonCore::component(), configuredMode(), requiresSecrets,
					 requiresSecrets && SecretStore::isAvailable(),
					 isLocalHost(VeyonCore::config().directoryBrokerHost()));
}



VeyonCore::DirectoryBrokerMode DirectoryBrokerClient::configuredMode()
{
	const auto& property = VeyonCore::config().directoryBrokerModeProperty();
	const auto mode = TlsConfiguration::enumValue<VeyonCore::DirectoryBrokerMode>(property.variantValue());
	if (mode.has_value())
	{
		return *mode;
	}

	vCritical() << "invalid value of" << property.absoluteKey() << "- using Required";
	return VeyonCore::DirectoryBrokerMode::Required;
}



bool DirectoryBrokerClient::isLocalHost(const QString& host)
{
	if (host.compare(QLatin1String("localhost"), Qt::CaseInsensitive) == 0)
	{
		return true;
	}

	const QHostAddress address(host);
	return address.isNull() == false && address.isLoopback();
}



std::shared_ptr<DirectoryBrokerClient> DirectoryBrokerClient::configuredClient()
{
	static std::weak_ptr<DirectoryBrokerClient> sharedClient;

	auto client = sharedClient.lock();
	if (client)
	{
		return client;
	}

	switch (VeyonCore::component())
	{
	case VeyonCore::Component::Server:
	case VeyonCore::Component::Worker:
	case VeyonCore::Component::Service:
		// the service only gets here in central mode, which needs a verified TLS connection and
		// KeyFileBound authentication to the central host (04 §3a) - not available yet, so all
		// requests fail (fail closed)
		client = std::make_shared<DirectoryBrokerIpcClient>();
		break;
	default:
		client = std::make_shared<DirectoryBrokerConnectionClient>(VeyonCore::config().directoryBrokerHost());
		break;
	}

	sharedClient = client;

	return client;
}



void DirectoryBrokerClient::requestNextPage(const QString& command, const QVariantMap& arguments, const QVariantList& items,
											const ListHandler& handler)
{
	requestAsync(command, arguments, [this, command, arguments, items, handler](const Reply& reply) {
		auto allItems = items;
		QString nextCursor;
		if (appendPage(reply, DirectoryBrokerMessage::listKey(command), allItems, nextCursor) == false)
		{
			handler(std::nullopt, reply);
			return;
		}

		if (nextCursor.isEmpty())
		{
			handler(allItems, reply);
			return;
		}

		auto nextArguments = arguments;
		nextArguments[Key::Cursor] = nextCursor;
		requestNextPage(command, nextArguments, allItems, handler);
	});
}



bool DirectoryBrokerClient::appendPage(const Reply& reply, QLatin1String listKey, QVariantList& items, QString& nextCursor)
{
	if (reply.isError())
	{
		vWarning() << "directory broker error" << reply.errorCode() << reply.errorMessage();
		return false;
	}

	const auto page = reply.arguments.value(listKey);
	const auto cursor = reply.arguments.value(Key::NextCursor);
	if ((page.userType() != QMetaType::QVariantList && page.userType() != QMetaType::QStringList) ||
		(cursor.isValid() && DirectoryBrokerMessage::isValidCursor(cursor) == false))
	{
		vWarning() << "malformed directory broker reply";
		return false;
	}

	const auto pageItems = page.toList();
	nextCursor = cursor.toString();

	if ((pageItems.isEmpty() && nextCursor.isEmpty() == false) ||
		items.size() + pageItems.size() > DirectoryBrokerLimits::MaximumResultItems)
	{
		vWarning() << "invalid or too large directory broker result";
		return false;
	}

	items.append(pageItems);

	return true;
}
