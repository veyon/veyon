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
#include <QProcess>

#include "Filesystem.h"
#include "Logger.h"
#include "OsXServiceFunctions.h"
#include "VeyonCore.h"

namespace {
QString resolvedExecutable(const QString& executable)
{
	const QString path = QDir::toNativeSeparators( QCoreApplication::applicationDirPath()
		+ QDir::separator() + executable + VeyonCore::executableSuffix() );
	return QFile::exists( path ) ? path : executable;
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



bool OsXServiceFunctions::isRunning( const QString& name )
{
	Q_UNUSED(name)

	QProcess pgrepProcess;
	pgrepProcess.start( QStringLiteral("/usr/bin/pgrep"),
						{ QStringLiteral("-f"), serviceExecutablePath() } );
	if( pgrepProcess.waitForFinished() == false )
	{
		return false;
	}

	return pgrepProcess.exitStatus() == QProcess::NormalExit &&
			pgrepProcess.exitCode() == 0;
}



bool OsXServiceFunctions::start( const QString& name )
{
	Q_UNUSED(name)

	if( isRunning( veyonServiceName() ) )
	{
		return true;
	}

	return QProcess::startDetached( serviceExecutablePath(), {} );
}



bool OsXServiceFunctions::stop( const QString& name )
{
	Q_UNUSED(name)

	QProcess pkillProcess;
	pkillProcess.start( QStringLiteral("/usr/bin/pkill"),
						{ QStringLiteral("-f"), serviceExecutablePath() } );
	if( pkillProcess.waitForFinished() == false )
	{
		return false;
	}

	// pkill returns 0 when at least one process was signalled and 1 if none matched.
	return pkillProcess.exitStatus() == QProcess::NormalExit &&
			( pkillProcess.exitCode() == 0 || pkillProcess.exitCode() == 1 );
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
	return resolvedExecutable( QStringLiteral("veyon-service") );
}
