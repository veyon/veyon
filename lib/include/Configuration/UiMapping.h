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
#define _INIT_WIDGET_FROM_PROPERTY(property,widgetType,setvalue)				\
			qobject_cast<widgetType *>( ui->property )->setvalue( ImcCore::config->property() );

#define INIT_WIDGET_FROM_BOOL_PROPERTY(property,slot)							\
			_INIT_WIDGET_FROM_PROPERTY(property,QAbstractButton,setChecked)

#define INIT_WIDGET_FROM_STRING_PROPERTY(property,slot)							\
			_INIT_WIDGET_FROM_PROPERTY(property,QLineEdit,setText)

#define INIT_WIDGET_FROM_INT_PROPERTY(property,slot)							\
			if(ui->property->inherits("QComboBox"))	{							\
				_INIT_WIDGET_FROM_PROPERTY(property,QComboBox,setCurrentIndex)	\
			} else {															\
				_INIT_WIDGET_FROM_PROPERTY(property,QSpinBox,setValue)			\
			}

#define INIT_WIDGET_FROM_PROPERTY(className, type, get, set, key, parentKey)	\
			INIT_WIDGET_FROM_##type##_PROPERTY(get,set)


// allow connecting widget signals to configuration property write methods
#define CONNECT_WIDGET_TO_BOOL_PROPERTY(property,slot)							\
			connect( ui->property, SIGNAL(toggled(bool)),						\
						ImcCore::config, SLOT(slot(bool)) );

#define CONNECT_WIDGET_TO_STRING_PROPERTY(property,slot)						\
			connect( ui->property, SIGNAL(textChanged(const QString &)),		\
						ImcCore::config, SLOT(slot(const QString &)) );

#define CONNECT_WIDGET_TO_INT_PROPERTY(property,slot)							\
			if(ui->property->inherits("QComboBox"))	{							\
				connect( ui->property, SIGNAL(currentIndexChanged(int)),		\
							ImcCore::config, SLOT(slot(int)) );					\
			} else {															\
				connect( ui->property, SIGNAL(valueChanged(int)),				\
							ImcCore::config, SLOT(slot(int)) );					\
			}

#define CONNECT_WIDGET_TO_PROPERTY(className, type, get, set, key, parentKey)	\
			CONNECT_WIDGET_TO_##type##_PROPERTY(get,set)

#endif
