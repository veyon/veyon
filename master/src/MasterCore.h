/*
 * MasterCore.h - global instances
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

#ifndef MASTER_CORE_H
#define MASTER_CORE_H

#include <QObject>

#include "Feature.h"
#include "Computer.h"
#include "ComputerControlInterface.h"

class QModelIndex;

class BuiltinFeatures;
class ComputerControlListModel;
class ComputerManager;
class FeatureManager;
class UserConfig;

class MasterCore : public QObject
{
	Q_OBJECT
public:
	MasterCore( QObject* parent = nullptr );
	~MasterCore() override;

	BuiltinFeatures& builtinFeatures()
	{
		return *m_builtinFeatures;
	}

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

	const FeatureList& features() const
	{
		return m_features;
	}

	const Feature::Uid& currentMode() const
	{
		return m_currentMode;
	}


public slots:
	void runFeature( const Feature& feature, QWidget* parent );
	void shutdownComputerControlInterface( const QModelIndex& index );
	void enforceDesignatedMode( const QModelIndex& index );
	void stopAllModeFeatures( const ComputerControlInterfaceList& computerControlInterfaces, QWidget* parent );

private:
	FeatureList featureList() const;

	BuiltinFeatures* m_builtinFeatures;
	FeatureManager* m_featureManager;
	const FeatureList m_features;
	UserConfig* m_userConfig;
	ComputerManager* m_computerManager;
	ComputerControlListModel* m_computerControlListModel;

	Feature::Uid m_currentMode;

} ;

#endif
