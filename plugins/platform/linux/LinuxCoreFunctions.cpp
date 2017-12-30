/*
 * LinuxCoreFunctions.cpp - implementation of LinuxCoreFunctions class
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

#include <QDir>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QWidget>

#include <unistd.h>

#include "LinuxCoreFunctions.h"
#include "PlatformUserFunctions.h"


QString LinuxCoreFunctions::personalAppDataPath() const
{
	return QDir::homePath() + QDir::separator() + QStringLiteral(".veyon");
}



QString LinuxCoreFunctions::globalAppDataPath() const
{
	return QStringLiteral( "/etc/veyon/" );
}



void LinuxCoreFunctions::initNativeLoggingSystem( const QString& appName )
{
	Q_UNUSED(appName)
}



void LinuxCoreFunctions::writeToNativeLoggingSystem( const QString& message, Logger::LogLevel loglevel )
{
	Q_UNUSED(message)
	Q_UNUSED(loglevel)
}



void LinuxCoreFunctions::reboot()
{
	if( isRunningAsAdmin() )
	{
		QProcess::startDetached( QStringLiteral("reboot") );
	}
	else
	{
		// Gnome reboot
		QProcess::startDetached( QStringLiteral("dbus-send --session --dest=org.gnome.SessionManager --type=method_call /org/gnome/SessionManager org.gnome.SessionManager.RequestReboot") );
		// KDE 4 reboot
		QProcess::startDetached( QStringLiteral("qdbus org.kde.ksmserver /KSMServer logout 0 1 0") );
		// KDE 5 reboot
		QProcess::startDetached( QStringLiteral("dbus-send --dest=org.kde.ksmserver /KSMServer org.kde.KSMServerInterface.logout int32:1 int32:1 int32:1") );
		// Xfce reboot
		QProcess::startDetached( QStringLiteral("xfce4-session-logout --reboot") );
		// generic reboot via consolekit
		QProcess::startDetached( QStringLiteral("dbus-send --system --dest=org.freedesktop.ConsoleKit /org/freedesktop/ConsoleKit/Manager org.freedesktop.ConsoleKit.Manager.Restart") );
	}
}



void LinuxCoreFunctions::powerDown()
{
	if( isRunningAsAdmin() )
	{
		QProcess::startDetached( QStringLiteral("poweroff") );
	}
	else
	{
		// Gnome shutdown
		QProcess::startDetached( QStringLiteral("dbus-send --session --dest=org.gnome.SessionManager --type=method_call /org/gnome/SessionManager org.gnome.SessionManager.RequestShutdown") );
		// KDE 4 shutdown
		QProcess::startDetached( QStringLiteral("qdbus org.kde.ksmserver /KSMServer logout 0 2 0") );
		// KDE 5 shutdown
		QProcess::startDetached( QStringLiteral("dbus-send --dest=org.kde.ksmserver /KSMServer org.kde.KSMServerInterface.logout int32:0 int32:2 int32:2") );
		// Xfce shutdown
		QProcess::startDetached( QStringLiteral("xfce4-session-logout --halt") );
		// generic shutdown via consolekit
		QProcess::startDetached( QStringLiteral("dbus-send --system --dest=org.freedesktop.ConsoleKit /org/freedesktop/ConsoleKit/Manager org.freedesktop.ConsoleKit.Manager.Stop") );
	}
}



void LinuxCoreFunctions::raiseWindow( QWidget* widget )
{
	widget->activateWindow();
	widget->raise();
}


void LinuxCoreFunctions::disableScreenSaver()
{
	// TODO
}



void LinuxCoreFunctions::restoreScreenSaverSettings()
{
	// TODO
}



QString LinuxCoreFunctions::activeDesktopName()
{
	return QString();
}



bool LinuxCoreFunctions::isRunningAsAdmin() const
{
	return getuid() == 0 || geteuid() == 0;
}



bool LinuxCoreFunctions::runProgramAsAdmin( const QString& program, const QStringList& parameters )
{
	const auto commandLine = QStringList( program ) + parameters;

	const auto desktop = QProcessEnvironment::systemEnvironment().value( QStringLiteral("XDG_CURRENT_DESKTOP") );
	if( desktop == QStringLiteral("KDE") &&
	        QStandardPaths::findExecutable( QStringLiteral("kdesudo") ).isEmpty() == false )
	{
		return QProcess::execute( QStringLiteral("kdesudo"), commandLine ) == 0;
	}

	if( QStandardPaths::findExecutable( QStringLiteral("gksudo") ).isEmpty() == false )
	{
		return QProcess::execute( QStringLiteral("gksudo"), commandLine ) == 0;
	}

	return QProcess::execute( QStringLiteral("pkexec"), commandLine );
}



bool LinuxCoreFunctions::runProgramAsUser( const QString& program, const QString& username, const QString& desktop )
{
	Q_UNUSED(username);
	Q_UNUSED(desktop);

	return QProcess::startDetached( program );
}
