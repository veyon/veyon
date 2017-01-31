/*
 * MessageBoxSlave.cpp - an IcaSlave providing message boxes
 *
 * Copyright (c) 2010-2016 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "MessageBoxSlave.h"
#include "ItalcSlaveManager.h"


MessageBoxSlave::MessageBoxSlave() :
	IcaSlave()
{
}



MessageBoxSlave::~MessageBoxSlave()
{
}



bool MessageBoxSlave::handleMessage( const Ipc::Msg &m )
{
	if( m.cmd() == ItalcSlaveManager::MessageBoxSlave::ShowMessageBox )
	{
		QMessageBox dialog;

		QString msg = m.arg( ItalcSlaveManager::MessageBoxSlave::Text );
		msg.replace(QRegExp("((?:https?|ftp)://\\S+)"), "<a href=\"\\1\">\\1</a>");
		msg.replace("\n","<br />");

		dialog.setTextInteractionFlags(Qt::TextBrowserInteraction);
		dialog.setTextFormat(Qt::RichText);
		dialog.setText(msg);
		dialog.setWindowTitle(m.arg( ItalcSlaveManager::MessageBoxSlave::Title ));


		if( m.arg( ItalcSlaveManager::MessageBoxSlave::Icon ).toInt() == QMessageBox::Warning )
		{
			dialog.setIcon(QMessageBox::Icon::Warning );
		}
		else if( m.arg( ItalcSlaveManager::MessageBoxSlave::Icon ).toInt() == QMessageBox::Critical )
		{
			dialog.setIcon(QMessageBox::Icon::Critical );
		}
		else
		{
			dialog.setIcon(QMessageBox::Icon::Information );
		}
		dialog.exec();
		return true;
	}

	return false;
}

