// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>

#include "ComputerControlInterface.h"
#include "FeatureWorkerManager.h"
#include "LockWidget.h"
#include "PlatformCoreFunctions.h"
#include "PlatformInputDeviceFunctions.h"
#include "PlatformSessionFunctions.h"
#include "ScreenLockFeaturePlugin.h"
#include "VeyonConfiguration.h"
#include "VeyonServerInterface.h"


ScreenLockFeaturePlugin::ScreenLockFeaturePlugin(QObject* parent) :
	QObject(parent),
	m_screenLockFeature(QStringLiteral("ScreenLock"),
						Feature::Flag::Mode | Feature::Flag::AllComponents,
						Feature::Uid("ccb535a2-1d24-4cc1-a709-8b47d2b2ac79"),
						Feature::Uid(),
						tr("Lock"), tr("Unlock"),
						tr("To reclaim all user's full attention you can lock "
						   "their computers using this button. "
						   "In this mode all input devices are locked and "
						   "the screens are blacked."),
						QStringLiteral(":/screenlock/system-lock-screen.png")),
	m_lockInputDevicesFeature(QStringLiteral("InputDevicesLock"),
							  Feature::Flag::Mode | Feature::Flag::AllComponents | Feature::Flag::Meta,
							  Feature::Uid("e4a77879-e544-4fec-bc18-e534f33b934c"),
							  {},
							  tr("Lock input devices"), tr("Unlock input devices"),
							  tr("To reclaim all user's full attention you can lock "
								 "their computers using this button. "
								 "In this mode all input devices are locked while the desktop is still visible."),
							  QStringLiteral(":/screenlock/system-lock-screen.png")),
	m_features({m_screenLockFeature, m_lockInputDevicesFeature}),
	m_lockWidget(nullptr),
	m_server(nullptr),
	m_timeoutTimer(this),
	m_customTimeoutMinutes(0)
{
	m_timeoutTimer.setSingleShot(true);
	connect(&m_timeoutTimer, &QTimer::timeout, this, &ScreenLockFeaturePlugin::handleTimeout);

	if (VeyonCore::component() == VeyonCore::Component::Service)
	{
		connect(VeyonCore::instance(), &VeyonCore::initialized,
				this, []() {
			VeyonCore::platform().inputDeviceFunctions().enableInputDevices();
		});
	}
}



ScreenLockFeaturePlugin::~ScreenLockFeaturePlugin()
{
	m_timeoutTimer.stop();
	delete m_lockWidget;
}



bool ScreenLockFeaturePlugin::controlFeature(Feature::Uid featureUid, Operation operation,
											 const QVariantMap& arguments,
											 const ComputerControlInterfaceList& computerControlInterfaces)
{
	if (hasFeature(featureUid) == false)
	{
		return false;
	}

	if (operation == Operation::Start)
	{
		// never lock the master computer itself: in multi-teacher lab layouts it
		// is listed as a regular room computer, but locking its screen would grab
		// the teacher's keyboard and mouse and cover Veyon Master, leaving no way
		// to unlock the running session
		auto lockControlInterfaces = computerControlInterfaces;
		lockControlInterfaces.removeLocalHostInterfaces();

		FeatureMessage message{featureUid, FeatureCommand::StartLock};
		const auto timeout = arguments.value(QStringLiteral("timeout"), VeyonCore::config().screenLockTimeout()).toInt();
		if (timeout > 0)
		{
			message.addArgument(0, timeout);
		}

		sendFeatureMessage(message, lockControlInterfaces);

		return true;
	}

	if (operation == Operation::Stop)
	{
		sendFeatureMessage(FeatureMessage{featureUid, FeatureCommand::StopLock}, computerControlInterfaces);

		return true;
	}

	return false;
}



bool ScreenLockFeaturePlugin::handleFeatureMessage(VeyonServerInterface& server,
												   const MessageContext& messageContext,
												   const FeatureMessage& message)
{
	Q_UNUSED(messageContext)

	if (message.featureUid() == m_screenLockFeature.uid() ||
		message.featureUid() == m_lockInputDevicesFeature.uid())
	{
		setServer(server);

		if (message.command<FeatureCommand>() == FeatureCommand::StopLock)
		{
			m_timeoutTimer.stop();
			m_customTimeoutMinutes = 0;

			if (server.featureWorkerManager().isWorkerRunning(message.featureUid()))
				server.featureWorkerManager().sendMessageToManagedSystemWorker(message);

			return true;
		}

		if (message.hasArgument(0))
		{
			m_customTimeoutMinutes = message.argument(0).toInt();
		}

		m_timeoutTimer.stop();

		if (VeyonCore::platform().sessionFunctions().currentSessionHasUser() == false)
		{
			vDebug() << "not locking screen since not running in a user session";
			return true;
		}

		// forward message to worker
		server.featureWorkerManager().sendMessageToManagedSystemWorker(message);

		if (server.activeClientCount() == 0)
		{
			handleClientDisconnected();
		}

		return true;
	}

	return false;
}



bool ScreenLockFeaturePlugin::handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message)
{
	Q_UNUSED(worker);

	if (message.featureUid() == m_screenLockFeature.uid() ||
		message.featureUid() == m_lockInputDevicesFeature.uid())
	{
		switch (message.command<FeatureCommand>())
		{
		case FeatureCommand::StartLock:
			if (m_lockWidget == nullptr)
			{
				VeyonCore::platform().coreFunctions().disableScreenSaver();

				auto mode = LockWidget::BackgroundPixmap;
				if (message.featureUid() == m_lockInputDevicesFeature.uid())
				{
					mode = LockWidget::DesktopVisible;
				}

				m_lockWidget = new LockWidget(mode,
											  QPixmap(QStringLiteral(":/screenlock/locked-screen-background.png")));
			}
			return true;

		case FeatureCommand::StopLock:
			delete m_lockWidget;
			m_lockWidget = nullptr;

			VeyonCore::platform().coreFunctions().restoreScreenSaverSettings();

			QCoreApplication::quit();

			return true;

		default:
			break;
		}
	}

	return false;
}



void ScreenLockFeaturePlugin::setServer(VeyonServerInterface& server)
{
	if (m_server != &server)
	{
		if (m_server)
		{
			disconnect(m_server, nullptr, this, nullptr);
		}

		m_server = &server;

		connect(m_server, &VeyonServerInterface::clientConnected,
				this, &ScreenLockFeaturePlugin::handleClientConnected);
		connect(m_server, &VeyonServerInterface::clientDisconnected,
				this, &ScreenLockFeaturePlugin::handleClientDisconnected);
	}
}



bool ScreenLockFeaturePlugin::isScreenLockActive() const
{
	if (m_server == nullptr)
	{
		return false;
	}

	return m_server->featureWorkerManager().isWorkerRunning(m_screenLockFeature.uid()) ||
		   m_server->featureWorkerManager().isWorkerRunning(m_lockInputDevicesFeature.uid());
}



void ScreenLockFeaturePlugin::handleClientConnected()
{
	if (m_timeoutTimer.isActive())
	{
		vInfo() << "Active Master connected, cancelling ScreenLock timeout timer";
		m_timeoutTimer.stop();
	}
}



void ScreenLockFeaturePlugin::handleClientDisconnected()
{
	if (m_server == nullptr)
	{
		return;
	}

	if (isScreenLockActive() && m_server->activeClientCount() == 0)
	{
		int timeoutMinutes = m_customTimeoutMinutes;
		if (timeoutMinutes <= 0)
		{
			timeoutMinutes = VeyonCore::config().screenLockTimeout();
		}

		if (timeoutMinutes > 0)
		{
			vInfo() << "All Master connections closed. ScreenLock timeout timer started for"
					<< timeoutMinutes << "minute(s).";
			m_timeoutTimer.start(timeoutMinutes * 60 * 1000);
		}
	}
}



void ScreenLockFeaturePlugin::handleTimeout()
{
	vInfo() << "ScreenLock timeout expired without Master connection. Automatically unlocking screen.";
	m_timeoutTimer.stop();

	if (m_server == nullptr)
	{
		return;
	}

	FeatureMessage stopScreenLock{m_screenLockFeature.uid(), FeatureCommand::StopLock};
	if (m_server->featureWorkerManager().isWorkerRunning(m_screenLockFeature.uid()))
	{
		m_server->featureWorkerManager().sendMessageToManagedSystemWorker(stopScreenLock);
	}

	FeatureMessage stopInputLock{m_lockInputDevicesFeature.uid(), FeatureCommand::StopLock};
	if (m_server->featureWorkerManager().isWorkerRunning(m_lockInputDevicesFeature.uid()))
	{
		m_server->featureWorkerManager().sendMessageToManagedSystemWorker(stopInputLock);
	}
}
