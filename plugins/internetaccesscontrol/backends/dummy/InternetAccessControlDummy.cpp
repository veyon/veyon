/*
 * InternetAccessControlDummy.cpp - implementation of InternetAccessControlDummy class
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@veyon.io>
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

#include <QLabel>
#include <QMessageBox>

#include "InternetAccessControlDummy.h"


InternetAccessControlDummy::InternetAccessControlDummy( QObject* parent ) :
	QObject( parent )
{
}



InternetAccessControlDummy::~InternetAccessControlDummy()
{
}



QWidget* InternetAccessControlDummy::configurationWidget()
{
	auto label = new QLabel( tr( "This is the dummy backend which has no effect on internet access. "
								 "Please choose a functional backend suitable for your operating system "
								 "and environment. Visit <a href=\"http://veyon.io\">http://veyon.io</a> "
								 "for more information on how to obtain additional backend plugins." ) );
	label->setWordWrap( true );
	label->setTextInteractionFlags( Qt::TextBrowserInteraction );

	return label;
}



void InternetAccessControlDummy::blockInternetAccess()
{
	qCritical( "InternetAccessControl: trying to block internet access using dummy backend!" );

	QMessageBox::critical( nullptr, tr( "Internet access control not available" ),
						   tr( "No internet access control backend has been configured. "
							   "Please use the Veyon Configurator to change the configuration." ) );
}



void InternetAccessControlDummy::unblockInternetAccess()
{
}
