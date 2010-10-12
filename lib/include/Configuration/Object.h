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

	Object( Store::Backend _backend, Store::Scope _scope );
	~Object();

	QString value( const QString & _key,
			const QString & _parentKey = QString() ) const;

	void setValue( const QString & _key,
			const QString & _value,
			const QString & _parentKey = QString() );

	void flushStore( void )
	{
		m_store->flush( this );
	}

	const DataMap & data( void ) const
	{
		return m_data;
	}


private:
	Configuration::Store * m_store;
	DataMap m_data;

} ;


#define MAP_CONFIG_PROPERTY(get,set,key,parentKey)		\
	public slots:						\
		inline void set( const QString & val )		\
		{						\
			setValue( key, val, parentKey );	\
		}						\
	public:							\
		inline QString get( void ) const		\
		{						\
			return value( key, parentKey );		\
		}

#define MAP_CONFIG_INT_PROPERTY(get,set,key,parentKey)		\
	public slots:						\
		inline void set( int val )			\
		{						\
			setValue( key, QString::number( val ),	\
						parentKey );	\
		}						\
	public:							\
		inline int get( void ) const			\
		{						\
			return value( key, parentKey ).toInt();	\
		}

}

#endif
