// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later


#include <QDateTime>
#include <QDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QScreen>
#include <QTextBrowser>
#include <QTextDocument>
#include <QVBoxLayout>

#include "TextMessageFeaturePlugin.h"
#include "TextMessageDialog.h"
#include "QuickUi.h"
#include "DesktopIcons.h"
#include "FeatureWorkerManager.h"
#include "ComputerControlInterface.h"
#include "PlatformCoreFunctions.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"


TextMessageFeaturePlugin::TextMessageFeaturePlugin(QObject* parent) :
	QObject(parent),
	m_textMessageFeature(Feature(QStringLiteral("TextMessage"),
								 Feature::Flag::Action | Feature::Flag::AllComponents,
								 Feature::Uid("e75ae9c8-ac17-4d00-8f0d-019348346208"),
								 Feature::Uid(),
								 tr("Text message"), {},
								 tr("Use this function to send a text message to all "
									"users e.g. to assign them new tasks."),
								 QStringLiteral(":/textmessage/dialog-information.png"))),
	m_features({m_textMessageFeature})
{
}



const FeatureList& TextMessageFeaturePlugin::featureList() const
{
	return m_features;
}



bool TextMessageFeaturePlugin::controlFeature(Feature::Uid featureUid,
											  Operation operation,
											  const QVariantMap& arguments,
											  const ComputerControlInterfaceList& computerControlInterfaces)
{
	if (operation != Operation::Start)
	{
		return false;
	}

	if (featureUid == m_textMessageFeature.uid())
	{
		const auto text = arguments.value(argToString(Argument::Text)).toString();
		const auto icon = arguments.value(argToString(Argument::Icon)).toInt();
		const auto title = arguments.value(argToString(Argument::Title)).toString();

		sendFeatureMessage(FeatureMessage{featureUid, FeatureCommand::ShowTextMessage}
							   .addArgument(Argument::Text, text)
							   .addArgument(Argument::Icon, icon)
							   .addArgument(Argument::Title, title),
						   computerControlInterfaces);

		return true;
	}

	return false;
}



bool TextMessageFeaturePlugin::startFeature(VeyonMasterInterface& master, const Feature& feature,
											const ComputerControlInterfaceList& computerControlInterfaces)
{
	if (feature.uid() != m_textMessageFeature.uid())
	{
		return false;
	}

	QString textMessage;
	QString messageTitle;

	QStringList recipients;
	for (const auto& computer : computerControlInterfaces)
	{
		recipients.append(computer->computerName());
	}
	auto summary = tr("Recipients: %n computer(s)", "", recipients.size());
	if (!recipients.isEmpty())
	{
		const auto shownNames = recipients.mid(0, 5).join(QStringLiteral(", "));
		summary += QStringLiteral("\n") + shownNames;
		if (recipients.size() > 5)
		{
			summary += tr(" and %n more", "", recipients.size() - 5);
		}
	}
	TextMessageDialog(textMessage, messageTitle, master.mainWindow(), summary).exec();

	if (textMessage.isEmpty() == false)
	{
		controlFeature( m_textMessageFeature.uid(), Operation::Start,
						{
							{ argToString(Argument::Text), textMessage },
							{ argToString(Argument::Icon), 1 },
							{ argToString(Argument::Title), messageTitle }
						},
						computerControlInterfaces );
	}

	return true;
}



bool TextMessageFeaturePlugin::handleFeatureMessage(VeyonServerInterface& server,
													const MessageContext& messageContext,
													const FeatureMessage& message)
{
	Q_UNUSED(messageContext)

	if (m_textMessageFeature.uid() == message.featureUid())
	{
		// forward message to worker
		server.featureWorkerManager().sendMessageToUnmanagedSessionWorker(message);

		return true;
	}

	return false;
}



bool TextMessageFeaturePlugin::handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message)
{
	Q_UNUSED(worker);

	if (message.featureUid() == m_textMessageFeature.uid())
	{
		auto title = message.argument(Argument::Title).toString();
		if (title.isEmpty())
		{
			title = tr("Message from teacher");
		}

		QuickUi::execDialog(QStringLiteral("notification"), {
			{QStringLiteral("title"), tr("Message from teacher")},
			{QStringLiteral("summary"), QString(title + QStringLiteral(" · ") + QLocale().toString(QTime::currentTime(), QLocale::ShortFormat))},
			{QStringLiteral("text"), message.argument(Argument::Text).toString()},
			{QStringLiteral("richText"), true},
			{QStringLiteral("iconName"), message.argument(Argument::Icon).toInt() == 2 ? QStringLiteral("triangle-alert") :
				message.argument(Argument::Icon).toInt() == 3 ? QStringLiteral("circle-alert") :
				message.argument(Argument::Icon).toInt() == 4 ? QStringLiteral("circle-help") : QStringLiteral("message-square-text")}});
		QCoreApplication::quit();

		return true;
	}

	return true;
}
