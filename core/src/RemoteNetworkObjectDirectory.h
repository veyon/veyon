// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QSet>

#include "NetworkObjectDirectory.h"

#include <memory>

class DirectoryBrokerClient;

// Network object directory of components that must not run a directory requiring secrets
// (04-directory-broker.md §3): the model is filled from the directory broker (update() and
// fetchObjects() asynchronously, the query functions synchronously). While the broker is not
// available the root contains a single label object describing the error, the query functions
// return empty results and the try…() variants std::nullopt (fail closed).
class VEYON_CORE_EXPORT RemoteNetworkObjectDirectory : public NetworkObjectDirectory
{
	Q_OBJECT
public:
	RemoteNetworkObjectDirectory(const std::shared_ptr<DirectoryBrokerClient>& client, QObject* parent);

	QVariant queryObjectAttribute(NetworkObject::Uid objectUid, NetworkObject::Attribute attribute) override;
	NetworkObjectList queryObjects(NetworkObject::Type type,
								   NetworkObject::Attribute attribute, const QVariant& value) override;
	NetworkObjectList queryParents(const NetworkObject& child) override;

	std::optional<NetworkObjectList> tryQueryObjects(NetworkObject::Type type,
													 NetworkObject::Attribute attribute, const QVariant& value) override;
	std::optional<NetworkObjectList> tryQueryParents(const NetworkObject& child) override;

	void update() override;
	void fetchObjects(const NetworkObject& object) override;

	// empty while the broker answers
	const QString& errorString() const
	{
		return m_errorString;
	}

	int generation() const
	{
		return m_generation;
	}

private:
	void setError(const QString& errorString);
	void clearError();

	static std::optional<NetworkObjectList> toObjects(const std::optional<QVariantList>& items);

	const std::shared_ptr<DirectoryBrokerClient> m_client;

	bool m_updatePending = false;
	QSet<NetworkObject::Uid> m_pendingFetches;
	QString m_errorString;
	int m_generation = -1;
};
