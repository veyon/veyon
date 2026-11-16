// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QIcon>
#include <QKeySequence>
#include <QString>
#include <QUuid>

#include "VeyonCore.h"

class VEYON_CORE_EXPORT Feature
{
	Q_GADGET
public:
	using Uid = QUuid;

	enum class Flag {
		None = 0x0000,
		Mode = 0x0001,
		Action = 0x0002,
		Session = 0x0004,
		Meta = 0x0008,
		Option = 0x0010,
		Checked = 0x0020,
		Master = 0x0100,
		Service = 0x0200,
		Worker = 0x0400,
		Builtin = 0x1000,
		AllComponents = Master | Service | Worker
	};

	Q_DECLARE_FLAGS(Flags, Flag)
	Q_FLAG(Flags)

	explicit Feature(const QString& name,
					 Flags flags,
					 Uid uid,
					 Uid parentUid,
					 const QString& displayName,
					 const QString& displayNameActive,
					 const QString& description,
					 const QString& iconUrl = {},
					 const QKeySequence& shortcut = {}) :
		m_name(name),
		m_flags(flags),
		m_uid(uid),
		m_parentUid(parentUid),
		m_displayName(displayName),
		m_displayNameActive(displayNameActive),
		m_description(description),
		m_iconUrl(iconUrl),
		m_shortcut(shortcut)
	{
	}

	explicit Feature(Uid uid = Uid()) :
		m_name(),
		m_flags(Flag::None),
		m_uid(uid),
		m_parentUid(QUuid()),
		m_displayName(),
		m_displayNameActive(),
		m_description(),
		m_iconUrl(),
		m_shortcut()
	{
	}

	Feature(const Feature& other) :
		m_name(other.name()),
		m_flags(other.flags()),
		m_uid(other.uid()),
		m_parentUid(other.parentUid()),
		m_displayName(other.displayName()),
		m_displayNameActive(other.displayNameActive()),
		m_description(other.description()),
		m_iconUrl(other.iconUrl()),
		m_shortcut(other.shortcut())
	{
	}

	~Feature() = default;

	Feature& operator=(const Feature& other)
	{
		m_name = other.name();
		m_flags = other.flags();
		m_uid = other.uid();
		m_parentUid = other.parentUid();
		m_displayName = other.displayName();
		m_displayNameActive = other.displayNameActive();
		m_description = other.description();
		m_iconUrl = other.iconUrl();
		m_shortcut = other.shortcut();

		return *this;
	}

	bool operator==(const Feature& other) const
	{
		return other.uid() == uid();
	}

	bool operator!=(const Feature& other) const
	{
		return other.uid() != uid();
	}

	bool isValid() const
	{
		return m_flags != QFlags<Flag>::Int(Flag::None);
	}

	bool testFlag(Flag flag) const
	{
		return m_flags.testFlag(flag);
	}

	const QString& name() const
	{
		return m_name;
	}

	const Uid& uid() const
	{
		return m_uid;
	}

	const Uid& parentUid() const
	{
		return m_parentUid;
	}

	void setParentUid(Uid uid)
	{
		m_parentUid = uid;
	}

	const QString& displayName() const
	{
		return m_displayName;
	}

	void setDisplayName(const QString& displayName)
	{
		m_displayName = displayName;
	}

	const QString& displayNameActive() const
	{
		return m_displayNameActive;
	}

	const QString& description() const
	{
		return m_description;
	}

	const QString& iconUrl() const
	{
		return m_iconUrl;
	}

	void setIconUrl(const QString& iconUrl)
	{
		m_iconUrl = iconUrl;
	}

	const QKeySequence& shortcut() const
	{
		return m_shortcut;
	}

private:
	Flags flags() const
	{
		return m_flags;
	}

	QString m_name;
	Flags m_flags;
	Uid m_uid;
	Uid m_parentUid;
	QString m_displayName;
	QString m_displayNameActive;
	QString m_description;
	QString m_iconUrl;
	QKeySequence m_shortcut;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Feature::Flags)

using FeatureList = QList<Feature>;
using FeatureUidList = QList<Feature::Uid>;
