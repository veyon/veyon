// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include "QuickUi.h"
#include <QPainter>
#include <QPixmap>
#include <QSystemTrayIcon>
#include <QTimer>

#include "FeatureWorkerManager.h"
#include "PlatformCoreFunctions.h"
#include "SystemTrayIcon.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"
#include "VeyonServerInterface.h"


SystemTrayIcon::SystemTrayIcon(QObject* parent) :
	QObject(parent),
	m_systemTrayIconFeature(Feature(QLatin1String(staticMetaObject.className()),
									Feature::Flag::Session | Feature::Flag::Service | Feature::Flag::Worker | Feature::Flag::Builtin,
									Feature::Uid("8e997d84-ebb9-430f-8f72-d45d9821963d"),
									Feature::Uid(),
									tr("System tray icon"), {}, {})),
	m_hidden(VeyonCore::config().isTrayIconHidden())
{
}



void SystemTrayIcon::setToolTip(const QString& toolTipText,
								FeatureWorkerManager& featureWorkerManager)
{
	if (m_hidden == false)
	{
		FeatureMessage featureMessage(m_systemTrayIconFeature.uid(), FeatureCommand::SetToolTip);
		featureMessage.addArgument(Argument::ToolTipText, toolTipText);

		featureWorkerManager.sendMessageToUnmanagedSessionWorker(featureMessage);
	}
}



void SystemTrayIcon::showMessage(const QString& messageTitle,
								 const QString& messageText,
								 FeatureWorkerManager& featureWorkerManager)
{
	FeatureMessage featureMessage(m_systemTrayIconFeature.uid(), FeatureCommand::ShowMessage);
	featureMessage.addArgument(Argument::MessageTitle, messageTitle);
	featureMessage.addArgument(Argument::MessageText, messageText);

	featureWorkerManager.sendMessageToUnmanagedSessionWorker(featureMessage);
}



void SystemTrayIcon::showMessage(const ComputerControlInterfaceList& computerControlInterfaces,
								 const QString& messageTitle, const QString& messageText)
{
	sendFeatureMessage(FeatureMessage{m_systemTrayIconFeature.uid(), FeatureCommand::ShowMessage}
						   .addArgument(Argument::MessageTitle, messageTitle)
						   .addArgument(Argument::MessageText, messageText),
					   computerControlInterfaces);
}



void SystemTrayIcon::setOverlay(const ComputerControlInterfaceList& computerControlInterfaces,
								const QString& iconUrl)
{
	if (m_hidden == false)
	{
		sendFeatureMessage(FeatureMessage{m_systemTrayIconFeature.uid(), FeatureCommand::SetOverlayIcon}
							   .addArgument(Argument::OverlayIconUrl, iconUrl),
						   computerControlInterfaces);
	}
}



bool SystemTrayIcon::handleFeatureMessage(VeyonServerInterface& server,
										  const MessageContext& messageContext,
										  const FeatureMessage& message)
{
	Q_UNUSED(messageContext)

	if (m_systemTrayIconFeature.uid() == message.featureUid())
	{
		// forward message to worker
		server.featureWorkerManager().sendMessageToUnmanagedSessionWorker(message);
		return true;
	}

	return false;
}



bool SystemTrayIcon::handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message)
{
	Q_UNUSED(worker)

	if (message.featureUid() != m_systemTrayIconFeature.uid())
	{
		return false;
	}

	if (m_systemTrayIcon == nullptr && m_hidden == false)
	{
		m_systemTrayIcon = new QSystemTrayIcon(this);

		updateIcon();

		m_systemTrayIcon->show();
	}

	switch (message.command<FeatureCommand>())
	{
	case FeatureCommand::SetToolTip:
		if (m_systemTrayIcon)
		{
			m_systemTrayIcon->setToolTip(message.argument(Argument::ToolTipText).toString());
		}
		return true;

	case FeatureCommand::ShowMessage:
		if (m_systemTrayIcon)
		{
			m_systemTrayIcon->showMessage(message.argument(Argument::MessageTitle).toString(),
										  message.argument(Argument::MessageText).toString());
		}
		else
		{
			QuickUi::showNotification(message.argument(Argument::MessageTitle).toString(),
											  message.argument(Argument::MessageText).toString());
			QTimer::singleShot(MessageBoxAutoCloseInterval, QCoreApplication::instance(), &QCoreApplication::quit);
		}
		return true;

	case FeatureCommand::SetOverlayIcon:
		updateIcon(message.argument(Argument::OverlayIconUrl).toString());
		return true;

	default:
		break;
	}

	return false;
}



void SystemTrayIcon::updateIcon(const QString& overlayIconUrl)
{
	if (m_systemTrayIcon)
	{
		QImage overlayIcon;
		if (overlayIconUrl.isEmpty() == false)
		{
			overlayIcon.load(overlayIconUrl);
		}

		QIcon icon;

		for (auto size : {16, 22, 32, 64})
		{
			QPixmap pixmap(QStringLiteral(":/core/icon%1.png").arg(size));
			if (overlayIcon.isNull() == false)
			{
				QPainter painter(&pixmap);
				painter.drawImage(0, 0, overlayIcon.scaled(pixmap.size()));
			}
			icon.addPixmap(pixmap);
		}

		m_systemTrayIcon->setIcon(icon);
	}
}
