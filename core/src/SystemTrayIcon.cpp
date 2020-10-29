/*
 * SystemTrayIcon.cpp - implementation of SystemTrayIcon class
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
#include <QSystemTrayIcon>

#include "SystemTrayIcon.h"
#include "FeatureWorkerManager.h"
#include "VeyonCore.h"
#include "VeyonConfiguration.h"
#include "VeyonServerInterface.h"


SystemTrayIcon::SystemTrayIcon( QObject* parent ) :
	QObject( parent ),
	m_systemTrayIconFeature( Feature( QLatin1String( staticMetaObject.className() ),
									  Feature::Session | Feature::Service | Feature::Worker | Feature::Builtin,
									  Feature::Uid( "8e997d84-ebb9-430f-8f72-d45d9821963d" ),
									  Feature::Uid(),
									  tr( "System tray icon"), {}, {} ) ),
	m_features( { m_systemTrayIconFeature } ),
	m_systemTrayIcon( nullptr )
{
}



void SystemTrayIcon::setToolTip( const QString& toolTipText,
								FeatureWorkerManager& featureWorkerManager )
{
	FeatureMessage featureMessage( m_systemTrayIconFeature.uid(), SetToolTipCommand );
	featureMessage.addArgument( Argument::ToolTipText, toolTipText );

	featureWorkerManager.sendMessageToUnmanagedSessionWorker( featureMessage );
}



void SystemTrayIcon::showMessage( const QString& messageTitle,
								 const QString& messageText,
								 FeatureWorkerManager& featureWorkerManager  )
{
	FeatureMessage featureMessage( m_systemTrayIconFeature.uid(), ShowMessageCommand );
	featureMessage.addArgument( Argument::MessageTitle, messageTitle );
	featureMessage.addArgument( Argument::MessageText, messageText );

	featureWorkerManager.sendMessageToUnmanagedSessionWorker( featureMessage );
}



bool SystemTrayIcon::handleFeatureMessage( VeyonServerInterface& server,
										  const MessageContext& messageContext,
										  const FeatureMessage& message )
{
	Q_UNUSED(messageContext)

	if( m_systemTrayIconFeature.uid() == message.featureUid() )
	{
		// forward message to worker
		server.featureWorkerManager().sendMessageToUnmanagedSessionWorker( message );
		return true;
	}

	return false;
}



bool SystemTrayIcon::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	Q_UNUSED(worker)

	if( message.featureUid() != m_systemTrayIconFeature.uid() )
	{
		return false;
	}

	if( m_systemTrayIcon == nullptr && VeyonCore::config().isTrayIconHidden() == false )
	{
		m_systemTrayIcon = new QSystemTrayIcon( this );

		QIcon icon( QStringLiteral( ":/core/icon16.png" ) );
		icon.addFile( QStringLiteral( ":/core/icon22.png" ) );
		icon.addFile( QStringLiteral( ":/core/icon32.png" ) );
		icon.addFile( QStringLiteral( ":/core/icon64.png" ) );

		m_systemTrayIcon->setIcon( icon );
		m_systemTrayIcon->show();
	}

	switch( message.command() )
	{
	case SetToolTipCommand:
		if( m_systemTrayIcon )
		{
			m_systemTrayIcon->setToolTip( message.argument( Argument::ToolTipText ).toString() );
		}
		return true;

	case ShowMessageCommand:
		if( m_systemTrayIcon )
		{
			m_systemTrayIcon->showMessage( message.argument( Argument::MessageTitle ).toString(),
										   message.argument( Argument::MessageText ).toString() );
		}
		else
		{
			QMessageBox::information( nullptr,
									  message.argument( Argument::MessageTitle ).toString(),
									  message.argument( Argument::MessageText ).toString() );
		}
		return true;

	default:
		break;
	}

	return false;
}
