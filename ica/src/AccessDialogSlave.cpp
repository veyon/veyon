/*
 * AccessDialogSlave.cpp - an IcaSlave providing an access dialog
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 * Copyright (c) 2010 Univention GmbH
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

#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>

#include "AccessDialogSlave.h"
#include "DesktopAccessPermission.h"
#include "ItalcSlaveManager.h"
#include "LocalSystem.h"


AccessDialogSlave::AccessDialogSlave() :
	IcaSlave()
{
}



AccessDialogSlave::~AccessDialogSlave()
{
}


static int exec( const QString &user, const QString &host,
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
		AccessDialogSlave::tr( "Confirm desktop access" ),
		AccessDialogSlave::tr( "The user %1 at host %2 wants to access your "
								"desktop. Do you want to grant access?" ).
									arg( user ).arg( host ), btns );

	QPushButton *neverBtn = NULL, *alwaysBtn = NULL;
	if( choiceFlags & DesktopAccessPermission::ChoiceNever )
	{
		neverBtn = m.addButton(
						AccessDialogSlave::tr( "Never for this session" ),
														QMessageBox::NoRole );
		m.setDefaultButton( neverBtn );
	}
	if( choiceFlags & DesktopAccessPermission::ChoiceAlways )
	{
		alwaysBtn = m.addButton(
						AccessDialogSlave::tr( "Always for this session" ),
														QMessageBox::YesRole );
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




bool AccessDialogSlave::handleMessage( const Ipc::Msg &m )
{
	if( m.cmd() == ItalcSlaveManager::AccessDialog::Ask )
	{
		QString user = m.arg( ItalcSlaveManager::AccessDialog::User );
		QString host = m.arg( ItalcSlaveManager::AccessDialog::Host );
		int choiceFlags = m.argV( ItalcSlaveManager::AccessDialog::ChoiceFlags ).toInt();

		choiceFlags = exec( user, host, choiceFlags );

		Ipc::Msg( ItalcSlaveManager::AccessDialog::ReportChoice ).
			addArg( ItalcSlaveManager::AccessDialog::ChoiceFlags, choiceFlags ).
				send( this );

		return true;
	}

	return false;
}

