// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QPointer>

#include "DirectoryBrokerClient.h"
#include "DirectoryBrokerLimits.h"
#include "RemoteNetworkObjectDirectory.h"


using namespace DirectoryBrokerProtocol;


RemoteNetworkObjectDirectory::RemoteNetworkObjectDirectory(const std::shared_ptr<DirectoryBrokerClient>& client, QObject* parent) :
	NetworkObjectDirectory(parent),
	m_client(client)
{
}



QVariant RemoteNetworkObjectDirectory::queryObjectAttribute(NetworkObject::Uid objectUid, NetworkObject::Attribute attribute)
{
	const auto reply = m_client->request(Command::DirectoryQueryObjectAttribute,
										 {{Key::ObjectUid, objectUid.toString()},
										  {Key::Attribute, DirectoryBrokerMessage::attributeName(attribute)}});
	const auto value = reply.arguments.value(Key::Value);
	if (reply.isError() || value.userType() != QMetaType::QString ||
		value.toString().size() > DirectoryBrokerLimits::MaximumReplyStringSize)
	{
		vWarning() << "attribute" << attribute << "of object" << objectUid << "not available";
		return {};
	}

	return value.toString();
}



NetworkObjectList RemoteNetworkObjectDirectory::queryObjects(NetworkObject::Type type,
															 NetworkObject::Attribute attribute, const QVariant& value)
{
	return tryQueryObjects(type, attribute, value).value_or(NetworkObjectList{});
}



NetworkObjectList RemoteNetworkObjectDirectory::queryParents(const NetworkObject& child)
{
	return tryQueryParents(child).value_or(NetworkObjectList{});
}



std::optional<NetworkObjectList> RemoteNetworkObjectDirectory::tryQueryObjects(NetworkObject::Type type,
																			   NetworkObject::Attribute attribute,
																			   const QVariant& value)
{
	const auto valueString = value.toString();
	if (DirectoryBrokerMessage::isValidString(valueString) == false)
	{
		vWarning() << "invalid query value";
		return {};
	}

	QVariantMap arguments{{Key::Type, DirectoryBrokerMessage::typeName(type)},
						  {Key::Attribute, DirectoryBrokerMessage::attributeName(attribute)}};
	if (valueString.isEmpty() == false)
	{
		arguments[Key::Value] = valueString;
	}

	return toObjects(m_client->requestList(Command::DirectoryQueryObjects, arguments));
}



std::optional<NetworkObjectList> RemoteNetworkObjectDirectory::tryQueryParents(const NetworkObject& child)
{
	return toObjects(m_client->requestList(Command::DirectoryQueryParents, {{Key::ObjectUid, child.uid().toString()}}));
}



void RemoteNetworkObjectDirectory::update()
{
	if (m_updatePending)
	{
		return;
	}

	m_updatePending = true;

	const QPointer<RemoteNetworkObjectDirectory> directory(this);
	m_client->requestListAsync(Command::DirectoryUpdate, {},
							   [directory](const std::optional<QVariantList>& items, const DirectoryBrokerClient::Reply& reply) {
		if (directory.isNull())
		{
			return;
		}

		directory->m_updatePending = false;

		const auto objects = toObjects(items);
		if (objects.has_value() == false)
		{
			directory->setError(reply.isError() ? reply.errorMessage() : tr("invalid reply"));
			return;
		}

		directory->clearError();
		directory->m_generation = reply.arguments.value(Key::Generation).toInt();
		directory->replaceObjects(*objects, directory->rootObject());
		directory->setObjectPopulated(directory->rootObject());
	});
}



void RemoteNetworkObjectDirectory::fetchObjects(const NetworkObject& object)
{
	if (object.type() == NetworkObject::Type::Root)
	{
		update();
		return;
	}

	if (object.isContainer() == false)
	{
		setObjectPopulated(object);
		return;
	}

	if (m_pendingFetches.contains(object.uid()))
	{
		return;
	}

	m_pendingFetches.insert(object.uid());

	const QPointer<RemoteNetworkObjectDirectory> directory(this);
	m_client->requestListAsync(Command::DirectoryFetchObjects, {{Key::ObjectUid, object.uid().toString()}},
							   [directory, object](const std::optional<QVariantList>& items, const DirectoryBrokerClient::Reply& reply) {
		if (directory.isNull())
		{
			return;
		}

		directory->m_pendingFetches.remove(object.uid());

		const auto objects = toObjects(items);
		if (objects.has_value())
		{
			directory->replaceObjects(*objects, object);
		}
		else
		{
			vWarning() << "failed to fetch objects of" << object.name() << reply.errorCode() << reply.errorMessage();
		}

		// also on failure, otherwise views would request the objects over and over again
		directory->setObjectPopulated(object);
	});
}



void RemoteNetworkObjectDirectory::setError(const QString& errorString)
{
	const auto message = errorString.isEmpty() ? tr("unknown error") : errorString;

	if (m_errorString != message)
	{
		vWarning() << "directory not available:" << message;
	}

	m_errorString = message;

	// persistent, visible error state instead of the (possibly stale) objects
	replaceObjects({NetworkObject(NetworkObject::Type::Label, tr("Directory not available: %1").arg(message))}, rootObject());
	setObjectPopulated(rootObject());
}



void RemoteNetworkObjectDirectory::clearError()
{
	m_errorString.clear();
}



std::optional<NetworkObjectList> RemoteNetworkObjectDirectory::toObjects(const std::optional<QVariantList>& items)
{
	if (items.has_value() == false)
	{
		return {};
	}

	NetworkObjectList objects;
	objects.reserve(items->size());

	for (const auto& item : *items)
	{
		const auto object = DirectoryBrokerMessage::objectFromVariant(item);
		if (object.has_value() == false)
		{
			vWarning() << "malformed object in directory broker reply";
			return {};
		}
		objects.append(*object);
	}

	return objects;
}
