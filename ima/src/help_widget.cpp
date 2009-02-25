/*
 * help_widget.cpp - implementation of help-widget for side-bar
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
#include <QtGui/QPushButton>

#include "help_widget.h"
#include "main_window.h"



helpWidget::helpWidget( mainWindow * _main_window, QWidget * _parent ) :
	sideBarWidget( QPixmap( ":/resources/help.png" ),
			tr( "Help on iTALC" ),
			tr( "Need help? This workspace might hold an answer "
				"for your question." ),
			_main_window, _parent )
{
	QBoxLayout * l = dynamic_cast<QBoxLayout *>(
						contentParent()->layout() );
	l->setSpacing( 15 );

	QLabel * wt = new QLabel( tr( "If you want to know, what a specific "						"button etc. does, you can "
						"activate the \"What's this\"-"
						"mode. Then click on the "
						"corresponding button etc. and "
						"a short explanation to it "
						"will be shown." ),
							contentParent() );
	wt->setWordWrap( TRUE );
	l->addWidget( wt );

	QPushButton * wtb = new QPushButton(
					QPixmap( ":/resources/whatsthis.png" ),
					tr( "Enter \"What's this\"-mode" ),
							contentParent() );
	l->addWidget( wtb );
	connect( wtb, SIGNAL( clicked() ), getMainWindow(),
						SLOT( enterWhatsThisMode() ) );

	QPushButton * aib = new QPushButton(
					QPixmap( ":/resources/info_22.png" ),
					tr( "About iTALC" ), contentParent() );
	l->addWidget( aib );
	connect( aib, SIGNAL( clicked() ), getMainWindow(),
						SLOT( aboutITALC() ) );
	l->addStretch();
}


#include "help_widget.moc"

