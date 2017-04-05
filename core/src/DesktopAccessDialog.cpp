/*
 * DesktopAccessDialog.cpp - implementation of DesktopAccessDialog class
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

#include <QCoreApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QThread>
#include <QTime>

#include "DesktopAccessDialog.h"
#include "FeatureWorkerManager.h"
#include "LocalSystem.h"


DesktopAccessDialog::DesktopAccessDialog() :
	m_desktopAccessDialogFeature( Feature( Feature::Dialog | Feature::Service | Feature::Worker | Feature::Builtin,
										   Feature::Uid( "3dd8ec3e-7004-4936-8f2a-70699b9819be" ),
										   tr( "Desktop access dialog" ), QString(), QString() ) ),
	m_features(),
	m_result( DesktopAccessPermission::ChoiceNone )
{
	m_features += m_desktopAccessDialogFeature;
}



DesktopAccessPermission::Choice DesktopAccessDialog::exec( FeatureWorkerManager& featureWorkerManager,
														   const QString& user,
														   const QString& host,
														   int choiceFlags )
{
	if( featureWorkerManager.isWorkerRunning( m_desktopAccessDialogFeature ) == false )
	{
		featureWorkerManager.startWorker( m_desktopAccessDialogFeature );
	}

	m_result = DesktopAccessPermission::ChoiceNone;

	featureWorkerManager.sendMessage( FeatureMessage( m_desktopAccessDialogFeature.uid(), GetDesktopAccessPermission ).
									  addArgument( UserArgument, user ).
									  addArgument( HostArgument, host ).
									  addArgument( ChoiceFlagsArgument, choiceFlags ) );

	// wait for answer
	QTime t;
	t.start();

	while( m_result == DesktopAccessPermission::ChoiceNone &&
		   featureWorkerManager.isWorkerRunning( m_desktopAccessDialogFeature ) )
	{
		QCoreApplication::processEvents();
		if( t.elapsed() > DialogTimeout )
		{
			featureWorkerManager.stopWorker( m_desktopAccessDialogFeature );
			break;
		}

		QThread::msleep( SleepTime );
	}

	if( m_result != DesktopAccessPermission::ChoiceYes )
	{
		return DesktopAccessPermission::ChoiceNo;
	}

	return DesktopAccessPermission::ChoiceYes;
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
			message.command() == ReportDesktopAccessPermission )
	{
		m_result = static_cast<DesktopAccessPermission::Choice>( message.argument( ResultArgument ).toInt() );

		return true;
	}

	return false;
}



bool DesktopAccessDialog::handleWorkerFeatureMessage( const FeatureMessage& message )
{
	if( message.featureUid() != m_desktopAccessDialogFeature.uid() ||
			message.command() != GetDesktopAccessPermission )
	{
		return false;
	}

	int result = getDesktopAccessPermission( message.argument( UserArgument ).toString(),
											 message.argument( HostArgument ).toString(),
											 message.argument( ChoiceFlagsArgument ).toInt() );

	return FeatureMessage( m_desktopAccessDialogFeature.uid(), ReportDesktopAccessPermission ).
			addArgument( ResultArgument, result ).
			send( message.ioDevice() );
}



DesktopAccessPermission::Choice DesktopAccessDialog::getDesktopAccessPermission( const QString &user,
																				 const QString &host,
																				 int choiceFlags )
{
	QMessageBox::StandardButtons btns = QMessageBox::NoButton;

	if( choiceFlags & DesktopAccessPermission::ChoiceYes )
	{
		btns |= QMessageBox::Yes;
	}
	if( choiceFlags & DesktopAccessPermission::ChoiceNo )
	{
		btns |= QMessageBox::No;
	}

	QMessageBox m( QMessageBox::Question,
				   tr( "Confirm desktop access" ),
				   tr( "The user %1 at host %2 wants to access your desktop. Do you want to grant access?" ).
				   arg( user ).arg( host ), btns );

	QPushButton *neverBtn = NULL, *alwaysBtn = NULL;
	if( choiceFlags & DesktopAccessPermission::ChoiceNever )
	{
		neverBtn = m.addButton( tr( "Never for this session" ), QMessageBox::NoRole );
		m.setDefaultButton( neverBtn );
	}
	if( choiceFlags & DesktopAccessPermission::ChoiceAlways )
	{
		alwaysBtn = m.addButton( tr( "Always for this session" ), QMessageBox::YesRole );
	}

	m.setEscapeButton( m.button( QMessageBox::No ) );

	LocalSystem::activateWindow( &m );

	const int res = m.exec();
	if( m.clickedButton() == neverBtn )
	{
		return DesktopAccessPermission::ChoiceNever;
	}
	else if( m.clickedButton() == alwaysBtn )
	{
		return DesktopAccessPermission::ChoiceAlways;
	}
	else if( res == QMessageBox::Yes )
	{
		return DesktopAccessPermission::ChoiceYes;
	}

	return DesktopAccessPermission::ChoiceNo;
}
