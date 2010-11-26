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

// widget initialization
#define _INIT_WIDGET_FROM_PROPERTY(config,property,widgetType,setvalue)				\
			qobject_cast<widgetType *>( ui->property )->setvalue( config->property() );

#define INIT_WIDGET_FROM_BOOL_PROPERTY(config,property,slot)							\
			_INIT_WIDGET_FROM_PROPERTY(config,property,QAbstractButton,setChecked)

#define INIT_WIDGET_FROM_STRING_PROPERTY(config,property,slot)							\
			_INIT_WIDGET_FROM_PROPERTY(config,property,QLineEdit,setText)

#define INIT_WIDGET_FROM_STRINGLIST_PROPERTY(config,property,slot)							\
/*			if(ui->property->inherits("QListWidget")) {					\
				qobject_cast<QListWidget *>( ui->property )->clear();			\
				qobject_cast<QListWidget *>( ui->property )->addItems( config->property() );	\
			}*/

#define INIT_WIDGET_FROM_INT_PROPERTY(config,property,slot)							\
			if(ui->property->inherits("QComboBox"))	{							\
				_INIT_WIDGET_FROM_PROPERTY(config,property,QComboBox,setCurrentIndex)	\
			} else {															\
				_INIT_WIDGET_FROM_PROPERTY(config,property,QSpinBox,setValue)			\
			}

#define INIT_WIDGET_FROM_PROPERTY(className, config, type, get, set, key, parentKey)	\
			INIT_WIDGET_FROM_##type##_PROPERTY(config,get,set)


// allow connecting widget signals to configuration property write methods
#define CONNECT_WIDGET_TO_BOOL_PROPERTY(config,property,slot)							\
			connect( ui->property, SIGNAL(toggled(bool)),						\
						config, SLOT(slot(bool)) );

#define CONNECT_WIDGET_TO_STRINGLIST_PROPERTY(config,property,slot)

#define CONNECT_WIDGET_TO_STRING_PROPERTY(config,property,slot)						\
			connect( ui->property, SIGNAL(textChanged(const QString &)),		\
						config, SLOT(slot(const QString &)) );

#define CONNECT_WIDGET_TO_INT_PROPERTY(config,property,slot)							\
			if(ui->property->inherits("QComboBox"))	{							\
				connect( ui->property, SIGNAL(currentIndexChanged(int)),		\
							config, SLOT(slot(int)) );				\
			} else {															\
				connect( ui->property, SIGNAL(valueChanged(int)),				\
							config, SLOT(slot(int)) );				\
			}

#define CONNECT_WIDGET_TO_PROPERTY(className, config, type, get, set, key, parentKey)	\
			CONNECT_WIDGET_TO_##type##_PROPERTY(config,get,set)

#endif
