// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VeyonCore.h"

class AccessControlProvider;
class DesktopAccessDialog;
class MonitoringMode;
class SystemTrayIcon;

// clazy:excludeall=rule-of-three

class VEYON_CORE_EXPORT BuiltinFeatures
{
public:
	BuiltinFeatures();
	~BuiltinFeatures();

	SystemTrayIcon& systemTrayIcon()
	{
		return *m_systemTrayIcon;
	}

	MonitoringMode& monitoringMode()
	{
		return *m_monitoringMode;
	}

	DesktopAccessDialog& desktopAccessDialog()
	{
		return *m_desktopAccessDialog;
	}

	AccessControlProvider& accessControlProvider()
	{
		return *m_accessControlProvider;
	}

private:
	SystemTrayIcon* m_systemTrayIcon;
	MonitoringMode* m_monitoringMode;
	DesktopAccessDialog* m_desktopAccessDialog;
	AccessControlProvider* m_accessControlProvider;
};
