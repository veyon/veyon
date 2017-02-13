/*
 * SystemTrayIcon.cpp - implementation of SystemTrayIcon class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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
#include "ItalcCore.h"
#include "ItalcConfiguration.h"


SystemTrayIcon::SystemTrayIcon() :
	m_systemTrayIconFeature( Feature( Feature::BuiltinService,
									  Feature::ScopeAll,
									  Feature::Uid( "8e997d84-ebb9-430f-8f72-d45d9821963d" ),
									  QString(), QString(), QString() ) ),
	m_features(),
	m_systemTrayIcon( nullptr )
{
	m_features += m_systemTrayIconFeature;
}



void SystemTrayIcon::setToolTip( const QString& toolTipText,
								 FeatureWorkerManager& featureWorkerManager )
{
	if( featureWorkerManager.isWorkerRunning( m_systemTrayIconFeature ) == false )
	{
		featureWorkerManager.startWorker( m_systemTrayIconFeature );
	}

	FeatureMessage featureMessage( m_systemTrayIconFeature.uid(), SetToolTipCommand );
	featureMessage.addArgument( ToolTipTextArgument, toolTipText );

	featureWorkerManager.sendMessage( featureMessage );
}



void SystemTrayIcon::showMessage( const QString& messageTitle,
								  const QString& messageText,
								  FeatureWorkerManager& featureWorkerManager  )
{
	if( featureWorkerManager.isWorkerRunning( m_systemTrayIconFeature ) == false )
	{
		featureWorkerManager.startWorker( m_systemTrayIconFeature );
	}

	FeatureMessage featureMessage( m_systemTrayIconFeature.uid(), ShowMessageCommand );
	featureMessage.addArgument( MessageTitleArgument, messageTitle );
	featureMessage.addArgument( MessageTextArgument, messageText );

	featureWorkerManager.sendMessage( featureMessage );
}



const FeatureList &SystemTrayIcon::featureList() const
{
	return m_features;
}



bool SystemTrayIcon::startMasterFeature( const Feature& feature,
										 const ComputerControlInterfaceList& computerControlInterfaces,
										 QWidget* parent )
{
	Q_UNUSED(feature);
	Q_UNUSED(computerControlInterfaces);
	Q_UNUSED(parent);

	return false;
}



bool SystemTrayIcon::stopMasterFeature( const Feature& feature,
										const ComputerControlInterfaceList& computerControlInterfaces,
										QWidget* parent )
{
	Q_UNUSED(feature);
	Q_UNUSED(computerControlInterfaces);
	Q_UNUSED(parent);

	return false;
}



bool SystemTrayIcon::handleServiceFeatureMessage( const FeatureMessage& message, QIODevice* ioDevice,
												  FeatureWorkerManager& featureWorkerManager )
{
	if( m_systemTrayIconFeature.uid() == message.featureUid() )
	{
		// forward message to worker
		if( featureWorkerManager.isWorkerRunning( m_systemTrayIconFeature ) == false )
		{
			featureWorkerManager.startWorker( m_systemTrayIconFeature );
		}

		featureWorkerManager.sendMessage( message );

		return true;
	}

	return false;
}



bool SystemTrayIcon::handleWorkerFeatureMessage( const FeatureMessage& message, QIODevice* ioDevice )
{
	if( message.featureUid() != m_systemTrayIconFeature.uid() )
	{
		return false;
	}

	if( m_systemTrayIcon == nullptr && ItalcCore::config->isTrayIconHidden() == false )
	{
		m_systemTrayIcon = new QSystemTrayIcon;

		QIcon icon( ":/resources/icon16.png" );
		icon.addFile( ":/resources/icon22.png" );
		icon.addFile( ":/resources/icon32.png" );

		m_systemTrayIcon->setIcon( icon );
		m_systemTrayIcon->show();
	}

	switch( message.command() )
	{
	case SetToolTipCommand:
		if( m_systemTrayIcon )
		{
			m_systemTrayIcon->setToolTip( message.argument( ToolTipTextArgument ).toString() );
		}
		return true;

	case ShowMessageCommand:
		if( m_systemTrayIcon )
		{
			m_systemTrayIcon->showMessage( message.argument( MessageTitleArgument ).toString(),
										   message.argument( MessageTextArgument ).toString() );
		}
		else
		{
			QMessageBox::information( nullptr,
									  message.argument( MessageTitleArgument ).toString(),
									  message.argument( MessageTextArgument ).toString() );
		}
		return true;

	default:
		break;
	}

	return false;
}
