/*
 * ConfigWidget.cpp - implementation of configuration-widget for side-bar
 *
 * Copyright (c) 2004-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "ConfigWidget.h"
#include "ClassroomManager.h"
#include "ItalcCore.h"
#include "MainWindow.h"
#include "ToolButton.h"
#ifdef ITALC3
#include "MasterCore.h"
#include "PersonalConfig.h"
#endif


ConfigWidget::ConfigWidget( MainWindow * _main_window, QWidget * _parent ) :
	SideBarWidget( QPixmap( ":/resources/config.png" ),
			tr( "Your iTALC-configuration" ),
			tr( "In this workspace you can customize iTALC to "
				"fit your needs." ),
			_main_window, _parent )
{
	setupUi( contentParent() );

#ifdef ITALC3

#define LOAD_AND_CONNECT_PROPERTY(property,type,widget,setvalue,signal,slot) \
		widget->setvalue( MasterCore::personalConfig->property() );  \
		connect( widget, SIGNAL(signal(type)),                       \
			MasterCore::personalConfig, SLOT(slot(type)) );

	LOAD_AND_CONNECT_PROPERTY(clientUpdateInterval,
					int,
					updateIntervalSB,
					setValue,
					valueChanged,
					setClientUpdateInterval);

	LOAD_AND_CONNECT_PROPERTY(demoQuality,
					int,
					demoQualityCB,
					setCurrentIndex,
					activated,
					setDemoQuality);

	LOAD_AND_CONNECT_PROPERTY(defaultRole,
					int,
					roleCB,
					setCurrentIndex,
					activated,
					setDefaultRole);

	LOAD_AND_CONNECT_PROPERTY(defaultDomain,
					const QString &,
					domainEdit,
					setText,
					textChanged,
					setDefaultDomain);

	LOAD_AND_CONNECT_PROPERTY(clientDoubleClickAction,
					int,
					clientDoubleClickActionCB,
					setCurrentIndex,
					activated,
					setClientDoubleClickAction);

	LOAD_AND_CONNECT_PROPERTY(toolButtonIconOnlyMode,
					bool,
					iconOnlyToolButtons,
					setChecked,
					toggled,
					setToolButtonIconOnlyMode);

	LOAD_AND_CONNECT_PROPERTY(noToolTips,
					bool,
					balloonToolTips,
					setChecked,
					toggled,
					setNoToolTips);
#else
	connect( updateIntervalSB, SIGNAL( valueChanged( double ) ),
				mainWindow()->getClassroomManager(),
					SLOT( updateIntervalChanged( double ) ) );
	mainWindow()->getClassroomManager()->setUpdateIntervalSpinBox( updateIntervalSB );


	roleCB->setCurrentIndex( ItalcCore::role - 1 );

	connect( roleCB, SIGNAL( activated( int ) ), this,
						SLOT( roleSelected( int ) ) );


	balloonToolTips->setChecked( ToolButton::toolTipsDisabled() );
	connect( balloonToolTips, SIGNAL( toggled( bool ) ),
			this, SLOT( toggleToolButtonTips( bool ) ) );

	iconOnlyToolButtons->setChecked( ToolButton::iconOnlyMode() );
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
#endif
}




ConfigWidget::~ConfigWidget()
{
}




void ConfigWidget::roleSelected( int _role )
{
	ItalcCore::role = static_cast<ItalcCore::UserRole>( _role );
}




#ifndef ITALC3
void ConfigWidget::toggleToolButtonTips( bool _on )
{
	ToolButton::setToolTipsDisabled( _on );
}




void ConfigWidget::toggleIconOnlyToolButtons( bool _on )
{
	ToolButton::setIconOnlyMode( _on );
}




void ConfigWidget::domainChanged( const QString & _domain )
{
	__default_domain = _domain;
}

#endif




