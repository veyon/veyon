/*
 * TextMessageFeaturePlugin.cpp - implementation of TextMessageFeaturePlugin class
 *
 * Copyright (c) 2017-2020 Tobias Junghans <tobydox@veyon.io>
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
#include <QQuickWindow>

#include "ComputerControlInterface.h"
#include "FeatureWorkerManager.h"
#include "QmlCore.h"
#include "TextMessageDialog.h"
#include "TextMessageFeaturePlugin.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"


TextMessageFeaturePlugin::TextMessageFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_textMessageFeature( Feature( QStringLiteral( "TextMessage" ),
								   Feature::Action | Feature::AllComponents,
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



bool TextMessageFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
											 const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature.uid() != m_textMessageFeature.uid() )
	{
		return false;
	}

	if( master.appWindow() )
	{
		auto dialog = VeyonCore::qmlCore().createObjectFromFile( QStringLiteral("qrc:/textmessage/TextMessageDialog.qml"),
														 master.appWindow(),
														 this );
		connect( this, &TextMessageFeaturePlugin::acceptTextMessage, dialog, // clazy:exclude=connect-non-signal
				 [this, computerControlInterfaces]( const QString& text )
		{
			sendTextMessage( text, computerControlInterfaces );
		} );

		return true;
	}

	QString textMessage;

	TextMessageDialog( textMessage, master.mainWindow() ).exec();

	sendTextMessage( textMessage, computerControlInterfaces );

	return true;
}



bool TextMessageFeaturePlugin::stopFeature( VeyonMasterInterface& master, const Feature& feature,
											const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(master)
	Q_UNUSED(feature)
	Q_UNUSED(computerControlInterfaces)

	return false;
}



bool TextMessageFeaturePlugin::handleFeatureMessage( VeyonMasterInterface& master, const FeatureMessage& message,
													 ComputerControlInterface::Pointer computerControlInterface )
{
	Q_UNUSED(master)
	Q_UNUSED(message)
	Q_UNUSED(computerControlInterface)

	return false;
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
	Q_UNUSED(worker)

	if( message.featureUid() == m_textMessageFeature.uid() )
	{
		QMessageBox* messageBox = new QMessageBox( static_cast<QMessageBox::Icon>( message.argument( MessageIcon ).toInt() ),
												   tr( "Message from teacher" ),
												   message.argument( MessageTextArgument ).toString() );
		messageBox->show();

		connect( messageBox, &QMessageBox::accepted, messageBox, &QMessageBox::deleteLater );

		return true;
	}

	return true;
}



void TextMessageFeaturePlugin::sendTextMessage( const QString& textMessage,
												const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( textMessage.isEmpty() == false )
	{
		FeatureMessage featureMessage( m_textMessageFeature.uid(), ShowTextMessage );
		featureMessage.addArgument( MessageTextArgument, textMessage );
		featureMessage.addArgument( MessageIcon, QMessageBox::Information );

		sendFeatureMessage( featureMessage, computerControlInterfaces );
	}
}
