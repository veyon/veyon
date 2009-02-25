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


#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QKeyEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QTextEdit>

#include "cmd_input_dialog.h"



cmdInputDialog::cmdInputDialog( QString & _cmds_str, QWidget * _parent ) :
	QDialog( _parent ),
	m_cmdsStr( _cmds_str )
{
	setWindowTitle( tr( "Run commands on client(s)" ) );

	m_iconLbl = new QLabel( this );
	m_iconLbl->setPixmap( QPixmap( ":/resources/client_exec_cmds.png" ) );
	m_iconLbl->setGeometry( 10, 16, 48, 48 );

	m_appNameLbl = new QLabel( tr( "Enter commands, which should be run "
						"on client(s):" ), this );
	m_appNameLbl->setGeometry( 70, 20, 380, 40 );


	m_cmdInputTextEdit = new QTextEdit( this );
	m_cmdInputTextEdit->setReadOnly( FALSE );
	m_cmdInputTextEdit->setGeometry( 10, 64, 460, 256 );
	m_cmdInputTextEdit->setWordWrapMode( QTextOption::NoWrap );

	m_cancelBtn = new QPushButton( QPixmap( ":/resources/cancel.png" ),
							tr( "Cancel" ), this );
	m_okBtn = new QPushButton( QPixmap( ":/resources/apply.png" ),
							tr( "&Run!" ), this );
	connect( m_okBtn, SIGNAL( clicked() ), this, SLOT( accept() ) );
	connect( m_cancelBtn, SIGNAL( clicked() ), this, SLOT( reject() ) );

	resize( 480, 390 );
}




cmdInputDialog::~cmdInputDialog()
{
}




void cmdInputDialog::keyPressEvent( QKeyEvent * _ke )
{
	if( _ke->key() == Qt::Key_Escape )
	{
		reject();
	}
}




void cmdInputDialog::resizeEvent( QResizeEvent * _re )
{
	m_cmdInputTextEdit->setGeometry( 10, 90, _re->size().width()-20,
						_re->size().height()-140 );
	m_okBtn->setGeometry( _re->size().width()-120, _re->size().height()-40,
								110, 30 );
	m_cancelBtn->setGeometry( _re->size().width()-240,
					_re->size().height()-40, 110, 30 );
}




void cmdInputDialog::accept( void )
{
	m_cmdsStr = m_cmdInputTextEdit->toPlainText();
	QDialog::accept();
}


#include "cmd_input_dialog.moc"

