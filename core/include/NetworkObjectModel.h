/*
 * NetworkObjectModel.h - base class for data models providing grouped network objects
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef NETWORK_OBJECT_MODEL_H
#define NETWORK_OBJECT_MODEL_H

#include <QAbstractItemModel>

#include "NetworkObject.h"

class VEYON_CORE_EXPORT NetworkObjectModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	NetworkObjectModel( QObject* parent = nullptr) :
		QAbstractItemModel( parent )
	{
	}

	typedef enum Roles
	{
		CheckStateRole = Qt::CheckStateRole,
		NameRole = Qt::DisplayRole,
		UidRole = Qt::UserRole,
		TypeRole,
		HostAddressRole,
		MacAddressRole,
		DirectoryAddressRole,
		ParentUidRole
	} Role;

};

#endif // NETWORK_OBJECT_MODEL_H
