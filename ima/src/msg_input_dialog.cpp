/*
 * cmd_input_dialog.cpp - implementation of command-input-dialog
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
#include <QtGui/QTextEdit>
#include <QtGui/QPushButton>
#include <QtGui/QKeyEvent>
#include <QtGui/QResizeEvent>

#include "msg_input_dialog.h"



msgInputDialog::msgInputDialog( QString & _msg_str, QWidget * _parent) :
	QDialog( _parent ),
	m_msgStr( _msg_str )
{
	setWindowTitle( tr( "Send message to client(s)" ) );

	m_iconLbl = new QLabel( this );
	m_iconLbl->setPixmap( QPixmap( ":/resources/client_msg.png" ) );
	m_iconLbl->setGeometry( 10, 16, 48, 48 );

	m_appNameLbl = new QLabel( tr( "Enter the message, which should "
					"displayed on client(s):" ), this );
	m_appNameLbl->setGeometry( 70, 20, 400, 40 );


	m_msgInputTextEdit = new QTextEdit( this );
	m_msgInputTextEdit->setReadOnly( FALSE );
	m_msgInputTextEdit->setGeometry( 10, 64, 460, 256 );
	m_msgInputTextEdit->setWordWrapMode( QTextOption::NoWrap );

	m_cancelBtn = new QPushButton( QPixmap( ":/resources/cancel.png" ),
							tr( "Cancel" ), this );
	m_okBtn = new QPushButton( QPixmap( ":/resources/apply.png" ),
							tr( "&Send!" ), this );
	connect( m_okBtn, SIGNAL( clicked() ), this, SLOT( accept() ) );
	connect( m_cancelBtn, SIGNAL( clicked() ), this, SLOT( reject() ) );

	resize( 480, 390 );
}




msgInputDialog::~msgInputDialog()
{
}




void msgInputDialog::keyPressEvent( QKeyEvent * _ke )
{
	if( _ke->key() == Qt::Key_Escape )
	{
		reject();
	}
}




void msgInputDialog::resizeEvent( QResizeEvent * _re )
{
	m_msgInputTextEdit->setGeometry( 10, 90, _re->size().width()-20,
						_re->size().height()-140 );
	m_okBtn->setGeometry( _re->size().width()-120, _re->size().height()-40,
								110, 30 );
	m_cancelBtn->setGeometry( _re->size().width()-240,
					_re->size().height()-40, 110, 30 );
}




void msgInputDialog::accept( void )
{
	m_msgStr = m_msgInputTextEdit->toPlainText();
	QDialog::accept();
}



#include "msg_input_dialog.moc"

