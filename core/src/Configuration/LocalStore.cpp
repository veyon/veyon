/*
 * ConfigurationLocalStore.cpp - implementation of LocalStore
 *
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QtCore/QSettings>
#include <QtCore/QStringList>

#include "Configuration/LocalStore.h"
#include "Configuration/Object.h"
#include "Logger.h"


namespace Configuration
{

LocalStore::LocalStore( Scope _scope ) :
	Store( Store::LocalBackend, _scope )
{
}




static void loadSettingsTree( Object *obj, QSettings *s,
								const QString &parentKey )
{
	foreach( const QString &g, s->childGroups() )
	{
		const QString subParentKey = parentKey +
									( parentKey.isEmpty() ? "" : "/" ) + g;
		s->beginGroup( g );
		loadSettingsTree( obj, s, subParentKey );
		s->endGroup();
	}

	foreach( const QString &k, s->childKeys() )
	{
		obj->setValue( k, s->value( k ).toString(), parentKey );
	}
}



void LocalStore::load( Object *obj )
{
	QSettings *s = createSettingsObject();
	loadSettingsTree( obj, s, QString() );
	delete s;
}




static void saveSettingsTree( const Object::DataMap &dataMap,
								QSettings *s )
{
	for( Object::DataMap::ConstIterator it = dataMap.begin();
						it != dataMap.end(); ++it )
	{
		if( it.value().type() == QVariant::Map )
		{
			s->beginGroup( it.key() );
			saveSettingsTree( it.value().toMap(), s );
			s->endGroup();
		}
		else if( it.value().type() == QVariant::String )
		{
			s->setValue( it.key(), it.value().toString() );
		}
	}
}



void LocalStore::flush( Object *obj )
{
	QSettings *s = createSettingsObject();
	saveSettingsTree( obj->data(), s );
	delete s;
}



bool LocalStore::isWritable() const
{
	QSettings *s = createSettingsObject();
	bool ret = s->isWritable();
	delete s;

	return ret;
}



QSettings *LocalStore::createSettingsObject() const
{
	return new QSettings( scope() == System ?
							QSettings::SystemScope : QSettings::UserScope,
						QSettings().organizationName(),
						QSettings().applicationName() );
}


}

