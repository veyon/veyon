/*
 * VeyonMaster.h - global instances
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QObject>

#include "Feature.h"
#include "Computer.h"
#include "ComputerControlInterface.h"
#include "VeyonMasterInterface.h"

class QModelIndex;

class BuiltinFeatures;
class ComputerControlListModel;
class ComputerManager;
class ComputerMonitoringModel;
class FeatureManager;
class MainWindow;
class UserConfig;

class VeyonMaster : public VeyonMasterInterface
{
	Q_OBJECT
public:
	explicit VeyonMaster( QObject* parent = nullptr );
	~VeyonMaster() override;

	FeatureManager& featureManager()
	{
		return *m_featureManager;
	}

	UserConfig& userConfig()
	{
		return *m_userConfig;
	}

	ComputerManager& computerManager()
	{
		return *m_computerManager;
	}

	ComputerControlListModel& computerControlListModel()
	{
		return *m_computerControlListModel;
	}

	ComputerMonitoringModel* computerMonitoringModel() const
	{
		return m_computerMonitoringModel;
	}

	const FeatureList& features() const
	{
		return m_features;
	}

	FeatureList subFeatures( Feature::Uid parentFeatureUid ) const;
	FeatureUidList subFeaturesUids( Feature::Uid parentFeatureUid ) const;
	FeatureUidList metaFeaturesUids( const FeatureUidList& featureUids ) const;

	FeatureList modeFeatures() const;

	const Feature::Uid& currentMode() const
	{
		return m_currentMode;
	}

	QWidget* mainWindow() override;
	Configuration::Object* userConfigurationObject() override;
	void reloadSubFeatures() override;

	ComputerControlInterface& localSessionControlInterface() override
	{
		return m_localSessionControlInterface;
	}

	ComputerControlInterfaceList selectedComputerControlInterfaces() const override;

	ComputerControlInterfaceList filteredComputerControlInterfaces();

public Q_SLOTS:
	void runFeature( const Feature& feature );
	void enforceDesignatedMode( const QModelIndex& index );
	void stopAllModeFeatures( const ComputerControlInterfaceList& computerControlInterfaces );

private:
	FeatureList featureList() const;

	UserConfig* m_userConfig;
	FeatureManager* m_featureManager;
	const FeatureList m_features;
	ComputerManager* m_computerManager;
	ComputerControlListModel* m_computerControlListModel;
	ComputerMonitoringModel* m_computerMonitoringModel;

	ComputerControlInterface m_localSessionControlInterface;

	MainWindow* m_mainWindow;

	Feature::Uid m_currentMode;

} ;
