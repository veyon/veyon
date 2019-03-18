/*
 * VeyonConfiguration.cpp - a Configuration object storing system wide
 *                          configuration values
 *
 * Copyright (c) 2010-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <QDir>

#include "VeyonConfiguration.h"
#include "VeyonCore.h"
#include "Logger.h"
#include "NetworkObjectDirectory.h"


VeyonConfiguration::VeyonConfiguration() :
	Configuration::Object( Configuration::Store::LocalBackend,
						   Configuration::Store::System )
{
}



VeyonConfiguration::VeyonConfiguration( Configuration::Store* store ) :
	Configuration::Object( store )
{
}



void VeyonConfiguration::upgrade()
{
	if( applicationVersion() < VeyonCore::ApplicationVersion::Version_4_2 )
	{
		setAutoSelectCurrentLocation( legacyAutoSwitchToCurrentRoom() );
		setShowCurrentLocationOnly( legacyOnlyCurrentRoomVisible() );
		setAllowAddingHiddenLocations( legacyManualRoomAdditionAllowed() );
		setHideEmptyLocations( legacyEmptyRoomsHidden() );
		setAutoOpenComputerSelectPanel( legacyOpenComputerManagementAtStart() );
		setConfirmUnsafeActions( legacyConfirmDangerousActions() );

		setApplicationVersion( VeyonCore::ApplicationVersion::Version_4_2 );
	}
}
