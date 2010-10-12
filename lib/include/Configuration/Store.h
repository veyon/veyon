/*
 * Configuration/Store.h - ConfigurationStore class
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

#ifndef _CONFIGURATION_STORE_H
#define _CONFIGURATION_STORE_H

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QVariant>

namespace Configuration
{

class Object;

class Store
{
public:
	enum Backends
	{
		LocalBackend,	// registry or similiar
		XmlFile
	} ;
	typedef Backends Backend;

	enum Scopes
	{
		Personal,
		Global
	} ;
	typedef Scopes Scope;


	Store( Backend _backend, Scope _scope ) :
		m_backend( _backend ),
		m_scope( _scope )
	{
	}

	inline Backend backend( void ) const
	{
		return m_backend;
	}

	inline Scope scope( void ) const
	{
		return m_scope;
	}

	virtual void load( Object * _obj ) = 0;
	virtual void flush( Object * _obj ) = 0;


private:
	const Backend m_backend;
	const Scope m_scope;

} ;

}

#endif
