/*
 * ComputerManagementView.h - provides a view for a network object tree
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef COMPUTER_MANAGER_VIEW_H
#define COMPUTER_MANAGER_VIEW_H

#include <QWidget>

namespace Ui {
class ComputerManagementView;
}

class ComputerManager;

class ComputerManagementView : public QWidget
{
	Q_OBJECT
public:
	ComputerManagementView( ComputerManager& computerManager, QWidget *parent = 0 );
	~ComputerManagementView();

private slots:
	void addRoom();

private:
	Ui::ComputerManagementView *ui;
	ComputerManager& m_computerManager;

};

#endif // COMPUTER_MANAGER_VIEW_H
