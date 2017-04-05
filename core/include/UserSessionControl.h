/*
 * UserSessionControl.h - declaration of UserSessionControl class
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

#ifndef USER_SESSION_CONTROL_H
#define USER_SESSION_CONTROL_H

#include "FeaturePluginInterface.h"

class ITALC_CORE_EXPORT UserSessionControl : public QObject, public FeaturePluginInterface, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeaturePluginInterface PluginInterface)
public:
	UserSessionControl();
	virtual ~UserSessionControl() {}

	bool getUserSessionInfo( const ComputerControlInterfaceList& computerControlInterfaces );

	Plugin::Uid uid() const override
	{
		return "80580500-2e59-4297-9e35-e53959b028cd";
	}

	QString version() const override
	{
		return "1.0";
	}

	QString name() const override
	{
		return "UserSessionControl";
	}

	QString description() const override
	{
		return tr( "User session control" );
	}

	QString vendor() const override
	{
		return "iTALC Community";
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

	bool handleMasterFeatureMessage( const FeatureMessage& message,
									 ComputerControlInterface& computerControlInterface ) override;

	bool stopMasterFeature( const Feature& feature,
							const ComputerControlInterfaceList& computerControlInterfaces,
							ComputerControlInterface& localComputerControlInterface,
							QWidget* parent ) override;

	bool handleServiceFeatureMessage( const FeatureMessage& message,
									  FeatureWorkerManager& featureWorkerManager ) override;

	bool handleWorkerFeatureMessage( const FeatureMessage& message ) override;

private:
	enum Commands
	{
		GetInfo,
		LogonUser,
		LogoutUser
	};

	enum Arguments
	{
		UserName,
		HomeDir,
	};

	Feature m_userSessionInfoFeature;
	Feature m_userLogoutFeature;
	FeatureList m_features;

};

#endif // SYSTEM_TRAY_ICON_H
