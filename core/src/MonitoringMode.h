/*
 * MonitoringMode.h - header for the MonitoringMode class
 *
 * Copyright (c) 2017-2022 Tobias Junghans <tobydox@veyon.io>
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

#include <QTimer>

#include "FeatureProviderInterface.h"
#include "PlatformSessionFunctions.h"

class VEYON_CORE_EXPORT MonitoringMode : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	enum class Argument
	{
		UserLoginName,
		UserFullName,
		SessionId,
		ScreenInfoList,
		MinimumFramebufferUpdateInterval,
		ApplicationVersion,
		SessionUptime,
		SessionHostName,
		SessionClientAddress,
		SessionClientName,
		ActiveFeaturesList = 0 // for compatibility after migration from FeatureControl
	};
	Q_ENUM(Argument)

	explicit MonitoringMode( QObject* parent = nullptr );

	const Feature& feature() const
	{
		return m_monitoringModeFeature;
	}

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("1a6a59b1-c7a1-43cc-bcab-c136a4d91be8") };
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

	void ping(const ComputerControlInterfaceList& computerControlInterfaces);

	void setMinimumFramebufferUpdateInterval(const ComputerControlInterfaceList& computerControlInterfaces,
											 int interval);

	void queryApplicationVersion(const ComputerControlInterfaceList& computerControlInterfaces);

	void queryActiveFeatures(const ComputerControlInterfaceList& computerControlInterfaces);

	void queryUserInfo(const ComputerControlInterfaceList& computerControlInterfaces);

	void querySessionInfo(const ComputerControlInterfaceList& computerControlInterfaces);

	void queryScreens( const ComputerControlInterfaceList& computerControlInterfaces );

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

	void sendAsyncFeatureMessages(VeyonServerInterface& server, const MessageContext& messageContext) override;

private:
	bool sendActiveFeatures(VeyonServerInterface& server, const MessageContext& messageContext);
	bool sendUserInformation(VeyonServerInterface& server, const MessageContext& messageContext);
	bool sendSessionInfo(VeyonServerInterface& server, const MessageContext& messageContext);
	bool sendScreenInfoList(VeyonServerInterface& server, const MessageContext& messageContext);

	static const char* activeFeaturesVersionProperty()
	{
		return "activeFeaturesListVersion";
	}

	static const char* userInfoVersionProperty()
	{
		return "userInfoVersion";
	}

	static const char* sessionInfoVersionProperty()
	{
		return "sessionInfoVersion";
	}

	static const char* screenInfoListVersionProperty()
	{
		return "screenInfoListVersion";
	}

	void updateActiveFeatures();
	void updateUserData();
	void updateSessionInfo();
	void updateScreenInfoList();

	enum Command
	{
		Ping,
		SetMinimumFramebufferUpdateInterval
	};

	static constexpr int ActiveFeaturesUpdateInterval = 250;
	static constexpr int SessionInfoUpdateInterval = 1000;

	const Feature m_monitoringModeFeature;
	const Feature m_queryApplicationVersionFeature;
	const Feature m_queryActiveFeatures;
	const Feature m_queryUserInfoFeature;
	const Feature m_querySessionInfoFeature;
	const Feature m_queryScreensFeature;
	const FeatureList m_features;

	int m_activeFeaturesVersion{0};
	QStringList m_activeFeatures;
	QTimer m_activeFeaturesUpdateTimer;

	QReadWriteLock m_userDataLock;
	QString m_userLoginName;
	QString m_userFullName;
	QAtomicInt m_userInfoVersion{0};

	QVariantList m_screenInfoList;
	int m_screenInfoListVersion{0};

	QReadWriteLock m_sessionInfoLock;
	PlatformSessionFunctions::SessionInfo m_sessionInfo{};
	QAtomicInt m_sessionInfoVersion = 0;
	QTimer m_sessionInfoUpdateTimer;

};
