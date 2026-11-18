// SPDX-FileCopyrightText: 2009-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Configuration/Store.h"
#include "VeyonCore.h"

namespace Configuration {

// clazy:excludeall=ctor-missing-parent-argument,copyable-polymorphic

class VEYON_CORE_EXPORT Object : public QObject
{
	Q_OBJECT
public:
	using DataMap = QMap<QString, QVariant>;

	Object() = default;
	Object(Store::Backend backend, Store::Scope scope, const QString& storeName = {});
	explicit Object(Store* store);
	Object(const Object&);
	~Object() override;

	Object& operator=(const Object& ref);
	Object& operator+=(const Object& ref);

	bool hasValue(const QString& key, const QString& parentKey) const;

	QVariant value(const QString& key, const QString& parentKey, const QVariant& defaultValue) const;

	void setValue(const QString& key, const QVariant& value, const QString& parentKey);

	void removeValue(const QString& key, const QString& parentKey);

	void addSubObject(Object* obj, const QString& parentKey);

	void reloadFromStore()
	{
		if (m_store)
		{
			m_store->load(this);
		}
	}

	void flushStore()
	{
		if (m_store)
		{
			m_store->flush(this);
		}
	}

	bool isStoreWritable() const
	{
		return m_store && m_store->isWritable();
	}

	void replaceData(const DataMap& data);

	void clear()
	{
		replaceData({});
	}

	const DataMap& data() const
	{
		return m_data;
	}


Q_SIGNALS:
	void configurationChanged();


private:
	static Store* createStore(Store::Backend backend, Store::Scope scope);

	Configuration::Store* m_store{nullptr};
	bool m_customStore{false};
	DataMap m_data{};
};

}
