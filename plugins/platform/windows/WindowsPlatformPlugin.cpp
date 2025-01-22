/*
 * WindowsPlatformPlugin.cpp - implementation of WindowsPlatformPlugin class
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
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

#include <QGuiApplication>
#include <QWindow>

#include "WindowsPlatformPlugin.h"
#include "WindowsPlatformConfigurationPage.h"

WindowsPlatformPlugin::WindowsPlatformPlugin( QObject* parent ) :
	QObject( parent )
{
	if( qobject_cast<QGuiApplication *>(QCoreApplication::instance()) )
	{
		// create invisible dummy window to make the Qt Windows platform plugin receive
		// WM_DISPLAYCHANGE events and update the screens returned by QGuiApplication::screens()
		// even if the current Veyon component such as the Veyon Server does not create a window
		m_dummyWindow = new QWindow;
		m_dummyWindow->create();
	}
}



ConfigurationPage* WindowsPlatformPlugin::createConfigurationPage()
{
	return new WindowsPlatformConfigurationPage();
}


IMPLEMENT_CONFIG_PROXY(WindowsPlatformConfiguration)
