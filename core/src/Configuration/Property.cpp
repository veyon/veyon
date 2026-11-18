// SPDX-FileCopyrightText: 2009-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/Object.h"
#include "Configuration/Property.h"
#include "Configuration/Proxy.h"
#include <QMetaEnum>


namespace Configuration {

Property& Property::setUiMetadata(UiMetadata metadata)
{
	m_uiMetadata = std::move(metadata);
	Q_EMIT metadataChanged();
	return *this;
}

Property::Editor Property::editor() const
{
	if (m_uiMetadata.editor != Editor::Automatic) return m_uiMetadata.editor;
	if (!m_uiMetadata.choices.isEmpty() || valueType().flags().testFlag(QMetaType::IsEnumeration)) return Editor::Choice;
	if (valueType() == QMetaType::fromType<Password>()) return Editor::Password;
	switch (valueType().id())
	{
	case QMetaType::Bool: return Editor::Boolean;
	case QMetaType::Int:
	case QMetaType::UInt:
	case QMetaType::LongLong:
	case QMetaType::ULongLong: return Editor::Integer;
	case QMetaType::QColor: return Editor::Color;
	case QMetaType::QString: return Editor::Text;
	case QMetaType::QUuid: return Editor::Choice;
	default: return Editor::Custom;
	}
}

bool Property::isEnabled() const
{
	return (m_object || m_proxy) && (!m_uiMetadata.enabled || m_uiMetadata.enabled());
}

bool Property::isVisible() const
{
	return (m_object || m_proxy) && !flags().testFlag(Flag::Hidden) && !flags().testFlag(Flag::Legacy) &&
		(!m_uiMetadata.visible || m_uiMetadata.visible());
}

bool Property::validateEditorValue(const QVariant& value, QString* error) const
{
	if (error) error->clear();
	if (!value.isValid())
	{
		if (error) *error = tr("A value is required.");
		return false;
	}
	if (valueType().flags().testFlag(QMetaType::IsEnumeration) && m_uiMetadata.choices.isEmpty())
	{
		bool ok = false;
		const auto numeric = value.toInt(&ok);
		const auto metaObject = valueType().metaObject();
		const auto enumName = QByteArray(valueType().name()).split(':').last();
		const auto index = metaObject ? metaObject->indexOfEnumerator(enumName.constData()) : -1;
		if (!ok || (index >= 0 && !metaObject->enumerator(index).valueToKey(numeric)))
		{
			if (error) *error = tr("The selected value is not a valid option.");
			return false;
		}
	}
	if (!m_uiMetadata.choices.isEmpty())
	{
		bool found = false;
		for (const auto& choice : m_uiMetadata.choices)
		{
			if (choice.value == value ||
				(valueType() == QMetaType::fromType<QUuid>() && choice.value.toUuid() == value.toUuid()) ||
				(valueType().flags().testFlag(QMetaType::IsEnumeration) && choice.value.toInt() == value.toInt() &&
				 value.toString() == QString::number(value.toInt())))
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			if (error) *error = tr("Select one of the available values.");
			return false;
		}
	}
	if (m_uiMetadata.minimum.isValid() || m_uiMetadata.maximum.isValid())
	{
		bool ok = false;
		const auto numeric = value.toDouble(&ok);
		if (!ok || !std::isfinite(numeric) ||
			(m_uiMetadata.minimum.isValid() && numeric < m_uiMetadata.minimum.toDouble()) ||
			(m_uiMetadata.maximum.isValid() && numeric > m_uiMetadata.maximum.toDouble()))
		{
			if (error) *error = tr("The value is outside the allowed range.");
			return false;
		}
	}
	return true;
}

bool Property::setEditorValue(const QVariant& value, QString* error) const
{
	if (!validateEditorValue(value, error)) return false;
	setVariantValue(value);
	return true;
}

Configuration::Property::Property( Object* object, const QString& key, const QString& parentKey,
								   const QVariant& defaultValue, Flags flags ) :
	QObject( object ),
	m_object( object ),
	m_proxy( nullptr ),
	m_key( key ),
	m_parentKey( parentKey ),
	m_defaultValue( defaultValue ),
	m_flags( flags )
{
}



Configuration::Property::Property(Proxy* proxy, const QString& key, const QString& parentKey,
								  const QVariant& defaultValue, Flags flags) :
	QObject(proxy->object()),
	m_object(nullptr),
	m_proxy(proxy),
	m_key(key),
	m_parentKey(parentKey),
	m_defaultValue(defaultValue),
	m_flags(flags)
{
	connect (proxy, &QObject::destroyed, this, [this]() {
		m_proxy = nullptr;
		setParent(nullptr);
		Q_EMIT metadataChanged();
		deleteLater();
	});
}



QObject* Property::lambdaContext() const
{
	if (m_object)
	{
		return m_object;
	}

	return m_proxy;
}

QString Property::absoluteKey() const
{
	auto parent = m_parentKey;
	if (m_proxy && !m_proxy->instanceId().isEmpty())
		parent += (parent.isEmpty() ? QString() : QStringLiteral("/")) + m_proxy->instanceId();
	return parent.isEmpty() ? m_key : parent + QLatin1Char('/') + m_key;
}



QVariant Property::variantValue() const
{
	if (m_object)
	{
		return m_object->value(m_key, m_parentKey, m_defaultValue);
	}
	else if (m_proxy)
	{
		return m_proxy->value(m_key, m_parentKey, m_defaultValue);
	}

	return m_defaultValue;
}



void Property::setVariantValue(const QVariant& value) const
{
	if (m_object)
	{
		m_object->setValue(m_key, value, m_parentKey);
	}
	else if (m_proxy)
	{
		m_proxy->setValue(m_key, value, m_parentKey);
	}
	else
	{
		qFatal("%s", Q_FUNC_INFO);
	}
}



Property* Property::find(QObject* parent, const QString& key, const QString& parentKey)
{
	const auto properties = parent->findChildren<Property*>();
	for (auto property : properties)
	{
		if (property->m_key == key && property->m_parentKey == parentKey)
		{
			return property;
		}
	}

	return nullptr;
}



template<>
Password Configuration::TypedProperty<Password>::value() const
{
	return Password::fromEncrypted(variantValue().toString());
}



template<>
void Configuration::TypedProperty<Password>::setValue(const Password& value) const
{
	setVariantValue(value.encrypted());
}

template<>
QVariant TypedProperty<Password>::editorValue() const
{
	return QString::fromUtf8(value().plainText().toByteArray());
}

template<>
bool TypedProperty<Password>::setEditorValue(const QVariant& input, QString* error) const
{
	if (!validateEditorValue(input, error)) return false;
	if (input.metaType().id() != QMetaType::QString)
	{
		if (error) *error = tr("A password must be text.");
		return false;
	}
	setValue(Password::fromPlainText(input.toString().toUtf8()));
	return true;
}


}
