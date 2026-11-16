// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDesktopServices>
#include <QLabel>
#include <QMessageBox>
#include <QTextDocumentFragment>

#include "ComputerControlInterface.h"
#include "FeatureWorkerManager.h"
#include "PlatformCoreFunctions.h"
#include "TextMessageDialog.h"
#include "TextMessageFeaturePlugin.h"
#include "TextMessageSanitizer.h"
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

	TextMessageDialog(textMessage, messageTitle, master.mainWindow()).exec();

	if (textMessage.isEmpty() == false)
	{
		controlFeature(m_textMessageFeature.uid(), Operation::Start,
					   {{argToString(Argument::Text), textMessage},
						{argToString(Argument::Icon), QMessageBox::Information},
						{argToString(Argument::Title), messageTitle}},
					   computerControlInterfaces);
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
		auto title = TextMessageSanitizer::sanitizeTitle(message.argument(Argument::Title).toString());
		if (title.isEmpty())
		{
			title = tr("Message from teacher");
		}

		const auto text = message.argument(Argument::Text).toString();
		if (text.size() > TextMessageSanitizer::MaxTextLength)
		{
			vWarning() << "text message exceeds length limit, truncating";
		}

		auto messageBox = new QMessageBox(static_cast<QMessageBox::Icon>(message.argument(Argument::Icon).toInt()),
										  title,
										  TextMessageSanitizer::sanitizeHtml(text));
		messageBox->setTextFormat(Qt::RichText);
		messageBox->setTextInteractionFlags(Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard);

		// QMessageBox opens links on its own and has no linkActivated signal, so take over
		// link handling in its label and only open links which pass the scheme check again
		auto label = messageBox->findChild<QLabel*>(QStringLiteral("qt_msgbox_label"));
		if (label)
		{
			label->setOpenExternalLinks(false);
			connect(label, &QLabel::linkActivated, label, [](const QString& link) {
				const QUrl url(link.trimmed(), QUrl::StrictMode);
				if (TextMessageSanitizer::isAllowedLink(url))
				{
					QDesktopServices::openUrl(url);
				}
				else
				{
					vWarning() << "refusing to open link with disallowed scheme" << url.scheme();
				}
			});
		}
		else
		{
			// fail closed: without control over link handling show plain text only
			vWarning() << "message box label not found, disabling rich text";
			messageBox->setTextFormat(Qt::PlainText);
			messageBox->setText(QTextDocumentFragment::fromHtml(messageBox->text()).toPlainText());
		}

		messageBox->setAttribute(Qt::WA_DeleteOnClose);
		messageBox->show();
		VeyonCore::platform().coreFunctions().raiseWindow(messageBox, true);

		return true;
	}

	return true;
}
