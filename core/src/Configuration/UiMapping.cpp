/*
 * ConfigurationObject.cpp - implementation of ConfigurationObject
 *
 * Copyright (c) 2009-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <QCheckBox>
#include <QColorDialog>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>

#include "Configuration/UiMapping.h"

namespace Configuration
{

static constexpr auto WidgetConfigPropertyFlags = "ConfigPropertyFlags";


void UiMapping::initWidgetFromProperty( const Configuration::TypedProperty<bool>& property, QCheckBox* widget )
{
	widget->setChecked( property.value() );
}



void UiMapping::initWidgetFromProperty( const Configuration::TypedProperty<bool>& property, QGroupBox* widget )
{
	widget->setChecked( property.value() );
}



void UiMapping::initWidgetFromProperty( const Configuration::TypedProperty<bool>& property, QRadioButton* widget )
{
	widget->setChecked( property.value() );
}



void UiMapping::initWidgetFromProperty( const Configuration::TypedProperty<QString>& property, QComboBox* widget )
{
	widget->setCurrentText( property.value() );
}



void UiMapping::initWidgetFromProperty( const Configuration::TypedProperty<QString>& property, QLineEdit* widget )
{
	widget->setText( property.value() );
}



void UiMapping::initWidgetFromProperty( const Configuration::TypedProperty<Password>& property, QLineEdit* widget )
{
	widget->setText( property.value().plainText() );
}



void UiMapping::initWidgetFromProperty( const Configuration::TypedProperty<QColor>& property, QPushButton* widget )
{
	auto palette = widget->palette();
	palette.setColor( QPalette::Button, property.value() );
	widget->setPalette( palette );
}



void UiMapping::initWidgetFromProperty( const Configuration::TypedProperty<int>& property, QComboBox* widget )
{
	widget->setCurrentIndex( property.value() );
}



void UiMapping::initWidgetFromProperty( const Configuration::TypedProperty<int>& property, QSpinBox* widget )
{
	widget->setValue( property.value() );
}



void UiMapping::initWidgetFromProperty( const Configuration::TypedProperty<QUuid>& property, QComboBox* widget )
{
	widget->setCurrentIndex( widget->findData( property.value() ) );
}



void UiMapping::setFlags( QObject* object, Property::Flags flags )
{
#if QT_VERSION >= 0x051200
	object->setProperty( WidgetConfigPropertyFlags, QVariant::fromValue( flags ) );
#else
	object->setProperty( WidgetConfigPropertyFlags, static_cast<unsigned int>( flags ) );
#endif
}



Property::Flags UiMapping::flags( QObject* object )
{
	const auto flagsData = object->property( WidgetConfigPropertyFlags );
#if QT_VERSION >= 0x051200
	return flagsData.value<Object::PropertyFlags>();
#else
	return static_cast<Property::Flags>( flagsData.toUInt() );
#endif
}



void UiMapping::connectWidgetToProperty( Configuration::TypedProperty<bool>& property, QCheckBox* widget )
{
	QObject::connect( widget, &QCheckBox::toggled, property.lambdaContext(),
					  [&property]( bool value ) { property.setValue( value ); } );
}



void UiMapping::connectWidgetToProperty( Configuration::TypedProperty<bool>& property, QGroupBox* widget )
{
	QObject::connect( widget, &QGroupBox::toggled, property.lambdaContext(),
					  [&property]( bool value ) { property.setValue( value ); } );
}



void UiMapping::connectWidgetToProperty( Configuration::TypedProperty<bool>& property, QRadioButton* widget )
{
	QObject::connect( widget, &QCheckBox::toggled, property.lambdaContext(),
					  [&property]( bool value ) { property.setValue( value ); } );
}



void UiMapping::connectWidgetToProperty( Configuration::TypedProperty<QString>& property, QComboBox* widget )
{
	QObject::connect( widget, &QComboBox::currentTextChanged, property.lambdaContext(),
					  [&property]( const QString& value ) { property.setValue( value ); } );
}



void UiMapping::connectWidgetToProperty( Configuration::TypedProperty<QString>& property, QLineEdit* widget )
{
	QObject::connect( widget, &QLineEdit::textChanged, property.lambdaContext(),
					  [&property]( const QString& value ) { property.setValue( value ); } );
}



void UiMapping::connectWidgetToProperty( Configuration::TypedProperty<Password>& property, QLineEdit* widget )
{
	QObject::connect( widget, &QLineEdit::textChanged, property.lambdaContext(),
					  [&property]( const QString& plainText ) { property.setValue( Password::fromPlainText( plainText ) ); } );
}



void UiMapping::connectWidgetToProperty( Configuration::TypedProperty<QColor>& property, QPushButton* widget )
{
	QObject::connect( widget, &QAbstractButton::clicked, property.lambdaContext(), [&property, widget]() {
		auto palette = widget->palette();
		QColorDialog d( widget->palette().color( QPalette::Button ), widget );
		if( d.exec() )
		{
			property.setValue( d.selectedColor() );
			palette.setColor( QPalette::Button, d.selectedColor() );
			widget->setPalette( palette );
		}
	} );
}



void UiMapping::connectWidgetToProperty( Configuration::TypedProperty<int>& property, QComboBox* widget )
{
	QObject::connect( widget, QOverload<int>::of(&QComboBox::currentIndexChanged), property.lambdaContext(),
					  [&property]( int index ) { property.setValue( index ); } );
}



void UiMapping::connectWidgetToProperty( Configuration::TypedProperty<int>& property, QSpinBox* widget )
{
	QObject::connect( widget, QOverload<int>::of(&QSpinBox::valueChanged), property.lambdaContext(),
					  [&property]( int index ) { property.setValue( index ); } );
}



void UiMapping::connectWidgetToProperty( Configuration::TypedProperty<QUuid>& property, QComboBox* widget )
{
	QObject::connect( widget, QOverload<int>::of(&QComboBox::currentIndexChanged), property.lambdaContext(),
					  [widget, &property]( int index ) { property.setValue( widget->itemData( index ).toUuid() ); } );

}


}
