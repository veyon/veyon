/*
 * UserSessionControlPlugin.h - declaration of UserSessionControlPlugin class
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QReadWriteLock>

#include "FeatureProviderInterface.h"

class UserSessionControlPlugin : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.UserSessionControl")
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	enum class Argument
	{
		Username,
		Password
	};
	Q_ENUM(Argument)

	explicit UserSessionControlPlugin( QObject* parent = nullptr );
	~UserSessionControlPlugin() override = default;

	Plugin::Uid uid() const override
	{
		return QStringLiteral("80580500-2e59-4297-9e35-e53959b028cd");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 2 );
	}

	QString name() const override
	{
		return QStringLiteral( "UserSessionControl" );
	}

	QString description() const override
	{
		return tr( "User session control" );
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

	bool controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool startFeature( VeyonMasterInterface& master, const Feature& feature,
					   const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message ) override;

private:
	bool confirmFeatureExecution( const Feature& feature, bool all, QWidget* parent );

	const Feature m_userLoginFeature;
	const Feature m_userLogoffFeature;
	const FeatureList m_features;

};
