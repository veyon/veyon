/*
 * Proxy.cpp - implementation of Configuration::Proxy
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "Configuration/Proxy.h"


namespace Configuration
{

Proxy::Proxy(Object *object, QObject *parent) :
	QObject( parent ),
	m_object( object ),
	m_instanceId()
{
}



bool Proxy::hasValue( const QString& key, const QString& parentKey ) const
{
	return m_object->hasValue( key, instanceParentKey( parentKey ) );
}



QVariant Proxy::value( const QString& key, const QString& parentKey, const QVariant& defaultValue ) const
{
	return m_object->value( key, instanceParentKey( parentKey ), defaultValue );
}



void Proxy::setValue( const QString& key, const QVariant& value, const QString& parentKey )
{
	m_object->setValue( key, value, instanceParentKey( parentKey ) );
}



void Proxy::removeValue( const QString& key, const QString& parentKey )
{
	m_object->removeValue( key, instanceParentKey( parentKey ) );
}



void Proxy::reloadFromStore()
{
	m_object->reloadFromStore();
}



void Proxy::flushStore()
{
	m_object->flushStore();
}



void Proxy::removeInstance( const QString& parentKey )
{
	m_object->removeValue( instanceId(), parentKey );
}



QString Proxy::instanceParentKey( const QString& parentKey ) const
{
	if( m_instanceId.isEmpty() )
	{
		return parentKey;
	}

	return parentKey + QLatin1Char('/') + m_instanceId;
}

}
