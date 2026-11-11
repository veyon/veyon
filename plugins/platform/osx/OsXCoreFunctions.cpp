/*
 * OsXCoreFunctions.cpp - implementation of OsXCoreFunctions class
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

#include <QFileInfo>
#include <QProcess>
#include <QScreen>
#include <QStandardPaths>
#include <QWidget>

#include <pwd.h>
#include <unistd.h>

#include "Logger.h"
#include "OsXCoreFunctions.h"
#include "VeyonCore.h"

namespace {
QString absoluteExecutablePath(const QString& program)
{
	const QFileInfo info(program);
	if( info.isAbsolute() )
	{
		vDebug() << "absoluteExecutablePath: program is already absolute:" << program;
		return info.absoluteFilePath();
	}

	vDebug() << "absoluteExecutablePath: resolving relative program:" << program;
	const auto resolved = QStandardPaths::findExecutable( program );
	if( resolved.isEmpty() )
	{
		vWarning() << "absoluteExecutablePath: could not resolve" << program << "- PATH may be incomplete";
		vWarning() << "absoluteExecutablePath: current PATH:" << QString::fromUtf8(qgetenv("PATH"));
	}
	else
	{
		vDebug() << "absoluteExecutablePath: resolved to:" << resolved;
	}
	return resolved.isEmpty() ? program : resolved;
}
}

bool OsXCoreFunctions::applyConfiguration()
{
	return true;
}



void OsXCoreFunctions::initNativeLoggingSystem( const QString& appName )
{
	Q_UNUSED(appName)
}



void OsXCoreFunctions::writeToNativeLoggingSystem( const QString& message, Logger::LogLevel loglevel )
{
	Q_UNUSED(message)
	Q_UNUSED(loglevel)
}



void OsXCoreFunctions::reboot()
{
	const QString shutdownTool = absoluteExecutablePath( QStringLiteral("/sbin/shutdown") );
	const QStringList arguments{ QStringLiteral("-r"), QStringLiteral("now") };

	if( isRunningAsAdmin() )
	{
		startDetached( shutdownTool, arguments );
	}
	else
	{
		if( runProgramAsAdmin( shutdownTool, arguments ) == false )
		{
			vWarning() << "OsXCoreFunctions: failed to request system reboot via shutdown tool";
		}
	}
}



void OsXCoreFunctions::powerDown( bool installUpdates )
{
	Q_UNUSED(installUpdates)

	const QString shutdownTool = absoluteExecutablePath( QStringLiteral("/sbin/shutdown") );
	const QStringList arguments{ QStringLiteral("-h"), QStringLiteral("now") };

	if( isRunningAsAdmin() )
	{
		startDetached( shutdownTool, arguments );
	}
	else
	{
		if( runProgramAsAdmin( shutdownTool, arguments ) == false )
		{
			vWarning() << "OsXCoreFunctions: failed to request system shutdown via shutdown tool";
		}
	}
}



void OsXCoreFunctions::raiseWindow( QWidget* widget, bool stayOnTop )
{
	if( widget == nullptr )
	{
		return;
	}

	const auto originalFlags = widget->windowFlags();

	if( stayOnTop && ( originalFlags.testFlag( Qt::WindowStaysOnTopHint ) == false ) )
	{
		widget->setWindowFlag( Qt::WindowStaysOnTopHint, true );
		widget->show();
	}

	widget->raise();
	widget->activateWindow();

	if( stayOnTop == false && originalFlags.testFlag( Qt::WindowStaysOnTopHint ) == false )
	{
		widget->setWindowFlag( Qt::WindowStaysOnTopHint, false );
		widget->show();
	}
}



void OsXCoreFunctions::disableScreenSaver()
{
}



void OsXCoreFunctions::restoreScreenSaverSettings()
{
}



void OsXCoreFunctions::setSystemUiState( bool enabled )
{
	Q_UNUSED(enabled)
}



QString OsXCoreFunctions::activeDesktopName()
{
	return QStringLiteral("Finder");
}



bool OsXCoreFunctions::isRunningAsAdmin() const
{
	return geteuid() == 0;
}



bool OsXCoreFunctions::runProgramAsAdmin( const QString& program, const QStringList& parameters )
{
	const auto osascript = QStandardPaths::findExecutable( QStringLiteral("osascript") );
	if( osascript.isEmpty() )
	{
		return false;
	}

	const auto shellQuote = []( const QString& argument ) {
		QString escaped = argument;
		escaped.replace( QStringLiteral("\\"), QStringLiteral("\\\\") );
		escaped.replace( QStringLiteral("'"), QStringLiteral("'\\''") );
		return QStringLiteral("'%1'").arg( escaped );
	};

	QStringList commandParts;
	commandParts << QStringLiteral("/usr/bin/env");
	commandParts << QStringLiteral("VEYON_CONFIGURATOR_NO_ELEVATION=1");
	commandParts << absoluteExecutablePath( program );
	commandParts += parameters;

	QString commandString;
	for( const auto& part : commandParts )
	{
		if( commandString.isEmpty() )
		{
			commandString = shellQuote( part );
		}
		else
		{
			commandString += QLatin1Char(' ') + shellQuote( part );
		}
	}

	auto escapedCommand = commandString;
	escapedCommand.replace( QStringLiteral("\\"), QStringLiteral("\\\\") );
	escapedCommand.replace( QStringLiteral("\""), QStringLiteral("\\\"") );

	const auto script = QStringLiteral("do shell script \"%1\" with administrator privileges").arg( escapedCommand );

	return startDetached( osascript, { QStringLiteral("-e"), script } );
}



bool OsXCoreFunctions::runProgramAsUser( const QString& program,
										  const QStringList& parameters,
										  const QString& username,
										  const QString& desktop )
{
	Q_UNUSED(desktop)

	vDebug() << "runProgramAsUser: attempting to run" << program << "as user" << username;
	vDebug() << "runProgramAsUser: parameters:" << parameters;

	const auto strippedUser = VeyonCore::stripDomain( username );
	const auto pwEntry = getpwnam( strippedUser.toUtf8().constData() );
	if( pwEntry == nullptr )
	{
		vWarning() << "OsXCoreFunctions: unable to resolve user" << username;
		return false;
	}

	const QString resolvedProgram = absoluteExecutablePath( program );
	vDebug() << "runProgramAsUser: resolved program path:" << resolvedProgram;

	QStringList args{ QStringLiteral("asuser"), QString::number( pwEntry->pw_uid ), resolvedProgram };
	args += parameters;

	vDebug() << "runProgramAsUser: executing launchctl with args:" << args;
	const bool result = startDetached( QStringLiteral("/bin/launchctl"), args );
	if( !result )
	{
		vWarning() << "runProgramAsUser: startDetached failed for" << resolvedProgram;
	}
	return result;
}



QString OsXCoreFunctions::genericUrlHandler() const
{
	return QStringLiteral("open");
}



QString OsXCoreFunctions::queryDisplayDeviceName( const QScreen& screen ) const
{
	return screen.name();
}



bool OsXCoreFunctions::startDetached( const QString& program, const QStringList& arguments, const QString& workingDirectory )
{
	return QProcess::startDetached( program, arguments, workingDirectory );
}
