/*
 * Configuration/UiMapping.h - helper macros and functions for connecting config with UI
 *
 * Copyright (c) 2010-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef CONFIGURATION_UI_MAPPING_H
#define CONFIGURATION_UI_MAPPING_H

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QUuid>

class QLabel;

template<class Config, typename DataType, class WidgetType>
inline void initWidgetFromProperty( Config* config, DataType (Config::*getter)() const, WidgetType* widget );

template<>
inline void initWidgetFromProperty( ItalcConfiguration* config, bool (ItalcConfiguration::*getter)() const, QCheckBox* widget )
{
	widget->setChecked( (config->*getter)() );
}

template<>
inline void initWidgetFromProperty( ItalcConfiguration* config, bool (ItalcConfiguration::*getter)() const, QGroupBox* widget )
{
	widget->setChecked( (config->*getter)() );
}

template<>
inline void initWidgetFromProperty( ItalcConfiguration* config, bool (ItalcConfiguration::*getter)() const, QRadioButton* widget )
{
	widget->setChecked( (config->*getter)() );
}

template<>
inline void initWidgetFromProperty( ItalcConfiguration* config, QString (ItalcConfiguration::*getter)() const, QComboBox* widget )
{
	widget->setCurrentText( (config->*getter)() );
}

template<>
inline void initWidgetFromProperty( ItalcConfiguration* config, QString (ItalcConfiguration::*getter)() const, QLineEdit* widget )
{
	widget->setText( (config->*getter)() );
}

template<>
inline void initWidgetFromProperty( ItalcConfiguration* config, int (ItalcConfiguration::*getter)() const, QComboBox* widget )
{
	widget->setCurrentIndex( (config->*getter)() );
}

template<>
inline void initWidgetFromProperty( ItalcConfiguration* config, int (ItalcConfiguration::*getter)() const, QSpinBox* widget )
{
	widget->setValue( (config->*getter)() );
}

template<>
inline void initWidgetFromProperty( ItalcConfiguration* config, QUuid (ItalcConfiguration::*getter)() const, QComboBox* widget )
{
	widget->setCurrentIndex( widget->findData( (config->*getter)() ) );
}

// specializations for special properties which can't be mapped to widgets
template<>
inline void initWidgetFromProperty( ItalcConfiguration* config, QJsonArray (ItalcConfiguration::*getter)() const, QGroupBox* ptr) { }

template<>
inline void initWidgetFromProperty( ItalcConfiguration* config, QStringList (ItalcConfiguration::*getter)() const, QGroupBox* ptr) { }

// widget initialization
#define INIT_WIDGET_FROM_PROPERTY(className, config, type, get, set, key, parentKey)	\
			initWidgetFromProperty<>( config, &className::get, ui->get );


// connect widget signals to configuration property write methods

template<class Config, typename DataType, class WidgetType>
inline void connectWidgetToProperty( Config* config, void (Config::*setter)( DataType ), WidgetType* widget );

template<>
inline void connectWidgetToProperty( ItalcConfiguration* config, void (ItalcConfiguration::*setter)( bool ), QCheckBox* widget )
{
	QObject::connect( widget, &QCheckBox::toggled, config, setter );
}

template<>
inline void connectWidgetToProperty( ItalcConfiguration* config, void (ItalcConfiguration::*setter)( bool ), QGroupBox* widget )
{
	QObject::connect( widget, &QGroupBox::toggled, config, setter );
}

template<>
inline void connectWidgetToProperty( ItalcConfiguration* config, void (ItalcConfiguration::*setter)( bool ), QRadioButton* widget )
{
	QObject::connect( widget, &QCheckBox::toggled, config, setter );
}


template<>
inline void connectWidgetToProperty( ItalcConfiguration* config, void (ItalcConfiguration::*setter)( const QString& ), QComboBox* widget )
{
	QObject::connect( widget, &QComboBox::currentTextChanged, config, setter );
}


template<>
inline void connectWidgetToProperty( ItalcConfiguration* config, void (ItalcConfiguration::*setter)( const QString& ), QLineEdit* widget )
{
	QObject::connect( widget, &QLineEdit::textChanged, config, setter );
}

template<>
inline void connectWidgetToProperty( ItalcConfiguration* config, void (ItalcConfiguration::*setter)( int ), QComboBox* widget )
{
	QObject::connect( widget, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), config, setter );
}

template<>
inline void connectWidgetToProperty( ItalcConfiguration* config, void (ItalcConfiguration::*setter)( int ), QSpinBox* widget )
{
	QObject::connect( widget, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), config, setter );
}

template<>
inline void connectWidgetToProperty( ItalcConfiguration* config, void (ItalcConfiguration::*setter)( const QUuid& ), QComboBox* widget )
{
	QObject::connect( widget, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
				[=] () { (config->*setter)( widget->itemData( widget->currentIndex() ).toUuid() ); } );

}

// specializations for special properties which can't be connected to widgets

template<>
inline void connectWidgetToProperty( ItalcConfiguration* config, void (ItalcConfiguration::*setter)( const QStringList& ), QGroupBox* widget ) { }

template<>
inline void connectWidgetToProperty( ItalcConfiguration* config, void (ItalcConfiguration::*setter)( const QJsonArray& ), QGroupBox* widget ) { }

#define CONNECT_WIDGET_TO_PROPERTY(className, config, type, get, set, key, parentKey)	\
	connectWidgetToProperty( config, &className::set, ui->get );

#endif
