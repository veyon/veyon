/*
 * SnapshotList.h - declaration of snapshot-list for side-bar
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#ifndef _SNAPSHOT_LIST_H
#define _SNAPSHOT_LIST_H

#include <QtGui/QWidget>

#include "SideBarWidget.h"
#include "ui_Snapshots.h"


class QLabel;
class QPushButton;
class QListWidget;


class SnapshotList : public SideBarWidget, private Ui::Snapshots
{
	Q_OBJECT
public:
	SnapshotList( MainWindow * _main_window, QWidget * _parent );
	virtual ~SnapshotList();


public slots:
	void reloadList( void );


private slots:
	void snapshotSelected( const QString & _s );
	void snapshotActivated( QListWidgetItem * _item )
	{
		snapshotDoubleClicked( _item->text() );
	}

	void showSnapshot( void );
	void deleteSnapshot( void );


private:
	void snapshotDoubleClicked( const QString & _s );

} ;


#endif
