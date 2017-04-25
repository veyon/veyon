/*
 * SystemTrayIcon.h - declaration of SystemTrayIcon class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef SYSTEM_TRAY_ICON_H
#define SYSTEM_TRAY_ICON_H

#include "FeaturePluginInterface.h"

class QSystemTrayIcon;

class VEYON_CORE_EXPORT SystemTrayIcon : public QObject, public FeaturePluginInterface, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeaturePluginInterface PluginInterface)
public:
	SystemTrayIcon();
	~SystemTrayIcon() override {}

	void setToolTip( const QString& toolTipText,
					 FeatureWorkerManager& featureWorkerManager );

	void showMessage( const QString& messageTitle,
					  const QString& messageText,
					  FeatureWorkerManager& featureWorkerManager );

	Plugin::Uid uid() const override
	{
		return "3cb1adb1-6b4d-4934-a641-db767df83eea";
	}

	QString version() const override
	{
		return "1.0";
	}

	QString name() const override
	{
		return "SystemTrayIcon";
	}

	QString description() const override
	{
		return tr( "System tray icon" );
	}

	QString vendor() const override
	{
		return "Veyon Community";
	}

	QString copyright() const override
	{
		return "Tobias Doerffel";
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
		SetToolTipCommand,
		ShowMessageCommand
	};

	enum Arguments
	{
		ToolTipTextArgument,
		MessageTitleArgument,
		MessageTextArgument
	};

	Feature m_systemTrayIconFeature;
	FeatureList m_features;

	QSystemTrayIcon* m_systemTrayIcon;

};

#endif // SYSTEM_TRAY_ICON_H
