// SPDX-FileCopyrightText: 2009-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCheckBox>
#include <QColorDialog>
#include <QCoreApplication>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>

#include "Configuration/SecretPropertyRegistry.h"
#include "Configuration/UiMapping.h"
#include "SecretStore.h"

namespace Configuration {

static constexpr auto WidgetConfigPropertyFlags = "ConfigPropertyFlags";


void UiMapping::initWidgetFromProperty(const Configuration::TypedProperty<bool>& property, QCheckBox* widget)
{
	widget->setChecked(property.value());
}



void UiMapping::initWidgetFromProperty(const Configuration::TypedProperty<bool>& property, QGroupBox* widget)
{
	widget->setChecked(property.value());
}



void UiMapping::initWidgetFromProperty(const Configuration::TypedProperty<bool>& property, QRadioButton* widget)
{
	widget->setChecked(property.value());
}



void UiMapping::initWidgetFromProperty(const Configuration::TypedProperty<QString>& property, QComboBox* widget)
{
	widget->setCurrentText(property.value());
}



void UiMapping::initWidgetFromProperty(const Configuration::TypedProperty<QString>& property, QLineEdit* widget)
{
	widget->setText(property.value());
}



// The widget never shows a stored secret. Without access to the SecretStore (configurator not
// elevated) it is disabled; otherwise an entered value is written to the store when editing
// has finished (03-secretstore-config.md §3).
static void updateSecretWidget(const Configuration::TypedProperty<SecretReference>& property, QLineEdit* widget)
{
	widget->clear();

	if (SecretStore::isAvailable() == false)
	{
		widget->setEnabled(false);
		widget->setPlaceholderText(QCoreApplication::translate("Configuration::UiMapping",
															   "requires administrator privileges"));
		return;
	}

	// e.g. a conflicting row of another plugin: the property is disabled instead of silently
	// ignoring what is entered (03-secretstore-config.md §3a)
	if (Configuration::SecretPropertyRegistry::isRegistered(property) == false)
	{
		widget->setEnabled(false);
		widget->setPlaceholderText(QCoreApplication::translate("Configuration::UiMapping", "not available"));
		widget->setToolTip(QCoreApplication::translate("Configuration::UiMapping",
													   "This secret property is not registered with its secret key. "
													   "See the log file for details."));
		return;
	}

	const auto reference = property.value();
	const auto isStored = reference.isValid() && SecretStore::keys().contains(reference.key());
	widget->setPlaceholderText(isStored ? QCoreApplication::translate("Configuration::UiMapping", "stored securely") : QString());
}



void UiMapping::initWidgetFromProperty(const Configuration::TypedProperty<SecretReference>& property, QLineEdit* widget)
{
	updateSecretWidget(property, widget);
}



void UiMapping::initWidgetFromProperty(const Configuration::TypedProperty<QColor>& property, QPushButton* widget)
{
	auto palette = widget->palette();
	palette.setColor(QPalette::Button, property.value());
	widget->setPalette(palette);
}



void UiMapping::initWidgetFromProperty(const Configuration::TypedProperty<int>& property, QComboBox* widget)
{
	widget->setCurrentIndex(property.value());
}



void UiMapping::initWidgetFromProperty(const Configuration::TypedProperty<int>& property, QSpinBox* widget)
{
	widget->setValue(property.value());
}



void UiMapping::initWidgetFromProperty(const Configuration::TypedProperty<QUuid>& property, QComboBox* widget)
{
	widget->setCurrentIndex(widget->findData(property.value()));
}



void UiMapping::setFlags(QObject* object, Property::Flags flags)
{
	object->setProperty(WidgetConfigPropertyFlags, QVariant::fromValue(flags));
}



Property::Flags UiMapping::flags(QObject* object)
{
	return object->property(WidgetConfigPropertyFlags).value<Property::Flags>();
}



void UiMapping::connectWidgetToProperty(const Configuration::TypedProperty<bool>& property, QCheckBox* widget)
{
	QObject::connect(widget, &QCheckBox::toggled, property.lambdaContext(),
					 [&property](bool value) { property.setValue(value); });
}



void UiMapping::connectWidgetToProperty(const Configuration::TypedProperty<bool>& property, QGroupBox* widget)
{
	QObject::connect(widget, &QGroupBox::toggled, property.lambdaContext(),
					 [&property](bool value) { property.setValue(value); });
}



void UiMapping::connectWidgetToProperty(const Configuration::TypedProperty<bool>& property, QRadioButton* widget)
{
	QObject::connect(widget, &QCheckBox::toggled, property.lambdaContext(),
					 [&property](bool value) { property.setValue(value); });
}



void UiMapping::connectWidgetToProperty(const Configuration::TypedProperty<QString>& property, QComboBox* widget)
{
	QObject::connect(widget, &QComboBox::currentTextChanged, property.lambdaContext(),
					 [&property](const QString& value) { property.setValue(value); });
}



void UiMapping::connectWidgetToProperty(const Configuration::TypedProperty<QString>& property, QLineEdit* widget)
{
	QObject::connect(widget, &QLineEdit::textChanged, property.lambdaContext(),
					 [&property](const QString& value) { property.setValue(value); });
}



void UiMapping::connectWidgetToProperty(const Configuration::TypedProperty<SecretReference>& property, QLineEdit* widget)
{
	QObject::connect(widget, &QLineEdit::editingFinished, property.lambdaContext(), [&property, widget]() {
		const auto reference = Configuration::SecretPropertyRegistry::ownReference(property);
		if (widget->text().isEmpty() || reference.isValid() == false || SecretStore::isAvailable() == false)
		{
			return;
		}

		auto plainText = widget->text().toUtf8();
		const auto written = SecretStore::write(reference.key(), Veyon::Crypto::SecureBuffer::fromByteArray(plainText));
		plainText.fill(0);

		if (written)
		{
			property.setValue(reference);
		}
		else
		{
			vCritical() << "failed to store secret" << reference.key();
		}

		updateSecretWidget(property, widget);
	});
}



void UiMapping::connectWidgetToProperty(const Configuration::TypedProperty<QColor>& property, QPushButton* widget)
{
	QObject::connect(widget, &QAbstractButton::clicked, property.lambdaContext(), [&property, widget]() {
		auto palette = widget->palette();
		QColorDialog d(widget->palette().color(QPalette::Button), widget);
		if (d.exec())
		{
			property.setValue(d.selectedColor());
			palette.setColor(QPalette::Button, d.selectedColor());
			widget->setPalette(palette);
		}
	});
}



void UiMapping::connectWidgetToProperty(const Configuration::TypedProperty<int>& property, QComboBox* widget)
{
	QObject::connect(widget, QOverload<int>::of(&QComboBox::currentIndexChanged), property.lambdaContext(),
					 [&property](int index) { property.setValue(index); });
}



void UiMapping::connectWidgetToProperty(const Configuration::TypedProperty<int>& property, QSpinBox* widget)
{
	QObject::connect(widget, QOverload<int>::of(&QSpinBox::valueChanged), property.lambdaContext(),
					 [&property](int index) { property.setValue(index); });
}



void UiMapping::connectWidgetToProperty(const Configuration::TypedProperty<QUuid>& property, QComboBox* widget)
{
	QObject::connect(widget, QOverload<int>::of(&QComboBox::currentIndexChanged), property.lambdaContext(),
					 [widget, &property](int index) { property.setValue(widget->itemData(index).toUuid()); });
}


}
