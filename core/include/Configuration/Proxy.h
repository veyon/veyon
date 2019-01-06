/*
 * Configuration/Proxy.h - ConfigurationProxy class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef CONFIGURATION_PROXY_H
#define CONFIGURATION_PROXY_H

#include "Configuration/Object.h"

namespace Configuration
{

class VEYON_CORE_EXPORT Proxy : public QObject
{
	Q_OBJECT
public:
	Proxy( Object* object, QObject* parent = nullptr ) :
		QObject( parent ),
		m_object( object )
	{
	}

	~Proxy() override
	{
	}

	bool hasValue( const QString& key, const QString& parentKey = QString() ) const
	{
		return m_object->hasValue( key, parentKey );
	}

	QVariant value( const QString& key, const QString& parentKey = QString() ) const
	{
		return m_object->value( key, parentKey );
	}

	void setValue( const QString& key, const QVariant& value, const QString& parentKey = QString() )
	{
		m_object->setValue( key, value, parentKey );
	}

	void removeValue( const QString &key, const QString &parentKey )
	{
		m_object->removeValue( key, parentKey );
	}

	void reloadFromStore()
	{
		m_object->reloadFromStore();
	}

private:
	Object* m_object;

} ;

}

#endif
