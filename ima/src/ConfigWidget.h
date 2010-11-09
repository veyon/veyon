/*
 * ConfigWidget.h - configuration-widget for side-bar
 *
 * Copyright (c) 2004-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef _CONFIG_WIDGET_H
#define _CONFIG_WIDGET_H

#include "SideBarWidget.h"
#include "ui_Config.h"


class ConfigWidget : public SideBarWidget, private Ui::Config
{
	Q_OBJECT
public:
	ConfigWidget( MainWindow * _main_window, QWidget * _parent );
	virtual ~ConfigWidget();


protected slots:
	void roleSelected( int );
#ifndef ITALC3
	void toggleToolButtonTips( bool _on );
	void toggleIconOnlyToolButtons( bool _on );
	void domainChanged( const QString & _domain );
#endif

} ;


#endif
