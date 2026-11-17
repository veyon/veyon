// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "LdapDirectory.h"
#include "NetworkObjectDirectory.h"

class LdapNetworkObjectDirectory : public NetworkObjectDirectory
{
	Q_OBJECT
public:
	LdapNetworkObjectDirectory(const LdapConfiguration& ldapConfiguration, QObject* parent);

	NetworkObjectList queryObjects(NetworkObject::Type type,
								   NetworkObject::Attribute attribute, const QVariant& value) override;
	NetworkObjectList queryParents(const NetworkObject& childId) override;

	// std::nullopt if the LDAP server is not available (no "empty" results for access control)
	std::optional<NetworkObjectList> tryQueryObjects(NetworkObject::Type type,
													 NetworkObject::Attribute attribute, const QVariant& value) override;
	std::optional<NetworkObjectList> tryQueryParents(const NetworkObject& child) override;

	// veyon-service never resolves host names sent by clients (the session server sends the FQDN),
	// so that a client cannot block the directory worker with name lookups
	void setHostNameResolutionEnabled(bool enabled)
	{
		m_hostNameResolutionEnabled = enabled;
	}

	static NetworkObject computerToObject(LdapDirectory* directory, const QString& computerDn);
	static NetworkObject computerToObject(LdapDirectory* directory, const QString& computerDn,
										  const QMap<QString, QStringList>& attributes);

private:
	void update() override;
	void fetchObjects(const NetworkObject& parent) override;

	void updateObjects(const NetworkObject& parent);

	NetworkObjectList queryLocations(NetworkObject::Attribute attribute, const QVariant& value);
	NetworkObjectList queryHosts(NetworkObject::Attribute attribute, const QVariant& value);

	static QString computerHostNameAttribute(LdapDirectory* directory);
	static QStringList computerAttributes(LdapDirectory* directory);

	void updateLocations(const NetworkObject& parent);
	void updateComputers(const NetworkObject& parent);

	LdapDirectory m_ldapDirectory;
	bool m_hostNameResolutionEnabled = true;
};
