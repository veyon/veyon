/*
 * LocalAccessControlDataBackend.h - implementation of AccessControlDataPluginInterface
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

#ifndef LOCAL_ACCESS_CONTROL_DATA_BACKEND_H
#define LOCAL_ACCESS_CONTROL_DATA_BACKEND_H

#include "AccessControlDataBackendInterface.h"

class ITALC_CORE_EXPORT LocalAccessControlDataBackend : public QObject, public AccessControlDataBackendInterface
{
	Q_OBJECT
public:
	QString accessControlDataBackendName() const override
	{
		return tr( "Default (local users/groups and computers/rooms from configuration)" );
	}

	QStringList users() const override;
	QStringList userGroups() const override;
	QStringList groupsOfUser( const QString& userName ) const override;
	QStringList allRooms() const override;
	QStringList roomsOfComputer( const QString& computerName ) const override;

};

#endif
