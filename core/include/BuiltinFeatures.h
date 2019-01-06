/*
 * BuiltinFeatures.h - declaration of BuiltinFeatures class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "VeyonCore.h"

class DesktopAccessDialog;
class FeatureControl;
class MonitoringMode;
class SystemTrayIcon;
class UserSessionControl;

// clazy:excludeall=rule-of-three

class VEYON_CORE_EXPORT BuiltinFeatures
{
public:
	BuiltinFeatures();
	~BuiltinFeatures();

	FeatureControl& featureControl()
	{
		return *m_featureControl;
	}

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

	UserSessionControl& userSessionControl()
	{
		return *m_userSessionControl;
	}

private:
	FeatureControl* m_featureControl;
	SystemTrayIcon* m_systemTrayIcon;
	MonitoringMode* m_monitoringMode;
	DesktopAccessDialog* m_desktopAccessDialog;
	UserSessionControl* m_userSessionControl;
};
