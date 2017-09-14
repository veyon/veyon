/*
 * Feature.h - declaration of the Feature class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#ifndef FEATURE_H
#define FEATURE_H

#include <QIcon>
#include <QKeySequence>
#include <QString>
#include <QUuid>

#include "VeyonCore.h"

class VEYON_CORE_EXPORT Feature
{
	Q_GADGET
public:
	typedef QUuid Uid;

	enum FeatureFlag
	{
		NoFlags,
		Mode = 0x0001,
		Action = 0x0002,
		Session = 0x0004,
		Operation = 0x0008,
		Dialog = 0x0010,
		Master = 0x0100,
		Service = 0x0200,
		Worker = 0x0400,
		Builtin = 0x1000,
		AllComponents = Master | Service | Worker
	} ;

	Q_DECLARE_FLAGS(Flags, FeatureFlag)
	Q_FLAG(Flags)

	Feature( Flags flags,
			 Uid uid,
			 const QString& displayName,
			 const QString& displayNameActive,
			 const QString& description,
			 const QString& iconUrl = QString(),
			 const QKeySequence& shortcut = QKeySequence() ) :
		m_flags( flags ),
		m_uid( uid ),
		m_displayName( displayName ),
		m_displayNameActive( displayNameActive ),
		m_description( description ),
		m_iconUrl( iconUrl ),
		m_shortcut( shortcut )
	{
	}

	Feature( Uid uid = Uid() ) :
		m_flags( NoFlags ),
		m_uid( uid ),
		m_displayName(),
		m_displayNameActive(),
		m_description(),
		m_iconUrl(),
		m_shortcut()
	{
	}

	Feature( const Feature& other ) :
		m_flags( other.flags() ),
		m_uid( other.uid() ),
		m_displayName( other.displayName() ),
		m_displayNameActive( other.displayNameActive() ),
		m_description( other.description() ),
		m_iconUrl( other.iconUrl() ),
		m_shortcut( other.shortcut() )
	{
	}

	~Feature() {}

	Feature& operator=( const Feature& other )
	{
		m_flags = other.flags();
		m_uid = other.uid();
		m_displayName = other.displayName();
		m_displayNameActive = other.displayNameActive();
		m_description = other.description();
		m_iconUrl = other.iconUrl();
		m_shortcut = other.shortcut();

		return *this;
	}

	bool operator==( const Feature& other ) const
	{
		return other.uid() == uid();
	}

	bool operator!=( const Feature& other ) const
	{
		return other.uid() != uid();
	}

	bool isValid() const
	{
		return m_flags != NoFlags;
	}

	bool testFlag( FeatureFlag flag ) const
	{
		return m_flags.testFlag( flag );
	}

	const Uid& uid() const
	{
		return m_uid;
	}

	const QString& displayName() const
	{
		return m_displayName;
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

	const QKeySequence& shortcut() const
	{
		return m_shortcut;
	}

private:
	Flags flags() const
	{
		return m_flags;
	}

	Flags m_flags;
	Uid m_uid;
	QString m_displayName;
	QString m_displayNameActive;
	QString m_description;
	QString m_iconUrl;
	QKeySequence m_shortcut;

};

Q_DECLARE_OPERATORS_FOR_FLAGS(Feature::Flags)

typedef QList<Feature> FeatureList;
typedef QStringList FeatureUidList;

#endif // FEATURE_H
