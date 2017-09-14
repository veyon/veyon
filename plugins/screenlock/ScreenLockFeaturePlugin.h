/*
 * ScreenLockFeaturePlugin.h - declaration of ScreenLockFeaturePlugin class
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

#ifndef SCREEN_LOCK_FEATURE_PLUGIN_H
#define SCREEN_LOCK_FEATURE_PLUGIN_H

#include "FeaturePluginInterface.h"

class LockWidget;

class ScreenLockFeaturePlugin : public QObject, FeaturePluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.veyon.Veyon.Plugins.PluginFeatureInterface")
	Q_INTERFACES(PluginInterface FeaturePluginInterface)
public:
	ScreenLockFeaturePlugin( QObject* parent = nullptr );
	~ScreenLockFeaturePlugin() override;

	Plugin::Uid uid() const override
	{
		return "2ad98ccb-e9a5-43ef-8c4c-876ac5efbcb1";
	}

	QString version() const override
	{
		return QStringLiteral("1.0");
	}

	QString name() const override
	{
		return QStringLiteral("ScreenLock");
	}

	QString description() const override
	{
		return tr( "Lock screen and input devices of a computer" );
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
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
	enum Commands
	{
		StartLockCommand,
		StopLockCommand,
		CommandCount
	};

	Feature m_screenLockFeature;
	FeatureList m_features;

	LockWidget* m_lockWidget;

};

#endif // SCREEN_LOCK_FEATURE_PLUGIN_H
