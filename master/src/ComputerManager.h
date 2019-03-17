/*
 * ComputerManager.h - maintains and provides a computer object list
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include "CheckableItemProxyModel.h"
#include "ComputerControlInterface.h"

class QHostAddress;
class NetworkObjectDirectory;
class NetworkObjectFilterProxyModel;
class NetworkObjectOverlayDataModel;
class UserConfig;

class ComputerManager : public QObject
{
	Q_OBJECT
public:
	ComputerManager( UserConfig& config, QObject* parent );
	~ComputerManager() override;

	QAbstractItemModel* networkObjectModel()
	{
		return m_networkObjectModel;
	}

	QAbstractItemModel* computerTreeModel()
	{
		return m_computerTreeModel;
	}

	ComputerList selectedComputers( const QModelIndex& parent );

	void addLocation( const QString& location );
	void removeLocation( const QString& location );

	bool saveComputerAndUsersList( const QString& fileName );

	void updateUser( const ComputerControlInterface::Pointer& controlInterface );

signals:
	void computerSelectionReset();
	void computerSelectionChanged();

private:
	void checkChangedData( const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles );

	void initLocations();
	void initNetworkObjectLayer();
	void initComputerTreeModel();
	void updateLocationFilterList();

	QString findLocationOfComputer( const QStringList& hostNames, const QList<QHostAddress>& hostAddresses, const QModelIndex& parent );

	ComputerList getComputersAtLocation( const QString& locationName, const QModelIndex& parent = QModelIndex() );

	QModelIndex findNetworkObject( NetworkObject::Uid networkObjectUid, const QModelIndex& parent = QModelIndex() );

	QModelIndex mapToUserNameModelIndex( const QModelIndex& networkObjectIndex );

	static constexpr int OverlayDataUsernameColumn = 1;

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

};
