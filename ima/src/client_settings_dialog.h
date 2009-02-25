/*
 * client_settings_dialog.h - class clientSettingsDialog
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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


#ifndef _CLIENT_SETTINGS_DIALOG_H
#define _CLIENT_SETTINGS_DIALOG_H

#include "dialogs/client_settings.uic"


class client;
class mainWindow;


class clientSettingsDialog : public QDialog, Ui::clientSettings
{
	Q_OBJECT
public:
	clientSettingsDialog( client * _c, mainWindow * _main_window,
						const QString & _classroom,
							QWidget * _parent );

private:
	virtual void accept( void );

	client * m_client;
	mainWindow * m_mainWindow;

} ;


#endif
