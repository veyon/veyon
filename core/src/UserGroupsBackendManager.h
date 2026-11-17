// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "UserGroupsBackendInterface.h"

#include <memory>

class RemoteUserGroupsBackend;

class VEYON_CORE_EXPORT UserGroupsBackendManager : public QObject
{
	Q_OBJECT
public:
	using Backends = QMap<Plugin::Uid, UserGroupsBackendInterface*>;

	explicit UserGroupsBackendManager(QObject* parent = nullptr);
	~UserGroupsBackendManager() override;

	const Backends& backends() const
	{
		return m_backends;
	}

	QMap<Plugin::Uid, QString> availableBackends();

	// in this process or broker-backed, depending on DirectoryBroker/Mode and the component
	// (04-directory-broker.md §2)
	UserGroupsBackendInterface* configuredBackend();

	// always the backend of this process (e.g. for the directory broker service)
	UserGroupsBackendInterface* configuredLocalBackend() const;

	void reloadConfiguration();

private:
	Backends m_backends{};
	UserGroupsBackendInterface* m_defaultBackend{nullptr};
	UserGroupsBackendInterface* m_configuredBackend{nullptr};
	std::unique_ptr<RemoteUserGroupsBackend> m_remoteBackend;
};
