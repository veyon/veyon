// SPDX-FileCopyrightText: 2009-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QVariant>

namespace Configuration {

class Object;

// clazy:excludeall=copyable-polymorphic

class Store
{
	Q_GADGET
public:
	enum class Backend {
		Local, // registry or similar
		JsonFile,
		None
	};
	Q_ENUM(Backend)

	enum class Scope {
		User,	// for current user
		System, // system-wide (service settings etc.)
	};

	Store(Backend backend, Scope scope) :
		m_backend(backend),
		m_scope(scope)
	{
	}

	virtual ~Store() = default;

	Backend backend() const
	{
		return m_backend;
	}

	Scope scope() const
	{
		return m_scope;
	}

	QString configurationNameFromScope() const
	{
		switch (scope())
		{
		case Scope::User: return QStringLiteral("UserConfig");
		case Scope::System: return QStringLiteral("SystemConfig");
		}

		return {};
	}

	void setName(const QString& name)
	{
		m_name = name;
	}

	const QString& name() const
	{
		return m_name;
	}

	virtual void load(Object* obj) = 0;
	virtual void flush(const Object* obj) = 0;
	virtual bool isWritable() const = 0;
	virtual void clear() = 0;


private:
	const Backend m_backend;
	const Scope m_scope;
	QString m_name;
};

}
