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

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "Filesystem.h"
#include "Logger.h"
#include "OsXServiceFunctions.h"
#include "VeyonCore.h"

namespace {

constexpr auto LaunchAgentLabel = "com.veyon.vnc";
constexpr auto LaunchAgentDirectory = "/Library/LaunchAgents";
constexpr auto LaunchAgentFileName = "com.veyon.vnc.plist";
constexpr auto DefaultServerExecutable = "/Applications/Veyon/veyon-server.app/Contents/MacOS/veyon-server";

// Paths for LaunchAgent installation (matching launchAgents.sh script)
constexpr auto SourceScriptsDirectory = "/Applications/Veyon/veyon-configurator.app/Contents/Resources/Scripts";

// Static cache for home path to avoid Qt string operations in thread pool
static QString g_cachedHomePath;

// Initialize home path cache (must be called from main thread before using threads)
void initializePathCache()
{
	if (g_cachedHomePath.isEmpty())
	{
		g_cachedHomePath = QDir::homePath();
	}
}

QString userLaunchAgentPath()
{
	initializePathCache();
	return g_cachedHomePath + QStringLiteral("/Library/LaunchAgents/") + QString::fromLatin1(LaunchAgentFileName);
}

QString sourcePlistPath()
{
	return QString::fromLatin1(SourceScriptsDirectory) + QLatin1Char('/') + QString::fromLatin1(LaunchAgentFileName);
}

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

// NOTE: This function is kept for potential future use but marked unused to avoid warnings
[[maybe_unused]] bool consoleUser(quint32& uid)
{
	const QFileInfo consoleDevice(QStringLiteral("/dev/console"));
	if (consoleDevice.exists() == false)
	{
		return false;
	}

	uid = consoleDevice.ownerId();
	return uid != 0u;
}

// Check if LaunchAgent is loaded and running
bool isLaunchAgentRunning()
{
	const auto uid = QString::number(getuid());
	const auto guiDomain = QStringLiteral("gui/") + uid;
	const auto labelPath = guiDomain + QLatin1Char('/') + QString::fromLatin1(LaunchAgentLabel);

	// Use launchctl print to check if the service is loaded
	QProcess process;
	process.start(QStringLiteral("/bin/launchctl"),
				  {QStringLiteral("print"), labelPath});

	if (!process.waitForFinished())
	{
		return false;
	}

	// If launchctl print succeeds (exit code 0), the service is loaded
	if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0)
	{
		// Check if it's actually running by looking for the PID in the output
		const QString output = QString::fromUtf8(process.readAllStandardOutput());
		return output.contains(QStringLiteral("pid = "));
	}

	return false;
}

// Install LaunchAgent for current user (equivalent to option 2 in launchAgents.sh)
// Steps:
//   1. Create LaunchAgents directory if needed
//   2. Copy plist file to user's LaunchAgents
//   3. Unload any existing instance (bootout)
//   4. Load the new agent (bootstrap)
//   5. Enable and start the service (kickstart)
//
// NOTE: This function is called from a thread pool, so we pass pre-computed
// strings to avoid any QString operations inside the thread.
bool installUserLaunchAgent(const std::string& sourcePlistStd,
                             const std::string& userPlistStd,
                             const std::string& launchAgentsDirStd,
                             const std::string& uidStd,
                             const std::string& guiDomainStd,
                             const std::string& labelPathStd)
{
	vDebug() << "OsXServiceFunctions: installing LaunchAgent for current user";

	// Check if source plist exists using POSIX
	if (access(sourcePlistStd.c_str(), R_OK) != 0)
	{
		vWarning() << "OsXServiceFunctions: source plist not found at" << sourcePlistStd.c_str();
		return false;
	}

	// Create user's LaunchAgents directory if needed using POSIX
	struct stat st;
	if (stat(launchAgentsDirStd.c_str(), &st) != 0)
	{
		// Directory doesn't exist, create it
		if (mkdir(launchAgentsDirStd.c_str(), 0755) != 0)
		{
			vWarning() << "OsXServiceFunctions: failed to create" << launchAgentsDirStd.c_str() << "- error:" << strerror(errno);
			return false;
		}
	}

	// Remove existing plist file if any (ignore errors)
	unlink(userPlistStd.c_str());

	// Copy file using POSIX I/O (thread-safe)
	int srcFd = open(sourcePlistStd.c_str(), O_RDONLY);
	if (srcFd < 0)
	{
		vWarning() << "OsXServiceFunctions: failed to open source plist" << sourcePlistStd.c_str() << "- error:" << strerror(errno);
		return false;
	}

	int dstFd = open(userPlistStd.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (dstFd < 0)
	{
		vWarning() << "OsXServiceFunctions: failed to create destination plist" << userPlistStd.c_str() << "- error:" << strerror(errno);
		close(srcFd);
		return false;
	}

	// Copy data in chunks
	char buffer[8192];
	ssize_t bytesRead;
	bool copySuccess = true;

	while ((bytesRead = read(srcFd, buffer, sizeof(buffer))) > 0)
	{
		ssize_t bytesWritten = write(dstFd, buffer, bytesRead);
		if (bytesWritten != bytesRead)
		{
			vWarning() << "OsXServiceFunctions: failed to write plist data - error:" << strerror(errno);
			copySuccess = false;
			break;
		}
	}

	if (bytesRead < 0)
	{
		vWarning() << "OsXServiceFunctions: failed to read source plist - error:" << strerror(errno);
		copySuccess = false;
	}

	close(srcFd);
	close(dstFd);

	if (!copySuccess)
	{
		unlink(userPlistStd.c_str());
		return false;
	}

	vInfo() << "OsXServiceFunctions: copied plist to" << userPlistStd.c_str();

	// Convert pre-computed strings back to QString for launchctl operations
	// These were computed in the main thread to avoid QString initialization issues
	const QString guiDomain = QString::fromStdString(guiDomainStd);
	const QString userPlist = QString::fromStdString(userPlistStd);
	const QString labelPath = QString::fromStdString(labelPathStd);

	// Unload any existing instance (ignore errors)
	vDebug() << "OsXServiceFunctions: bootout existing LaunchAgent (if any)";
	runLaunchctl({QStringLiteral("bootout"), guiDomain, userPlist}, true);

	// Bootstrap the LaunchAgent
	vDebug() << "OsXServiceFunctions: bootstrap LaunchAgent";
	if (!runLaunchctl({QStringLiteral("bootstrap"), guiDomain, userPlist}))
	{
		vWarning() << "OsXServiceFunctions: failed to bootstrap LaunchAgent";
		return false;
	}

	// Enable the service
	vDebug() << "OsXServiceFunctions: enable service" << labelPath;
	runLaunchctl({QStringLiteral("enable"), labelPath}, true);

	// Kickstart the service
	vDebug() << "OsXServiceFunctions: kickstart service" << labelPath;
	runLaunchctl({QStringLiteral("kickstart"), QStringLiteral("-k"), labelPath}, true);

	vInfo() << "OsXServiceFunctions: LaunchAgent loaded for UID" << uidStd.c_str();
	return true;
}

// Uninstall LaunchAgent (equivalent to option 5 in launchAgents.sh)
// Steps:
//   1. Check if user plist exists
//   2. Unload from current user's GUI session (bootout)
//   3. Remove the plist file from ~/Library/LaunchAgents/
bool uninstallUserLaunchAgent()
{
	vDebug() << "OsXServiceFunctions: uninstalling user LaunchAgent";

	const auto userPlist = userLaunchAgentPath();
	if (!QFile::exists(userPlist))
	{
		vInfo() << "OsXServiceFunctions: no user plist found";
		return true;
	}

	// Unload from current user's session
	const auto uid = QString::number(getuid());
	const auto guiDomain = QStringLiteral("gui/") + uid;

	vDebug() << "OsXServiceFunctions: bootout LaunchAgent from" << guiDomain;
	runLaunchctl({QStringLiteral("bootout"), guiDomain, userPlist}, true);

	// Remove user plist file
	if (!QFile::remove(userPlist))
	{
		vWarning() << "OsXServiceFunctions: failed to remove" << userPlist;
		return false;
	}

	vInfo() << "OsXServiceFunctions: removed user plist";
	return true;
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

// NOTE: This function is kept for potential future use but marked unused to avoid warnings
[[maybe_unused]] bool terminateMatchingProcesses(const QString& executable)
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

	// First, check if the LaunchAgent is loaded and running
	if (isLaunchAgentRunning())
	{
		return true;
	}

	// Fallback: check if processes are running directly
	// (for backwards compatibility or manual starts)
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

	// Initialize path cache before any thread operations
	// This must be done in the main thread to avoid Qt crashes
	initializePathCache();

	// Pre-compute ALL QString values in the main thread before the thread pool
	// to avoid QString initialization issues in secondary threads
	const std::string sourcePlistStd = sourcePlistPath().toStdString();
	const std::string userPlistStd = userLaunchAgentPath().toStdString();
	const QString launchAgentsDir = g_cachedHomePath + QStringLiteral("/Library/LaunchAgents");
	const std::string launchAgentsDirStd = launchAgentsDir.toStdString();

	// Pre-compute UID and launchctl domain strings
	const QString uid = QString::number(getuid());
	const std::string uidStd = uid.toStdString();
	const QString guiDomain = QStringLiteral("gui/") + uid;
	const std::string guiDomainStd = guiDomain.toStdString();
	const QString labelPath = guiDomain + QLatin1Char('/') + QString::fromLatin1(LaunchAgentLabel);
	const std::string labelPathStd = labelPath.toStdString();

	// Install and start the LaunchAgent for the current user
	// This follows the logic of option 2 in launchAgents.sh script
	vInfo() << "OsXServiceFunctions: installing and starting user LaunchAgent";
	return installUserLaunchAgent(sourcePlistStd, userPlistStd, launchAgentsDirStd,
	                               uidStd, guiDomainStd, labelPathStd);
}



bool OsXServiceFunctions::stop( const QString& name )
{
	Q_UNUSED(name)

	if( !isRunning( veyonServiceName() ) )
	{
		vDebug() << "OsXServiceFunctions: service not running";
		return true;
	}

	// Uninstall the LaunchAgent
	// This follows the logic of option 5 in launchAgents.sh script
	vInfo() << "OsXServiceFunctions: stopping and uninstalling user LaunchAgent";
	return uninstallUserLaunchAgent();
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
