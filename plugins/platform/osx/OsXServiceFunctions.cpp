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

QStringList scriptSearchDirectories()
{
	QStringList directories;

	auto addDirectory = [&](const QString& path) {
		if (path.isEmpty() || directories.contains(path))
		{
			return;
		}
		directories.append(path);
	};

	QDir appDir(QCoreApplication::applicationDirPath());
	if (appDir.dirName() == QStringLiteral("MacOS"))
	{
		QDir scriptsDir(appDir);
		if (scriptsDir.cdUp() &&
			scriptsDir.cd(QStringLiteral("Resources")) &&
			scriptsDir.cd(QStringLiteral("Scripts")))
		{
			addDirectory(scriptsDir.absolutePath());
		}
	}

	addDirectory(QStringLiteral("/Applications/Veyon/veyon-configurator.app/Contents/Resources/Scripts"));

	return directories;
}

QString resolveBundledScript(const QString& scriptName)
{
	for (const auto& directory : scriptSearchDirectories())
	{
		const QFileInfo scriptInfo(directory + QLatin1Char('/') + scriptName);
		if (scriptInfo.isFile())
		{
			return scriptInfo.absoluteFilePath();
		}
	}

	return {};
}

bool runBundledScript(const QString& scriptName)
{
	const auto scriptPath = resolveBundledScript(scriptName);

	if (scriptPath.isEmpty())
	{
		return false;
	}

	QProcess process;
	process.start(QStringLiteral("/bin/bash"), QStringList{scriptPath});

	if (process.waitForFinished() == false)
	{
		vWarning() << "OsXServiceFunctions: script" << scriptPath << "did not finish";
		return false;
	}

	if (process.exitStatus() != QProcess::NormalExit ||
		process.exitCode() != 0)
	{
		vWarning() << "OsXServiceFunctions: script" << scriptPath
				   << "failed with exit code" << process.exitCode();
		vWarning() << "stdout:" << process.readAllStandardOutput();
		vWarning() << "stderr:" << process.readAllStandardError();
		return false;
	}

	return true;
}

QString launchAgentPath()
{
	return QString::fromLatin1(LaunchAgentDirectory) + QLatin1Char('/') + QString::fromLatin1(LaunchAgentFileName);
}

QString defaultServerExecutablePath()
{
	return QString::fromLatin1( DefaultServerExecutable );
}

QString resolveServerExecutablePath()
{
	const QFileInfo serverBinaryInfo( VeyonCore::filesystem().serverFilePath() );

	if( serverBinaryInfo.exists() && serverBinaryInfo.isExecutable() )
	{
		return serverBinaryInfo.absoluteFilePath();
	}

	return defaultServerExecutablePath();
}

void writeKey(QXmlStreamWriter& writer, const QString& key)
{
	writer.writeTextElement(QStringLiteral("key"), key);
}

	QString logPath(const QString& fileName)
	{
		return QStringLiteral("/var/tmp/") + fileName;
	}

	QByteArray buildLaunchAgentPlist(const QString& serverExecutablePath)
{
	QString xml;
	QXmlStreamWriter writer(&xml);

	writer.setAutoFormatting(true);
	writer.writeStartDocument(QStringLiteral("1.0"));
	writer.writeDTD(QStringLiteral(
		"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
		"\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"));
	writer.writeStartElement(QStringLiteral("plist"));
	writer.writeAttribute(QStringLiteral("version"), QStringLiteral("1.0"));
	writer.writeStartElement(QStringLiteral("dict"));

	writer.writeComment(QStringLiteral(
		" LaunchAgent for all users: starts Veyon VNC app hidden, without activating Dock or focus "));

	writeKey(writer, QStringLiteral("Label"));
	writer.writeTextElement(QStringLiteral("string"), QString::fromLatin1(LaunchAgentLabel));

	writeKey(writer, QStringLiteral("ProgramArguments"));
	writer.writeStartElement(QStringLiteral("array"));
	writer.writeTextElement(QStringLiteral("string"), serverExecutablePath);
	writer.writeEndElement(); // array

	writeKey(writer, QStringLiteral("RunAtLoad"));
	writer.writeEmptyElement(QStringLiteral("true"));

	writeKey(writer, QStringLiteral("KeepAlive"));
	writer.writeEmptyElement(QStringLiteral("true"));

	writeKey(writer, QStringLiteral("LimitLoadToSessionType"));
	writer.writeTextElement(QStringLiteral("string"), QStringLiteral("Aqua"));

	writeKey(writer, QStringLiteral("ProcessType"));
	writer.writeTextElement(QStringLiteral("string"), QStringLiteral("Interactive"));

	writeKey(writer, QStringLiteral("StandardOutPath"));
	writer.writeTextElement(QStringLiteral("string"), logPath(QStringLiteral("veyon-vnc.out.log")));

	writeKey(writer, QStringLiteral("StandardErrorPath"));
	writer.writeTextElement(QStringLiteral("string"), logPath(QStringLiteral("veyon-vnc.err.log")));

	writer.writeEndElement(); // dict
	writer.writeEndElement(); // plist
	writer.writeEndDocument();

	return xml.toUtf8();
}

bool ensureLaunchAgentDirectory()
{
	QDir dir(QString::fromLatin1(LaunchAgentDirectory));
	if (dir.exists())
	{
		return true;
	}

	if (dir.mkpath(QStringLiteral(".")) == false)
	{
		vWarning() << "OsXServiceFunctions: failed to create" << dir.absolutePath();
		return false;
	}

	return true;
}

bool writeLaunchAgentFile(const QString& serverExecutablePath)
{
	if (ensureLaunchAgentDirectory() == false)
	{
		return false;
	}

	const auto plistData = buildLaunchAgentPlist(serverExecutablePath);
	QSaveFile file(launchAgentPath());

	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate) == false)
	{
		vWarning() << "OsXServiceFunctions: unable to open" << file.fileName() << "for writing";
		return false;
	}

	if (file.write(plistData) != plistData.size())
	{
		vWarning() << "OsXServiceFunctions: unable to write launch agent to" << file.fileName();
		return false;
	}

	if (file.commit() == false)
	{
		vWarning() << "OsXServiceFunctions: failed to commit launch agent file";
		return false;
	}

	QFile::setPermissions(file.fileName(),
						  QFile::ReadOwner | QFile::WriteOwner |
						  QFile::ReadGroup | QFile::ReadOther);

	return true;
}

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

bool fixLaunchAgentOwnership()
{
	return runCommand(QStringLiteral("/usr/sbin/chown"),
					  {QStringLiteral("root:wheel"), launchAgentPath()},
					  true);
}

bool bootstrapLaunchAgent(quint32 uid)
{
	const auto guiDomain = QStringLiteral("gui/%1").arg(uid);
	const auto labelPath = guiDomain + QLatin1Char('/') + QString::fromLatin1(LaunchAgentLabel);

	runLaunchctl({QStringLiteral("bootout"), guiDomain, launchAgentPath()}, true);

	if (runLaunchctl({QStringLiteral("bootstrap"), guiDomain, launchAgentPath()}) == false)
	{
		return false;
	}

	runLaunchctl({QStringLiteral("enable"), labelPath}, true);
	runLaunchctl({QStringLiteral("kickstart"), QStringLiteral("-k"), labelPath}, true);

	return true;
}

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

bool installLaunchAgent(const QString& serverExecutablePath)
{
	if (writeLaunchAgentFile(serverExecutablePath) == false)
	{
		return false;
	}

	fixLaunchAgentOwnership();
	return true;
}

bool removeLaunchAgent()
{
	QFile file(launchAgentPath());
	if (file.exists() == false)
	{
		return true;
	}

	if (file.remove() == false)
	{
		vWarning() << "OsXServiceFunctions: failed to remove" << file.fileName();
		return false;
	}

	return true;
}

bool startLaunchAgent()
{
	const auto executablePath = resolveServerExecutablePath();

	if (installLaunchAgent(executablePath) == false)
	{
		vWarning() << "OsXServiceFunctions: failed to install LaunchAgent";
		return false;
	}

	quint32 uid = 0;
	if (consoleUser(uid) == false)
	{
		vWarning() << "OsXServiceFunctions: no GUI console user detected; service will start on next login";
		return true;
	}

	if (bootstrapLaunchAgent(uid) == false)
	{
		vWarning() << "OsXServiceFunctions: failed to bootstrap LaunchAgent for uid" << uid;
		return false;
	}

	return true;
}

bool stopLaunchAgent()
{
	quint32 uid = 0;
	if (consoleUser(uid))
	{
		const auto guiDomain = QStringLiteral("gui/%1").arg(uid);
		return runLaunchctl({QStringLiteral("bootout"), guiDomain, launchAgentPath()}, true);
	}

	return true;
}

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
		return true;
	}

	if (runBundledScript(QStringLiteral("install_veyon_vnc_agent.sh")))
	{
		return true;
	}

	if (startLaunchAgent())
	{
		return true;
	}

	// fall back to launching the server directly if the service wrapper could not be started
	if( QProcess::startDetached( serviceExecutablePath(), {} ) )
	{
		return true;
	}

	return QProcess::startDetached( VeyonCore::filesystem().serverFilePath(), {} );
}



bool OsXServiceFunctions::stop( const QString& name )
{
	Q_UNUSED(name)

	const bool scriptSuccess = runBundledScript(QStringLiteral("uninstall_veyon_vnc_agent.sh"));

	const bool agentStopped = scriptSuccess ? true : stopLaunchAgent();

	bool success = agentStopped;

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

	return runBundledScript(QStringLiteral("install_veyon_vnc_agent.sh")) ||
		   installLaunchAgent(resolveServerExecutablePath());
}



bool OsXServiceFunctions::uninstall( const QString& name )
{
	Q_UNUSED(name)

	const bool scriptSuccess = runBundledScript(QStringLiteral("uninstall_veyon_vnc_agent.sh"));

	if (scriptSuccess)
	{
		return true;
	}

	stopLaunchAgent();
	return removeLaunchAgent();
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
