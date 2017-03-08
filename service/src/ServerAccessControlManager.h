/*
 * ServerAccessControlManager.h - header file for ServerAccessControlManager
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

#ifndef SERVER_ACCESS_CONTROL_MANAGER_H
#define SERVER_ACCESS_CONTROL_MANAGER_H

#include "RfbItalcAuth.h"
#include "DesktopAccessPermission.h"

class VariantArrayMessage;
class VncServerClient;

class ServerAccessControlManager : public QObject
{
	Q_OBJECT
public:
	ServerAccessControlManager( FeatureWorkerManager& featureWorkerManager,
								DesktopAccessDialog& desktopAccessDialog,
								QObject* parent );

	bool processClient( VncServerClient* client );


signals:
	void accessControlError( QString host, QString user );

private:
	bool performAccessControl( VncServerClient* client,
							   DesktopAccessPermission::AuthenticationMethod authenticationMethod );

	FeatureWorkerManager& m_featureWorkerManager;
	DesktopAccessDialog& m_desktopAccessDialog;

} ;

#endif
