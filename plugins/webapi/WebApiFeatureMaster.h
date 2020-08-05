/*
 * WebApiFeatureMaster.h - declaration of WebApiFeatureMaster class
 *
 * Copyright (c) 2020 Tobias Junghans <tobydox@veyon.io>
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

#include "Configuration/Object.h"
#include "FeatureManager.h"
#include "VeyonMasterInterface.h"

class WebApiFeatureMaster : public VeyonMasterInterface
{
	Q_OBJECT
public:
	explicit WebApiFeatureMaster( QObject* parent = nullptr );
	~WebApiFeatureMaster() override = default;

	QWidget* mainWindow() override;
	Configuration::Object* userConfigurationObject() override;
	void reloadSubFeatures() override;

	ComputerControlInterface& localSessionControlInterface() override;

	ComputerControlInterfaceList selectedComputerControlInterfaces() const override
	{
		return {};
	}

	void registerMessageHandler( ComputerControlInterface* controlInterface );

	FeatureList availableFeatures() const;
	bool hasFeature( Feature::Uid ) const;
	void startFeature( Feature::Uid featureUid, ComputerControlInterface* controlInterface );
	void stopFeature( Feature::Uid featureUid, ComputerControlInterface* controlInterface );

private:
	FeatureManager m_featureManager;
	Configuration::Object m_dummyConfiguration{};

	ComputerControlInterface m_sessionControlInterface;

};
