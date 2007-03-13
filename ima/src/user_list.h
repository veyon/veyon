/*
 * user_list.h - declaration of user-list for side-bar
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef _USER_LIST_H
#define _USER_LIST_H

#include "side_bar_widget.h"


class QListWidget;
class QPushButton;

class client;
class classroomManager;


class userList : public sideBarWidget
{
	Q_OBJECT
public:
	userList( mainWindow * _main_window, QWidget * _parent );
	virtual ~userList();

	static QStringList getLoggedInUsers(
					classroomManager * _classroom_manager );
	static client * getClientFromUser( const QString & _user,
					classroomManager * _classroom_manager );

	void reload( void );


private slots:
	void contextMenuHandler( const QPoint & _pos );
	void clickedExportToFile( void );


private:
	QListWidget * m_list;
	QPushButton * m_exportToFileBtn;

} ;


#endif
