// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "LdapConfiguration.h"
#include "LdapDirectory.h"
#include "LdapNetworkObjectDirectory.h"


LdapNetworkObjectDirectory::LdapNetworkObjectDirectory(const LdapConfiguration& ldapConfiguration,
													   QObject* parent) :
	NetworkObjectDirectory(parent),
	m_ldapDirectory(ldapConfiguration)
{
}



NetworkObjectList LdapNetworkObjectDirectory::queryObjects(NetworkObject::Type type,
														   NetworkObject::Attribute attribute, const QVariant& value)
{
	switch (type)
	{
	case NetworkObject::Type::Location: return queryLocations(attribute, value);
	case NetworkObject::Type::Host: return queryHosts(attribute, value);
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
			const auto parentDn = LdapClient::parentDn(object.directoryAddress());
			if (parentDn.isEmpty() == false)
			{
				const NetworkObject parentObject(NetworkObject::Type::Location, parentDn, QString(), QString(), parentDn);
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
			const auto locations = m_ldapDirectory.locationsOfComputer(object.directoryAddress());
			const NetworkObject parentObject(NetworkObject::Type::Location, locations.value(0));
			parents.append(parentObject);
		}
	}

	return parents;
}



std::optional<NetworkObjectList> LdapNetworkObjectDirectory::tryQueryObjects(NetworkObject::Type type,
																			 NetworkObject::Attribute attribute,
																			 const QVariant& value)
{
	m_ldapDirectory.client().resetQueryStatus();

	const auto objects = queryObjects(type, attribute, value);
	if (m_ldapDirectory.client().hasQueryFailed())
	{
		vWarning() << "LDAP query for objects failed";
		return {};
	}

	return objects;
}



std::optional<NetworkObjectList> LdapNetworkObjectDirectory::tryQueryParents(const NetworkObject& child)
{
	m_ldapDirectory.client().resetQueryStatus();

	const auto parents = queryParents(child);
	if (m_ldapDirectory.client().hasQueryFailed())
	{
		vWarning() << "LDAP query for parents failed";
		return {};
	}

	return parents;
}



void LdapNetworkObjectDirectory::update()
{
	m_ldapDirectory.client().resetQueryStatus();

	updateObjects(rootObject());

	setUpdateFailed(m_ldapDirectory.client().hasQueryFailed());
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
		m_ldapDirectory.client().resetQueryStatus();

		updateObjects(parent);

		setUpdateFailed(m_ldapDirectory.client().hasQueryFailed());
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
			addOrUpdateObject(NetworkObject{NetworkObject::Type::Location, locationName}, parent);
		}

		removeObjects(parent, [&locationNames](const NetworkObject& object) { return object.type() == NetworkObject::Type::Location && locationNames.contains(object.name()) == false; });
	}
	else
	{
		const auto computerDns = m_ldapDirectory.computerLocationEntries(parent.name());

		for (const auto& computerDn : std::as_const(computerDns))
		{
			const auto hostObject = computerToObject(&m_ldapDirectory, computerDn);
			if (hostObject.type() == NetworkObject::Type::Host)
			{
				addOrUpdateObject(hostObject, parent);
			}
		}

		removeObjects(parent, [&computerDns](const NetworkObject& object) { return object.type() == NetworkObject::Type::Host &&
																				computerDns.contains(object.directoryAddress()) == false; });
	}
}



NetworkObjectList LdapNetworkObjectDirectory::queryLocations(NetworkObject::Attribute attribute, const QVariant& value)
{
	QString name;

	switch (attribute)
	{
	case NetworkObject::Attribute::None:
		break;

	case NetworkObject::Attribute::Name:
		name = value.toString();
		break;

	default:
		vCritical() << "Can't query locations by attribute" << attribute;
		return {};
	}

	const auto locations = m_ldapDirectory.computerLocations(name);

	NetworkObjectList locationObjects;
	locationObjects.reserve(locations.size());

	for (const auto& location : locations)
	{
		locationObjects.append(NetworkObject(NetworkObject::Type::Location, location));
	}

	return locationObjects;
}



NetworkObjectList LdapNetworkObjectDirectory::queryHosts(NetworkObject::Attribute attribute, const QVariant& value)
{
	QString filterAttribute;
	QString filterValue;

	switch (attribute)
	{
	case NetworkObject::Attribute::None:
		filterAttribute = m_ldapDirectory.computerHostNameAttribute();
		break;

	case NetworkObject::Attribute::Name:
		filterAttribute = m_ldapDirectory.computerDisplayNameAttribute();
		filterValue = value.toString();
		break;

	case NetworkObject::Attribute::HostAddress:
		filterAttribute = m_ldapDirectory.computerHostNameAttribute();
		filterValue = m_hostNameResolutionEnabled ? m_ldapDirectory.hostToLdapFormat(value.toString()) : m_ldapDirectory.hostNameToLdapFormat(value.toString());
		if (filterValue.isEmpty())
		{
			return {};
		}
		break;

	default:
		vCritical() << "Can't query hosts by attribute" << attribute;
		return {};
	}

	// one search with all attributes instead of one search per computer
	const auto computerAttributes = this->computerAttributes(&m_ldapDirectory);
	const auto computers = m_ldapDirectory.client().queryObjects(m_ldapDirectory.computersDn(), computerAttributes,
																 LdapClient::constructQueryFilter(filterAttribute, filterValue,
																								  m_ldapDirectory.computersFilter()),
																 m_ldapDirectory.computerSearchScope());

	NetworkObjectList hostObjects;
	hostObjects.reserve(computers.size());

	for (auto it = computers.constBegin(), end = computers.constEnd(); it != end; ++it)
	{
		const auto hostObject = computerToObject(&m_ldapDirectory, it.key(), it.value());
		if (hostObject.isValid())
		{
			hostObjects.append(hostObject);
		}
	}

	return hostObjects;
}



void LdapNetworkObjectDirectory::updateLocations(const NetworkObject& parent)
{
	auto baseDn = parent.directoryAddress();
	if (parent.type() == NetworkObject::Type::Root)
	{
		baseDn = m_ldapDirectory.computersDn();
	}

	const auto locations = m_ldapDirectory.client().queryObjects(baseDn, {m_ldapDirectory.locationNameAttribute()},
																 m_ldapDirectory.computerContainersFilter(), LdapClient::Scope::One);
	for (auto it = locations.begin(), end = locations.end(); it != end; ++it)
	{
		for (const auto& locationName : std::as_const(it.value().first()))
		{
			addOrUpdateObject(NetworkObject{
								  NetworkObject::Type::Location, locationName, QString(), QString(), it.key()},
							  parent);
		}
	}

	const auto locationDNs = locations.keys();

	removeObjects(parent, [locationDNs](const NetworkObject& object) { return object.type() == NetworkObject::Type::Location &&
																		   locationDNs.contains(object.directoryAddress()) == false; });
}



void LdapNetworkObjectDirectory::updateComputers(const NetworkObject& parent)
{
	auto baseDn = parent.directoryAddress();
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

		addOrUpdateObject(NetworkObject{NetworkObject::Type::Host, displayName, hostName, macAddress, it.key()}, parent);

		computerDns.append(it.key());
	}

	removeObjects(parent, [computerDns](const NetworkObject& object) {
		return object.type() == NetworkObject::Type::Host &&
			computerDns.contains(object.directoryAddress()) == false;
	});
}



NetworkObject LdapNetworkObjectDirectory::computerToObject(LdapDirectory* directory, const QString& computerDn)
{
	const auto computers = directory->client().queryObjects(computerDn, computerAttributes(directory),
															directory->computersFilter(), LdapClient::Scope::Base);
	if (computers.isEmpty() == false)
	{
		return computerToObject(directory, computers.firstKey(), computers.first());
	}

	return NetworkObject{NetworkObject::Type::None};
}



NetworkObject LdapNetworkObjectDirectory::computerToObject(LdapDirectory* directory, const QString& computerDn,
														   const QMap<QString, QStringList>& attributes)
{
	const auto macAddressAttribute = directory->computerMacAddressAttribute();

	return NetworkObject{NetworkObject::Type::Host,
						 attributes.value(directory->computerDisplayNameAttribute()).value(0),
						 attributes.value(computerHostNameAttribute(directory)).value(0),
						 macAddressAttribute.isEmpty() ? QString() : attributes.value(macAddressAttribute).value(0),
						 computerDn};
}



QString LdapNetworkObjectDirectory::computerHostNameAttribute(LdapDirectory* directory)
{
	const auto hostNameAttribute = directory->computerHostNameAttribute();
	return hostNameAttribute.isEmpty() ? LdapClient::cn() : hostNameAttribute;
}



QStringList LdapNetworkObjectDirectory::computerAttributes(LdapDirectory* directory)
{
	QStringList attributes{directory->computerDisplayNameAttribute(), computerHostNameAttribute(directory)};

	const auto macAddressAttribute = directory->computerMacAddressAttribute();
	if (macAddressAttribute.isEmpty() == false)
	{
		attributes.append(macAddressAttribute);
	}

	attributes.removeDuplicates();

	return attributes;
}
