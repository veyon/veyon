/*
 * NetworkObject.h - data class representing a network object
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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
	Q_GADGET
public:
	using Uid = QUuid;
	using Name = QString;
	using ModelId = quintptr;

	enum class Type
	{
		None,
		Root,
		Location,
		Host,
		Label,
		DesktopGroup
	} ;
	Q_ENUM(Type)

	enum class Attribute
	{
		None,
		Type,
		Name,
		HostAddress,
		MacAddress,
		DirectoryAddress,
		Uid,
		ParentUid
	};
	Q_ENUM(Attribute)

	NetworkObject( const NetworkObject& other );
	explicit NetworkObject( Type type = Type::None,
							const Name& name = {},
							const QString& hostAddress = {},
							const QString& macAddress = {},
							const QString& directoryAddress = {},
							Uid uid = {},
							Uid parentUid = {} );
	explicit NetworkObject( const QJsonObject& jsonObject );
	~NetworkObject() = default;

	NetworkObject& operator=( const NetworkObject& other );

	bool operator==( const NetworkObject& other ) const;
	bool exactMatch( const NetworkObject& other ) const;

	bool isValid() const
	{
		return type() != Type::None;
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

	void setParentUid( Uid parentUid )
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

	QVariant attributeValue( Attribute attribute ) const;

	bool isAttributeValueEqual( Attribute attribute, const QVariant& value, Qt::CaseSensitivity cs ) const;

private:
	Uid calculateUid() const;

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

using NetworkObjectList = QVector<NetworkObject>;
using NetworkObjectUidList = HashList<NetworkObject::Uid>;
