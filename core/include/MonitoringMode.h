/*
 * MonitoringMode.h - header for the MonitoringMode class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#ifndef MONITORING_MODE_H
#define MONITORING_MODE_H

#include "FeaturePluginInterface.h"

class MonitoringMode : public QObject, FeaturePluginInterface, PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeaturePluginInterface PluginInterface)
public:
	MonitoringMode( QObject* parent = nullptr );

	const Feature& feature() const
	{
		return m_monitoringModeFeature;
	}

	Plugin::Uid uid() const override
	{
		return "1a6a59b1-c7a1-43cc-bcab-c136a4d91be8";
	}

	QString version() const override
	{
		return QStringLiteral( "1.0" );
	}

	QString name() const override
	{
		return QStringLiteral( "MonitoringMode" );
	}

	QString description() const override
	{
		return tr( "Builtin monitoring mode" );
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
							 ComputerControlInterface& localComputerControlInterface,
							 QWidget* parent ) override;

	bool stopMasterFeature( const Feature& feature,
	                        const ComputerControlInterfaceList& computerControlInterfaces,
							ComputerControlInterface& localComputerControlInterface,
							QWidget* parent ) override;

	bool handleMasterFeatureMessage( const FeatureMessage& message,
									 ComputerControlInterface& computerControlInterface ) override;

	bool handleServiceFeatureMessage( const FeatureMessage& message,
	                                  FeatureWorkerManager& featureWorkerManager ) override;

	bool handleWorkerFeatureMessage( const FeatureMessage& message ) override;

private:
	Feature m_monitoringModeFeature;
	FeatureList m_features;

};

#endif // MONITORING_MODE_H
