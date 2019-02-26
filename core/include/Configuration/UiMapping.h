/*
 * Configuration/UiMapping.h - helper macros and functions for connecting config with UI
 *
 * Copyright (c) 2010-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
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

#pragma once

#include <QComboBox>

#include "Configuration/Property.h"

class QCheckBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;

namespace Configuration
{

class VEYON_CORE_EXPORT UiMapping
{
public:
	static void initWidgetFromProperty( const Configuration::TypedProperty<bool>& property, QCheckBox* widget );
	static void initWidgetFromProperty( const Configuration::TypedProperty<bool>& property, QGroupBox* widget );
	static void initWidgetFromProperty( const Configuration::TypedProperty<bool>& property, QRadioButton* widget );
	static void initWidgetFromProperty( const Configuration::TypedProperty<QString>& property, QComboBox* widget );
	static void initWidgetFromProperty( const Configuration::TypedProperty<QString>& property, QLineEdit* widget );
	static void initWidgetFromProperty( const Configuration::TypedProperty<Password>& property, QLineEdit* widget );
	static void initWidgetFromProperty( const Configuration::TypedProperty<QColor>& property, QPushButton* widget );
	static void initWidgetFromProperty( const Configuration::TypedProperty<int>& property, QComboBox* widget );
	static void initWidgetFromProperty( const Configuration::TypedProperty<int>& property, QSpinBox* widget );
	static void initWidgetFromProperty( const Configuration::TypedProperty<QUuid>& property, QComboBox* widget );

	// SFINAE overload for enum classes
	template<typename T>
	static typename std::enable_if<std::is_enum<T>::value, void>::type
	initWidgetFromProperty( const Configuration::TypedProperty<T>& property, QComboBox* widget )
	{
		widget->setCurrentIndex( static_cast<int>( property.value() ) );
	}

	// overloads for special properties which can't be mapped to widgets
	static void initWidgetFromProperty( const Configuration::TypedProperty<QJsonArray>& property, QLabel* widget )
	{
		Q_UNUSED(property)
		Q_UNUSED(widget)
	}

	static void initWidgetFromProperty( const Configuration::TypedProperty<QStringList>& property, QLabel* widget )
	{
		Q_UNUSED(property)
		Q_UNUSED(widget)
	}

	static void setFlags( QObject* object, Configuration::Property::Flags flags );

	static Property::Flags flags( QObject* object );

	// widget initialization
#define INIT_WIDGET_FROM_PROPERTY(className, config, type, get, set, key, parentKey, defaultValue, flags)	\
	Configuration::UiMapping::initWidgetFromProperty( config.get##Property(), ui->get ); \
	Configuration::UiMapping::setFlags( ui->get, flags );


	// connect widget signals to configuration property write methods

	static void connectWidgetToProperty( Configuration::TypedProperty<bool>& property, QCheckBox* widget );
	static void connectWidgetToProperty( Configuration::TypedProperty<bool>& property, QGroupBox* widget );
	static void connectWidgetToProperty( Configuration::TypedProperty<bool>& property, QRadioButton* widget );
	static void connectWidgetToProperty( Configuration::TypedProperty<QString>& property, QComboBox* widget );
	static void connectWidgetToProperty( Configuration::TypedProperty<QString>& property, QLineEdit* widget );
	static void connectWidgetToProperty( Configuration::TypedProperty<Password>& property, QLineEdit* widget );
	static void connectWidgetToProperty( Configuration::TypedProperty<QColor>& property, QPushButton* widget );
	static void connectWidgetToProperty( Configuration::TypedProperty<int>& property, QComboBox* widget );
	static void connectWidgetToProperty( Configuration::TypedProperty<int>& property, QSpinBox* widget );
	static void connectWidgetToProperty( Configuration::TypedProperty<QUuid>& property, QComboBox* widget );

	// SFINAE overload for enum classes
	template<typename T>
	static typename std::enable_if<std::is_enum<T>::value, void>::type
	connectWidgetToProperty( Configuration::TypedProperty<T>& property, QComboBox* widget )
	{
		QObject::connect( widget, QOverload<int>::of(&QComboBox::currentIndexChanged), property.lambdaContext(),
						  [&property]( int index ) { property.setValue( static_cast<T>( index ) ); } );
	}


	// overloads for special properties which can't be connected to widgets
	static void connectWidgetToProperty( Configuration::TypedProperty<QStringList>& property, QLabel* widget )
	{
		Q_UNUSED(property)
		Q_UNUSED(widget)
	}

	static void connectWidgetToProperty( Configuration::TypedProperty<QJsonArray>& property, QLabel* widget )
	{
		Q_UNUSED(property)
		Q_UNUSED(widget)
	}

#define CONNECT_WIDGET_TO_PROPERTY(className, config, type, get, set, key, parentKey, defaultValue, flags)	\
	Configuration::UiMapping::connectWidgetToProperty( config.get##Property(), ui->get );

};

}
