/*
 * DecoratedMessageBox.cpp - simple message-box
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "DecoratedMessageBox.h"
#include "LocalSystem.h"

#include <QtCore/QThread>
#include <QtGui/QApplication>
#include <QtGui/QWidget>
#include <QtGui/QPixmap>
#include <QtGui/QIcon>
#include <QtGui/QLabel>
#include <QtGui/QBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QSystemTrayIcon>

QSystemTrayIcon * __systray_icon = NULL;


DecoratedMessageBox::DecoratedMessageBox( const QString & _title,
						const QString & _msg,
						const QPixmap & _pixmap ) :
	QDialog()
{
	QVBoxLayout * vl = new QVBoxLayout( this );

	QWidget * content = new QWidget( this );

	QHBoxLayout * hl1 = new QHBoxLayout( content );
	hl1->setSpacing( 20 );

	QLabel * icon_lbl = new QLabel( content );
	if( _pixmap.isNull() == FALSE )
	{
		icon_lbl->setPixmap( _pixmap );
	}
	else
	{
		icon_lbl->setPixmap( QPixmap( ":/resources/info.png" ) );
	}
	icon_lbl->setFixedSize( icon_lbl->pixmap()->size() );

	QLabel * txt_lbl = new QLabel( _msg, content );
	txt_lbl->setMinimumWidth( 400 );
	txt_lbl->setWordWrap( TRUE );

	hl1->addWidget( icon_lbl );
	hl1->addWidget( txt_lbl );

	QWidget * btn_area = new QWidget( this );
	QHBoxLayout * hl2 = new QHBoxLayout( btn_area );

	QPushButton * ok_btn = new QPushButton( QPixmap( ":/resources/ok.png" ),
						tr( "OK" ), btn_area );
	connect( ok_btn, SIGNAL( clicked() ), this, SLOT( accept() ) );

	hl2->addStretch();
	hl2->addWidget( ok_btn );
	hl2->addStretch();

	vl->addWidget( content );
	vl->addWidget( btn_area );

	setWindowTitle( _title );
	setWindowIcon( *icon_lbl->pixmap() );
	setAttribute( Qt::WA_DeleteOnClose, TRUE );
	setModal( TRUE );
	show();
	LocalSystem::activateWindow( this );
}




void DecoratedMessageBox::information( const QString & _title,
						const QString & _msg,
						const QPixmap & _pixmap )
{
	DecoratedMessageBox * m =
			new DecoratedMessageBox( _title, _msg, _pixmap );
	m->exec();
}



void DecoratedMessageBox::trySysTrayMessage( const QString & _title,
					const QString & _msg,
					MessageIcon _msg_icon )
{
	qWarning( "%s", _msg.toUtf8().constData() );
	if( QThread::currentThreadId() !=
		QCoreApplication::instance()->thread()->currentThreadId() )
	{
		return;
	}

	// OS X does not support messages
	if( QSystemTrayIcon::supportsMessages() && __systray_icon )
	{
		__systray_icon->showMessage( _title, _msg,
				(QSystemTrayIcon::MessageIcon) _msg_icon, -1 );
		return;
	}

	QPixmap p;
	switch( _msg_icon )
	{
		case Information:
		case Warning:
			p = QPixmap( ":/resources/info.png" );
			break;
		case Critical:
			p = QPixmap( ":/resources/stop.png" );
			break;
	}

	new DecoratedMessageBox( _title, _msg, p );

}


