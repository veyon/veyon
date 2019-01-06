/*
 * Configuration/UiMapping.h - helper macros and functions for connecting config with UI
 *
 * Copyright (c) 2010-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef CONFIGURATION_UI_MAPPING_H
#define CONFIGURATION_UI_MAPPING_H

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QUuid>

class QLabel;

template<class Config, typename DataType, class WidgetType>
inline void initWidgetFromProperty( Config* config, DataType (Config::*getter)() const, WidgetType* widget );

template<class Config>
inline void initWidgetFromProperty( Config* config, bool (Config::*getter)() const, QCheckBox* widget )
{
	widget->setChecked( (config->*getter)() );
}

template<class Config>
inline void initWidgetFromProperty( Config* config, bool (Config::*getter)() const, QGroupBox* widget )
{
	widget->setChecked( (config->*getter)() );
}

template<class Config>
inline void initWidgetFromProperty( Config* config, bool (Config::*getter)() const, QRadioButton* widget )
{
	widget->setChecked( (config->*getter)() );
}

template<class Config>
inline void initWidgetFromProperty( Config* config, QString (Config::*getter)() const, QComboBox* widget )
{
	widget->setCurrentText( (config->*getter)() );
}

template<class Config>
inline void initWidgetFromProperty( Config* config, QString (Config::*getter)() const, QLineEdit* widget )
{
	widget->setText( (config->*getter)() );
}

template<class Config>
inline void initWidgetFromProperty( Config* config, QString (Config::*getter)() const, QPlainTextEdit* widget )
{
	widget->setPlainText( (config->*getter)() );
}

template<class Config>
inline void initWidgetFromProperty( Config* config, QColor (Config::*getter)() const, QPushButton* widget )
{
	auto palette = widget->palette();
	palette.setColor( QPalette::Button, (config->*getter)() );
	widget->setPalette( palette );
}

template<class Config>
inline void initWidgetFromProperty( Config* config, int (Config::*getter)() const, QComboBox* widget )
{
	widget->setCurrentIndex( (config->*getter)() );
}

template<class Config>
inline void initWidgetFromProperty( Config* config, int (Config::*getter)() const, QSpinBox* widget )
{
	widget->setValue( (config->*getter)() );
}

template<class Config>
inline void initWidgetFromProperty( Config* config, QUuid (Config::*getter)() const, QComboBox* widget )
{
	widget->setCurrentIndex( widget->findData( (config->*getter)() ) );
}

// specializations for special properties which can't be mapped to widgets
template<class Config>
inline void initWidgetFromProperty( Config* config, QJsonArray (Config::*getter)() const, QGroupBox* ptr) { }

template<class Config>
inline void initWidgetFromProperty( Config* config, QStringList (Config::*getter)() const, QGroupBox* ptr) { }

// widget initialization
#define INIT_WIDGET_FROM_PROPERTY(className, config, type, get, set, key, parentKey)	\
			initWidgetFromProperty<>( &config, &className::get, ui->get );


// connect widget signals to configuration property write methods

template<class Config, typename DataType, class WidgetType>
inline void connectWidgetToProperty( Config* config, void (Config::*setter)( DataType ), WidgetType* widget );

template<class Config>
inline void connectWidgetToProperty( Config* config, void (Config::*setter)( bool ), QCheckBox* widget )
{
	QObject::connect( widget, &QCheckBox::toggled, config, setter );
}

template<class Config>
inline void connectWidgetToProperty( Config* config, void (Config::*setter)( bool ), QGroupBox* widget )
{
	QObject::connect( widget, &QGroupBox::toggled, config, setter );
}

template<class Config>
inline void connectWidgetToProperty( Config* config, void (Config::*setter)( bool ), QRadioButton* widget )
{
	QObject::connect( widget, &QCheckBox::toggled, config, setter );
}


template<class Config>
inline void connectWidgetToProperty( Config* config, void (Config::*setter)( const QString& ), QComboBox* widget )
{
	QObject::connect( widget, &QComboBox::currentTextChanged, config, setter );
}


template<class Config>
inline void connectWidgetToProperty( Config* config, void (Config::*setter)( const QString& ), QLineEdit* widget )
{
	QObject::connect( widget, &QLineEdit::textChanged, config, setter );
}

template<class Config>
inline void connectWidgetToProperty( Config* config, void (Config::*setter)( const QString& ), QPlainTextEdit* widget )
{
	QObject::connect( widget, &QPlainTextEdit::textChanged, config, [=]() {
		(config->*setter)( widget->toPlainText() ); } );
}

template<class Config>
inline void connectWidgetToProperty( Config* config, void (Config::*setter)( const QColor& ), QPushButton* widget )
{
	QObject::connect( widget, &QAbstractButton::clicked, config, [=]() {
		auto palette = widget->palette();
		QColorDialog d( widget->palette().color( QPalette::Button ), widget );
		if( d.exec() )
		{
			(config->*setter)( d.selectedColor() );
			palette.setColor( QPalette::Button, d.selectedColor() );
			widget->setPalette( palette );
		}
	} );
}

template<class Config>
inline void connectWidgetToProperty( Config* config, void (Config::*setter)( int ), QComboBox* widget )
{
	QObject::connect( widget, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), config, setter );
}

template<class Config>
inline void connectWidgetToProperty( Config* config, void (Config::*setter)( int ), QSpinBox* widget )
{
	QObject::connect( widget, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), config, setter );
}

template<class Config>
inline void connectWidgetToProperty( Config* config, void (Config::*setter)( QUuid ), QComboBox* widget )
{
	QObject::connect( widget, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
				widget, [=] () { (config->*setter)( widget->itemData( widget->currentIndex() ).toUuid() ); } );

}

// specializations for special properties which can't be connected to widgets

template<class Config>
inline void connectWidgetToProperty( Config* config, void (Config::*setter)( const QStringList& ), QGroupBox* widget ) { }

template<class Config>
inline void connectWidgetToProperty( Config* config, void (Config::*setter)( const QJsonArray& ), QGroupBox* widget ) { }

#define CONNECT_WIDGET_TO_PROPERTY(className, config, type, get, set, key, parentKey)	\
	connectWidgetToProperty( &config, &className::set, ui->get );

#endif
