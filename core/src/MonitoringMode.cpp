/*
 * MonitoringMode.cpp - implementation of MonitoringMode class
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
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

#include <QtConcurrent>
#include <QGuiApplication>
#include <QScreen>

#include "FeatureManager.h"
#include "MonitoringMode.h"
#include "PlatformUserFunctions.h"
#include "VeyonConfiguration.h"
#include "VeyonServerInterface.h"


MonitoringMode::MonitoringMode( QObject* parent ) :
	QObject( parent ),
	m_monitoringModeFeature( QLatin1String( staticMetaObject.className() ),
							 Feature::Flag::Mode | Feature::Flag::Master | Feature::Flag::Builtin,
							 Feature::Uid( "edad8259-b4ef-4ca5-90e6-f238d0fda694" ),
							 Feature::Uid(),
							 tr( "Monitoring" ), tr( "Monitoring" ),
							 tr( "This mode allows you to monitor all computers at one or more locations." ),
							 QStringLiteral( ":/core/presentation-none.png" ) ),
	m_queryApplicationVersionFeature( QStringLiteral("QueryApplicationVersion"),
									  Feature::Flag::Service | Feature::Flag::Builtin,
									  Feature::Uid{"58f5d5d5-9929-48f4-a995-f221c150ae26"}, {},
									  tr("Query application version of the server"), {}, {} ),
	m_queryActiveFeatures( QStringLiteral("QueryActiveFeatures"),
						   Feature::Flag::Service | Feature::Flag::Builtin,
						   Feature::Uid{"a0a96fba-425d-414a-aaf4-352b76d7c4f3"}, {},
						   tr("Query active features"), {}, {} ),
	m_queryUserInfoFeature(QStringLiteral("UserInfo"),
						   Feature::Flag::Session | Feature::Flag::Service | Feature::Flag::Builtin,
						   Feature::Uid("79a5e74d-50bd-4aab-8012-0e70dc08cc72"),
						   Feature::Uid(), {}, {}, {} ),
	m_querySessionInfoFeature(QStringLiteral("SessionInfo"),
							  Feature::Flag::Session | Feature::Flag::Service | Feature::Flag::Builtin,
							  Feature::Uid("699ed9dd-f58b-477b-a0af-df8105571b3c"),
							  Feature::Uid(), {}, {}, {}),
	m_queryScreensFeature( QStringLiteral("QueryScreens"),
						   Feature::Flag::Meta,
						   Feature::Uid("d5bbc486-7bc5-4c36-a9a8-1566c8b0091a"),
						   Feature::Uid(), tr("Query properties of remotely available screens"), {}, {} ),
	m_features({ m_monitoringModeFeature, m_queryApplicationVersionFeature, m_queryActiveFeatures,
			   m_queryUserInfoFeature, m_querySessionInfoFeature, m_queryScreensFeature}),
	m_sessionMetaDataContent(VeyonCore::config().sessionMetaDataContent()),
	m_sessionMetaDataEnvironmentVariable(VeyonCore::config().sessionMetaDataEnvironmentVariable()),
	m_sessionMetaDataRegistryKey(VeyonCore::config().sessionMetaDataRegistryKey())
{
	if(VeyonCore::component() == VeyonCore::Component::Server)
	{
		connect(&m_activeFeaturesUpdateTimer, &QTimer::timeout, this, &MonitoringMode::updateActiveFeatures);
		m_activeFeaturesUpdateTimer.start(ActiveFeaturesUpdateInterval);

		connect(&m_sessionInfoUpdateTimer, &QTimer::timeout, this, &MonitoringMode::updateSessionInfo);
		m_sessionInfoUpdateTimer.start(SessionInfoUpdateInterval);

		updateUserInfo();
		updateSessionInfo();
		updateScreenInfoList();

		connect(qGuiApp, &QGuiApplication::screenAdded, this, &MonitoringMode::updateScreenInfoList);
		connect(qGuiApp, &QGuiApplication::screenRemoved, this, &MonitoringMode::updateScreenInfoList);
	}
}



void MonitoringMode::ping(const ComputerControlInterfaceList& computerControlInterfaces)
{
	sendFeatureMessage(FeatureMessage{m_monitoringModeFeature.uid(), FeatureCommand::Ping}, computerControlInterfaces);
}



void MonitoringMode::setMinimumFramebufferUpdateInterval(const ComputerControlInterfaceList& computerControlInterfaces,
														 int interval)
{
	sendFeatureMessage(FeatureMessage{m_monitoringModeFeature.uid(), FeatureCommand::SetMinimumFramebufferUpdateInterval}
					   .addArgument(Argument::MinimumFramebufferUpdateInterval, interval),
					   computerControlInterfaces);
}



void MonitoringMode::queryApplicationVersion(const ComputerControlInterfaceList& computerControlInterfaces)
{
	sendFeatureMessage(FeatureMessage{m_queryApplicationVersionFeature.uid()}, computerControlInterfaces);
}



void MonitoringMode::queryActiveFeatures(const ComputerControlInterfaceList& computerControlInterfaces)
{
	sendFeatureMessage(FeatureMessage{m_queryActiveFeatures.uid()}, computerControlInterfaces);
}



void MonitoringMode::queryUserInfo(const ComputerControlInterfaceList& computerControlInterfaces)
{
	sendFeatureMessage(FeatureMessage{m_queryUserInfoFeature.uid()}, computerControlInterfaces);
}



void MonitoringMode::querySessionInfo(const ComputerControlInterfaceList& computerControlInterfaces)
{
	sendFeatureMessage(FeatureMessage{m_querySessionInfoFeature.uid()}, computerControlInterfaces);
}



void MonitoringMode::queryScreens(const ComputerControlInterfaceList& computerControlInterfaces)
{
	sendFeatureMessage(FeatureMessage{m_queryScreensFeature.uid()}, computerControlInterfaces);
}



bool MonitoringMode::handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface,
										   const FeatureMessage& message )
{
	if (message.featureUid() == m_monitoringModeFeature.uid())
	{
		if (message.command<FeatureCommand>() == FeatureCommand::Ping)
		{
			// successful ping reply implicitly handled through the featureMessageReceived() signal
			return true;
		}
	}

	if (message.featureUid() == m_queryApplicationVersionFeature.uid())
	{
		computerControlInterface->setServerVersion(message.argument(Argument::ApplicationVersion)
												   .value<VeyonCore::ApplicationVersion>());
		return true;
	}

	if( message.featureUid() == m_queryActiveFeatures.uid() )
	{
		const auto featureUidStrings = message.argument(Argument::ActiveFeaturesList).toStringList();

		FeatureUidList activeFeatures{};
		activeFeatures.reserve(featureUidStrings.size());

		for(const auto& featureUidString : featureUidStrings)
		{
			activeFeatures.append(Feature::Uid{featureUidString});
		}

		computerControlInterface->setActiveFeatures(activeFeatures);

		return true;
	}

	if( message.featureUid() == m_queryUserInfoFeature.uid() )
	{
		computerControlInterface->setUserInformation(message.argument( Argument::UserLoginName ).toString(),
													 message.argument( Argument::UserFullName ).toString());

		return true;
	}

	if (message.featureUid() == m_querySessionInfoFeature.uid())
	{
		computerControlInterface->setSessionInfo(PlatformSessionFunctions::SessionInfo{
													 message.argument(Argument::SessionId).toInt(),
													 message.argument(Argument::SessionUptime).toInt(),
													 message.argument(Argument::SessionClientAddress).toString(),
													 message.argument(Argument::SessionClientName).toString(),
													 message.argument(Argument::SessionHostName).toString(),
													 message.argument(Argument::SessionMetaData).toString(),
												 });

		return true;
	}

	if( message.featureUid() == m_queryScreensFeature.uid() )
	{
		const auto screenInfoList = message.argument(Argument::ScreenInfoList).toList();

		ComputerControlInterface::ScreenList screens;
		screens.reserve(screenInfoList.size());

		for(int i = 0; i < screenInfoList.size(); ++i)
		{
			const auto screenInfo = screenInfoList.at(i).toMap();
			ComputerControlInterface::ScreenProperties screenProperties;
			screenProperties.index = i + 1;
			screenProperties.name = screenInfo.value(QStringLiteral("name")).toString();
			screenProperties.geometry = screenInfo.value(QStringLiteral("geometry")).toRect();
			screens.append(screenProperties);
		}

		computerControlInterface->setScreens(screens);
	}

	return false;
}



bool MonitoringMode::handleFeatureMessage(VeyonServerInterface& server,
										  const MessageContext& messageContext,
										  const FeatureMessage& message)
{
	if (message.featureUid() == m_monitoringModeFeature.uid())
	{
		if (message.command<FeatureCommand>() == FeatureCommand::Ping)
		{
			return server.sendFeatureMessageReply(messageContext, message);
		}

		if (message.command<FeatureCommand>() == FeatureCommand::SetMinimumFramebufferUpdateInterval)
		{
			server.setMinimumFramebufferUpdateInterval(messageContext,
													   message.argument(Argument::MinimumFramebufferUpdateInterval).toInt());
			return true;
		}
	}

	if (message.featureUid() == m_queryApplicationVersionFeature.uid())
	{
		server.sendFeatureMessageReply(messageContext,
									   FeatureMessage{m_queryApplicationVersionFeature.uid()}
									   .addArgument(Argument::ApplicationVersion, int(VeyonCore::config().applicationVersion())));
	}

	if (m_queryActiveFeatures.uid() == message.featureUid())
	{
		return sendActiveFeatures(server, messageContext);
	}

	if (message.featureUid() == m_queryUserInfoFeature.uid())
	{
		return sendUserInformation(server, messageContext);
	}

	if (message.featureUid() == m_querySessionInfoFeature.uid())
	{
		return sendSessionInfo(server, messageContext);
	}

	if (message.featureUid() == m_queryScreensFeature.uid())
	{
		return sendScreenInfoList(server, messageContext);
	}

	return false;
}



void MonitoringMode::sendAsyncFeatureMessages(VeyonServerInterface& server, const MessageContext& messageContext)
{
	const auto activeFeaturesVersion = messageContext.ioDevice()->property(activeFeaturesVersionProperty()).toInt();

	if (activeFeaturesVersion != m_activeFeaturesVersion)
	{
		sendActiveFeatures(server, messageContext);
		messageContext.ioDevice()->setProperty(activeFeaturesVersionProperty(), m_activeFeaturesVersion);
	}

	const auto currentUserInfoVersion = m_userInfoVersion.loadAcquire();
	const auto contextUserInfoVersion = messageContext.ioDevice()->property(userInfoVersionProperty()).toInt();

	if(contextUserInfoVersion != currentUserInfoVersion)
	{
		sendUserInformation(server, messageContext);
		messageContext.ioDevice()->setProperty(userInfoVersionProperty(), currentUserInfoVersion);
	}

	const auto currentSessionInfoVersion = m_sessionInfoVersion.loadAcquire();
	const auto sessionInfoVersion = messageContext.ioDevice()->property(sessionInfoVersionProperty()).toInt();

	if (sessionInfoVersion != currentSessionInfoVersion)
	{
		sendSessionInfo(server, messageContext);
		messageContext.ioDevice()->setProperty(sessionInfoVersionProperty(), currentSessionInfoVersion);
	}

	const auto screenInfoVersion = messageContext.ioDevice()->property(screenInfoListVersionProperty()).toInt();

	if (screenInfoVersion != m_screenInfoListVersion)
	{
		sendScreenInfoList(server, messageContext);
		messageContext.ioDevice()->setProperty(screenInfoListVersionProperty(), m_screenInfoListVersion);
	}
}



bool MonitoringMode::sendActiveFeatures(VeyonServerInterface& server, const MessageContext& messageContext)
{
	return server.sendFeatureMessageReply(messageContext,
										  FeatureMessage{m_queryActiveFeatures.uid()}
										  .addArgument(Argument::ActiveFeaturesList, m_activeFeatures));
}



bool MonitoringMode::sendUserInformation(VeyonServerInterface& server, const MessageContext& messageContext)
{
	FeatureMessage message{m_queryUserInfoFeature.uid()};

	m_userDataLock.lockForRead();
	if (m_userLoginName.isEmpty())
	{
		updateUserInfo();
		message.addArgument(Argument::UserLoginName, QString{});
		message.addArgument(Argument::UserFullName, QString{});
	}
	else
	{
		message.addArgument(Argument::UserLoginName, m_userLoginName);
		message.addArgument(Argument::UserFullName, m_userFullName);
	}
	m_userDataLock.unlock();

	return server.sendFeatureMessageReply(messageContext, message);
}



bool MonitoringMode::sendSessionInfo(VeyonServerInterface& server, const MessageContext& messageContext)
{
	FeatureMessage message{m_querySessionInfoFeature.uid()};

	m_sessionInfoLock.lockForRead();
	message.addArgument(Argument::SessionId, m_sessionInfo.id);
	message.addArgument(Argument::SessionUptime, m_sessionInfo.uptime);
	message.addArgument(Argument::SessionClientAddress, m_sessionInfo.clientAddress);
	message.addArgument(Argument::SessionClientName, m_sessionInfo.clientName);
	message.addArgument(Argument::SessionHostName, m_sessionInfo.hostName);
	message.addArgument(Argument::SessionMetaData, m_sessionInfo.metaData);
	m_sessionInfoLock.unlock();

	return server.sendFeatureMessageReply(messageContext,message);
}



bool MonitoringMode::sendScreenInfoList(VeyonServerInterface& server, const MessageContext& messageContext)
{
	return server.sendFeatureMessageReply(messageContext,
										  FeatureMessage{m_queryScreensFeature.uid()}
										  .addArgument(Argument::ScreenInfoList, m_screenInfoList));
}



void MonitoringMode::updateActiveFeatures()
{
	const auto server = VeyonCore::instance()->findChild<VeyonServerInterface *>();
	if (server)
	{
		const auto activeFeaturesUids = VeyonCore::featureManager().activeFeatures(*server);

		QStringList activeFeatures;
		activeFeatures.reserve(activeFeaturesUids.size());

		for (const auto& activeFeatureUid : activeFeaturesUids)
		{
			activeFeatures.append(activeFeatureUid.toString());
		}

		if (activeFeatures != m_activeFeatures)
		{
			m_activeFeatures = activeFeatures;
			m_activeFeaturesVersion++;
		}
	}
}



void MonitoringMode::updateUserInfo()
{
	// asynchronously query information about logged on user (which might block
	// due to domain controller queries and timeouts etc.)
	(void) QtConcurrent::run( [=]() {
		if( VeyonCore::platform().sessionFunctions().currentSessionHasUser() )
		{
			const auto userLoginName = VeyonCore::platform().userFunctions().currentUser();
			const auto userFullName = VeyonCore::platform().userFunctions().fullName( userLoginName );
			m_userDataLock.lockForWrite();
			if(m_userLoginName != userLoginName ||
			   m_userFullName != userFullName)
			{
				m_userLoginName = userLoginName;
				m_userFullName = userFullName;
				++m_userInfoVersion;
			}
			m_userDataLock.unlock();
		}

		m_userDataLock.lockForRead();
		if (m_userLoginName.isEmpty() && m_userFullName.isEmpty())
		{
			QTimer::singleShot(UserInfoUpdateRetryInterval, this, &MonitoringMode::updateUserInfo);
		}
		m_userDataLock.unlock();
	} );
}



void MonitoringMode::updateSessionInfo()
{
	(void) QtConcurrent::run([=]() {
		QString sessionMetaData;
		switch (m_sessionMetaDataContent)
		{
		case PlatformSessionFunctions::SessionMetaDataContent::EnvironmentVariable:
			sessionMetaData = VeyonCore::platform().sessionFunctions().currentSessionEnvironmentVariables().value(m_sessionMetaDataEnvironmentVariable);
			break;
		case PlatformSessionFunctions::SessionMetaDataContent::RegistryKey:
			sessionMetaData = VeyonCore::platform().sessionFunctions().querySettingsValueInCurrentSession(m_sessionMetaDataRegistryKey).toString();
			break;
		case PlatformSessionFunctions::SessionMetaDataContent::None:
			break;
		}

		const PlatformSessionFunctions::SessionInfo currentSessionInfo{
			VeyonCore::sessionId(),
					VeyonCore::platform().sessionFunctions().currentSessionUptime(),
					VeyonCore::platform().sessionFunctions().currentSessionClientAddress(),
					VeyonCore::platform().sessionFunctions().currentSessionClientName(),
					VeyonCore::platform().sessionFunctions().currentSessionHostName(),
					sessionMetaData
		};

		m_sessionInfoLock.lockForWrite();
		if (currentSessionInfo != m_sessionInfo)
		{
			m_sessionInfo = currentSessionInfo;
			++m_sessionInfoVersion;
		}
		m_sessionInfoLock.unlock();
	});
}




void MonitoringMode::updateScreenInfoList()
{
	const auto screens = QGuiApplication::screens();

	QVariantList screenInfoList;
	screenInfoList.reserve(screens.size());

	int index = 1;
	for(const auto* screen : screens)
	{
		QVariantMap screenInfo;
		screenInfo[QStringLiteral("name")] = VeyonCore::screenName(*screen, index);
		screenInfo[QStringLiteral("geometry")] = screen->geometry();
		screenInfoList.append(screenInfo);
		++index;

		connect(screen, &QScreen::geometryChanged, this, &MonitoringMode::updateScreenInfoList, Qt::UniqueConnection);
	}

	if(screenInfoList != m_screenInfoList)
	{
		m_screenInfoList = screenInfoList;
		++m_screenInfoListVersion;
	}
}
