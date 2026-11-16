// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QMetaType>
#include <QString>
#include <QUuid>

class QJsonObject;

class DesktopServiceObject
{
public:
	using Uid = QUuid;
	using Name = QString;

	enum class Type {
		None,
		Application,
		Website,
	};

	DesktopServiceObject(const DesktopServiceObject& other);
	explicit DesktopServiceObject(Type type = Type::None,
								  const Name& name = {},
								  const QString& path = {},
								  Uid uid = Uid());
	explicit DesktopServiceObject(const QJsonObject& jsonObject);

	bool operator==(const DesktopServiceObject& other) const;

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
