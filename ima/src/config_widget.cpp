/*
 * config_widget.cpp - implementation of configuration-widget for side-bar
 *
 * Copyright (c) 2004-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QMessageBox>
#include <QtNetwork/QHostInfo>


#include "config_widget.h"
#include "classroom_manager.h"
#include "MainWindow.h"
#include "tool_button.h"
#include "isd_base.h"



configWidget::configWidget( MainWindow * _main_window, QWidget * _parent ) :
	SideBarWidget( QPixmap( ":/resources/config.png" ),
			tr( "Your iTALC-configuration" ),
			tr( "In this workspace you can customize iTALC to "
				"fit your needs." ),
			_main_window, _parent )
{
	setupUi( contentParent() );

	connect( updateIntervalSB, SIGNAL( valueChanged( int ) ),
				mainWindow()->getClassroomManager(),
					SLOT( updateIntervalChanged( int ) ) );
	mainWindow()->getClassroomManager()->setUpdateIntervalSpinBox(
							updateIntervalSB );


	demoQualityCB->setCurrentIndex( __demo_quality );

	connect( demoQualityCB, SIGNAL( activated( int ) ), this,
					SLOT( demoQualitySelected( int ) ) );


	roleCB->setCurrentIndex( __role - 1 );

	connect( roleCB, SIGNAL( activated( int ) ), this,
						SLOT( roleSelected( int ) ) );


	balloonToolTips->setChecked( toolButton::toolTipsDisabled() );
	connect( balloonToolTips, SIGNAL( toggled( bool ) ),
			this, SLOT( toggleToolButtonTips( bool ) ) );

	iconOnlyToolButtons->setChecked( toolButton::iconOnlyMode() );
	connect( iconOnlyToolButtons, SIGNAL( toggled( bool ) ),
			this, SLOT( toggleIconOnlyToolButtons( bool ) ) );

	domainEdit->setText( __default_domain );
	connect( domainEdit, SIGNAL( textChanged( const QString & ) ),
			this, SLOT( domainChanged( const QString & ) ) );

	clientDoubleClickActionCB->setCurrentIndex( mainWindow()->
				getClassroomManager()->clientDblClickAction() );
	connect( clientDoubleClickActionCB, SIGNAL( activated( int ) ),
				mainWindow()->getClassroomManager(),
				SLOT( setClientDblClickAction( int ) ) );
}




configWidget::~configWidget()
{
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




void configWidget::toggleIconOnlyToolButtons( bool _on )
{
	toolButton::setIconOnlyMode( _on );
}




void configWidget::domainChanged( const QString & _domain )
{
	__default_domain = _domain;
}



#include "config_widget.moc"

