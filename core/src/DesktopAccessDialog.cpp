/*
 * DesktopAccessDialog.cpp - implementation of DesktopAccessDialog class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <QGuiApplication>
#include <QHostAddress>
#include <QHostInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>

#include "DesktopAccessDialog.h"
#include "FeatureWorkerManager.h"
#include "PlatformCoreFunctions.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"


DesktopAccessDialog::DesktopAccessDialog( QObject* parent ) :
	QObject( parent ),
	m_desktopAccessDialogFeature( Feature( QLatin1String( staticMetaObject.className() ),
										   Feature::Dialog | Feature::Service | Feature::Worker | Feature::Builtin,
										   Feature::Uid( "3dd8ec3e-7004-4936-8f2a-70699b9819be" ),
										   Feature::Uid(),
										   tr( "Desktop access dialog" ), {}, {} ) ),
	m_features( { m_desktopAccessDialogFeature } ),
	m_choice( ChoiceNone ),
	m_abortTimer( this )
{
	m_abortTimer.setSingleShot( true );
}



bool DesktopAccessDialog::isBusy( FeatureWorkerManager* featureWorkerManager ) const
{
	return featureWorkerManager->isWorkerRunning( m_desktopAccessDialogFeature );
}



void DesktopAccessDialog::exec( FeatureWorkerManager* featureWorkerManager, const QString& user, const QString& host )
{
	featureWorkerManager->startWorker( m_desktopAccessDialogFeature, FeatureWorkerManager::ManagedSystemProcess );

	m_choice = ChoiceNone;

	featureWorkerManager->sendMessage( FeatureMessage( m_desktopAccessDialogFeature.uid(), RequestDesktopAccess ).
									   addArgument( UserArgument, user ).
									   addArgument( HostArgument, host ) );

	connect( &m_abortTimer, &QTimer::timeout, this, [=]() { abort( featureWorkerManager ); } );
	m_abortTimer.start( DialogTimeout );
}



void DesktopAccessDialog::abort( FeatureWorkerManager* featureWorkerManager )
{
	featureWorkerManager->stopWorker( m_desktopAccessDialogFeature );

	m_choice = ChoiceNone;

	emit finished();
}



bool DesktopAccessDialog::handleFeatureMessage( VeyonServerInterface& server,
												const MessageContext& messageContext,
												const FeatureMessage& message )
{
	Q_UNUSED(messageContext)

	if( m_desktopAccessDialogFeature.uid() == message.featureUid() &&
		message.command() == ReportDesktopAccessChoice )
	{
		m_choice = QVariantHelper<Choice>::value( message.argument( ChoiceArgument ) );

		server.featureWorkerManager().stopWorker( m_desktopAccessDialogFeature );

		m_abortTimer.stop();

		emit finished();

		return true;
	}

	return false;
}



bool DesktopAccessDialog::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	if( message.featureUid() != m_desktopAccessDialogFeature.uid() ||
		message.command() != RequestDesktopAccess )
	{
		return false;
	}

	const auto result = requestDesktopAccess( message.argument( UserArgument ).toString(),
											  message.argument( HostArgument ).toString() );

	FeatureMessage reply( m_desktopAccessDialogFeature.uid(), ReportDesktopAccessChoice );
	reply.addArgument( ChoiceArgument, result );

	return worker.sendFeatureMessageReply( reply );
}



DesktopAccessDialog::Choice DesktopAccessDialog::requestDesktopAccess( const QString& user, const QString& host )
{
	auto hostName = host;

	const QHostAddress address( host );
	if( address.protocol() == QAbstractSocket::IPv4Protocol ||
		address.protocol() == QAbstractSocket::IPv6Protocol )
	{
		hostName = QHostInfo::fromName( host ).hostName();
		if( hostName.isEmpty() )
		{
			hostName = host;
		}
		else
		{
			hostName = QStringLiteral( "%1 (%2)" ).arg( hostName, host );
		}
	}

	qApp->setQuitOnLastWindowClosed( false );

	QMessageBox m( QMessageBox::Question,
				   tr( "Confirm desktop access" ),
				   tr( "The user %1 at computer %2 wants to access your desktop. Do you want to grant access?" ).
				   arg( user, hostName ), QMessageBox::Yes | QMessageBox::No );

	auto neverBtn = m.addButton( tr( "Never for this session" ), QMessageBox::NoRole );
	auto alwaysBtn = m.addButton( tr( "Always for this session" ), QMessageBox::YesRole );

	m.setEscapeButton( m.button( QMessageBox::No ) );
	m.setDefaultButton( neverBtn );

	VeyonCore::platform().coreFunctions().raiseWindow( &m );

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
