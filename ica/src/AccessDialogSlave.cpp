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
#include "ItalcSlaveManager.h"
#include "LocalSystem.h"


AccessDialogSlave::AccessDialogSlave() :
	IcaSlave()
{
}



AccessDialogSlave::~AccessDialogSlave()
{
}


ItalcSlaveManager::AccessDialogResult AccessDialogSlave::exec(
														const QString &host )
{
	QMessageBox m( QMessageBox::Question,
					tr( "Confirm access" ),
					tr( "Somebody at host %1 tries to access your screen. "
						"Do you want to grant him/her access?" ).
							arg( host ),
					QMessageBox::Yes | QMessageBox::No );

	QPushButton * neverBtn = m.addButton( tr( "Never for this session" ),
														QMessageBox::NoRole );
	QPushButton * alwaysBtn = m.addButton( tr( "Always for this session" ),
														QMessageBox::YesRole );
	m.setDefaultButton( neverBtn );
	m.setEscapeButton( m.button( QMessageBox::No ) );

	LocalSystem::activateWindow( &m );

	const int res = m.exec();
	if( m.clickedButton() == neverBtn )
	{
		return ItalcSlaveManager::AccessNever;
	}
	else if( m.clickedButton() == alwaysBtn )
	{
		return ItalcSlaveManager::AccessAlways;
	}
	else if( res == QMessageBox::Yes )
	{
		return ItalcSlaveManager::AccessYes;
	}

	return ItalcSlaveManager::AccessNo;
}




bool AccessDialogSlave::handleMessage( const Ipc::Msg &m )
{
	if( m.cmd() == ItalcSlaveManager::AccessDialog::Ask )
	{
		QString host = m.arg( ItalcSlaveManager::AccessDialog::Host );

		ItalcSlaveManager::AccessDialogResult res = exec( host );

		Ipc::Msg( ItalcSlaveManager::AccessDialog::ReportResult ).
			addArg( ItalcSlaveManager::AccessDialog::Result, res ).
				send( this );

		return true;
	}

	return false;
}

