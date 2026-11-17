// SPDX-FileCopyrightText: 2009-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/Object.h"
#include "Configuration/Property.h"
#include "Configuration/Proxy.h"


namespace Configuration {

Configuration::Property::Property(Object* object, const QString& key, const QString& parentKey,
								  const QVariant& defaultValue, Flags flags) :
	QObject(object),
	m_object(object),
	m_proxy(nullptr),
	m_key(key),
	m_parentKey(parentKey),
	m_defaultValue(defaultValue),
	m_flags(flags)
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
	connect(proxy, &QObject::destroyed, this, [this]() {
		setParent(nullptr);
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
SecretReference Configuration::TypedProperty<SecretReference>::value() const
{
	const auto ownReference = SecretReference::fromString(defaultValue().toString());
	const auto storedValue = variantValue().toString();

	if (storedValue.isEmpty() || storedValue == ownReference.toString())
	{
		return ownReference;
	}

	if (SecretReference::isLegacyValue(storedValue))
	{
		vDebug() << "legacy value of" << absoluteKey() << "not migrated yet";
	}
	else
	{
		vWarning() << "ignoring reference to a different secret in" << absoluteKey();
	}

	return {};
}



template<>
void Configuration::TypedProperty<SecretReference>::setValue(const SecretReference& value) const
{
	setVariantValue(value.toString());
}


}
