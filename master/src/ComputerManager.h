// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CheckableItemProxyModel.h"
#include "ComputerControlInterface.h"
#include "NetworkObjectDirectory.h"
#include "PlatformUserFunctions.h"

class QHostAddress;
class NetworkObjectDirectory;
class NetworkObjectFilterProxyModel;
class NetworkObjectOverlayDataModel;
class UserConfig;

class ComputerManager : public QObject
{
	Q_OBJECT
public:
	ComputerManager(UserConfig& config, QObject* parent);
	~ComputerManager() override;

	QAbstractItemModel* networkObjectModel() const
	{
		return m_networkObjectModel;
	}

	QAbstractItemModel* computerTreeModel() const
	{
		return m_computerTreeModel;
	}

	ComputerList selectedComputers(const QModelIndex& parent) const;

	void addLocation(const QString& location);
	void removeLocation(const QString& location);

	bool saveComputerAndUsersList(const QString& fileName);

	void updateUser(const ComputerControlInterface::Pointer& controlInterface) const;
	void updateSessionInfo(const ComputerControlInterface::Pointer& controlInterface) const;
	void clearOverlayModelData(const ComputerControlInterface::Pointer& controlInterface) const;

Q_SIGNALS:
	void computerSelectionReset();
	void computerSelectionChanged();

private:
	void checkChangedData(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);

	void initLocations();
	void initNetworkObjectLayer();
	void initComputerTreeModel();
	void updateLocationFilterList();

	QString findLocationOfComputer(const QStringList& hostNames, const QList<QHostAddress>& hostAddresses,
								   const QModelIndex& parent) const;

	ComputerList getComputersAtLocation(const QString& locationName, const QModelIndex& parent = {}, bool parentMatches = false) const;
	bool hasSubLocations(const QModelIndex& index) const;

	QModelIndex findNetworkObject(NetworkObject::Uid networkObjectUid, const QModelIndex& parent = {}) const;

	QModelIndex mapToUserNameModelIndex(const QModelIndex& networkObjectIndex) const;
	QModelIndex mapToSessionUptimeModelIndex(const QModelIndex& networkObjectIndex) const;

	static constexpr int OverlayDataUsernameColumn = 1;
	static constexpr int OverlayDataSessionUptimeColumn = 2;

	UserConfig& m_config;

	NetworkObjectDirectory* m_networkObjectDirectory;
	QAbstractItemModel* m_networkObjectModel;
	NetworkObjectOverlayDataModel* m_networkObjectOverlayDataModel;
	CheckableItemProxyModel* m_computerTreeModel;
	NetworkObjectFilterProxyModel* m_networkObjectFilterProxyModel;

	QStringList m_currentLocations;
	QStringList m_locationFilterList;

	QStringList m_localHostNames;
	QList<QHostAddress> m_localHostAddresses;

	const PlatformUserFunctions::UserProperty m_guestUserProperty;
	const QString m_guestUserValue;
};
