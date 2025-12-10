// Copyright (c) 2019-2025 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "LdapConfiguration.h"
#include "LdapDirectory.h"
#include "LdapNetworkObjectDirectory.h"


LdapNetworkObjectDirectory::LdapNetworkObjectDirectory(const LdapConfiguration& ldapConfiguration,
													   QObject* parent) :
	NetworkObjectDirectory(ldapConfiguration.directoryName(), parent),
	m_ldapDirectory(ldapConfiguration)
{
}



NetworkObjectList LdapNetworkObjectDirectory::queryObjects(NetworkObject::Type type,
														   NetworkObject::Property property, const QVariant& value)
{
	switch (type)
	{
	case NetworkObject::Type::Location: return queryLocations(property, value);
	case NetworkObject::Type::Host: return queryHosts(property, value);
	default: break;
	}

	return {};
}



NetworkObjectList LdapNetworkObjectDirectory::queryParents(const NetworkObject& object)
{
	NetworkObjectList parents;

	if (object.type() == NetworkObject::Type::Host || object.type() == NetworkObject::Type::Location)
	{
		if (m_ldapDirectory.computerLocationsByContainer())
		{
			const auto parentDn = LdapClient::parentDn(object.property(NetworkObject::Property::DirectoryAddress).toString());
			if (parentDn.isEmpty() == false)
			{
				const NetworkObject parentObject{this, NetworkObject::Type::Location, parentDn, {
						{ NetworkObject::propertyKey(NetworkObject::Property::DirectoryAddress), parentDn}
					}};
				parents.append(parentObject);

				// computer tree root DN not yet reached?
				if (QString::compare(parentDn, m_ldapDirectory.computersDn(), Qt::CaseInsensitive) != 0)
				{
					parents.append(queryParents(parentObject));
				}
			}
		}
		else if (object.type() == NetworkObject::Type::Host)
		{
			const auto locations = m_ldapDirectory.locationsOfComputer(object.property(NetworkObject::Property::DirectoryAddress).toString());
			const NetworkObject parentObject(this, NetworkObject::Type::Location, locations.value(0));
			parents.append(parentObject);
		}
	}

	return parents;
}



void LdapNetworkObjectDirectory::update()
{
	updateObjects(rootObject());

	setObjectPopulated(rootObject());
}



void LdapNetworkObjectDirectory::fetchObjects(const NetworkObject& parent)
{
	if (parent.type() == NetworkObject::Type::Root)
	{
		update();
	}
	else if (parent.type() != NetworkObject::Type::Location)
	{
		setObjectPopulated(parent);
	}
	else
	{
		updateObjects(parent);

		setObjectPopulated(parent);
	}
}



void LdapNetworkObjectDirectory::updateObjects(const NetworkObject& parent)
{
	if (m_ldapDirectory.computerLocationsByContainer() && m_ldapDirectory.mapContainerStructureToLocations())
	{
		updateLocations(parent);
		updateComputers(parent);
	}
	else if (parent.type() == NetworkObject::Type::Root)
	{
		const auto locationNames = m_ldapDirectory.computerLocations();

		for (const auto& locationName : std::as_const(locationNames))
		{
			addOrUpdateObject(NetworkObject{this, NetworkObject::Type::Location, locationName}, parent);
		}

		removeObjects(parent, [&locationNames](const NetworkObject& object) {
			return object.type() == NetworkObject::Type::Location && locationNames.contains(object.name()) == false; });
	}
	else
	{
		const auto computerDns = m_ldapDirectory.computerLocationEntries(parent.name());

		for (const auto& computerDn : std::as_const(computerDns))
		{
			const auto hostObject = computerToObject(computerDn);
			if (hostObject.type() == NetworkObject::Type::Host)
			{
				addOrUpdateObject(hostObject, parent);
			}
		}

		removeObjects(parent, [&computerDns](const NetworkObject& object) {
			return object.type() == NetworkObject::Type::Host &&
					computerDns.contains(object.property(NetworkObject::Property::DirectoryAddress).toString()) == false;
		});
	}
}



NetworkObjectList LdapNetworkObjectDirectory::queryLocations(NetworkObject::Property property, const QVariant& value)
{
	QString name;

	switch (property)
	{
	case NetworkObject::Property::None:
		break;

	case NetworkObject::Property::Name:
		name = value.toString();
		break;

	default:
		vCritical() << "Can't query locations by property" << property;
		return {};
	}

	const auto locations = m_ldapDirectory.computerLocations(name);

	NetworkObjectList locationObjects;
	locationObjects.reserve( locations.size() );

	for (const auto& location : locations)
	{
		locationObjects.append(NetworkObject{this, NetworkObject::Type::Location, location});
	}

	return locationObjects;
}



NetworkObjectList LdapNetworkObjectDirectory::queryHosts(NetworkObject::Property property, const QVariant& value)
{
	QStringList computers;

	switch (property)
	{
	case NetworkObject::Property::None:
		computers = m_ldapDirectory.computersByHostName({});
		break;

	case NetworkObject::Property::Name:
		computers = m_ldapDirectory.computersByDisplayName(value.toString());
		break;

	case NetworkObject::Property::HostAddress:
	{
		const auto hostName = m_ldapDirectory.hostToLdapFormat(value.toString());
		if (hostName.isEmpty())
		{
			return {};
		}
		computers = m_ldapDirectory.computersByHostName(hostName);
		break;
	}

	default:
		vCritical() << "Can't query hosts by attribute" << property;
		return {};
	}

	NetworkObjectList hostObjects;
	hostObjects.reserve(computers.size());

	for (const auto& computer : std::as_const(computers))
	{
		const auto hostObject = computerToObject(computer);
		if (hostObject.isValid())
		{
			hostObjects.append(hostObject);
		}
	}

	return hostObjects;
}



void LdapNetworkObjectDirectory::updateLocations(const NetworkObject& parent)
{
	auto baseDn = parent.property(NetworkObject::Property::DirectoryAddress).toString();
	if (parent.type() == NetworkObject::Type::Root)
	{
		baseDn = m_ldapDirectory.computersDn();
	}

	const auto locations = m_ldapDirectory.client().queryObjects(baseDn, { m_ldapDirectory.locationNameAttribute() },
																 m_ldapDirectory.computerContainersFilter(), LdapClient::Scope::One);
	for (auto it = locations.begin(), end = locations.end(); it != end; ++it)
	{
		for (const auto& locationName : std::as_const(it.value().first()))
		{
			addOrUpdateObject(NetworkObject{
								  this, NetworkObject::Type::Location, locationName, {
									  { NetworkObject::propertyKey(NetworkObject::Property::DirectoryAddress), it.key()},
								  }
							  },
							  parent);
		}
	}

	const auto locationDNs = locations.keys();

	removeObjects(parent, [locationDNs](const NetworkObject& object) {
		return object.type() == NetworkObject::Type::Location &&
				locationDNs.contains(object.property(NetworkObject::Property::DirectoryAddress).toString()) == false; });
}



void LdapNetworkObjectDirectory::updateComputers(const NetworkObject& parent)
{
	auto baseDn = parent.property(NetworkObject::Property::DirectoryAddress).toString();
	if (parent.type() == NetworkObject::Type::Root)
	{
		baseDn = m_ldapDirectory.computersDn();
	}

	const auto displayNameAttribute = m_ldapDirectory.computerDisplayNameAttribute();
	auto hostNameAttribute = m_ldapDirectory.computerHostNameAttribute();
	if (hostNameAttribute.isEmpty())
	{
		hostNameAttribute = LdapClient::cn();
	}

	QStringList computerAttributes{displayNameAttribute, hostNameAttribute};

	const auto macAddressAttribute = m_ldapDirectory.computerMacAddressAttribute();
	if (macAddressAttribute.isEmpty() == false)
	{
		computerAttributes.append(macAddressAttribute);
	}

	computerAttributes.removeDuplicates();

	const auto computers = m_ldapDirectory.client().queryObjects(baseDn, computerAttributes,
																 m_ldapDirectory.computersFilter(), LdapClient::Scope::One);

	QStringList computerDns;

	for (auto it = computers.begin(), end = computers.end(); it != end; ++it)
	{
		const auto displayName = it.value()[displayNameAttribute].value(0);
		const auto hostName = it.value()[hostNameAttribute].value(0);
		const auto macAddress = (macAddressAttribute.isEmpty() == false) ? it.value()[macAddressAttribute].value(0) : QString();

		addOrUpdateObject(NetworkObject{this, NetworkObject::Type::Host, displayName, {
											{ NetworkObject::propertyKey(NetworkObject::Property::HostAddress), hostName},
											{ NetworkObject::propertyKey(NetworkObject::Property::MacAddress), macAddress},
											{ NetworkObject::propertyKey(NetworkObject::Property::DirectoryAddress), it.key()},
										}
						  },
						  parent);

		computerDns.append(it.key());
	}

	removeObjects(parent, [computerDns](const NetworkObject& object) {
		return object.type() == NetworkObject::Type::Host &&
				computerDns.contains(object.property(NetworkObject::Property::DirectoryAddress).toString()) == false;
	});
}



NetworkObject LdapNetworkObjectDirectory::computerToObject(const QString& computerDn)
{
	const auto displayNameAttribute = m_ldapDirectory.computerDisplayNameAttribute();

	auto hostNameAttribute = m_ldapDirectory.computerHostNameAttribute();
	if (hostNameAttribute.isEmpty())
	{
		hostNameAttribute = LdapClient::cn();
	}

	QStringList computerAttributes{displayNameAttribute, hostNameAttribute};

	auto macAddressAttribute = m_ldapDirectory.computerMacAddressAttribute();
	if (macAddressAttribute.isEmpty() == false)
	{
		computerAttributes.append(macAddressAttribute);
	}

	computerAttributes.removeDuplicates();

	const auto computers = m_ldapDirectory.client().queryObjects(computerDn, computerAttributes,
																 m_ldapDirectory.computersFilter(), LdapClient::Scope::Base);
	if (computers.isEmpty() == false)
	{
		const auto& computerDn = computers.firstKey();
		const auto& computer = computers.first();
		const auto displayName = computer[displayNameAttribute].value(0);
		const auto hostName = computer[hostNameAttribute].value(0);
		const auto macAddress = (macAddressAttribute.isEmpty() == false) ? computer[macAddressAttribute].value(0) : QString();

		return NetworkObject{this, NetworkObject::Type::Host, displayName, {
				{ NetworkObject::propertyKey(NetworkObject::Property::HostAddress), hostName},
				{ NetworkObject::propertyKey(NetworkObject::Property::MacAddress), macAddress},
				{ NetworkObject::propertyKey(NetworkObject::Property::DirectoryAddress), computerDn},
			}
		};
	}

	return NetworkObject{this, NetworkObject::Type::None};
}
