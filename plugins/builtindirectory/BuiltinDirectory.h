// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "NetworkObjectDirectory.h"

class BuiltinDirectoryConfiguration;

class BuiltinDirectory : public NetworkObjectDirectory
{
	Q_OBJECT
public:
	BuiltinDirectory(BuiltinDirectoryConfiguration& configuration, QObject* parent);

	void update() override;

private:
	void updateLocation(const NetworkObject& locationObject, const QJsonArray& networkObjects);

	BuiltinDirectoryConfiguration& m_configuration;
};
