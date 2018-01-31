/*
 * AuthKeysListModel.cpp - implementation of AuthKeysListModel class
 *
 * Copyright (c) 2018 Tobias Junghans <tobydox@users.sf.net>
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

#include "AuthKeysListModel.h"
#include "AuthKeysManager.h"

AuthKeysListModel::AuthKeysListModel( QObject* parent ) :
	QAbstractListModel( parent ),
	m_manager( new AuthKeysManager( this ) ),
	m_keys()
{
}



AuthKeysListModel::~AuthKeysListModel()
{
	delete m_manager;
}



void AuthKeysListModel::reload()
{
	m_keys = m_manager->listKeys();
}



int AuthKeysListModel::rowCount( const QModelIndex& parent ) const
{
	Q_UNUSED(parent)

	return m_keys.size();
}



QVariant AuthKeysListModel::data( const QModelIndex& index, int role ) const
{
	if( index.isValid() == false )
	{
		return QVariant();
	}

	const auto& key = m_keys[index.row()];

	switch( role )
	{
	case Qt::DisplayRole:
		return QStringLiteral("%1 (%2)").arg( key, m_manager->assignedGroup( key ) );
	case KeyNameRole:
		return key;
	default:
		break;
	}

	return QVariant();
}
