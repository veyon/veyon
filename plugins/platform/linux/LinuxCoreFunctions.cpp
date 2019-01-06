/*
 * LinuxCoreFunctions.cpp - implementation of LinuxCoreFunctions class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <QDBusPendingCall>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QWidget>

#include <unistd.h>

#include "LinuxCoreFunctions.h"
#include "LinuxDesktopIntegration.h"
#include "LinuxUserFunctions.h"
#include "PlatformUserFunctions.h"

#include <X11/XKBlib.h>
#include <X11/extensions/dpms.h>


LinuxCoreFunctions::LinuxCoreFunctions() :
	m_screenSaverTimeout( 0 ),
	m_screenSaverPreferBlanking( 0 ),
	m_dpmsEnabled( false ),
	m_dpmsStandbyTimeout( 0 ),
	m_dpmsSuspendTimeout( 0 ),
	m_dpmsOffTimeout( 0 )
{
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
		kdeSessionManager()->asyncCall( QStringLiteral("logout"),
										static_cast<int>( LinuxDesktopIntegration::KDE::ShutdownConfirmNo ),
										static_cast<int>( LinuxDesktopIntegration::KDE::ShutdownTypeReboot ),
										static_cast<int>( LinuxDesktopIntegration::KDE::ShutdownModeForceNow ) );
		gnomeSessionManager()->asyncCall( QStringLiteral("RequestReboot") );
		mateSessionManager()->asyncCall( QStringLiteral("RequestReboot") );
		xfcePowerManager()->asyncCall( QStringLiteral("Reboot") );
		systemdLoginManager()->asyncCall( QStringLiteral("Reboot") );
		consoleKitManager()->asyncCall( QStringLiteral("Restart") );
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
		kdeSessionManager()->asyncCall( QStringLiteral("logout"),
										static_cast<int>( LinuxDesktopIntegration::KDE::ShutdownConfirmNo ),
										static_cast<int>( LinuxDesktopIntegration::KDE::ShutdownTypeHalt ),
										static_cast<int>( LinuxDesktopIntegration::KDE::ShutdownModeForceNow ) );
		gnomeSessionManager()->asyncCall( QStringLiteral("RequestShutdown") );
		mateSessionManager()->asyncCall( QStringLiteral("RequestShutdown") );
		xfcePowerManager()->asyncCall( QStringLiteral("Shutdown") );
		systemdLoginManager()->asyncCall( QStringLiteral("PowerOff") );
		consoleKitManager()->asyncCall( QStringLiteral("Stop") );
	}
}



void LinuxCoreFunctions::raiseWindow( QWidget* widget )
{
	widget->activateWindow();
	widget->raise();
}


void LinuxCoreFunctions::disableScreenSaver()
{
	auto display = XOpenDisplay( nullptr );

	// query and disable screen saver
	int interval, allowExposures;
	XGetScreenSaver( display, &m_screenSaverTimeout, &interval, &m_screenSaverPreferBlanking, &allowExposures );
	XSetScreenSaver( display, 0, interval, 0, allowExposures );

	// query and disable DPMS
	int dummy;
	if( DPMSQueryExtension( display, &dummy, &dummy ) )
	{
		CARD16 powerLevel;
		BOOL state;
		if( DPMSInfo( display, &powerLevel, &state ) && state )
		{
			m_dpmsEnabled = true;
			DPMSDisable( display );
		}
		else
		{
			m_dpmsEnabled = false;
		}

		DPMSGetTimeouts( display, &m_dpmsStandbyTimeout, &m_dpmsSuspendTimeout, &m_dpmsOffTimeout );
		DPMSSetTimeouts( display, 0, 0, 0 );
	}
	else
	{
		qWarning() << Q_FUNC_INFO << "DPMS extension not supported!";
	}

	XFlush( display );
	XCloseDisplay( display );
}



void LinuxCoreFunctions::restoreScreenSaverSettings()
{
	auto display = XOpenDisplay( nullptr );

	// restore screensaver settings
	int timeout, interval, preferBlanking, allowExposures;
	XGetScreenSaver( display, &timeout, &interval, &preferBlanking, &allowExposures );
	XSetScreenSaver( display, m_screenSaverTimeout, interval, m_screenSaverPreferBlanking, allowExposures );

	// restore DPMS settings
	int dummy;
	if( DPMSQueryExtension( display, &dummy, &dummy ) )
	{
		if( m_dpmsEnabled )
		{
			DPMSEnable( display );
		}

		DPMSSetTimeouts( display, m_dpmsStandbyTimeout, m_dpmsSuspendTimeout, m_dpmsOffTimeout );
	}

	XFlush( display );
	XCloseDisplay( display );
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

	return QProcess::execute( QStringLiteral("pkexec"), commandLine ) == 0;
}



bool LinuxCoreFunctions::runProgramAsUser( const QString& program, const QStringList& parameters,
										   const QString& username, const QString& desktop )
{
	Q_UNUSED(desktop);

	class UserProcess : public QProcess {
	public:
		UserProcess( uid_t uid ) :
			m_uid( uid )
		{
		}

		void setupChildProcess() override
		{
			if( setuid( m_uid ) != 0 )
			{
				qFatal( "Could not set UID for child process!" );
			}
		}

	private:
		const uid_t m_uid;
	};

	const auto uid = LinuxUserFunctions::userIdFromName( username );
	if( uid <= 0 )
	{
		return false;
	}

	auto process = new UserProcess( uid );
	process->connect( process, QOverload<int>::of( &QProcess::finished ), &QProcess::deleteLater );
	process->start( program, parameters );

	return true;
}



QString LinuxCoreFunctions::genericUrlHandler() const
{
	return QStringLiteral( "xdg-open" );
}



/*! Returns DBus interface for session manager of KDE desktop */
LinuxCoreFunctions::DBusInterfacePointer LinuxCoreFunctions::kdeSessionManager()
{
	return DBusInterfacePointer::create( QStringLiteral("org.kde.ksmserver"),
										 QStringLiteral("/KSMServer"),
										 QStringLiteral("org.kde.KSMServerInterface"),
										 QDBusConnection::sessionBus() );
}



/*! Returns DBus interface for session manager of Gnome desktop */
LinuxCoreFunctions::DBusInterfacePointer LinuxCoreFunctions::gnomeSessionManager()
{
	return DBusInterfacePointer::create( QStringLiteral("org.gnome.SessionManager"),
										 QStringLiteral("/org/gnome/SessionManager"),
										 QStringLiteral("org.gnome.SessionManager"),
										 QDBusConnection::sessionBus() );
}



/*! Returns DBus interface for session manager of Mate desktop */
LinuxCoreFunctions::DBusInterfacePointer LinuxCoreFunctions::mateSessionManager()
{
	return DBusInterfacePointer::create( QStringLiteral("org.mate.SessionManager"),
										 QStringLiteral("/org/mate/SessionManager"),
										 QStringLiteral("org.mate.SessionManager"),
										 QDBusConnection::sessionBus() );
}



/*! Returns DBus interface for Xfce/LXDE power manager */
LinuxCoreFunctions::DBusInterfacePointer LinuxCoreFunctions::xfcePowerManager()
{
	return DBusInterfacePointer::create( QStringLiteral("org.freedesktop.PowerManagement"),
										 QStringLiteral("/org/freedesktop/PowerManagement"),
										 QStringLiteral("org.freedesktop.PowerManagement"),
										 QDBusConnection::sessionBus() );
}



/*! Returns DBus interface for systemd login manager */
LinuxCoreFunctions::DBusInterfacePointer LinuxCoreFunctions::systemdLoginManager()
{
	return DBusInterfacePointer::create( QStringLiteral("org.freedesktop.login1"),
										 QStringLiteral("/org/freedesktop/login1"),
										 QStringLiteral("org.freedesktop.login1.Manager"),
										 QDBusConnection::systemBus() );
}



/*! Returns DBus interface for ConsoleKit manager */
LinuxCoreFunctions::DBusInterfacePointer LinuxCoreFunctions::consoleKitManager()
{
	return DBusInterfacePointer::create( QStringLiteral("org.freedesktop.ConsoleKit"),
										 QStringLiteral("/org/freedesktop/ConsoleKit/Manager"),
										 QStringLiteral("org.freedesktop.ConsoleKit.Manager"),
										 QDBusConnection::systemBus() );
}
