// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

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

namespace Configuration {

class VEYON_CORE_EXPORT UiMapping
{
public:
	static void initWidgetFromProperty(const Configuration::TypedProperty<bool>& property, QCheckBox* widget);
	static void initWidgetFromProperty(const Configuration::TypedProperty<bool>& property, QGroupBox* widget);
	static void initWidgetFromProperty(const Configuration::TypedProperty<bool>& property, QRadioButton* widget);
	static void initWidgetFromProperty(const Configuration::TypedProperty<QString>& property, QComboBox* widget);
	static void initWidgetFromProperty(const Configuration::TypedProperty<QString>& property, QLineEdit* widget);
	static void initWidgetFromProperty(const Configuration::TypedProperty<Password>& property, QLineEdit* widget);
	static void initWidgetFromProperty(const Configuration::TypedProperty<QColor>& property, QPushButton* widget);
	static void initWidgetFromProperty(const Configuration::TypedProperty<int>& property, QComboBox* widget);
	static void initWidgetFromProperty(const Configuration::TypedProperty<int>& property, QSpinBox* widget);
	static void initWidgetFromProperty(const Configuration::TypedProperty<QUuid>& property, QComboBox* widget);

	// SFINAE overload for enum classes
	template<typename T>
	static typename std::enable_if<std::is_enum<T>::value, void>::type
	initWidgetFromProperty(const Configuration::TypedProperty<T>& property, QComboBox* widget)
	{
		widget->setCurrentIndex(static_cast<int>(property.value()));
	}

	// overloads for special properties which can't be mapped to widgets
	static void initWidgetFromProperty(const Configuration::TypedProperty<QJsonArray>& property, QLabel* widget)
	{
		Q_UNUSED(property)
		Q_UNUSED(widget)
	}

	static void initWidgetFromProperty(const Configuration::TypedProperty<QStringList>& property, QLabel* widget)
	{
		Q_UNUSED(property)
		Q_UNUSED(widget)
	}

	static void setFlags(QObject* object, Configuration::Property::Flags flags);

	static Property::Flags flags(QObject* object);

	// widget initialization
#define INIT_WIDGET_FROM_PROPERTY(className, config, type, get, set, key, parentKey, defaultValue, flags) \
	Configuration::UiMapping::initWidgetFromProperty(config.get##Property(), ui->get); \
	Configuration::UiMapping::setFlags(ui->get, flags);


	// connect widget signals to configuration property write methods

	static void connectWidgetToProperty(const Configuration::TypedProperty<bool>& property, QCheckBox* widget);
	static void connectWidgetToProperty(const Configuration::TypedProperty<bool>& property, QGroupBox* widget);
	static void connectWidgetToProperty(const Configuration::TypedProperty<bool>& property, QRadioButton* widget);
	static void connectWidgetToProperty(const Configuration::TypedProperty<QString>& property, QComboBox* widget);
	static void connectWidgetToProperty(const Configuration::TypedProperty<QString>& property, QLineEdit* widget);
	static void connectWidgetToProperty(const Configuration::TypedProperty<Password>& property, QLineEdit* widget);
	static void connectWidgetToProperty(const Configuration::TypedProperty<QColor>& property, QPushButton* widget);
	static void connectWidgetToProperty(const Configuration::TypedProperty<int>& property, QComboBox* widget);
	static void connectWidgetToProperty(const Configuration::TypedProperty<int>& property, QSpinBox* widget);
	static void connectWidgetToProperty(const Configuration::TypedProperty<QUuid>& property, QComboBox* widget);

	// SFINAE overload for enum classes
	template<typename T>
	static typename std::enable_if<std::is_enum<T>::value, void>::type
	connectWidgetToProperty(const Configuration::TypedProperty<T>& property, QComboBox* widget)
	{
		QObject::connect(widget, QOverload<int>::of(&QComboBox::currentIndexChanged), property.lambdaContext(),
						 [&property](int index) { property.setValue(static_cast<T>(index)); });
	}


	// overloads for special properties which can't be connected to widgets
	static void connectWidgetToProperty(const Configuration::TypedProperty<QStringList>& property, QLabel* widget)
	{
		Q_UNUSED(property)
		Q_UNUSED(widget)
	}

	static void connectWidgetToProperty(const Configuration::TypedProperty<QJsonArray>& property, QLabel* widget)
	{
		Q_UNUSED(property)
		Q_UNUSED(widget)
	}

#define CONNECT_WIDGET_TO_PROPERTY(className, config, type, get, set, key, parentKey, defaultValue, flags) \
	Configuration::UiMapping::connectWidgetToProperty(config.get##Property(), ui->get);
};

}
