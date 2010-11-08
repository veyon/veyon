/*
 * Configuration/UiMapping.h - helper macros for connecting config with UI
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef _CONFIGURATION_UI_MAPPING_H
#define _CONFIGURATION_UI_MAPPING_H

#define LOAD_AND_CONNECT_PROPERTY(property,type,widgetType,setvalue,signal,slot)				\
			qobject_cast<widgetType *>( ui->property )->setvalue( ImcCore::config->property() );\
			connect( ui->property, SIGNAL(signal(type)),										\
						ImcCore::config, SLOT(slot(type)) );

#define LOAD_AND_CONNECT_BOOL_PROPERTY(property,slot)											\
			LOAD_AND_CONNECT_PROPERTY(property,bool,QAbstractButton,setChecked,toggled,slot)

#define LOAD_AND_CONNECT_STRING_PROPERTY(property,slot)											\
			LOAD_AND_CONNECT_PROPERTY(property,const QString &,QLineEdit,setText,textChanged,slot)

#define LOAD_AND_CONNECT_INT_PROPERTY(property,slot)											\
			if(ui->property->inherits("QComboBox"))	{											\
				LOAD_AND_CONNECT_PROPERTY(property,int,QComboBox,setCurrentIndex,currentIndexChanged,slot)	\
			} else {																			\
				LOAD_AND_CONNECT_PROPERTY(property,int,QSpinBox,setValue,valueChanged,slot)		\
			}

#define MAP_CONFIG_TO_UI(className, type, get, set, key, parentKey)					\
			LOAD_AND_CONNECT_##type##_PROPERTY(get,set)

#endif
