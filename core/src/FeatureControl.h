/*
 * FeatureControl.h - declaration of FeatureControl class
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

#include "FeatureProviderInterface.h"

class VEYON_CORE_EXPORT FeatureControl : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	enum class Argument
	{
		ActiveFeaturesList
	};
	Q_ENUM(Argument)

	explicit FeatureControl( QObject* parent = nullptr );
	~FeatureControl() override = default;

	void queryActiveFeatures( const ComputerControlInterfaceList& computerControlInterfaces );

	Plugin::Uid uid() const override
	{
		return QStringLiteral("f5ec79e0-186c-4af0-89a2-7e5687cc32b2");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
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

	bool controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces ) override
	{
		Q_UNUSED(featureUid)
		Q_UNUSED(operation)
		Q_UNUSED(arguments)
		Q_UNUSED(computerControlInterfaces)

		return false;
	}

	bool handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface,
							  const FeatureMessage& message ) override;

	bool handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message ) override;

private:
	enum Commands
	{
		QueryActiveFeatures,
	};

	const Feature m_featureControlFeature;
	const FeatureList m_features;

	FeatureUidList m_activeFeatures;

};
