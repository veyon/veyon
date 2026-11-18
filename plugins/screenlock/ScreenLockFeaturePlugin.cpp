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
	m_lockWidget(nullptr)
{
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
	delete m_lockWidget;
}



bool ScreenLockFeaturePlugin::controlFeature(Feature::Uid featureUid, Operation operation,
											 const QVariantMap& arguments,
											 const ComputerControlInterfaceList& computerControlInterfaces)
{
	Q_UNUSED(arguments)

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

		sendFeatureMessage(FeatureMessage{featureUid, FeatureCommand::StartLock}, lockControlInterfaces);

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
		if (message.command<FeatureCommand>() == FeatureCommand::StopLock)
		{
			if (server.featureWorkerManager().isWorkerRunning(message.featureUid()))
				server.featureWorkerManager().sendMessageToManagedSystemWorker(message);

			return true;
		}

		if (VeyonCore::platform().sessionFunctions().currentSessionHasUser() == false)
		{
			vDebug() << "not locking screen since not running in a user session";
			return true;
		}

		// forward message to worker
		server.featureWorkerManager().sendMessageToManagedSystemWorker(message);

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

				auto mode = LockWidget::ClassroomLock;
				if( message.featureUid() == m_lockInputDevicesFeature.uid() )
				{
					mode = LockWidget::DesktopVisible;
				}

				m_lockWidget = new LockWidget( mode,
											   QPixmap(QStringLiteral(":/screenlock/system-lock-screen.png")) );
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
