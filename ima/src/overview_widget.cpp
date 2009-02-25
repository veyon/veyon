/*
 * overview_widget.cpp - implementation of overview-widget for side-bar
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


#include <QtGui/QLabel>
#include <QtGui/QLayout>

#include "overview_widget.h"



overviewWidget::overviewWidget( mainWindow * _main_window, QWidget * _parent ) :
	sideBarWidget( QPixmap( ":/resources/overview.png" ),
			tr( "Overview" ),
			tr( "Some basic information on iTALC and how to use "
				"it." ), _main_window, _parent )
{
	QFont f;

	QLabel * welcome = new QLabel( tr( "Welcome to iTALC!" ) + "\n",
							contentParent() );
	contentParent()->layout()->addWidget( welcome );
	f.setPixelSize( 14 );
	f.setBold( TRUE );
	f.setItalic( TRUE );	// leads to crashes sometimes because of
	//			// bug in qt's font-db-mgm
	welcome->setFont( f );
	welcome->setWordWrap( TRUE );
	f.setBold( FALSE );
	f.setItalic( FALSE );

	f.setPixelSize( 12 );
	QLabel * here = new QLabel( contentParent() );
	contentParent()->layout()->addWidget( here );
	here->setPixmap( QPixmap( ":/resources/back.png" ) ); 

	QLabel * workbar_desc = new QLabel(
		tr( "Here you see the working-bar which contains several "
			"buttons. Each button is connected to a workspace. "
			"Just take a look at the available workspaces by "
			"clicking on the corresponding button.\n\n"
			"If you need help take a look at the help-workspace!"
						) + "\n ", contentParent() );
	contentParent()->layout()->addWidget( workbar_desc );
	workbar_desc->setFont( f );
	workbar_desc->setWordWrap( TRUE );

	QLabel * cm_desc = new QLabel( tr( "With the client-manager you can "
						"add or remove clients "
						"(computers) and switch "
						"between classrooms." ) + "\n ",
							contentParent() );
	contentParent()->layout()->addWidget( cm_desc );
	cm_desc->setFont( f );
	cm_desc->setWordWrap( TRUE );

	QLabel * ul_desc = new QLabel(
			tr( "If you want to know, which users are logged in, "
				"you can open the user-list." ) + "\n ",
					contentParent() );
	contentParent()->layout()->addWidget( ul_desc );
	ul_desc->setFont( f );
	ul_desc->setWordWrap( TRUE );

	QLabel * screenshot_desc = new QLabel(
			tr( "The snapshot-workspace is a very useful tool. "
				"It let's you manage the snapshots you made."
						) + "\n ", contentParent() );
	contentParent()->layout()->addWidget( screenshot_desc );
	screenshot_desc->setFont( f );
	screenshot_desc->setWordWrap( TRUE );

	QLabel * config_desc = new QLabel(
			tr( "Of course you can configure iTALC. This is "
				"usually done with the configuration-"
				"workspace." ) + "\n ",
							contentParent() );
	contentParent()->layout()->addWidget( config_desc );
	config_desc->setFont( f );
	config_desc->setWordWrap( TRUE );

	QLabel * filler = new QLabel( contentParent() );
	contentParent()->layout()->addWidget( filler );
	filler->setFixedHeight( 2048 );

}



#include "overview_widget.moc"

