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
#include <QSaveFile>
#include <QStringList>
#include <QXmlStreamWriter>

#include "Filesystem.h"
#include "Logger.h"
#include "OsXServiceFunctions.h"
#include "VeyonCore.h"

namespace {

constexpr auto LaunchAgentLabel = "com.veyon.vnc";
constexpr auto LaunchAgentDirectory = "/Library/LaunchAgents";
constexpr auto LaunchAgentFileName = "com.veyon.vnc.plist";
constexpr auto DefaultServerExecutable = "/Applications/Veyon/veyon-server.app/Contents/MacOS/veyon-server";

// NOTE: Script and helper resolution functions removed - no longer needed
// Service installation is handled by external script 0_agents.sh

// NOTE: Functions removed - no longer needed for simplified start/stop operations
// Service installation is handled by external script 0_agents.sh

// NOTE: Plist generation functions removed - no longer needed
// Service installation is handled by external script 0_agents.sh

// NOTE: The following functions are no longer used.
// Service installation/uninstallation is now handled by the external 0_agents.sh script.

// bool ensureLaunchAgentDirectory()
// {
// 	QDir dir(QString::fromLatin1(LaunchAgentDirectory));
// 	if (dir.exists())
// 	{
// 		return true;
// 	}
//
// 	if (dir.mkpath(QStringLiteral(".")) == false)
// 	{
// 		vWarning() << "OsXServiceFunctions: failed to create" << dir.absolutePath();
// 		return false;
// 	}
//
// 	return true;
// }
//
// bool writeLaunchAgentFile(const QString& serverExecutablePath)
// {
// 	if (ensureLaunchAgentDirectory() == false)
// 	{
// 		return false;
// 	}
//
// 	const auto plistData = buildLaunchAgentPlist(serverExecutablePath);
// 	QSaveFile file(launchAgentPath());
//
// 	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate) == false)
// 	{
// 		vWarning() << "OsXServiceFunctions: unable to open" << file.fileName() << "for writing";
// 		return false;
// 	}
//
// 	if (file.write(plistData) != plistData.size())
// 	{
// 		vWarning() << "OsXServiceFunctions: unable to write launch agent to" << file.fileName();
// 		return false;
// 	}
//
// 	if (file.commit() == false)
// 	{
// 		vWarning() << "OsXServiceFunctions: failed to commit launch agent file";
// 		return false;
// 	}
//
// 	QFile::setPermissions(file.fileName(),
// 						  QFile::ReadOwner | QFile::WriteOwner |
// 						  QFile::ReadGroup | QFile::ReadOther);
//
// 	return true;
// }

bool runCommand(const QString& program, const QStringList& arguments, bool quiet = false)
{
	QProcess process;
	process.start(program, arguments);

	if (process.waitForFinished() == false)
	{
		if (quiet == false)
		{
			vWarning() << "OsXServiceFunctions:" << program << arguments << "did not finish";
		}
		return false;
	}

	if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
	{
		if (quiet == false)
		{
			vWarning() << "OsXServiceFunctions:" << program << arguments
					   << "failed. exitStatus=" << process.exitStatus()
					   << "exitCode=" << process.exitCode();
			vWarning() << "stdout:" << process.readAllStandardOutput();
			vWarning() << "stderr:" << process.readAllStandardError();
		}
		return false;
	}

	return true;
}

bool runLaunchctl(const QStringList& arguments, bool quiet = false)
{
	return runCommand(QStringLiteral("/bin/launchctl"), arguments, quiet);
}

// NOTE: The following functions are no longer used.
// Service installation/uninstallation is now handled by the external 0_agents.sh script.

// bool fixLaunchAgentOwnership()
// {
// 	return runCommand(QStringLiteral("/usr/sbin/chown"),
// 					  {QStringLiteral("root:wheel"), launchAgentPath()},
// 					  true);
// }
//
// bool bootstrapLaunchAgent(quint32 uid)
// {
// 	const auto guiDomain = QStringLiteral("gui/%1").arg(uid);
// 	const auto labelPath = guiDomain + QLatin1Char('/') + QString::fromLatin1(LaunchAgentLabel);
//
// 	runLaunchctl({QStringLiteral("bootout"), guiDomain, launchAgentPath()}, true);
//
// 	if (runLaunchctl({QStringLiteral("bootstrap"), guiDomain, launchAgentPath()}) == false)
// 	{
// 		return false;
// 	}
//
// 	runLaunchctl({QStringLiteral("enable"), labelPath}, true);
// 	runLaunchctl({QStringLiteral("kickstart"), QStringLiteral("-k"), labelPath}, true);
//
// 	return true;
// }

bool consoleUser(quint32& uid)
{
	const QFileInfo consoleDevice(QStringLiteral("/dev/console"));
	if (consoleDevice.exists() == false)
	{
		return false;
	}

	uid = consoleDevice.ownerId();
	return uid != 0u;
}

// NOTE: The following functions are no longer used.
// Service installation/uninstallation is now handled by the external 0_agents.sh script.
// These functions are kept here for reference only.

// bool installLaunchAgent(const QString& serverExecutablePath)
// {
// 	if (writeLaunchAgentFile(serverExecutablePath) == false)
// 	{
// 		return false;
// 	}
//
// 	fixLaunchAgentOwnership();
// 	return true;
// }
//
// bool removeLaunchAgent()
// {
// 	QFile file(launchAgentPath());
// 	if (file.exists() == false)
// 	{
// 		return true;
// 	}
//
// 	if (file.remove() == false)
// 	{
// 		vWarning() << "OsXServiceFunctions: failed to remove" << file.fileName();
// 		return false;
// 	}
//
// 	return true;
// }
//
// bool startLaunchAgent()
// {
// 	const auto executablePath = resolveServerExecutablePath();
//
// 	if (installLaunchAgent(executablePath) == false)
// 	{
// 		vWarning() << "OsXServiceFunctions: failed to install LaunchAgent";
// 		return false;
// 	}
//
// 	quint32 uid = 0;
// 	if (consoleUser(uid) == false)
// 	{
// 		vWarning() << "OsXServiceFunctions: no GUI console user detected; service will start on next login";
// 		return true;
// 	}
//
// 	if (bootstrapLaunchAgent(uid) == false)
// 	{
// 		vWarning() << "OsXServiceFunctions: failed to bootstrap LaunchAgent for uid" << uid;
// 		return false;
// 	}
//
// 	return true;
// }
//
// bool stopLaunchAgent()
// {
// 	quint32 uid = 0;
// 	if (consoleUser(uid))
// 	{
// 		const auto guiDomain = QStringLiteral("gui/%1").arg(uid);
// 		return runLaunchctl({QStringLiteral("bootout"), guiDomain, launchAgentPath()}, true);
// 	}
//
// 	return true;
// }

}

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
		vDebug() << "OsXServiceFunctions: service already running";
		return true;
	}

	// Try to start the service executable first (veyon-service wrapper)
	// This will run in the background without showing a window
	const auto servicePath = serviceExecutablePath();
	vDebug() << "OsXServiceFunctions: attempting to start veyon-service:" << servicePath;

	if( QProcess::startDetached( servicePath, {} ) )
	{
		vDebug() << "OsXServiceFunctions: successfully started veyon-service";
		return true;
	}

	// Fallback: try to start veyon-server directly
	const auto serverPath = VeyonCore::filesystem().serverFilePath();
	vDebug() << "OsXServiceFunctions: veyon-service failed, trying veyon-server:" << serverPath;

	if( QProcess::startDetached( serverPath, {} ) )
	{
		vDebug() << "OsXServiceFunctions: successfully started veyon-server";
		return true;
	}

	vWarning() << "OsXServiceFunctions: failed to start both veyon-service and veyon-server";
	return false;
}



bool OsXServiceFunctions::stop( const QString& name )
{
	Q_UNUSED(name)

	if( !isRunning( veyonServiceName() ) )
	{
		vDebug() << "OsXServiceFunctions: service not running";
		return true;
	}

	// Attempt to stop the LaunchAgent via launchctl
	quint32 uid = 0;
	if (consoleUser(uid))
	{
		const auto guiDomain = QStringLiteral("gui/%1").arg(uid);
		const auto labelPath = guiDomain + QLatin1Char('/') + QString::fromLatin1(LaunchAgentLabel);

		vDebug() << "OsXServiceFunctions: attempting to stop LaunchAgent" << labelPath;

		// Try to kill the service (this stops it but doesn't unload it)
		if (runLaunchctl({QStringLiteral("kill"), QStringLiteral("SIGTERM"), labelPath}, true))
		{
			vDebug() << "OsXServiceFunctions: successfully stopped LaunchAgent";
			return true;
		}
	}
	else
	{
		vWarning() << "OsXServiceFunctions: no GUI console user detected";
	}

	// Fallback: terminate processes directly
	vDebug() << "OsXServiceFunctions: launchctl stop failed, terminating processes directly";
	bool success = false;

	for (const auto& executable : { serviceExecutablePath(),
									VeyonCore::filesystem().serverFilePath(),
									VeyonCore::filesystem().workerFilePath() })
	{
		if (terminateMatchingProcesses(executable))
		{
			success = true;
		}
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

	// Service installation is handled externally via 0_agents.sh script
	// The configurator should not install the LaunchAgent
	vInfo() << "OsXServiceFunctions: service installation should be done via external script (0_agents.sh)";
	return false;
}



bool OsXServiceFunctions::uninstall( const QString& name )
{
	Q_UNUSED(name)

	// Service uninstallation is handled externally via 0_agents.sh script
	// The configurator should not uninstall the LaunchAgent
	vInfo() << "OsXServiceFunctions: service uninstallation should be done via external script (0_agents.sh)";
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
