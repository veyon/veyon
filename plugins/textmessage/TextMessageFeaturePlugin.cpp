/*
 * TextMessageFeaturePlugin.cpp - implementation of TextMessageFeaturePlugin class
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QMessageBox>

#include "TextMessageFeaturePlugin.h"
#include "TextMessageDialog.h"
#include "FeatureWorkerManager.h"
#include "ComputerControlInterface.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"


TextMessageFeaturePlugin::TextMessageFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_textMessageFeature( Feature( QStringLiteral( "TextMessage" ),
								   Feature::Flag::Action | Feature::Flag::AllComponents,
								   Feature::Uid( "e75ae9c8-ac17-4d00-8f0d-019348346208" ),
								   Feature::Uid(),
								   tr( "Text message" ), {},
								   tr( "Use this function to send a text message to all "
									   "users e.g. to assign them new tasks." ),
								   QStringLiteral(":/textmessage/dialog-information.png") ) ),
	m_features( { m_textMessageFeature } )
{
}



const FeatureList &TextMessageFeaturePlugin::featureList() const
{
	return m_features;
}



bool TextMessageFeaturePlugin::controlFeature( Feature::Uid featureUid,
											  Operation operation,
											  const QVariantMap& arguments,
											  const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( operation != Operation::Start )
	{
		return false;
	}

	if( featureUid == m_textMessageFeature.uid() )
	{
		const auto text = arguments.value( argToString(Argument::Text) ).toString();
		const auto icon = arguments.value( argToString(Argument::Icon) ).toInt();
		const auto title = arguments.value( argToString(Argument::Title) ).toString();

		sendFeatureMessage(FeatureMessage{featureUid, FeatureCommand::ShowTextMessage}
						   .addArgument(Argument::Text, text)
						   .addArgument(Argument::Icon, icon)
						   .addArgument(Argument::Title, title), computerControlInterfaces);

		return true;
	}

	return false;
}



bool TextMessageFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
											 const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature.uid() != m_textMessageFeature.uid() )
	{
		return false;
	}

	QString textMessage;
	QString messageTitle;

	TextMessageDialog( textMessage, messageTitle, master.mainWindow() ).exec();

	if( textMessage.isEmpty() == false )
	{
		controlFeature( m_textMessageFeature.uid(), Operation::Start,
						{
							{ argToString(Argument::Text), textMessage },
							{ argToString(Argument::Icon), QMessageBox::Information },
							{ argToString(Argument::Title), messageTitle }
						},
						computerControlInterfaces );
	}

	return true;
}




bool TextMessageFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server,
													 const MessageContext& messageContext,
													 const FeatureMessage& message )
{
	Q_UNUSED(messageContext)

	if( m_textMessageFeature.uid() == message.featureUid() )
	{
		// forward message to worker
		server.featureWorkerManager().sendMessageToUnmanagedSessionWorker( message );

		return true;
	}

	return false;
}



bool TextMessageFeaturePlugin::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	Q_UNUSED(worker);

	if( message.featureUid() == m_textMessageFeature.uid() )
	{
		auto title = message.argument( Argument::Title ).toString();
		if( title.isEmpty() )
		{
			title = tr( "Message from teacher" );
		}

		auto messageBox = new QMessageBox( static_cast<QMessageBox::Icon>( message.argument( Argument::Icon ).toInt() ),
										   title,
										   message.argument( Argument::Text ).toString() );
		messageBox->setTextFormat( Qt::RichText );
		messageBox->setTextInteractionFlags( Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard );
		messageBox->setAttribute( Qt::WA_DeleteOnClose );
		messageBox->setWindowFlags( messageBox->windowFlags() | Qt::WindowStaysOnTopHint );
		messageBox->show();
		messageBox->raise();
		messageBox->activateWindow();

		return true;
	}

	return true;
}
