/*
 * LinuxCoreFunctions.cpp - implementation of LinuxCoreFunctions class
 *
 * Copyright (c) 2017-2022 Tobias Junghans <tobydox@veyon.io>
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

#include <QElapsedTimer>
#include <QFileInfo>
#include <QDBusPendingCall>
#include <QProcess>
#include <QProcessEnvironment>
#include <QScreen>
#include <QStandardPaths>
#include <QWidget>

#include <unistd.h>
#include <grp.h>
#include <proc/readproc.h>

#include "LinuxCoreFunctions.h"
#include "LinuxDesktopIntegration.h"
#include "LinuxUserFunctions.h"
#include "PlatformUserFunctions.h"

#include <X11/XKBlib.h>
#include <X11/extensions/dpms.h>


bool LinuxCoreFunctions::applyConfiguration()
{
	return true;
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
	if( systemdLoginManager()->call( QStringLiteral("Reboot"), false ).type() != QDBusMessage::ReplyMessage &&
		consoleKitManager()->call( QStringLiteral("Restart") ).type() != QDBusMessage::ReplyMessage )
	{
		prepareSessionBusAccess();

		kdeSessionManager()->asyncCall( QStringLiteral("logout"),
										LinuxDesktopIntegration::KDE::ShutdownConfirmNo,
										LinuxDesktopIntegration::KDE::ShutdownTypeReboot,
										LinuxDesktopIntegration::KDE::ShutdownModeForceNow );
		gnomeSessionManager()->asyncCall( QStringLiteral("RequestReboot") );
		mateSessionManager()->asyncCall( QStringLiteral("RequestReboot") );
		xfcePowerManager()->asyncCall( QStringLiteral("Reboot") );
	}
}



void LinuxCoreFunctions::powerDown( bool installUpdates )
{
	Q_UNUSED(installUpdates)

	if( systemdLoginManager()->call( QStringLiteral("PowerOff"), false ).type() != QDBusMessage::ReplyMessage &&
		consoleKitManager()->call( QStringLiteral("Stop") ).type() != QDBusMessage::ReplyMessage )
	{
		prepareSessionBusAccess();

		kdeSessionManager()->asyncCall( QStringLiteral("logout"),
										LinuxDesktopIntegration::KDE::ShutdownConfirmNo,
										LinuxDesktopIntegration::KDE::ShutdownTypeHalt,
										LinuxDesktopIntegration::KDE::ShutdownModeForceNow );
		gnomeSessionManager()->asyncCall( QStringLiteral("RequestShutdown") );
		mateSessionManager()->asyncCall( QStringLiteral("RequestShutdown") );
		xfcePowerManager()->asyncCall( QStringLiteral("Shutdown") );
	}
}



void LinuxCoreFunctions::raiseWindow( QWidget* widget, bool stayOnTop )
{
	widget->activateWindow();
	widget->raise();

	if( stayOnTop )
	{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
		widget->setWindowFlag( Qt::WindowStaysOnTopHint, true );
#else
		widget->setWindowFlags( widget->windowFlags() | Qt::WindowStaysOnTopHint );
#endif
	}
}


void LinuxCoreFunctions::disableScreenSaver()
{
	auto display = XOpenDisplay( nullptr );

	// query and disable screen saver
	int interval;
	int allowExposures;
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
	else if( qEnvironmentVariableIsSet("XRDP_SESSION") == false )
	{
		vWarning() << "DPMS extension not supported!";
	}

	XFlush( display );
	XCloseDisplay( display );
}



void LinuxCoreFunctions::restoreScreenSaverSettings()
{
	auto display = XOpenDisplay( nullptr );

	// restore screensaver settings
	int timeout;
	int interval;
	int preferBlanking;
	int allowExposures;
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



void LinuxCoreFunctions::setSystemUiState( bool enabled )
{
	Q_UNUSED(enabled)
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

	return QProcess::execute( QStringLiteral("pkexec"), commandLine ) == 0;
}



bool LinuxCoreFunctions::runProgramAsUser( const QString& program, const QStringList& parameters,
										   const QString& username, const QString& desktop )
{
	Q_UNUSED(desktop);

	const auto uid = LinuxUserFunctions::userIdFromName( username );
	if( uid < 0 )
	{
		vCritical() << "failed to resolve uid from username" << username;
		return false;
	}

	const auto gid = LinuxUserFunctions::userGroupIdFromName( username );
	if( gid < 0 )
	{
		vCritical() << "failed to resolve gid from username" << username;
		return false;
	}

	const auto adjustChildProcessPrivileges = [uid, gid]()
	{
		const auto isRoot = getuid() == 0 || geteuid() == 0;
		if (setgroups(0, nullptr) != 0 && isRoot)
		{
			qFatal( "Could not drop all supplementary groups for child process!" );
		}
		if (setgid(gid) != 0 && isRoot)
		{
			qFatal( "Could not set GID for child process!" );
		}
		if (setuid(uid) != 0 && isRoot)
		{
			qFatal( "Could not set UID for child process!" );
		}
	};

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	auto process = new QProcess;
	process->setChildProcessModifier(adjustChildProcessPrivileges);
#else
	class UserProcess : public QProcess // clazy:exclude=missing-qobject-macro
	{
	public:
		explicit UserProcess(const std::function<void()>& modifier, QObject* parent = nullptr) :
			QProcess( parent ),
			m_modifier(modifier)
		{
		}

		void setupChildProcess() override
		{
			m_modifier();
		}

	private:
		const std::function<void ()>& m_modifier;
	};

	auto process = new UserProcess(adjustChildProcessPrivileges);
#endif

	QObject::connect( process, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ), &QProcess::deleteLater );
	process->start( program, parameters );

	return true;
}



QString LinuxCoreFunctions::genericUrlHandler() const
{
	return QStringLiteral( "xdg-open" );
}



QString LinuxCoreFunctions::queryDisplayDeviceName(const QScreen& screen) const
{
	QStringList nameParts;
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
	nameParts.append(screen.manufacturer());
	nameParts.append(screen.model());
#endif
	nameParts.removeAll({});
	if(nameParts.isEmpty())
	{
		return screen.name();
	}

	return QStringLiteral("%1 [%2]").arg(nameParts.join(QLatin1Char(' ')), screen.name());
}



bool LinuxCoreFunctions::prepareSessionBusAccess()
{
	const auto uid = LinuxUserFunctions::userIdFromName( VeyonCore::platform().userFunctions().currentUser() );
	if( uid > 0 )
	{
		if( seteuid(uid) == 0 )
		{
			return true;
		}

		vWarning() << "could not set effective UID - DBus calls on the session bus likely will fail";
	}
	else
	{
		vWarning() << "could not determine UID of current user - DBus calls on the session bus likely will fail";
	}

	return false;
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



int LinuxCoreFunctions::systemctl( const QStringList& arguments )
{
	QProcess process;
	process.start( QStringLiteral("systemctl"),
							  QStringList( { QStringLiteral("--no-pager"), QStringLiteral("-q") } ) + arguments );

	if( process.waitForFinished() && process.exitStatus() == QProcess::NormalExit )
	{
		return process.exitCode();
	}

	return -1;
}



void LinuxCoreFunctions::restartDisplayManagers()
{
	for( const auto& displayManager : {
		 QStringLiteral("gdm"),
		 QStringLiteral("lightdm"),
		 QStringLiteral("lxdm"),
		 QStringLiteral("nodm"),
		 QStringLiteral("sddm"),
		 QStringLiteral("wdm"),
		 QStringLiteral("xdm") } )
	{
		systemctl( { QStringLiteral("restart"), displayManager } );
	}
}



void LinuxCoreFunctions::forEachChildProcess( const std::function<bool(proc_t*)>& visitor,
											 int parentPid, int flags, bool visitParent )
{
	QProcessEnvironment sessionEnv;

	const auto proc = openproc( flags | PROC_FILLSTAT /* required for proc_t::ppid */ );
	proc_t* procInfo = nullptr;

	QList<int> ppids;

	while( ( procInfo = readproc( proc, nullptr ) ) )
	{
		if( procInfo->ppid == parentPid )
		{
			if( visitParent == false || visitor( procInfo ) )
			{
				ppids.append( procInfo->tid );
			}
		}
		else if( ppids.contains( procInfo->ppid ) && visitor( procInfo ) )
		{
			ppids.append( procInfo->tid );
		}

		freeproc( procInfo );
	}

	closeproc( proc );
}



bool LinuxCoreFunctions::waitForProcess( qint64 pid, int timeout, int sleepInterval )
{
	QElapsedTimer timeoutTimer;
	timeoutTimer.start();

	while( QFileInfo::exists( QStringLiteral("/proc/%1").arg( pid ) ) )
	{
		if( timeoutTimer.elapsed() >= timeout )
		{
			return false;
		}

		QThread::msleep( static_cast<uint64_t>( sleepInterval ) );
	}

	return true;
}
