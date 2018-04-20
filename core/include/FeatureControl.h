/*
 * FeatureControl.h - declaration of FeatureControl class
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

#ifndef FEATURE_CONTROL_H
#define FEATURE_CONTROL_H

#include "FeatureProviderInterface.h"

class VEYON_CORE_EXPORT FeatureControl : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	FeatureControl( QObject* parent = nullptr );
	~FeatureControl() override;

	bool queryActiveFeatures( const ComputerControlInterfaceList& computerControlInterfaces );

	Plugin::Uid uid() const override
	{
		return QStringLiteral("f5ec79e0-186c-4af0-89a2-7e5687cc32b2");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral( "FeatureControl" );
	}

	QString description() const override
	{
		return tr( "Feature control" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "Veyon Community" );
	}

	QString copyright() const override
	{
		return QStringLiteral( "Tobias Junghans" );
	}

	const FeatureList& featureList() const override
	{
		return m_features;
	}

	bool startMasterFeature( const Feature& feature,
							 const ComputerControlInterfaceList& computerControlInterfaces,
							 QWidget* parent ) override;

	bool stopMasterFeature( const Feature& feature,
							const ComputerControlInterfaceList& computerControlInterfaces,
							QWidget* parent ) override;

	bool handleMasterFeatureMessage( const FeatureMessage& message,
									 ComputerControlInterface::Pointer computerControlInterface ) override;

	bool handleServiceFeatureMessage( const FeatureMessage& message,
									  FeatureWorkerManager& featureWorkerManager ) override;

	bool handleWorkerFeatureMessage( const FeatureMessage& message ) override;

private:
	enum Commands
	{
		QueryActiveFeatures,
	};

	enum Arguments
	{
		ActiveFeatureList,
	};

	Feature m_featureControlFeature;
	FeatureList m_features;

	FeatureUidList m_activeFeatures;

};

#endif // FEATURE_CONTROL_H
