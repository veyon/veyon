/*
 * ScreenLockFeaturePlugin.h - declaration of ScreenLockFeaturePlugin class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "FeatureProviderInterface.h"

class LockWidget;

class ScreenLockFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.PluginFeatureInterface")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	ScreenLockFeaturePlugin( QObject* parent = nullptr );
	~ScreenLockFeaturePlugin() override;

	Plugin::Uid uid() const override
	{
		return QStringLiteral("2ad98ccb-e9a5-43ef-8c4c-876ac5efbcb1");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
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

	bool startFeature( VeyonMasterInterface& master, const Feature& feature,
					   const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool stopFeature( VeyonMasterInterface& master, const Feature& feature,
					  const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool handleFeatureMessage( VeyonMasterInterface& master, const FeatureMessage& message,
							   ComputerControlInterface::Pointer computerControlInterface ) override;

	bool handleFeatureMessage( VeyonServerInterface& server, const FeatureMessage& message ) override;

	bool handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message ) override;

private:
	enum Commands
	{
		StartLockCommand,
		StopLockCommand,
		CommandCount
	};

	const Feature m_screenLockFeature;
	const FeatureList m_features;

	LockWidget* m_lockWidget;

};

#endif // SCREEN_LOCK_FEATURE_PLUGIN_H
