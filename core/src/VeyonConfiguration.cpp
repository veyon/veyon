// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDir>

#include "Logger.h"
#include "NetworkObjectDirectory.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"
#include "Configuration/Metadata.h"


VeyonConfiguration::VeyonConfiguration() :
	Configuration::Object(Configuration::Store::Backend::Local,
						  Configuration::Store::Scope::System)
{
	Configuration::Metadata::initialize(*this);
}



VeyonConfiguration::VeyonConfiguration(Configuration::Store* store) :
	Configuration::Object(store)
{
	Configuration::Metadata::initialize(*this);
}



void VeyonConfiguration::upgrade()
{
	if (applicationVersion() < VeyonCore::ApplicationVersion::Version_4_2)
	{
		setAutoSelectCurrentLocation(legacyAutoSwitchToCurrentRoom());
		setShowCurrentLocationOnly(legacyOnlyCurrentRoomVisible());
		setAllowAddingHiddenLocations(legacyManualRoomAdditionAllowed());
		setHideEmptyLocations(legacyEmptyRoomsHidden());
		setAutoOpenComputerSelectPanel(legacyOpenComputerManagementAtStart());
		setConfirmUnsafeActions(legacyConfirmDangerousActions());

		setApplicationVersion(VeyonCore::ApplicationVersion::Version_4_2);
	}
	if (applicationVersion() < VeyonCore::ApplicationVersion::Version_4_5)
	{
		setVeyonServerPort(legacyPrimaryServicePort());
		setHideLocalComputer(legacyLocalComputerHidden());
		setHideComputerFilter(legacyComputerFilterHidden());
		setAutoAdjustMonitoringIconSize(legacyAutoAdjustGridSize());

		setApplicationVersion(VeyonCore::ApplicationVersion::Version_4_5);
	}
	if (applicationVersion() < VeyonCore::ApplicationVersion::Version_4_7)
	{
		setApplicationVersion(VeyonCore::ApplicationVersion::Version_4_7);
	}
	if (applicationVersion() < VeyonCore::ApplicationVersion::Version_4_8)
	{
		setApplicationVersion(VeyonCore::ApplicationVersion::Version_4_8);
	}
	if (applicationVersion() < VeyonCore::ApplicationVersion::Version_4_9)
	{
		setUserGroupsBackend(legacyAccessControlUserGroupsBackend());
		setUseDomainUserGroups(legacyDomainGroupsForAccessControlEnabled());
		setApplicationVersion(VeyonCore::ApplicationVersion::Version_4_9);
	}
	if (applicationVersion() < VeyonCore::ApplicationVersion::Version_4_10)
	{
		setApplicationVersion(VeyonCore::ApplicationVersion::Version_4_10);
	}
	if (applicationVersion() < VeyonCore::ApplicationVersion::Version_4_11)
	{
		setApplicationVersion(VeyonCore::ApplicationVersion::Version_4_11);
	}
}
