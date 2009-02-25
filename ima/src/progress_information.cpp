/*
 * progress_information.cpp - implementation of dialog for showing
 *                            status/progress of operation
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


#include <QtCore/QEventLoop>
#include <QtGui/QLabel>
#include <QtGui/QApplication>
#include <QtGui/QResizeEvent>

#include "progress_information.h"



progressInformation::progressInformation( const QString & _txt ):
	QWidget( NULL )
{
	setWindowTitle( tr( "Please wait..." ) );

	m_txtLbl = new QLabel( _txt, this );
	m_txtLbl->setWordWrap( TRUE );
	m_txtLbl->move( 5, 5 );
	m_txtLbl->setFixedSize( 500, 110 );
	m_txtLbl->adjustSize();

	resize( 510, 150 );
	show();

	qApp->processEvents( QEventLoop::AllEvents, 500 );
	QApplication::flush();
}




progressInformation::~progressInformation()
{
}




void progressInformation::resizeEvent( QResizeEvent * )
{
	m_txtLbl->setFixedWidth( width() - 10 );
	m_txtLbl->adjustSize();
//	m_stringLineEdit->setGeometry( 5, m_txtLbl->y()+m_txtLbl->height()+5,
//							width()-10, 24 );
}



#include "progress_information.moc"

