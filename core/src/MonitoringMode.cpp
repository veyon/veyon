// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QInputDialog>
#include <QLineEdit>
#include <QPointer>
#include <QScreen>
#include <QtConcurrent>

#include "FeatureManager.h"
#include "FeatureWorkerManager.h"
#include "MonitoringMode.h"
#include "PlatformCoreFunctions.h"
#include "PlatformUserFunctions.h"
#include "UserIdentificationCache.h"
#include "VeyonConfiguration.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"


namespace {

// there's only one MonitoringMode instance per process and the cache is used in the server only
UserIdentificationCache& userIdentificationCache()
{
	static UserIdentificationCache cache;
	return cache;
}

}



MonitoringMode::MonitoringMode(QObject* parent) :
	QObject(parent),
	m_monitoringModeFeature(QLatin1String(staticMetaObject.className()),
							Feature::Flag::Mode | Feature::Flag::Master | Feature::Flag::Builtin,
							Feature::Uid("edad8259-b4ef-4ca5-90e6-f238d0fda694"),
							Feature::Uid(),
							tr("Monitoring"), tr("Monitoring"),
							tr("This mode allows you to monitor all computers at one or more locations."),
							QStringLiteral(":/core/presentation-none.png")),
	m_queryApplicationVersionFeature(QStringLiteral("QueryApplicationVersion"),
									 Feature::Flag::Service | Feature::Flag::Builtin,
									 Feature::Uid{"58f5d5d5-9929-48f4-a995-f221c150ae26"}, {},
									 tr("Query application version of the server"), {}, {}),
	m_queryActiveFeatures(QStringLiteral("QueryActiveFeatures"),
						  Feature::Flag::Service | Feature::Flag::Builtin,
						  Feature::Uid{"a0a96fba-425d-414a-aaf4-352b76d7c4f3"}, {},
						  tr("Query active features"), {}, {}),
	m_queryUserInfoFeature(QStringLiteral("UserInfo"),
						   Feature::Flag::Session | Feature::Flag::Service | Feature::Flag::Builtin,
						   Feature::Uid("79a5e74d-50bd-4aab-8012-0e70dc08cc72"),
						   Feature::Uid(), {}, {}, {}),
	m_querySessionInfoFeature(QStringLiteral("SessionInfo"),
							  Feature::Flag::Session | Feature::Flag::Service | Feature::Flag::Builtin,
							  Feature::Uid("699ed9dd-f58b-477b-a0af-df8105571b3c"),
							  Feature::Uid(), {}, {}, {}),
	m_queryScreensFeature(QStringLiteral("QueryScreens"),
						  Feature::Flag::Meta,
						  Feature::Uid("d5bbc486-7bc5-4c36-a9a8-1566c8b0091a"),
						  Feature::Uid(), tr("Query properties of remotely available screens"), {}, {}),
	m_identifyUserFeature(QStringLiteral("IdentifyUser"),
						  Feature::Flag::Action | Feature::Flag::Service | Feature::Flag::Builtin,
						  Feature::Uid("7310707d-3918-460d-a949-b060bcc4074e"),
						  Feature::Uid(),
						  tr("Identify users in guest sessions"), {}, {}),
	m_sessionMetaDataContent(VeyonCore::config().sessionMetaDataContent()),
	m_sessionMetaDataEnvironmentVariable(VeyonCore::config().sessionMetaDataEnvironmentVariable()),
	m_sessionMetaDataRegistryKey(VeyonCore::config().sessionMetaDataRegistryKey()),
	m_guestUserIdentityExtensionType(VeyonCore::config().guestUserIdentityExtensionType()),
	m_guestUserIdentityExtensionValue(VeyonCore::config().guestUserIdentityExtensionValue())
{
	if (VeyonCore::component() == VeyonCore::Component::Server)
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



void MonitoringMode::identifyUser(const ComputerControlInterfaceList& computerControlInterfaces)
{
	sendFeatureMessage(FeatureMessage{m_identifyUserFeature.uid()}, computerControlInterfaces);
}



bool MonitoringMode::handleFeatureMessage(ComputerControlInterface::Pointer computerControlInterface,
										  const FeatureMessage& message)
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

	if (message.featureUid() == m_queryActiveFeatures.uid())
	{
		const auto featureUidStrings = message.argument(Argument::ActiveFeaturesList).toStringList();

		FeatureUidList activeFeatures{};
		activeFeatures.reserve(featureUidStrings.size());

		for (const auto& featureUidString : featureUidStrings)
		{
			activeFeatures.append(Feature::Uid{featureUidString});
		}

		computerControlInterface->setActiveFeatures(activeFeatures);

		return true;
	}

	if (message.featureUid() == m_queryUserInfoFeature.uid())
	{
		computerControlInterface->setUserInformation(message.argument(Argument::UserLoginName).toString(),
													 message.argument(Argument::UserFullName).toString());

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

	if (message.featureUid() == m_queryScreensFeature.uid())
	{
		const auto screenInfoList = message.argument(Argument::ScreenInfoList).toList();

		ComputerControlInterface::ScreenList screens;
		screens.reserve(screenInfoList.size());

		for (int i = 0; i < screenInfoList.size(); ++i)
		{
			const auto screenInfo = screenInfoList.at(i).toMap();
			ComputerControlInterface::ScreenProperties screenProperties;
			screenProperties.index = i + 1;
			screenProperties.name = screenInfo.value(QStringLiteral("name")).toString();
			screenProperties.geometry = screenInfo.value(QStringLiteral("geometry")).toRect();
			screens.append(screenProperties);
		}

		computerControlInterface->setScreens(screens);
		return true;
	}

	if (message.featureUid() == m_identifyUserFeature.uid())
	{
		auto userIdentity = message.argument(Argument::UserIdentity).toString();
		switch (m_guestUserIdentityExtensionType)
		{
		case GuestUserIdentityExtensionType::Prefix:
			userIdentity.prepend(m_guestUserIdentityExtensionValue);
			break;
		case GuestUserIdentityExtensionType::Suffix:
			userIdentity.append(m_guestUserIdentityExtensionValue);
			break;
		case GuestUserIdentityExtensionType::None:
			break;
		}

		computerControlInterface->setUserInformation(userIdentity, userIdentity);
		return true;
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
		return server.sendFeatureMessageReply(messageContext,
											  FeatureMessage{m_queryApplicationVersionFeature.uid()}
												  .addArgument(Argument::ApplicationVersion, int(VeyonCore::config().applicationVersion())));
	}

	if (message.featureUid() == m_queryActiveFeatures.uid())
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

	if (message.featureUid() == m_identifyUserFeature.uid())
	{
		// arguments of the message are ignored on purpose - the identity is only ever set by the
		// session's user through the dialog of the session worker
		auto& cache = userIdentificationCache();
		switch (cache.addRequest(messageContext, sessionUserLoginName(),
								 server.featureWorkerManager().isWorkerRunning(m_identifyUserFeature.uid())))
		{
		case UserIdentificationCache::RequestResult::Cached:
			return server.sendFeatureMessageReply(messageContext,
												  FeatureMessage{m_identifyUserFeature.uid()}
													  .addArgument(Argument::UserIdentity, cache.identity()));
		case UserIdentificationCache::RequestResult::StartDialog:
			server.featureWorkerManager().sendMessageToUnmanagedSessionWorker(
				FeatureMessage{m_identifyUserFeature.uid()}
					.addArgument(Argument::UserIdentificationContextId, cache.requestId()));
			break;
		case UserIdentificationCache::RequestResult::Pending:
			break;
		}
		return true;
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

	if (contextUserInfoVersion != currentUserInfoVersion)
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



bool MonitoringMode::handleFeatureMessageFromWorker(VeyonServerInterface& server, const FeatureMessage& message)
{
	if (message.featureUid() == m_identifyUserFeature.uid() &&
		message.hasArgument(Argument::UserIdentificationContextId))
	{
		server.featureWorkerManager().stopWorker(m_identifyUserFeature.uid());

		const auto userIdentity = message.argument(Argument::UserIdentity).toString().trimmed();

		// answer all masters which asked while the dialog was open - closed connections have
		// ioDevice=nullptr so sendFeatureMessageReply() will do nothing for them
		const auto requests = userIdentificationCache().complete(userIdentity, sessionUserLoginName());
		for (const auto& request : requests)
		{
			server.sendFeatureMessageReply(request,
										   FeatureMessage{m_identifyUserFeature.uid()}
											   .addArgument(Argument::UserIdentificationContextId,
															message.argument(Argument::UserIdentificationContextId))
											   .addArgument(Argument::UserIdentity, userIdentity));
		}

		return true;
	}

	return false;
}


bool MonitoringMode::handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message)
{
	if (message.featureUid() != m_identifyUserFeature.uid())
	{
		return false;
	}

	// the server only sends one request at a time but may send it again (e.g. if the worker could not
	// be started at first) - never stack dialogs as one answer is sufficient for all requests
	static bool dialogOpen = false;
	if (dialogOpen)
	{
		return true;
	}

	const auto contextId = message.argument(Argument::UserIdentificationContextId);

	dialogOpen = true;
	const auto userIdentity = queryUserIdentity();
	dialogOpen = false;

	return worker.sendFeatureMessageReply(FeatureMessage{m_identifyUserFeature.uid()}
											  .addArgument(Argument::UserIdentificationContextId, contextId)
											  .addArgument(Argument::UserIdentity, userIdentity));
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

	return server.sendFeatureMessageReply(messageContext, message);
}



bool MonitoringMode::sendScreenInfoList(VeyonServerInterface& server, const MessageContext& messageContext)
{
	return server.sendFeatureMessageReply(messageContext,
										  FeatureMessage{m_queryScreensFeature.uid()}
											  .addArgument(Argument::ScreenInfoList, m_screenInfoList));
}



void MonitoringMode::updateActiveFeatures()
{
	const auto server = VeyonCore::instance()->findChild<VeyonServerInterface*>();
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
	const auto guard = QPointer<MonitoringMode>(this);
	(void)QtConcurrent::run([guard]() {
		if (!guard)
		{
			return;
		}

		if (VeyonCore::platform().sessionFunctions().currentSessionHasUser())
		{
			const auto userLoginName = VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::LoginName);
			const auto userFullName = VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::FullName);

			if (!guard)
			{
				return;
			}

			guard->m_userDataLock.lockForWrite();
			if (guard->m_userLoginName != userLoginName ||
				guard->m_userFullName != userFullName)
			{
				guard->m_userLoginName = userLoginName;
				guard->m_userFullName = userFullName;
				++guard->m_userInfoVersion;
			}
			guard->m_userDataLock.unlock();
		}

		if (!guard)
		{
			return;
		}

		guard->m_userDataLock.lockForRead();
		if (guard->m_userLoginName.isEmpty() && guard->m_userFullName.isEmpty())
		{
			QTimer::singleShot(UserInfoUpdateRetryInterval, guard, &MonitoringMode::updateUserInfo);
		}
		guard->m_userDataLock.unlock();
	});
}



void MonitoringMode::updateSessionInfo()
{
	const auto guard = QPointer<MonitoringMode>(this);
	(void)QtConcurrent::run([guard]() {
		if (!guard)
		{
			return;
		}

		QString sessionMetaData;
		switch (guard->m_sessionMetaDataContent)
		{
		case PlatformSessionFunctions::SessionMetaDataContent::EnvironmentVariable:
			sessionMetaData = VeyonCore::platform().sessionFunctions().currentSessionEnvironmentVariables().value(guard->m_sessionMetaDataEnvironmentVariable);
			break;
		case PlatformSessionFunctions::SessionMetaDataContent::RegistryKey:
			sessionMetaData = VeyonCore::platform().sessionFunctions().querySettingsValueInCurrentSession(guard->m_sessionMetaDataRegistryKey).toString();
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
			sessionMetaData};

		if (!guard)
		{
			return;
		}

		guard->m_sessionInfoLock.lockForWrite();
		if (currentSessionInfo != guard->m_sessionInfo)
		{
			guard->m_sessionInfo = currentSessionInfo;
			++guard->m_sessionInfoVersion;
		}
		guard->m_sessionInfoLock.unlock();
	});
}



void MonitoringMode::updateScreenInfoList()
{
	const auto screens = QGuiApplication::screens();

	QVariantList screenInfoList;
	screenInfoList.reserve(screens.size());

	int index = 1;
	for (const auto* screen : screens)
	{
		QVariantMap screenInfo;
		screenInfo[QStringLiteral("name")] = VeyonCore::screenName(*screen, index);
		screenInfo[QStringLiteral("geometry")] = screen->geometry();
		screenInfoList.append(screenInfo);
		++index;

		connect(screen, &QScreen::geometryChanged, this, &MonitoringMode::updateScreenInfoList, Qt::UniqueConnection);
	}

	if (screenInfoList != m_screenInfoList)
	{
		m_screenInfoList = screenInfoList;
		++m_screenInfoListVersion;
	}
}



QString MonitoringMode::queryUserIdentity()
{
	qApp->setQuitOnLastWindowClosed(false);

	QInputDialog dialog;
	dialog.setWindowTitle(tr("Identification request"));
	dialog.setLabelText(tr("Please enter your name:"));
	dialog.setInputMode(QInputDialog::TextInput);
	dialog.setWindowFlags(dialog.windowFlags() & (~Qt::WindowCloseButtonHint));
	dialog.setStyleSheet(QStringLiteral("button-layout:%1").arg(QDialogButtonBox::WinLayout));

	auto lineEdit = dialog.findChild<QLineEdit*>();
	if (lineEdit)
	{
		lineEdit->setPlaceholderText(tr("First name + last name"));
	}

	VeyonCore::platform().coreFunctions().raiseWindow(&dialog, true);

	if (dialog.exec() != QDialog::Accepted)
	{
		return {};
	}

	return dialog.textValue();
}



QString MonitoringMode::sessionUserLoginName()
{
	m_userDataLock.lockForRead();
	const auto userLoginName = m_userLoginName;
	m_userDataLock.unlock();

	return userLoginName;
}
