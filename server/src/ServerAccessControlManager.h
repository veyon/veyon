/*
 * ServerAccessControlManager.h - header file for ServerAccessControlManager
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
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

#pragma once

#include "DesktopAccessDialog.h"
#include "RfbVeyonAuth.h"
#include "VncServerClient.h"

class VariantArrayMessage;

class ServerAccessControlManager : public QObject
{
	Q_OBJECT
public:
	ServerAccessControlManager( FeatureWorkerManager& featureWorkerManager,
								DesktopAccessDialog& desktopAccessDialog,
								QObject* parent );

	void addClient( VncServerClient* client );
	void removeClient( VncServerClient* client );


Q_SIGNALS:
	void finished( VncServerClient* client );

private:
	static constexpr int ClientWaitInterval = 1000;

	void performAccessControl( VncServerClient* client );
	VncServerClient::AccessControlState confirmDesktopAccess( VncServerClient* client );
	void finishDesktopAccessConfirmation( VncServerClient* client );

	QStringList connectedUsers() const;

	FeatureWorkerManager& m_featureWorkerManager;
	DesktopAccessDialog& m_desktopAccessDialog;

	VncServerClientList m_clients;

	using HostUserPair = QPair<QString, QString>;
	using DesktopAccessChoiceMap = QMap<HostUserPair, DesktopAccessDialog::Choice>;

	DesktopAccessChoiceMap m_desktopAccessChoices;

} ;
