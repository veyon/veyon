/*
 * MonitoringMode.h - header for the MonitoringMode class
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

#ifndef MONITORING_MODE_H
#define MONITORING_MODE_H

#include "FeaturePluginInterface.h"

class MonitoringMode : public QObject, public FeaturePluginInterface
{
	Q_OBJECT
public:
	MonitoringMode();

	const Feature& feature() const
	{
		return m_monitoringModeFeature;
	}

	Plugin::Uid uid() const override
	{
		return "1b08265b-348f-4978-acaa-45d4f6b90bd9";
	}

	QString version() const override
	{
		return "1.0";
	}

	QString name() const override
	{
		return "MonitoringMode";
	}

	QString description() const override
	{
		return tr( "Builtin monitoring mode" );
	}

	QString vendor() const override
	{
		return "iTALC Community";
	}

	QString copyright() const override
	{
		return "Tobias Doerffel";
	}

	const FeatureList& featureList() const override;

	bool startMasterFeature( const Feature& feature,
	                         const ComputerControlInterfaceList& computerControlInterfaces,
	                         QWidget* parent ) override;

	bool stopMasterFeature( const Feature& feature,
	                        const ComputerControlInterfaceList& computerControlInterfaces,
	                        QWidget* parent ) override;

	bool handleServiceFeatureMessage( const FeatureMessage& message,
	                                  QIODevice* ioDevice,
	                                  FeatureWorkerManager& featureWorkerManager ) override;

	bool handleWorkerFeatureMessage( const FeatureMessage& message, QIODevice* ioDevice ) override;

private:
	Feature m_monitoringModeFeature;
	FeatureList m_features;

};

#endif // MONITORING_MODE_H
