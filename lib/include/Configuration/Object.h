/*
 * Configuration/Object.h - ConfigurationObject class
 *
 * Copyright (c) 2009-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#ifndef _CONFIGURATION_OBJECT_H
#define _CONFIGURATION_OBJECT_H

#include <QtCore/QObject>

#include "Configuration/Store.h"

namespace Configuration
{

class Object : public QObject
{
	Q_OBJECT
public:
	typedef QMap<QString, QVariant> DataMap;

	Object( Store::Backend backend, Store::Scope scope );
	Object( Store *store );
	Object( const Object & );
	~Object();

	Object &operator=( const Object &ref );
	Object &operator+=( const Object &ref );

	QString value( const QString & _key,
			const QString & _parentKey = QString() ) const;

	void setValue( const QString & _key,
			const QString & _value,
			const QString & _parentKey = QString() );

	void removeValue( const QString &key, const QString &parentKey );

	void addSubObject( Object *obj, const QString &parentKey );

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
	Configuration::Store *m_store;
	bool m_customStore;
	DataMap m_data;

} ;


#define DECLARE_CONFIG_STRING_PROPERTY(get,key,parentKey)\
	public:											\
		inline QString get() const					\
		{											\
			return value( key, parentKey );			\
		}

#define DECLARE_CONFIG_STRINGLIST_PROPERTY(get,key,parentKey)\
	public:													\
		inline QStringList get() const						\
		{													\
			return value( key, parentKey ).split( ',' );	\
		}

#define DECLARE_CONFIG_INT_PROPERTY(get,key,parentKey)	\
	public:												\
		inline int get() const							\
		{												\
			return value( key, parentKey ).toInt();		\
		}

#define DECLARE_CONFIG_BOOL_PROPERTY(get,key,parentKey)	\
	public:												\
		bool get() const								\
		{												\
			return value( key, parentKey ).toInt() ?	\
										true : false;	\
		}

#define DECLARE_CONFIG_PROPERTY(className,config,type, get, set, key, parentKey)			\
			DECLARE_CONFIG_##type##_PROPERTY(get,key,parentKey)


#define IMPLEMENT_CONFIG_SET_STRING_PROPERTY(className,set,key,parentKey)\
		void className::set( const QString &val )						\
		{																\
			setValue( key, val,	parentKey );							\
		}

#define IMPLEMENT_CONFIG_SET_STRINGLIST_PROPERTY(className,set,key,parentKey)\
		void className::set( const QStringList &val )					\
		{																\
			setValue( key, val.join( "," ),	parentKey );				\
		}

#define IMPLEMENT_CONFIG_SET_INT_PROPERTY(className,set,key,parentKey)	\
		void className::set( int val )									\
		{																\
			setValue( key, QString::number( val ), parentKey );			\
		}

#define IMPLEMENT_CONFIG_SET_BOOL_PROPERTY(className,set,key,parentKey)	\
		void className::set( bool val )									\
		{																\
			setValue( key, QString::number( val ), parentKey );			\
		}

#define IMPLEMENT_CONFIG_SET_PROPERTY(className, config,type, get, set, key, parentKey)	\
			IMPLEMENT_CONFIG_SET_##type##_PROPERTY(className,set,key,parentKey)


}

#endif
