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

#include <QSortFilterProxyModel>

#include "Computer.h"
#include "CheckableItemProxyModel.h"
#include "Feature.h"

class FeatureManager;
class PersonalConfig;

class ComputerManager : public QObject
{
	Q_OBJECT
public:
	ComputerManager( PersonalConfig& config, QObject* parent );
	~ComputerManager() override;

	QAbstractItemModel* networkObjectModel()
	{
		return m_checkableNetworkObjectProxyModel;
	}

	ComputerList& computerList()
	{
		return m_computerList;
	}

	const ComputerList& computerList() const
	{
		return m_computerList;
	}

	void updateComputerScreenSize();

	const Feature::Uid& currentMode() const
	{
		return m_currentMode;
	}

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
	void runFeature( FeatureManager& featureManager, const Feature& feature, QWidget* parent );

	void reloadComputerList();
	void updateComputerList();

	void updateComputerScreens();

private:
	ComputerList getComputers( const QModelIndex& parent );
	QSize computerScreenSize() const;

	PersonalConfig& m_config;

	CheckableItemProxyModel* m_checkableNetworkObjectProxyModel;
	QSortFilterProxyModel* m_networkObjectSortProxyModel;

	ComputerList m_computerList;

	Feature::Uid m_currentMode;

};

#endif // COMPUTER_MANAGER_H
