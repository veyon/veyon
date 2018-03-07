/*
 * ComputerManager.h - maintains and provides a computer object list
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@users.sf.net>
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

#include "ComputerControlInterface.h"
#include "CheckableItemProxyModel.h"

class QHostAddress;
class BuiltinFeatures;
class FeatureManager;
class NetworkObjectDirectory;
class NetworkObjectFilterProxyModel;
class NetworkObjectOverlayDataModel;
class UserConfig;

class ComputerManager : public QObject
{
	Q_OBJECT
public:
	ComputerManager( UserConfig& config,
					 FeatureManager& featureManager,
					 BuiltinFeatures& builtinFeatures,
					 QObject* parent );
	~ComputerManager() override;

	QAbstractItemModel* networkObjectModel()
	{
		return m_networkObjectModel;
	}

	QAbstractItemModel* computerTreeModel()
	{
		return m_computerTreeModel;
	}

	const ComputerControlInterfaceList& computerControlInterfaces() const
	{
		return m_computerControlInterfaces;
	}

	void updateComputerScreenSize();

	void addRoom( const QString& room );
	void removeRoom( const QString& room );

	bool saveComputerAndUsersList( const QString& fileName );

signals:
	void computerListAboutToBeReset();
	void computerListReset();

	void computerAboutToBeInserted( int index );
	void computerInserted();

	void computerAboutToBeRemoved( int index );
	void computerRemoved();

	void computerScreenUpdated( int index );
	void activeFeaturesOfComputerChanged( int index );

public slots:
	void reloadComputerList();
	void updateComputerList();

	void updateComputerScreens();

private:
	void initRooms();
	void initNetworkObjectLayer();
	void initComputerTreeModel();
	void updateRoomFilterList();
	QString findRoomOfComputer( const QStringList& hostNames, const QList<QHostAddress>& hostAddresses, const QModelIndex& parent );

	ComputerList getComputersInRoom( const QString& roomName, const QModelIndex& parent = QModelIndex() );

	ComputerList getCheckedComputers( const QModelIndex& parent );
	QSize computerScreenSize() const;

	void startComputerControlInterface( ComputerControlInterface::Pointer controlInterface, int index );
	void updateUser( ComputerControlInterface::Pointer controlInterface );

	QModelIndex findNetworkObject( NetworkObject::Uid networkObjectUid, const QModelIndex& parent = QModelIndex() );

	UserConfig& m_config;
	FeatureManager& m_featureManager;
	BuiltinFeatures& m_builtinFeatures;

	NetworkObjectDirectory* m_networkObjectDirectory;
	QAbstractItemModel* m_networkObjectModel;
	NetworkObjectOverlayDataModel* m_networkObjectOverlayDataModel;
	CheckableItemProxyModel* m_computerTreeModel;
	NetworkObjectFilterProxyModel* m_networkObjectFilterProxyModel;

	QStringList m_currentRooms;
	QStringList m_roomFilterList;
	ComputerControlInterfaceList m_computerControlInterfaces;

	QStringList m_localHostNames;
	QList<QHostAddress> m_localHostAddresses;

};

#endif // COMPUTER_MANAGER_H
