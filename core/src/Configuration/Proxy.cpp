// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/Proxy.h"


namespace Configuration {

Proxy::Proxy(Object* object) :
	QObject(),
	m_object(object),
	m_instanceId()
{
}



bool Proxy::hasValue(const QString& key, const QString& parentKey) const
{
	return m_object->hasValue(key, instanceParentKey(parentKey));
}



QVariant Proxy::value(const QString& key, const QString& parentKey, const QVariant& defaultValue) const
{
	return m_object->value(key, instanceParentKey(parentKey), defaultValue);
}



void Proxy::setValue(const QString& key, const QVariant& value, const QString& parentKey)
{
	m_object->setValue(key, value, instanceParentKey(parentKey));
}



void Proxy::removeValue(const QString& key, const QString& parentKey)
{
	m_object->removeValue(key, instanceParentKey(parentKey));
}



void Proxy::reloadFromStore()
{
	m_object->reloadFromStore();
}



void Proxy::flushStore()
{
	m_object->flushStore();
}



void Proxy::removeInstance(const QString& parentKey)
{
	m_object->removeValue(instanceId(), parentKey);
}



QString Proxy::instanceParentKey(const QString& parentKey) const
{
	if (m_instanceId.isEmpty())
	{
		return parentKey;
	}

	return parentKey + QLatin1Char('/') + m_instanceId;
}

}
