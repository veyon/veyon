// SPDX-FileCopyrightText: 2009-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/JsonStore.h"
#include "Configuration/LocalStore.h"
#include "Configuration/Object.h"


namespace Configuration {

Object::Object(Store::Backend backend, Store::Scope scope, const QString& storeName) :
	m_store(createStore(backend, scope)),
	m_data()
{
	if (m_store) m_store->setName(storeName);

	reloadFromStore();
}



Object::Object(Store* store) :
	m_store(store),
	m_customStore(true),
	m_data()
{
	reloadFromStore();
}



Object::Object(const Object& obj)
{
	*this = obj;
}



Object::~Object()
{
	if (m_customStore == false)
	{
		delete m_store;
	}
}



Object& Object::operator=(const Object& ref)
{
	if (&ref == this)
	{
		return *this;
	}

	if (m_customStore == false &&
		ref.m_customStore == false &&
		ref.m_store)
	{
		const auto backend = ref.m_store->backend();
		const auto scope = ref.m_store->scope();

		delete m_store;

		m_store = createStore(backend, scope);
	}

	replaceData(ref.data());

	return *this;
}



// allow easy merging of two data maps - source is dominant over destination
static Object::DataMap operator+(Object::DataMap dst, const Object::DataMap& src)
{
	for (auto it = src.begin(), end = src.end(); it != end; ++it)
	{
		if (it.value().userType() == QMetaType::QVariantMap && dst.contains(it.key()))
		{
			dst[it.key()] = dst[it.key()].toMap() + it.value().toMap();
		}
		else
		{
			dst[it.key()] = it.value();
		}
	}
	return dst;
}



Object& Object::operator+=(const Object& ref)
{
	replaceData(m_data + ref.data());

	return *this;
}

void Object::replaceData(const DataMap& data)
{
	if (m_data != data)
	{
		m_data = data;
		Q_EMIT configurationChanged();
	}
}



bool Object::hasValue(const QString& key, const QString& parentKey) const
{
	// empty parentKey?
	if (parentKey.isEmpty())
	{
		// search for key in toplevel data map
		return m_data.contains(key);
	}

	// recursively search through data maps and sub data-maps until
	// all levels of the parentKey are processed
	const QStringList subLevels = parentKey.split(QLatin1Char('/'));
	DataMap currentMap = m_data;

	for (const auto& level : subLevels)
	{
		if (currentMap.contains(level) &&
			currentMap[level].userType() == QMetaType::QVariantMap)
		{
			currentMap = currentMap[level].toMap();
		}
		else
		{
			return false;
		}
	}

	// ok, we're there - does the current submap then contain our key?
	return currentMap.contains(key);
}



QVariant Object::value(const QString& key, const QString& parentKey, const QVariant& defaultValue) const
{
	// empty parentKey?
	if (parentKey.isEmpty())
	{
		// search for key in toplevel data map
		if (m_data.contains(key))
		{
			return m_data[key];
		}
		return defaultValue;
	}

	// recursively search through data maps and sub data-maps until
	// all levels of the parentKey are processed
	const QStringList subLevels = parentKey.split(QLatin1Char('/'));
	DataMap currentMap = m_data;
	for (const auto& level : subLevels)
	{
		if (currentMap.contains(level) &&
			currentMap[level].userType() == QMetaType::QVariantMap)
		{
			currentMap = currentMap[level].toMap();
		}
		else
		{
			return defaultValue;
		}
	}

	// ok, we're there - does the current submap then contain our key?
	if (currentMap.contains(key))
	{
		return currentMap[key];
	}
	return defaultValue;
}



static Object::DataMap setValueRecursive(Object::DataMap data,
										 QStringList subLevels,
										 const QString& key,
										 const QVariant& value)
{
	if (subLevels.isEmpty())
	{
		// search for key in toplevel data map
		if (!data.contains(key) || data[key].userType() != QMetaType::QVariantMap)
		{
			data[key] = value;
		}
		else
		{
			vWarning() << "cannot replace sub data map with a value!";
		}

		return data;
	}

	const QString level = subLevels.takeFirst();
	if (data.contains(level))
	{
		if (data[level].userType() != QMetaType::QVariantMap)
		{
			vWarning() << "parent key points doesn't point to a data map!";
			return data;
		}
	}
	else
	{
		data[level] = Object::DataMap();
	}

	data[level] = setValueRecursive(data[level].toMap(), subLevels, key, value);

	return data;
}



void Object::setValue(const QString& key, const QVariant& value, const QString& parentKey)
{
	// recursively search through data maps and sub data-maps until
	// all levels of the parentKey are processed
	QStringList subLevels = parentKey.split(QLatin1Char('/'), Qt::SkipEmptyParts);
	DataMap data = setValueRecursive( m_data, subLevels, key, value );

	if (data != m_data)
	{
		m_data = data;
		Q_EMIT configurationChanged();
	}
}



static Object::DataMap removeValueRecursive(Object::DataMap data,
											QStringList subLevels,
											const QString& key)
{
	if (subLevels.isEmpty())
	{
		// search for key in toplevel data map
		if (data.contains(key))
		{
			data.remove(key);
		}

		return data;
	}

	const QString level = subLevels.takeFirst();
	if (data.contains(level) && data[level].userType() == QMetaType::QVariantMap)
	{
		data[level] = removeValueRecursive(data[level].toMap(), subLevels, key);
	}

	return data;
}



void Object::removeValue(const QString& key, const QString& parentKey)
{
	QStringList subLevels = parentKey.split(QLatin1Char('/'), Qt::SkipEmptyParts);
	DataMap data = removeValueRecursive( m_data, subLevels, key );
	if( data != m_data )
	{
		m_data = data;
		Q_EMIT configurationChanged();
	}
}



static void addSubObjectRecursive(const Object::DataMap& dataMap,
								  Object* _this,
								  const QString& parentKey)
{
	for (auto it = dataMap.begin(), end = dataMap.end(); it != end; ++it)
	{
		if (it.value().userType() == QMetaType::QVariantMap)
		{
			auto newParentKey = it.key();
			if (parentKey.isEmpty() == false)
			{
				newParentKey = parentKey + QLatin1Char('/') + newParentKey;
			}
			addSubObjectRecursive(it.value().toMap(), _this, newParentKey);
		}
		else
		{
			_this->setValue(it.key(), it.value(), parentKey);
		}
	}
}



void Object::addSubObject(Object* obj, const QString& parentKey)
{
	addSubObjectRecursive(obj->data(), this, parentKey);
}



Store* Object::createStore(Store::Backend backend, Store::Scope scope)
{
	switch (backend)
	{
	case Store::Backend::Local: return new LocalStore(scope);
	case Store::Backend::JsonFile: return new JsonStore(scope);
	case Store::Backend::None:
		break;
	default:
		vCritical() << "invalid store" << backend << "selected";
		break;
	}

	return nullptr;
}


}
