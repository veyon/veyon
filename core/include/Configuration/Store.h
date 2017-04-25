/*
 * Configuration/Store.h - ConfigurationStore class
 *
 * Copyright (c) 2009-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef CONFIGURATION_STORE_H
#define CONFIGURATION_STORE_H

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include "VeyonCore.h"

namespace Configuration
{

class Object;

class Store
{
public:
	enum Backends
	{
		LocalBackend,	// registry or similiar
		JsonFile,
		NoBackend
	} ;
	typedef Backends Backend;

	enum Scopes
	{
		User,		// for current user
		System,		// system-wide (service settings etc.)
		BugReportArchive
	} ;
	typedef Scopes Scope;


	Store( Backend backend, Scope scope ) :
		m_backend( backend ),
		m_scope( scope )
	{
	}

	virtual ~Store()
	{
	}

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
		switch( scope() )
		{
			case User: return "UserConfig";
			case System: return "SystemConfig";
			case BugReportArchive: return "BugReportArchive";
		}

		return QString();
	}

	virtual void load( Object *obj ) = 0;
	virtual void flush( Object *obj ) = 0;
	virtual bool isWritable() const = 0;
	virtual void clear() = 0;


private:
	const Backend m_backend;
	const Scope m_scope;

} ;

}

#endif
