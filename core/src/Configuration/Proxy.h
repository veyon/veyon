/*
 * Configuration/Proxy.h - ConfigurationProxy class
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include "Configuration/Object.h"
#include "Configuration/Property.h"

namespace Configuration
{

class VEYON_CORE_EXPORT Proxy : public QObject
{
	Q_OBJECT
public:
	explicit Proxy( Object* object );
	~Proxy() override = default;

	bool hasValue( const QString& key, const QString& parentKey ) const;

	QVariant value( const QString& key, const QString& parentKey, const QVariant& defaultValue ) const;

	void setValue( const QString& key, const QVariant& value, const QString& parentKey );

	void removeValue( const QString &key, const QString &parentKey );

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

	void setInstanceId( const QString& instanceId )
	{
		m_instanceId = instanceId;
	}

	void removeInstance( const QString& parentKey );

private:
	QString instanceParentKey( const QString& parentKey ) const;

	Object* m_object;
	QString m_instanceId;

} ;

#define DECLARE_CONFIG_PROXY(name, ops) \
	class name : public Configuration::Proxy { \
	public: \
		explicit name( Configuration::Object* object ); \
		ops(DECLARE_CONFIG_PROPERTY) \
	};

#define IMPLEMENT_CONFIG_PROXY(name) \
	name::name( Configuration::Object* object ) : Configuration::Proxy( object ) { }

}
