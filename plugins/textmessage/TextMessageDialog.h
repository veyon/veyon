/*
 * TextMessageDialog.h - declaration of text message dialog class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
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

#ifndef TEXT_MESSAGE_DIALOG_H
#define TEXT_MESSAGE_DIALOG_H

#include <QDialog>

namespace Ui
{
	class TextMessageDialog;
}

class TextMessageDialog : public QDialog
{
	Q_OBJECT
public:
	TextMessageDialog( QString &msgStr, QWidget *parent );


private slots:
	void accept() override;


private:
	Ui::TextMessageDialog *ui;
	QString &m_msgStr;

} ;

#endif
