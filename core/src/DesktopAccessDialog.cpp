/*
 * DesktopAccessDialog.cpp - implementation of DesktopAccessDialog class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QCoreApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>

#include "DesktopAccessDialog.h"
#include "FeatureWorkerManager.h"
#include "LocalSystem.h"


DesktopAccessDialog::DesktopAccessDialog() :
	m_desktopAccessDialogFeature( Feature( Feature::Dialog | Feature::Service | Feature::Worker | Feature::Builtin,
										   Feature::Uid( "3dd8ec3e-7004-4936-8f2a-70699b9819be" ),
										   tr( "Desktop access dialog" ), QString(), QString() ) ),
	m_features(),
	m_choice( ChoiceNone ),
	m_abortTimer( new QTimer( this ) )
{
	m_features += m_desktopAccessDialogFeature;

	m_abortTimer->setSingleShot( true );
}



bool DesktopAccessDialog::isBusy( FeatureWorkerManager* featureWorkerManager ) const
{
	return featureWorkerManager->isWorkerRunning( m_desktopAccessDialogFeature );
}



void DesktopAccessDialog::exec( FeatureWorkerManager* featureWorkerManager, QString user, QString host )
{
	featureWorkerManager->startWorker( m_desktopAccessDialogFeature );

	m_choice = ChoiceNone;

	featureWorkerManager->sendMessage( FeatureMessage( m_desktopAccessDialogFeature.uid(), RequestDesktopAccess ).
									   addArgument( UserArgument, user ).
									   addArgument( HostArgument, host ) );

	connect( m_abortTimer, &QTimer::timeout, [=]() { abort( featureWorkerManager ); } );
	m_abortTimer->start( DialogTimeout );
}



void DesktopAccessDialog::abort( FeatureWorkerManager* featureWorkerManager )
{
	featureWorkerManager->stopWorker( m_desktopAccessDialogFeature );

	m_choice = ChoiceNone;

	emit finished();
}



bool DesktopAccessDialog::startMasterFeature( const Feature& feature,
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



bool DesktopAccessDialog::handleMasterFeatureMessage( const FeatureMessage& message,
													  ComputerControlInterface& computerControlInterface )
{
	Q_UNUSED(message);
	Q_UNUSED(computerControlInterface);

	return false;
}



bool DesktopAccessDialog::stopMasterFeature( const Feature& feature,
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



bool DesktopAccessDialog::handleServiceFeatureMessage( const FeatureMessage& message,
													   FeatureWorkerManager& featureWorkerManager )
{
	if( m_desktopAccessDialogFeature.uid() == message.featureUid() &&
			message.command() == ReportDesktopAccessChoice )
	{
		m_choice = static_cast<Choice>( message.argument( ChoiceArgument ).toInt() );

		featureWorkerManager.stopWorker( m_desktopAccessDialogFeature );

		m_abortTimer->stop();

		emit finished();

		return true;
	}

	return false;
}



bool DesktopAccessDialog::handleWorkerFeatureMessage( const FeatureMessage& message )
{
	if( message.featureUid() != m_desktopAccessDialogFeature.uid() ||
			message.command() != RequestDesktopAccess )
	{
		return false;
	}

	int result = requestDesktopAccess( message.argument( UserArgument ).toString(),
									   message.argument( HostArgument ).toString() );

	return FeatureMessage( m_desktopAccessDialogFeature.uid(), ReportDesktopAccessChoice ).
			addArgument( ChoiceArgument, result ).
			send( message.ioDevice() );
}



DesktopAccessDialog::Choice DesktopAccessDialog::requestDesktopAccess( QString user, QString host )
{
	QMessageBox m( QMessageBox::Question,
				   tr( "Confirm desktop access" ),
				   tr( "The user %1 at computer %2 wants to access your desktop. Do you want to grant access?" ).
				   arg( user, host ), QMessageBox::Yes | QMessageBox::No );

	auto neverBtn = m.addButton( tr( "Never for this session" ), QMessageBox::NoRole );
	auto alwaysBtn = m.addButton( tr( "Always for this session" ), QMessageBox::YesRole );

	m.setEscapeButton( m.button( QMessageBox::No ) );
	m.setDefaultButton( neverBtn );

	LocalSystem::activateWindow( &m );

	const auto result = m.exec();

	if( m.clickedButton() == neverBtn )
	{
		return ChoiceNever;
	}
	else if( m.clickedButton() == alwaysBtn )
	{
		return ChoiceAlways;
	}
	else if( result == QMessageBox::Yes )
	{
		return ChoiceYes;
	}

	return ChoiceNo;
}
