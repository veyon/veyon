// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "UserGroupsBackendInterface.h"

class VEYON_CORE_EXPORT UserGroupsBackendManager : public QObject
{
	Q_OBJECT
public:
	using Backends = QMap<Plugin::Uid, UserGroupsBackendInterface*>;

	explicit UserGroupsBackendManager(QObject* parent = nullptr);

	const Backends& backends() const
	{
		return m_backends;
	}

	QMap<Plugin::Uid, QString> availableBackends();

	UserGroupsBackendInterface* configuredBackend();

	void reloadConfiguration();

private:
	Backends m_backends{};
	UserGroupsBackendInterface* m_defaultBackend{nullptr};
	UserGroupsBackendInterface* m_configuredBackend{nullptr};
};
