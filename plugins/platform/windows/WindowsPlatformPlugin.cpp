// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QGuiApplication>
#include <QWindow>

#include "WindowsPlatformPlugin.h"
#include "WindowsPlatformConfigurationPage.h"

WindowsPlatformPlugin::WindowsPlatformPlugin(QObject* parent) :
	QObject(parent)
{
	if (qobject_cast<QGuiApplication*>(QCoreApplication::instance()))
	{
		// create invisible dummy window to make the Qt Windows platform plugin receive
		// WM_DISPLAYCHANGE events and update the screens returned by QGuiApplication::screens()
		// even if the current Veyon component such as the Veyon Server does not create a window
		m_dummyWindow = new QWindow;
		m_dummyWindow->create();
	}
}



WindowsPlatformPlugin::~WindowsPlatformPlugin()
{
	delete m_dummyWindow;
}



ConfigurationPage* WindowsPlatformPlugin::createConfigurationPage()
{
	return new WindowsPlatformConfigurationPage();
}


IMPLEMENT_CONFIG_PROXY(WindowsPlatformConfiguration)
