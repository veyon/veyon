/*
 *  RemoteAccessPage.h - logic for RemoteAccessPage
 *
 *  Copyright (c) 2019-2020 Tobias Junghans <tobydox@veyon.io>
 *
 *  This file is part of Veyon - https://veyon.io
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include "QmlCore.h"
#include "RemoteAccessPage.h"
#include "Screenshot.h"
#include "VncViewItem.h"


RemoteAccessPage::RemoteAccessPage( const ComputerControlInterface::Pointer& computerControlInterface,
									bool viewOnly,
									QQuickItem* parent ) :
	QObject( parent ),
	m_computerControlInterface( computerControlInterface ),
	m_view( new VncViewItem( m_computerControlInterface ) ),
	m_item( VeyonCore::qmlCore().createItemFromFile( QStringLiteral("qrc:/remoteaccess/RemoteAccessPage.qml"), parent, this ) )
{
	m_view->setViewOnly( viewOnly );

	connect( m_item, &QObject::destroyed, this, &RemoteAccessPage::deleteLater );
}



RemoteAccessPage::~RemoteAccessPage()
{
	delete m_view;
}



QQuickItem* RemoteAccessPage::view() const
{
	return m_view;
}


/*
void RemoteAccessPage::takeScreenshot()
{
	Screenshot().take( m_computerControlInterface );
}
*/
