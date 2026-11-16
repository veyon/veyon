// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QJsonObject>
#include <QString>
#include <QUuid>

#include "HashList.h"
#include "VeyonCore.h"

class VEYON_CORE_EXPORT NetworkObject
{
	Q_GADGET
public:
	using Uid = QUuid;
	using Name = QString;
	using ModelId = quintptr;

	enum class Type {
		None,
		Root,
		Location,
		Host,
		Label,
		DesktopGroup
	};
	Q_ENUM(Type)

	enum class Attribute {
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

	NetworkObject(const NetworkObject& other);
	explicit NetworkObject(Type type = Type::None,
						   const Name& name = {},
						   const QString& hostAddress = {},
						   const QString& macAddress = {},
						   const QString& directoryAddress = {},
						   Uid uid = {},
						   Uid parentUid = {});
	explicit NetworkObject(const QJsonObject& jsonObject);
	~NetworkObject() = default;

	NetworkObject& operator=(const NetworkObject& other);

	bool operator==(const NetworkObject& other) const;
	bool exactMatch(const NetworkObject& other) const;

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

	void setParentUid(Uid parentUid)
	{
		m_parentUid = parentUid;
	}

	ModelId modelId() const;

	Type type() const
	{
		return m_type;
	}

	bool isContainer() const
	{
		return isContainer(type());
	}

	static bool isContainer(Type type)
	{
		return type == Type::Location ||
			type == Type::DesktopGroup;
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

	QVariant attributeValue(Attribute attribute) const;

	bool isAttributeValueEqual(Attribute attribute, const QVariant& value, Qt::CaseSensitivity cs) const;

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
