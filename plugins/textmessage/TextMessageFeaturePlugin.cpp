/*
 * TextMessageFeaturePlugin.cpp - implementation of TextMessageFeaturePlugin class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
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


TextMessageFeaturePlugin::TextMessageFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_textMessageFeature( Feature( Feature::Action | Feature::AllComponents,
								   Feature::Uid( "e75ae9c8-ac17-4d00-8f0d-019348346208" ),
								   tr( "Text message" ), QString(),
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



bool TextMessageFeaturePlugin::startMasterFeature( const Feature& feature,
												   const ComputerControlInterfaceList& computerControlInterfaces,
												   ComputerControlInterface& localComputerControlInterface,
												   QWidget* parent )
{
	Q_UNUSED(localComputerControlInterface);

	if( feature.uid() != m_textMessageFeature.uid() )
	{
		return false;
	}

	QString textMessage;

	TextMessageDialog( textMessage, qobject_cast<QWidget *>( parent ) ).exec();

	if( textMessage.isEmpty() == false )
	{
		FeatureMessage featureMessage( m_textMessageFeature.uid(), ShowTextMessage );
		featureMessage.addArgument( MessageTextArgument, textMessage );
		featureMessage.addArgument( MessageIcon, QMessageBox::Information );

		sendFeatureMessage( featureMessage, computerControlInterfaces );
	}

	return true;
}



bool TextMessageFeaturePlugin::stopMasterFeature( const Feature& feature,
												  const ComputerControlInterfaceList& computerControlInterfaces,
												  ComputerControlInterface& localComputerControlInterface,
												  QWidget* parent )
{
	Q_UNUSED(feature);
	Q_UNUSED(localComputerControlInterface);
	Q_UNUSED(computerControlInterfaces);
	Q_UNUSED(parent);

	return false;
}



bool TextMessageFeaturePlugin::handleMasterFeatureMessage( const FeatureMessage& message,
														   ComputerControlInterface& computerControlInterface )
{
	Q_UNUSED(message);
	Q_UNUSED(computerControlInterface);

	return false;
}



bool TextMessageFeaturePlugin::handleServiceFeatureMessage( const FeatureMessage& message,
															FeatureWorkerManager& featureWorkerManager )
{
	if( m_textMessageFeature.uid() == message.featureUid() )
	{
		// forward message to worker
		if( featureWorkerManager.isWorkerRunning( m_textMessageFeature ) == false )
		{
			featureWorkerManager.startWorker( m_textMessageFeature );
		}
		featureWorkerManager.sendMessage( message );

		return true;
	}

	return false;
}



bool TextMessageFeaturePlugin::handleWorkerFeatureMessage( const FeatureMessage& message )
{
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
