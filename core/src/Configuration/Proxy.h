// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Configuration/Object.h"
#include "Configuration/Property.h"

namespace Configuration {

class VEYON_CORE_EXPORT Proxy : public QObject
{
	Q_OBJECT
public:
	explicit Proxy(Object* object);
	~Proxy() override = default;

	bool hasValue(const QString& key, const QString& parentKey) const;

	QVariant value(const QString& key, const QString& parentKey, const QVariant& defaultValue) const;

	void setValue(const QString& key, const QVariant& value, const QString& parentKey);

	void removeValue(const QString& key, const QString& parentKey);

	void reloadFromStore();
	void flushStore();

	QObject* object() const
	{
		return m_object;
	}

	const QString& instanceId() const
	{
		return m_instanceId;
	}

	void setInstanceId(const QString& instanceId)
	{
		m_instanceId = instanceId;
	}

	void removeInstance(const QString& parentKey);

private:
	QString instanceParentKey(const QString& parentKey) const;

	Object* m_object;
	QString m_instanceId;
};

#define DECLARE_CONFIG_PROXY(name, ops) \
	class name : public Configuration::Proxy \
	{ \
	public: \
		explicit name(Configuration::Object* object); \
		ops(DECLARE_CONFIG_PROPERTY) \
	};

#define IMPLEMENT_CONFIG_PROXY(name) \
	name::name(Configuration::Object* object) : Configuration::Proxy(object) \
	{ \
	}

}
