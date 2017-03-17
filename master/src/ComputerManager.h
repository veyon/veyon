/*
 * ComputerManager.h - maintains and provides a computer object list
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#include "Computer.h"
#include "CheckableItemProxyModel.h"

class QHostAddress;
class BuiltinFeatures;
class FeatureManager;
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

	ComputerList& computerList()
	{
		return m_computerList;
	}

	const ComputerList& computerList() const
	{
		return m_computerList;
	}

	ComputerControlInterfaceList computerControlInterfaces();

	void updateComputerScreenSize();

	void addRoom( const QString& room );

	bool saveComputerAndUsersList( const QString& fileName );

signals:
	void computerListAboutToBeReset();
	void computerListReset();

	void computerAboutToBeInserted( int index );
	void computerInserted();

	void computerAboutToBeRemoved( int index );
	void computerRemoved();

	void computerScreenUpdated( int index );
	void computerScreenSizeChanged();

public slots:
	void reloadComputerList();
	void updateComputerList();

	void updateComputerScreens();

private:
	void initRooms();
	void initNetworkObjectLayer();
	void initComputerTreeModel();
	void updateRoomFilterList();
	QString findRoomOfComputer( const QHostAddress& hostAddress, const QModelIndex& parent );

	ComputerList getComputersInRoom( const QString& roomName, const QModelIndex& parent = QModelIndex() );

	ComputerList getCheckedComputers( const QModelIndex& parent );
	QSize computerScreenSize() const;

	void startComputerControlInterface( Computer& computer );
	void updateUser( Computer& computer );
	QModelIndex findNetworkObject( const NetworkObject::Uid& networkObjectUid, const QModelIndex& parent = QModelIndex() );

	UserConfig& m_config;
	FeatureManager& m_featureManager;
	BuiltinFeatures& m_builtinFeatures;

	QAbstractItemModel* m_networkObjectModel;
	NetworkObjectOverlayDataModel* m_networkObjectOverlayDataModel;
	CheckableItemProxyModel* m_computerTreeModel;
	NetworkObjectFilterProxyModel* m_networkObjectFilterProxyModel;

	QStringList m_currentRooms;
	QStringList m_roomFilterList;
	ComputerList m_computerList;

};

#endif // COMPUTER_MANAGER_H
