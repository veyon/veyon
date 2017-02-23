/*
 * ClassroomSelectionDialog.h - header file for ClassroomSelectionDialog
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef CLASSROOM_SELECTION_DIALOG_H
#define CLASSROOM_SELECTION_DIALOG_H

#include <QDialog>
#include <QSortFilterProxyModel>

namespace Ui {
class ClassroomSelectionDialog;
}

class ClassroomSelectionDialog : public QDialog
{
	Q_OBJECT
public:
	ClassroomSelectionDialog( QAbstractItemModel* classroomListModel, QWidget *parent = 0 );
	~ClassroomSelectionDialog();

	const QString& selectedClassroom() const
	{
		return m_selectedClassroom;
	}

private slots:
	void updateSearchFilter();
	void updateSelection( const QModelIndex& current, const QModelIndex& previous );

private:
	Ui::ClassroomSelectionDialog *ui;

	QSortFilterProxyModel m_sortFilterProxyModel;
	QString m_selectedClassroom;
};

#endif // CLASSROOM_SELECTION_DIALOG_H
