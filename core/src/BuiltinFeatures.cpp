/*
 * BuiltinFeatures.cpp - implementation of BuiltinFeatures class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#include "BuiltinFeatures.h"
#include "FeatureControl.h"
#include "MonitoringMode.h"
#include "PluginManager.h"
#include "SystemTrayIcon.h"
#include "UserSessionControl.h"
#include "DesktopAccessDialog.h"


BuiltinFeatures::BuiltinFeatures() :
	m_featureControl( new FeatureControl ),
	m_systemTrayIcon( new SystemTrayIcon ),
	m_monitoringMode( new MonitoringMode ),
	m_desktopAccessDialog( new DesktopAccessDialog ),
	m_userSessionControl( new UserSessionControl )
{
	VeyonCore::pluginManager().registerExtraPluginInterface( m_featureControl );
	VeyonCore::pluginManager().registerExtraPluginInterface( m_systemTrayIcon );
	VeyonCore::pluginManager().registerExtraPluginInterface( m_monitoringMode );
	VeyonCore::pluginManager().registerExtraPluginInterface( m_desktopAccessDialog );
	VeyonCore::pluginManager().registerExtraPluginInterface( m_userSessionControl );
}



BuiltinFeatures::~BuiltinFeatures()
{
	delete m_userSessionControl;
	delete m_systemTrayIcon;
	delete m_monitoringMode;
	delete m_desktopAccessDialog;
	delete m_featureControl;
}
