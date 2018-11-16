/*
 * Configuration/Object.h - ConfigurationObject class
 *
 * Copyright (c) 2009-2018 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
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

#include <QColor>
#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>

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
	Object( Store::Backend backend, Store::Scope scope, const Object& defaults, const QString& storeName = QString() );
	Object( Store* store );
	Object( const Object& );
	~Object() override;

	Object& operator=( const Object& ref );
	Object& operator+=( const Object& ref );

	bool hasValue( const QString& key, const QString& parentKey = QString() ) const;

	QVariant value( const QString& key, const QString& parentKey = QString() ) const;

	void setValue( const QString& key, const QVariant& value, const QString& parentKey = QString() );

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


#define DECLARE_CONFIG_STRING_PROPERTY(get,set,key,parentKey)\
	public:											\
		QString get() const					\
		{											\
			return value( key, parentKey ).toString();			\
		} \
		void set( const QString &val )						\
		{																\
			setValue( key, val,	parentKey );							\
		}

#define DECLARE_CONFIG_STRINGLIST_PROPERTY(get,set,key,parentKey)\
	public:													\
		QStringList get() const						\
		{													\
			return value( key, parentKey ).toStringList();	\
		} \
		void set( const QStringList &val )					\
		{																\
			setValue( key, val,	parentKey );							\
		}

#define DECLARE_CONFIG_INT_PROPERTY(get,set,key,parentKey)	\
	public:												\
		int get() const							\
		{												\
			return value( key, parentKey ).toInt();		\
		} \
		void set( int val )									\
		{																\
			setValue( key, val, parentKey );			\
		}

#define DECLARE_CONFIG_BOOL_PROPERTY(get,set,key,parentKey)	\
	public:												\
		bool get() const								\
		{												\
			return value( key, parentKey ).toBool();	\
		} \
		void set( bool val )									\
		{																\
			setValue( key, val, parentKey );			\
		}

#define DECLARE_CONFIG_JSONOBJECT_PROPERTY(get,set,key,parentKey)\
	public:											\
		QJsonObject get() const					\
		{											\
			return value( key, parentKey ).toJsonObject();			\
		} \
		void set( const QJsonObject& val )									\
		{																\
			setValue( key, val, parentKey );			\
		}

#define DECLARE_CONFIG_JSONARRAY_PROPERTY(get,set,key,parentKey)\
	public:											\
		QJsonArray get() const					\
		{											\
			return value( key, parentKey ).toJsonArray();			\
		} \
		void set( const QJsonArray& val )									\
		{																\
			setValue( key, val, parentKey );			\
		}

#define DECLARE_CONFIG_UUID_PROPERTY(get,set,key,parentKey)\
	public:											\
		QUuid get() const					\
		{											\
			return value( key, parentKey ).toUuid();			\
		} \
		void set( QUuid val )									\
		{																\
			setValue( key, val, parentKey );			\
		}

#define DECLARE_CONFIG_COLOR_PROPERTY(get,set,key,parentKey)\
	public:											\
		QColor get() const					\
		{											\
			return value( key, parentKey ).value<QColor>();			\
		} \
		void set( const QColor& val )									\
		{																\
			setValue( key, val, parentKey );			\
		}

#define DECLARE_CONFIG_PASSWORD_PROPERTY(get,set,key,parentKey)\
	public:											\
		QString get() const					\
		{											\
			return VeyonCore::cryptoCore().decryptPassword( value( key, parentKey ).toString() );	\
		}	\
		QString get##Plain() const					\
		{											\
			return value( key, parentKey ).toString().toUtf8();			\
		}	\
		void set( const QString& val )									\
		{																\
			setValue( key, VeyonCore::cryptoCore().encryptPassword( val.toUtf8() ), parentKey );			\
		}

#define DECLARE_CONFIG_PROPERTY(className,config,type, get, set, key, parentKey)			\
			DECLARE_CONFIG_##type##_PROPERTY(get,set,QStringLiteral(key),QStringLiteral(parentKey))

}
