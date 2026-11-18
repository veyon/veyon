// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "QuickUi.h"

#include "ComputerControlInterface.h"
#include "Screenshot.h"
#include "ScreenshotFeaturePlugin.h"
#include "VeyonMasterInterface.h"


ScreenshotFeaturePlugin::ScreenshotFeaturePlugin(QObject* parent) :
	QObject(parent),
	m_screenshotFeature(Feature(QStringLiteral("Screenshot"),
								Feature::Flag::Action | Feature::Flag::Master,
								Feature::Uid("d5ee3aac-2a87-4d05-b827-0c20344490bd"),
								Feature::Uid(),
								tr("Screenshot"), {},
								tr("Use this function to take a screenshot of selected computers."),
								QStringLiteral(":/screenshot/camera-photo.png"))),
	m_features({m_screenshotFeature})
{
}



const FeatureList& ScreenshotFeaturePlugin::featureList() const
{
	return m_features;
}



bool ScreenshotFeaturePlugin::controlFeature(Feature::Uid featureUid,
											 Operation operation, const QVariantMap& arguments,
											 const ComputerControlInterfaceList& computerControlInterfaces)
{
	Q_UNUSED(arguments)

	if (hasFeature(featureUid) && operation == Operation::Start)
	{
		for (const auto& controlInterface : computerControlInterfaces)
		{
			if (VeyonCore::component() == VeyonCore::Component::CLI &&
				controlInterface->hasValidFramebuffer() == false)
			{
				// maximum time to wait for the first framebuffer update when the feature is
				// invoked right after connecting (e.g. via CLI or WebAPI)
				static constexpr int FramebufferWaitTimeout = 10 * 1000;

				QEventLoop eventLoop;
				QTimer::singleShot(FramebufferWaitTimeout, &eventLoop, &QEventLoop::quit);
				connect(controlInterface.data(), &ComputerControlInterface::framebufferUpdated,
						&eventLoop, &QEventLoop::quit);
				eventLoop.exec();

				if (controlInterface->hasValidFramebuffer() == false)
				{
					vWarning() << "no framebuffer data received for host"
							   << controlInterface->computer().hostAddress();
				}
			}

			Screenshot().take(controlInterface);
		}

		return true;
	}

	return false;
}



bool ScreenshotFeaturePlugin::startFeature(VeyonMasterInterface& master, const Feature& feature,
										   const ComputerControlInterfaceList& computerControlInterfaces)
{
	if (controlFeature(feature.uid(), Operation::Start, {}, computerControlInterfaces))
	{
		QuickUi::execDialog(QStringLiteral("information"), {{QStringLiteral("title"),
								  tr("Screenshots taken")}, {QStringLiteral("text"),
								  tr( "Screenshot of %1 computer have been taken successfully." ).
								  arg(computerControlInterfaces.count())}});

		return true;
	}

	return false;
}
