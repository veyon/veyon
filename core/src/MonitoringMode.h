/*
 * MonitoringMode.h - header for the MonitoringMode class
 *
 * Copyright (c) 2017-2020 Tobias Junghans <tobydox@veyon.io>
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

class MonitoringMode : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	explicit MonitoringMode( QObject* parent = nullptr );

	const Feature& feature() const
	{
		return m_monitoringModeFeature;
	}

	Plugin::Uid uid() const override
	{
		return QStringLiteral("1a6a59b1-c7a1-43cc-bcab-c136a4d91be8");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 2 );
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

	void queryLoggedOnUserInfo( const ComputerControlInterfaceList& computerControlInterfaces );

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
	void queryUserInformation();

	const Feature m_monitoringModeFeature;
	const Feature m_queryLoggedOnUserInfoFeature;
	const FeatureList m_features;

	enum class Argument
	{
		UserLoginName,
		UserFullName,
		UserSessionId,
	};

	QReadWriteLock m_userDataLock;
	QString m_userLoginName;
	QString m_userFullName;
	int m_userSessionId{0};

};
