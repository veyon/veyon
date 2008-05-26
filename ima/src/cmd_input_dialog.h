/*
 * cmd_input_dialog.h - declaration of class cmdInputDialog
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef _CMD_INPUT_DIALOG_H
#define _CMD_INPUT_DIALOG_H

#include <QtGui/QDialog>
#include <QtGui/QLabel>


class QLabel;
class QPushButton;
class QTextEdit;


class cmdInputDialog : public QDialog
{
	Q_OBJECT
public:
	cmdInputDialog( QString & _cmds_str, QWidget * _parent = 0 );
	virtual ~cmdInputDialog();


private slots:
	virtual void accept( void );


private:
	virtual void keyPressEvent( QKeyEvent * _ke );
	virtual void resizeEvent( QResizeEvent * _re );

	QPushButton * m_cancelBtn;
	QPushButton * m_okBtn;
	QLabel * m_iconLbl;
	QLabel * m_appNameLbl;
	QTextEdit * m_cmdInputTextEdit;

	QString & m_cmdsStr;

} ;

#endif
