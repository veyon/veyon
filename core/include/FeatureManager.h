/*
 * FeatureManager.h - header for the FeatureManager class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef FEATURE_MANAGER_H
#define FEATURE_MANAGER_H

#include <QObject>

#include "Feature.h"
#include "FeaturePluginInterface.h"
#include "ComputerControlInterface.h"
#include "Plugin.h"

class QWidget;

class FeatureWorkerManager;
class PluginManager;

class ITALC_CORE_EXPORT FeatureManager : public QObject
{
	Q_OBJECT
public:
	FeatureManager( PluginManager& pluginManager );

	const FeatureList& features() const
	{
		return m_features;
	}

	const FeatureList& features( const Plugin::Uid& pluginUid ) const;

	const Feature& feature( const Feature::Uid& featureUid ) const;

	Plugin::Uid pluginUid( const Feature& feature ) const;

	void startMasterFeature( const Feature& feature,
							 const ComputerControlInterfaceList& computerControlInterfaces,
							 ComputerControlInterface& localComputerControlInterface,
							 QWidget* parent );
	void stopMasterFeature( const Feature& feature,
							const ComputerControlInterfaceList& computerControlInterfaces,
							ComputerControlInterface& localComputerControlInterface,
							QWidget* parent );
	bool handleServiceFeatureMessage( const FeatureMessage& message, FeatureWorkerManager& featureWorkerManager );
	bool handleWorkerFeatureMessage( const FeatureMessage& message );

private:
	PluginManager& m_pluginManager;
	FeatureList m_features;
	FeatureList m_emptyFeatureList;
	FeaturePluginInterfaceList m_featureInterfaces;
	Feature m_dummyFeature;


};

#endif // FEATURE_MANAGER_H
