/*
 * NetworkObject.h - data class representing a network object
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

#pragma once

#include <QUuid>
#include <QString>

#include "HashList.h"
#include "VeyonCore.h"

class VEYON_CORE_EXPORT NetworkObject
{
public:
	typedef QUuid Uid;
	typedef QString Name;
	typedef quintptr ModelId;

	typedef enum Types
	{
		None,
		Root,
		Location,
		Host,
		Label,
		TypeCount
	} Type;

	NetworkObject( const NetworkObject& other );
	NetworkObject( Type type = None,
				   const Name& name = Name(),
				   const QString& hostAddress = QString(),
				   const QString& macAddress = QString(),
				   const QString& directoryAddress = QString(),
				   Uid uid = Uid(),
				   Uid parentUid = Uid() );
	NetworkObject( const QJsonObject& jsonObject );
	~NetworkObject() = default;

	NetworkObject& operator=( const NetworkObject& other );

	bool operator==( const NetworkObject& other ) const;
	bool exactMatch( const NetworkObject& other ) const;

	bool isValid() const
	{
		return type() != None;
	}

	bool isPopulated() const
	{
		return m_populated;
	}

	void setPopulated()
	{
		m_populated = true;
	}

	const Uid& uid() const
	{
		return m_uid;
	}

	const Uid& parentUid() const
	{
		return m_parentUid;
	}

	void setParentUid( const Uid& parentUid )
	{
		m_parentUid = parentUid;
	}

	ModelId modelId() const;

	Type type() const
	{
		return m_type;
	}

	const Name& name() const
	{
		return m_name;
	}

	const QString& hostAddress() const
	{
		return m_hostAddress;
	}

	const QString& macAddress() const
	{
		return m_macAddress;
	}

	const QString& directoryAddress() const
	{
		return m_directoryAddress;
	}

	QJsonObject toJson() const;

private:
	Uid calculateUid() const;
	static ModelId calculateModelId( Uid uid );

	Type m_type;
	QString m_name;
	QString m_hostAddress;
	QString m_macAddress;
	QString m_directoryAddress;
	Uid m_uid;
	Uid m_parentUid;
	bool m_populated;

	static const QUuid networkObjectNamespace;

};

Q_DECLARE_METATYPE(NetworkObject::Type)

typedef QList<NetworkObject> NetworkObjectList;
typedef HashList<NetworkObject::Uid> NetworkObjectUidList;
