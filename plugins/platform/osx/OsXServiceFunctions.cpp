/*
 * OsXServiceFunctions.cpp - implementation of OsXServiceFunctions class
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

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

#include "Filesystem.h"
#include "Logger.h"
#include "OsXServiceFunctions.h"
#include "VeyonCore.h"

QString OsXServiceFunctions::veyonServiceName() const
{
	return QStringLiteral("veyon");
}



bool OsXServiceFunctions::isRegistered( const QString& name )
{
	Q_UNUSED(name)
	vCritical() << "OsXServiceFunctions: querying service registration is not supported on this platform.";
	return false;
}



namespace {
bool hasMatchingProcess(const QString& executable)
{
	const QStringList patterns = {
		executable,
		QFileInfo(executable).fileName()
	};

	for (const auto& pattern : patterns)
	{
		if (pattern.isEmpty())
		{
			continue;
		}

		vDebug() << "OsXServiceFunctions: checking for process pattern" << pattern;
		QProcess pgrepProcess;
		pgrepProcess.start(QStringLiteral("/usr/bin/pgrep"),
						   {QStringLiteral("-f"), pattern});
		if (pgrepProcess.waitForFinished() &&
			pgrepProcess.exitStatus() == QProcess::NormalExit &&
			pgrepProcess.exitCode() == 0)
		{
			vDebug() << "OsXServiceFunctions: found matching process for" << pattern;
			return true;
		}
		vDebug() << "OsXServiceFunctions: no match for" << pattern
				 << "exitStatus" << pgrepProcess.exitStatus()
				 << "exitCode" << pgrepProcess.exitCode();
	}

	return false;
}

bool terminateMatchingProcesses(const QString& executable)
{
	const QStringList patterns = {
		executable,
		QFileInfo(executable).fileName()
	};

	for (const auto& pattern : patterns)
	{
		if (pattern.isEmpty())
		{
			continue;
		}

		vDebug() << "OsXServiceFunctions: terminating processes matching" << pattern;
		QProcess pkillProcess;
		pkillProcess.start(QStringLiteral("/usr/bin/pkill"),
						   {QStringLiteral("-f"), pattern});
		if (pkillProcess.waitForFinished() &&
			pkillProcess.exitStatus() == QProcess::NormalExit)
		{
			// pkill returns 0 when at least one process was signalled and 1 if none matched.
			if (pkillProcess.exitCode() == 0)
			{
				vDebug() << "OsXServiceFunctions: terminated processes matching" << pattern;
				return true;
			}
		}
	}

	return false;
}
}

bool OsXServiceFunctions::isRunning( const QString& name )
{
	Q_UNUSED(name)

	const auto executables = {
		serviceExecutablePath(),
		VeyonCore::filesystem().serverFilePath(),
		VeyonCore::filesystem().workerFilePath()
	};

	for (const auto& executable : executables)
	{
		if (hasMatchingProcess(executable))
		{
			return true;
		}
	}

	return false;
}



bool OsXServiceFunctions::start( const QString& name )
{
	Q_UNUSED(name)

	if( isRunning( veyonServiceName() ) )
	{
		return true;
	}

	if( QProcess::startDetached( serviceExecutablePath(), {} ) )
	{
		return true;
	}

	// fall back to launching the server directly if the service wrapper could not be started
	return QProcess::startDetached( VeyonCore::filesystem().serverFilePath(), {} );
}



bool OsXServiceFunctions::stop( const QString& name )
{
	Q_UNUSED(name)

	bool success = true;

	for (const auto& executable : { serviceExecutablePath(),
									VeyonCore::filesystem().serverFilePath(),
									VeyonCore::filesystem().workerFilePath() })
	{
		success = terminateMatchingProcesses(executable) && success;
	}

	return success;
}



bool OsXServiceFunctions::install( const QString& name, const QString& serviceFilePath,
								   StartMode startMode, const QString& displayName )
{
	Q_UNUSED(name)
	Q_UNUSED(serviceFilePath)
	Q_UNUSED(startMode)
	Q_UNUSED(displayName)

	vCritical() << "OsXServiceFunctions: installing launchd services is not supported.";
	return false;
}



bool OsXServiceFunctions::uninstall( const QString& name )
{
	Q_UNUSED(name)

	vCritical() << "OsXServiceFunctions: uninstalling launchd services is not supported.";
	return false;
}



bool OsXServiceFunctions::setStartMode( const QString& name, StartMode startMode )
{
	Q_UNUSED(name)
	Q_UNUSED(startMode)

	vWarning() << "OsXServiceFunctions: changing service start mode is not supported.";
	return true;
}



bool OsXServiceFunctions::runAsService( const QString& name, const ServiceEntryPoint& serviceEntryPoint )
{
	Q_UNUSED(name)

	serviceEntryPoint();
	return true;
}



void OsXServiceFunctions::manageServerInstances()
{
	const auto serverPath = VeyonCore::filesystem().serverFilePath();
	if( QProcess::startDetached( serverPath, {} ) == false )
	{
		vWarning() << "OsXServiceFunctions: failed to start Veyon Server" << serverPath;
	}
}



QString OsXServiceFunctions::serviceExecutablePath()
{
	return VeyonCore::filesystem().resolveExecutable(QStringLiteral("veyon-service"));
}
