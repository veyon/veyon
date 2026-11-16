// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QJsonObject>

#include "DesktopServiceObject.h"


DesktopServiceObject::DesktopServiceObject(const DesktopServiceObject& other) :
	m_type(other.type()),
	m_name(other.name()),
	m_path(other.path()),
	m_uid(other.uid())
{
}



DesktopServiceObject::DesktopServiceObject(DesktopServiceObject::Type type,
										   const Name& name,
										   const QString& path,
										   Uid uid) :
	m_type(type),
	m_name(name),
	m_path(path),
	m_uid(uid)
{
	if (m_uid.isNull())
	{
		m_uid = QUuid::createUuid();
	}
}



DesktopServiceObject::DesktopServiceObject(const QJsonObject& jsonObject) :
	m_type(static_cast<Type>(jsonObject.value(QStringLiteral("Type")).toInt())),
	m_name(jsonObject.value(QStringLiteral("Name")).toString()),
	m_path(jsonObject.value(QStringLiteral("Path")).toString()),
	m_uid(jsonObject.value(QStringLiteral("Uid")).toString())
{
}



bool DesktopServiceObject::operator==(const DesktopServiceObject& other) const
{
	return uid() == other.uid();
}



QJsonObject DesktopServiceObject::toJson() const
{
	QJsonObject json;
	json[QStringLiteral("Type")] = static_cast<int>(type());
	json[QStringLiteral("Name")] = name();
	json[QStringLiteral("Path")] = path();
	json[QStringLiteral("Uid")] = uid().toString();

	return json;
}
