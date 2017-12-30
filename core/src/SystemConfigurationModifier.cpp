/*
 * SystemConfigurationModifier.cpp - class for easy modification of Veyon-related
 *                                   settings in the operating system
 *
 * Copyright (c) 2010-2017 Tobias Junghans <tobydox@users.sf.net>
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

#include <veyonconfig.h>

#ifdef VEYON_BUILD_WIN32
#include <windows.h>
#endif

#include "FeatureWorkerManager.h"
#include "SystemConfigurationModifier.h"
#include "Filesystem.h"
#include "PlatformNetworkFunctions.h"


bool SystemConfigurationModifier::enableFirewallException( bool enabled )
{
	auto& network = VeyonCore::platform().networkFunctions();

	return network.configureFirewallException( VeyonCore::filesystem().serverFilePath(), QStringLiteral("Veyon Server"), enabled ) &&
			network.configureFirewallException( VeyonCore::filesystem().workerFilePath(), QStringLiteral("Veyon Worker"), enabled );
}



bool SystemConfigurationModifier::enableSoftwareSAS( bool enabled )
{
#ifdef VEYON_BUILD_WIN32
	HKEY hkLocal, hkLocalKey;
	DWORD dw;
	if( RegCreateKeyEx( HKEY_LOCAL_MACHINE,
						L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies",
						0, REG_NONE, REG_OPTION_NON_VOLATILE,
						KEY_READ, NULL, &hkLocal, &dw ) != ERROR_SUCCESS)
	{
		return false;
	}

	if( RegOpenKeyEx( hkLocal,
					  L"System",
					  0, KEY_WRITE | KEY_READ,
					  &hkLocalKey ) != ERROR_SUCCESS )
	{
		RegCloseKey( hkLocal );
		return false;
	}

	LONG pref = enabled ? 1 : 0;
	RegSetValueEx( hkLocalKey, L"SoftwareSASGeneration", 0, REG_DWORD, (LPBYTE) &pref, sizeof(pref) );
	RegCloseKey( hkLocalKey );
	RegCloseKey( hkLocal );
#endif

	return true;
}
