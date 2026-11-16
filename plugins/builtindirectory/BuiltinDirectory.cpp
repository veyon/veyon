// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QJsonArray>
#include <QJsonObject>

#include "BuiltinDirectory.h"
#include "BuiltinDirectoryConfiguration.h"


BuiltinDirectory::BuiltinDirectory(BuiltinDirectoryConfiguration& configuration, QObject* parent) :
	NetworkObjectDirectory(parent),
	m_configuration(configuration)
{
}



void BuiltinDirectory::update()
{
	m_configuration.reloadFromStore();

	const auto networkObjects = m_configuration.networkObjects();

	NetworkObjectUidList groupUids;

	for (const auto& networkObjectValue : networkObjects)
	{
		const NetworkObject networkObject(networkObjectValue.toObject());

		if (networkObject.type() == NetworkObject::Type::Location)
		{
			groupUids.append(networkObject.uid()); // clazy:exclude=reserve-candidates

			addOrUpdateObject(networkObject, rootObject());

			updateLocation(networkObject, networkObjects);
		}
	}

	removeObjects(rootObject(), [groupUids](const NetworkObject& object) {
		return object.type() == NetworkObject::Type::Location && groupUids.contains(object.uid()) == false;
	});
}



void BuiltinDirectory::updateLocation(const NetworkObject& locationObject, const QJsonArray& networkObjects)
{
	NetworkObjectUidList computerUids;

	for (const auto& networkObjectValue : networkObjects)
	{
		NetworkObject networkObject(networkObjectValue.toObject());

		if (networkObject.parentUid() == locationObject.uid())
		{
			computerUids.append(networkObject.uid()); // clazy:exclude=reserve-candidates
			addOrUpdateObject(networkObject, locationObject);
		}
	}

	removeObjects(locationObject, [computerUids](const NetworkObject& object) { return object.type() == NetworkObject::Type::Host && computerUids.contains(object.uid()) == false; });
}
