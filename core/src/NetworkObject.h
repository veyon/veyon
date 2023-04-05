/*
 * NetworkObject.h - data class representing a network object
 *
 * Copyright (c) 2017-2023 Tobias Junghans <tobydox@veyon.io>
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

class NetworkObjectDirectory;

class VEYON_CORE_EXPORT NetworkObject
{
	Q_GADGET
public:
	using Uid = QUuid;
	using Name = QString;
	using ModelId = quintptr;
	using Properties = QVariantMap;

	enum class Type
	{
		None,
		Root,
		Location,
		Host,
		Label,
		SubDirectory,
		DesktopGroup
	} ;
	Q_ENUM(Type)

	enum class Property
	{
		None,
		Type,
		Name,
		Uid,
		ParentUid,
		HostAddress,
		MacAddress,
		DirectoryAddress
	};
	Q_ENUM(Property)

	NetworkObject( const NetworkObject& other );
	explicit NetworkObject( NetworkObjectDirectory* directory = nullptr,
							Type type = Type::None,
							const Name& name = {},
							const Properties& properties = {},
							Uid uid = {},
							Uid parentUid = {} );
	explicit NetworkObject( const QJsonObject& jsonObject, NetworkObjectDirectory* directory = nullptr );
	~NetworkObject() = default;

	NetworkObject& operator=( const NetworkObject& other );

	bool operator==( const NetworkObject& other ) const;
	bool exactMatch( const NetworkObject& other ) const;

	NetworkObjectDirectory* directory() const
	{
		return m_directory;
	}

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

	void setParentUid( Uid parentUid );

	ModelId modelId() const;

	Type type() const
	{
		return m_type;
	}

	bool isContainer() const
	{
		return isContainer( type() );
	}

	static bool isContainer( Type type )
	{
		return type == Type::Location ||
			   type == Type::SubDirectory ||
			   type == Type::DesktopGroup;
	}

	const Name& name() const
	{
		return m_name;
	}

	const Properties& properties() const
	{
		return m_properties;
	}

	QVariant property( Property property ) const;
	static QString propertyKey( Property property );

	QJsonObject toJson() const;

	bool isPropertyValueEqual( Property property, const QVariant& value, Qt::CaseSensitivity cs ) const;

private:
	Uid calculateUid() const;

	NetworkObjectDirectory* m_directory;
	Properties m_properties;
	Type m_type;
	QString m_name;
	Uid m_uid;
	Uid m_parentUid;
	bool m_populated;
	bool m_calculatedUid{false};

	static const QUuid networkObjectNamespace;

};

Q_DECLARE_METATYPE(NetworkObject::Type)

using NetworkObjectList = QVector<NetworkObject>;
using NetworkObjectUidList = HashList<NetworkObject::Uid>;
