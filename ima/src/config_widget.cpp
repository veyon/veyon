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



configWidget::configWidget( mainWindow * _main_window, QWidget * _parent ) :
	sideBarWidget( QPixmap( ":/resources/config.png" ),
			tr( "Your iTALC-configuration" ),
			tr( "In this workspace you can customize iTALC to "
				"fit your needs." ),
			_main_window, _parent )
{
	QBoxLayout * l = static_cast<QBoxLayout *>(
						contentParent()->layout() );

	QPixmap pm( 1000, 1 );
	pm.fill( QColor( 0, 0, 0 ) );

	QFont f;
	f.setPixelSize( 12 );

	QWidget * ui_w = new QWidget( contentParent() );
	l->addWidget( ui_w );
	QHBoxLayout * ui_layout = new QHBoxLayout( ui_w );
	QLabel * uipm_lbl = new QLabel( ui_w );
	uipm_lbl->setPixmap( QPixmap( ":/resources/clock.png" ) );
	uipm_lbl->setFixedSize( 24, 16 );

	QLabel * ui_lbl = new QLabel( tr( "Update interval" ), ui_w );
	ui_lbl->setFont( f );

	ui_layout->addWidget( uipm_lbl );
	ui_layout->addWidget( ui_lbl );

	QLabel * hr1 = new QLabel( contentParent() );
	l->addWidget( hr1 );
	hr1->setPixmap( pm );
	hr1->setFixedSize( 1000, 1 );
	l->addSpacing( 6 );

	QSpinBox * ui_sb = new QSpinBox( contentParent() );
	l->addWidget( ui_sb );
	ui_sb->setRange( 1, 60 );
	ui_sb->setValue( 1 );
	ui_sb->setSuffix( " " + tr( "seconds" ) );
	ui_sb->setSpecialValueText( "1 " + tr( "second" ) );
	ui_sb->setFont( f );
	ui_sb->setWhatsThis( tr( "Here you can set the interval between "
					"updates of clients. Higher values "
					"result in lower network-traffic and "
					"lower CPU-usage." ) );
	connect( ui_sb, SIGNAL( valueChanged( int ) ),
			getMainWindow()->getClientManager(),
					SLOT( updateIntervalChanged( int ) ) );
	getMainWindow()->getClientManager()->setUpdateIntervalSpinBox( ui_sb );

	l->addSpacing( 20 );

/*	QWidget * bi_w = new QWidget( contentParent() );
	l->addWidget( bi_w );
	QHBoxLayout * bi_layout = new QHBoxLayout( bi_w );
	QLabel * bipm_lbl = new QLabel( bi_w );
	bipm_lbl->setPixmap( embed::getIconPixmap( "toolbar" ) );
	bipm_lbl->setFixedSize( 24, 16 );
	QLabel * bi_lbl = new QLabel( tr( "Toolbar" ), bi_w );
	bi_lbl->setFont( f );

	bi_layout->addWidget( bipm_lbl );
	bi_layout->addWidget( bi_lbl );

	QLabel * hr2 = new QLabel( contentParent() );
	l->addWidget( hr2 );
	hr2->setPixmap( pm );
	hr2->setFixedSize( 1000, 1 );
	l->addSpacing( 6 );

	QCheckBox * bigicons_cb = new QCheckBox( tr( "Big icons" ),
							contentParent() );
	l->addWidget( bigicons_cb );
	bigicons_cb->setFont( f );
	bigicons_cb->setWhatsThis( tr( "When activating this option, you'll "
						"have big icons in the "
						"toolbar. This is useful for "
						"faster access to important "
						"functions of iTALC. Small "
						"icons are good if the iTALC-"
						"window isn't maximized or the "
						"toolbar doesn't fit on the "
						"screen." ) );
	// TODO!!
	if( italc::inst()->usesBigPixmaps() )
	{
		bigicons_cb->setChecked( TRUE );
	}
        connect( bigicons_cb, SIGNAL( toggled( bool ) ), italc::inst(),
					SLOT( setUsesBigPixmaps( bool ) ) );

	// spacer
	l->addSpacing( 20 );*/

	QWidget * ni_w = new QWidget( contentParent() );
	l->addWidget( ni_w );
	QHBoxLayout * ni_layout = new QHBoxLayout( ni_w );
	QLabel * nipm_lbl = new QLabel( ni_w );
	nipm_lbl->setPixmap( QPixmap( ":/resources/network.png" ) );
	nipm_lbl->setFixedSize( 24, 16 );
	QLabel * if_lbl = new QLabel( tr( "Network interface/IP-address" ),
									ni_w );
	if_lbl->setFont( f );

	ni_layout->addWidget( nipm_lbl );
	ni_layout->addWidget( if_lbl );

	QLabel * hr3 = new QLabel( contentParent() );
	l->addWidget( hr3 );
	hr3->setPixmap( pm );
	hr3->setFixedSize( 1000, 1 );

	l->addSpacing( 6 );


	QComboBox * if_cb = new QComboBox( contentParent() );
	l->addWidget( if_cb );
	QList<QHostAddress> host_addresses = QHostInfo::fromName(
				QHostInfo::localHostName() ).addresses();
	for( QList<QHostAddress>::iterator it = host_addresses.begin();
					it != host_addresses.end(); ++it )
	{
		QString ip = it->toString();
		if_cb->addItem( ip );
		if( MASTER_HOST == ip )
		{
			if_cb->setCurrentIndex( if_cb->findText( ip ) );
		}
	}
	if_cb->setWhatsThis( tr( "Here you can select the network interface "
					"with the according IP-address. This "
					"is needed if you computer has more "
					"than one network-card and not all of "
					"one of them have a connection to the "
					"clients. Incorrect settings may "
					"result in being unable to show a "
					"demo." ) );
	connect( if_cb, SIGNAL( activated( const QString & ) ), this,
				SLOT( interfaceSelected( const QString & ) ) );


	l->addStretch();
}




configWidget::~configWidget()
{
}




void configWidget::interfaceSelected( const QString & _if_name )
{
	if( _if_name.section( ' ', -1 ) == "127.0.0.1" )
	{
		// TODO: handle this case...
		//QMessageBox::warning( NULL, tr( "Warning" ), tr( "You are trying to use the local loopback-device as network-interface. This will never work, because the local loopback is just the last alternative for running iTALC if no other network-interface was found." ), QMessageBox::Ok, QMessageBox::NoButton );
	}
	// the very right field/section contains the corresponding ip-address
	MASTER_HOST = _if_name.section( ' ', -1 );
}



#include "config_widget.moc"

