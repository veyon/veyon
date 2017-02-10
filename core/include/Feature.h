/*
 * Feature.h - declaration of the Feature class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

class Feature : public QObject
{
	Q_OBJECT
public:
	typedef QUuid Uid;
	typedef enum Types
	{
		Mode,
		Action,
		Session,
		Operation,
		TypeCount
	} Type;

	enum ScopeFlags
	{
		ScopeMaster = 0x01,
		ScopeSingleService = 0x02,
		ScopeAllServices = 0x04,
		ScopeAll = ScopeMaster | ScopeAllServices
	} ;

	Q_DECLARE_FLAGS(Scopes, ScopeFlags)
	Q_FLAG(Scopes)

	Feature( Type type, Scopes scopes, const Uid& uid,
			 const QString& name,
			 const QString& nameActive,
			 const QString& description,
			 const QIcon& icon = QIcon(),
			 const QKeySequence& shortcut = QKeySequence() ) :
		m_type( type ),
		m_scopes( scopes ),
		m_uid( uid ),
		m_name( name ),
		m_nameActive( nameActive ),
		m_description( description ),
		m_icon( icon ),
		m_shortcut( shortcut )
	{
	}

	Feature( const Feature& other ) :
		QObject(),
		m_type( other.type() ),
		m_scopes( other.scopes() ),
		m_uid( other.uid() ),
		m_name( other.name() ),
		m_description( other.description() ),
		m_icon( other.icon() ),
		m_shortcut( other.shortcut() )
	{
	}

	Type type() const
	{
		return m_type;
	}

	Scopes scopes() const
	{
		return m_scopes;
	}

	const Uid& uid() const
	{
		return m_uid;
	}

	const QString& name() const
	{
		return m_name;
	}

	const QString& nameActive() const
	{
		return m_nameActive;
	}

	const QString& description() const
	{
		return m_description;
	}

	const QIcon& icon() const
	{
		return m_icon;
	}

	const QKeySequence& shortcut() const
	{
		return m_shortcut;
	}

private:
	Type m_type;
	Scopes m_scopes;
	Uid m_uid;
	QString m_name;
	QString m_nameActive;
	QString m_description;
	QIcon m_icon;
	QKeySequence m_shortcut;

};

typedef QList<Feature> FeatureList;

#endif // FEATURE_H
