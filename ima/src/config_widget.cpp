/*
 * config_widget.cpp - implementation of configuration-widget for side-bar
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


class QColorGroup;

#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QMessageBox>
#include <QtNetwork/QHostInfo>


#include "config_widget.h"
#include "client_manager.h"
#include "main_window.h"
#include "qnetworkinterface.h"
#include "tool_button.h"
#include "isd_base.h"



configWidget::configWidget( mainWindow * _main_window, QWidget * _parent ) :
	sideBarWidget( QPixmap( ":/resources/config.png" ),
			tr( "Your iTALC-configuration" ),
			tr( "In this workspace you can customize iTALC to "
				"fit your needs." ),
			_main_window, _parent )
{
	setupUi( contentParent() );


	connect( updateIntervalSB, SIGNAL( valueChanged( int ) ),
			getMainWindow()->getClientManager(),
					SLOT( updateIntervalChanged( int ) ) );
	getMainWindow()->getClientManager()->setUpdateIntervalSpinBox(
							updateIntervalSB );


	QList<QNetworkInterface> ifs = QNetworkInterface::allInterfaces();
	for( QList<QNetworkInterface>::const_iterator it = ifs.begin();
							it != ifs.end(); ++it )
	{
		QList<QNetworkAddressEntry> nae = it->addressEntries();
		for( QList<QNetworkAddressEntry>::const_iterator it2 =
								nae.begin();
						it2 != nae.end(); ++it2 )
		{
			const QString txt = it->name() + ": " +
						it2->ip().toString();
			interfaceCB->addItem( txt );
			if( __demo_network_interface == it->name() ||
				( __demo_network_interface.isEmpty() &&
							it->name() != "lo" ) )
			{
				interfaceCB->setCurrentIndex(
						interfaceCB->findText( txt ) );
				__demo_network_interface = it->name();
				__demo_master_ip = it2->ip().toString();
			}
		}
	}

	// as fallback use host-name
	if( __demo_master_ip.isEmpty() )
	{
		__demo_master_ip = QHostInfo::localHostName();
	}

	connect( interfaceCB, SIGNAL( activated( const QString & ) ), this,
				SLOT( interfaceSelected( const QString & ) ) );

	demoQualityCB->setCurrentIndex( __demo_quality );

	connect( demoQualityCB, SIGNAL( activated( int ) ), this,
					SLOT( demoQualitySelected( int ) ) );


	roleCB->setCurrentIndex( __role - 1 );

	connect( roleCB, SIGNAL( activated( int ) ), this,
						SLOT( roleSelected( int ) ) );


	balloonToolTips->setChecked( toolButton::toolTipsDisabled() );
	connect( balloonToolTips, SIGNAL( toggled( bool ) ),
			this, SLOT( toggleToolButtonTips( bool ) ) );
}




configWidget::~configWidget()
{
}




void configWidget::interfaceSelected( const QString & _if_name )
{
	if( _if_name.section( ' ', -1 ) == "127.0.0.1" )
	{
		QMessageBox::warning( NULL,
			tr( "Warning" ),
			tr( "You are trying to use the local loopback-device "
				"as network-interface. This will never work, "
				"because the local loopback is just the last "
				"alternative for running iTALC if no other "
				"network-interface was found." ),
				QMessageBox::Ok, QMessageBox::NoButton );
	}
	__demo_network_interface = _if_name.section( ':', 0, 0 );
	__demo_master_ip = _if_name.section( ' ', -1 );
}




void configWidget::demoQualitySelected( int _q )
{
	__demo_quality = _q;
}




void configWidget::roleSelected( int _role )
{
	__role = static_cast<ISD::userRoles>( _role+1 );
}




void configWidget::toggleToolButtonTips( bool _on )
{
	toolButton::setToolTipsDisabled( _on );
}



#include "config_widget.moc"

