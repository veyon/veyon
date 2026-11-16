// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AccessControlProvider.h"
#include "BuiltinFeatures.h"
#include "DesktopAccessDialog.h"
#include "MonitoringMode.h"
#include "PluginManager.h"
#include "SystemTrayIcon.h"


BuiltinFeatures::BuiltinFeatures() :
	m_systemTrayIcon(new SystemTrayIcon),
	m_monitoringMode(new MonitoringMode),
	m_desktopAccessDialog(new DesktopAccessDialog),
	m_accessControlProvider(new AccessControlProvider)
{
	VeyonCore::pluginManager().registerExtraPluginInterface(m_systemTrayIcon);
	VeyonCore::pluginManager().registerExtraPluginInterface(m_monitoringMode);
	VeyonCore::pluginManager().registerExtraPluginInterface(m_desktopAccessDialog);
	VeyonCore::pluginManager().registerExtraPluginInterface(m_accessControlProvider);
}



BuiltinFeatures::~BuiltinFeatures()
{
	delete m_systemTrayIcon;
	delete m_monitoringMode;
	delete m_desktopAccessDialog;
	delete m_accessControlProvider;
}
