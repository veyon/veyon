// Copyright (c) 2019-2026 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
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
								   NetworkObject::Property property, const QVariant& value) override;
	NetworkObjectList queryParents(const NetworkObject& childId) override;

	NetworkObject computerToObject(const QString& computerDn);

private:
	void update() override;
	void fetchObjects(const NetworkObject& parent) override;

	void updateObjects(const NetworkObject& parent);

	NetworkObjectList queryLocations(NetworkObject::Property property, const QVariant& value);
	NetworkObjectList queryHosts(NetworkObject::Property property, const QVariant& value);

	void updateLocations(const NetworkObject& parent);
	void updateComputers(const NetworkObject& parent);

	LdapDirectory m_ldapDirectory;

};
