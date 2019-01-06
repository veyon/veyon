/*
 * ComputerManager.h - maintains and provides a computer object list
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
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

#ifndef COMPUTER_MANAGER_H
#define COMPUTER_MANAGER_H

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

	void addRoom( const QString& room );
	void removeRoom( const QString& room );

	bool saveComputerAndUsersList( const QString& fileName );

	void updateUser( const ComputerControlInterface::Pointer& controlInterface );

signals:
	void computerSelectionReset();
	void computerSelectionChanged();

private slots:
	void checkChangedData( const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles );

private:
	void initRooms();
	void initNetworkObjectLayer();
	void initComputerTreeModel();
	void updateRoomFilterList();

	QString findRoomOfComputer( const QStringList& hostNames, const QList<QHostAddress>& hostAddresses, const QModelIndex& parent );

	ComputerList getComputersInRoom( const QString& roomName, const QModelIndex& parent = QModelIndex() );

	QModelIndex findNetworkObject( NetworkObject::Uid networkObjectUid, const QModelIndex& parent = QModelIndex() );

	QModelIndex mapToUserNameModelIndex( const QModelIndex& networkObjectIndex );

	static const int OverlayDataColumnUsername = 1;

	UserConfig& m_config;

	NetworkObjectDirectory* m_networkObjectDirectory;
	QAbstractItemModel* m_networkObjectModel;
	NetworkObjectOverlayDataModel* m_networkObjectOverlayDataModel;
	CheckableItemProxyModel* m_computerTreeModel;
	NetworkObjectFilterProxyModel* m_networkObjectFilterProxyModel;

	QStringList m_currentRooms;
	QStringList m_roomFilterList;

	QStringList m_localHostNames;
	QList<QHostAddress> m_localHostAddresses;

};

#endif // COMPUTER_MANAGER_H
