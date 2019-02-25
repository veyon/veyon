/*
 * Configuration/Object.h - ConfigurationObject class
 *
 * Copyright (c) 2009-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "VeyonCore.h"
#include "Configuration/Store.h"

namespace Configuration
{

// clazy:excludeall=ctor-missing-parent-argument,copyable-polymorphic

class VEYON_CORE_EXPORT Object : public QObject
{
	Q_OBJECT
public:
	typedef QMap<QString, QVariant> DataMap;

	Object();
	Object( Store::Backend backend, Store::Scope scope, const QString& storeName = QString() );
	Object( Store* store );
	Object( const Object& );
	~Object() override;

	Object& operator=( const Object& ref );
	Object& operator+=( const Object& ref );

	bool hasValue( const QString& key, const QString& parentKey ) const;

	QVariant value( const QString& key, const QString& parentKey, const QVariant& defaultValue ) const;

	void setValue( const QString& key, const QVariant& value, const QString& parentKey );

	void removeValue( const QString& key, const QString& parentKey );

	void addSubObject( Object* obj, const QString& parentKey );

	void reloadFromStore()
	{
		if( m_store )
		{
			m_store->load( this );
		}
	}

	void flushStore()
	{
		if( m_store )
		{
			m_store->flush( this );
		}
	}

	bool isStoreWritable() const
	{
		return m_store->isWritable();
	}

	void clear()
	{
		m_data.clear();
	}

	const DataMap & data() const
	{
		return m_data;
	}


signals:
	void configurationChanged();


private:
	static Store* createStore( Store::Backend backend, Store::Scope scope );

	Configuration::Store *m_store;
	bool m_customStore;
	DataMap m_data;

} ;

}
