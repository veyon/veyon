/*
 * DesktopServiceObject.h - data class representing a desktop service object
 *
 * Copyright (c) 2018-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef DESKTOP_SERVICE_OBJECT_H
#define DESKTOP_SERVICE_OBJECT_H

#include <QUuid>
#include <QString>

class QJsonObject;

class DesktopServiceObject
{
public:
	typedef QUuid Uid;
	typedef QString Name;

	typedef enum Types
	{
		None,
		Program,
		Website,
		TypeCount
	} Type;

	DesktopServiceObject( const DesktopServiceObject& other );
	DesktopServiceObject( Type type = None,
						  const Name& name = QString(),
						  const QString& path = QString(),
						  Uid uid = Uid() );
	DesktopServiceObject( const QJsonObject& jsonObject );

	bool operator==( const DesktopServiceObject& other ) const;

	const Uid& uid() const
	{
		return m_uid;
	}

	Uid parentUid() const
	{
		return Uid();
	}

	Type type() const
	{
		return m_type;
	}

	const Name& name() const
	{
		return m_name;
	}

	const QString& path() const
	{
		return m_path;
	}

	QJsonObject toJson() const;

private:
	Type m_type;
	QString m_name;
	QString m_path;
	Uid m_uid;

};

Q_DECLARE_METATYPE(DesktopServiceObject::Type)

#endif // DESKTOP_SERVICE_OBJECT_H
