/*
 * ComputerManagementView.h - provides a view for a network object tree
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef COMPUTER_MANAGER_VIEW_H
#define COMPUTER_MANAGER_VIEW_H

#include <QModelIndexList>
#include <QWidget>

namespace Ui {
class ComputerManagementView;
}

class ComputerManager;
class RecursiveFilterProxyModel;

class ComputerManagementView : public QWidget
{
	Q_OBJECT
public:
	ComputerManagementView( ComputerManager& computerManager, QWidget *parent = nullptr );
	~ComputerManagementView() override;

	bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
	void addRoom();
	void removeRoom();
	void saveList();
	void updateFilter();

private:
	Ui::ComputerManagementView *ui;
	ComputerManager& m_computerManager;
	RecursiveFilterProxyModel* m_filterProxyModel;
	QString m_previousFilter;
	QModelIndexList m_expandedGroups;

};

#endif // COMPUTER_MANAGER_VIEW_H
