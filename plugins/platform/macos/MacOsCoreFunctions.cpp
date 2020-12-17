/*
 * MacOsCoreFunctions.cpp - implementation of MacOsCoreFunctions class
 *
 * Copyright (c) 2017-2020 Tobias Junghans <tobydox@veyon.io>
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
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QWidget>

#include <unistd.h>

#include "MacOsCoreFunctions.h"
#include "MacOsUserFunctions.h"
#include "PlatformUserFunctions.h"


bool MacOsCoreFunctions::applyConfiguration()
{
	return true;
}



void MacOsCoreFunctions::initNativeLoggingSystem( const QString& appName )
{
	Q_UNUSED(appName)
}



void MacOsCoreFunctions::writeToNativeLoggingSystem( const QString& message, Logger::LogLevel loglevel )
{
	Q_UNUSED(message)
	Q_UNUSED(loglevel)
}



void MacOsCoreFunctions::reboot()
{
	if( isRunningAsAdmin() )
	{
		for( const auto& file : { QStringLiteral("/sbin/reboot"), QStringLiteral("/usr/sbin/reboot") } )
		{
			if( QFileInfo::exists( file ) )
			{
				QProcess::startDetached( file, {} );
				return;
			}
		}

		QProcess::startDetached( QStringLiteral("reboot"), {} );
	}
	else
	{
		QProcess::startDetached( QStringLiteral("osascript -e 'tell app \"System Events\" to restart'"), {} );
	}
}



void MacOsCoreFunctions::powerDown( bool installUpdates )
{
	//Q_UNUSED(installUpdates)

	if( isRunningAsAdmin() )
	{
		for( const auto& file : { QStringLiteral("/sbin/poweroff"), QStringLiteral("/usr/sbin/poweroff") } )
		{
			if( QFileInfo::exists( file ) )
			{
				QProcess::startDetached( file, {} );
				return;
			}
		}

		QProcess::startDetached( QStringLiteral("poweroff"), {} );
	}
	else
	{
		QProcess::startDetached( QStringLiteral("osascript -e 'tell app \"System Events\" to power off'"), {} );
	}
}



void MacOsCoreFunctions::raiseWindow( QWidget* widget, bool stayOnTop )
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



void MacOsCoreFunctions::disableScreenSaver()
{
	//TODO: Implement. Maybe take a look at Quartz
}



void MacOsCoreFunctions::restoreScreenSaverSettings()
{
	//TODO: Implement. Maybe take a look at Quartz
}



void MacOsCoreFunctions::setSystemUiState( bool enabled )
{
	Q_UNUSED(enabled)
}

QString MacOsCoreFunctions::activeDesktopName()
{
	return QString();
}



bool MacOsCoreFunctions::isRunningAsAdmin() const
{
	return getuid() == 0 || geteuid() == 0;
}



bool MacOsCoreFunctions::runProgramAsAdmin( const QString& program, const QStringList& parameters )
{
	const auto commandLine = QStringList( program ) + parameters;
	QString script = QStringLiteral("do shell script \"") + program + QStringLiteral(" ") +
			commandLine.join(QStringLiteral(" ")) + QStringLiteral("  > /dev/null 2>&1 &\" with administrator privileges");
	 runAppleScript(script);
	return true;
}



bool MacOsCoreFunctions::runProgramAsUser( const QString& program, const QStringList& parameters,
										   const QString& username, const QString& desktop )
{
	Q_UNUSED(desktop)

	class UserProcess : public QProcess // clazy:exclude=missing-qobject-macro
	{
	public:
		explicit UserProcess( uid_t uid, QObject* parent = nullptr ) :
			QProcess( parent ),
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

	const auto uid = MacOsUserFunctions::userIdFromName( username );
	if( uid <= 0 )
	{
		return false;
	}

	auto process = new UserProcess( uid );
	QObject::connect( process, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ), &QProcess::deleteLater );
	process->start( program, parameters );

	return true;
}



QString MacOsCoreFunctions::genericUrlHandler() const
{
	return QStringLiteral( "open" );
}



bool MacOsCoreFunctions::runAppleScript(const QString &script) {
	QString osascriptBin = QStringLiteral("/usr/bin/osascript");
	QStringList processArguments;
	processArguments << QStringLiteral("-l") << QStringLiteral("AppleScript");
	QProcess p;
	p.start(osascriptBin, processArguments);
	p.write(script.toUtf8());
	p.closeWriteChannel();
	p.waitForReadyRead(-1);
	QByteArray result = p.readAll();
	return true;
}