/*
 * Logger.cpp - a global clas for easily logging messages to log files
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#include <italcconfig.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>

#include "ItalcConfiguration.h"
#include "Logger.h"
#include "LocalSystem.h"

#ifdef ITALC_BUILD_WIN32
#include "3rdparty/XEventLog.h"
#include "3rdparty/XEventLog.cpp"
#endif

Logger::LogLevel Logger::logLevel = Logger::LogLevelDefault;
Logger *Logger::instance = NULL;
QMutex Logger::logMutex( QMutex::Recursive );
Logger::LogLevel Logger::lastMsgLevel = Logger::LogLevelNothing;
QString Logger::lastMsg;
int Logger::lastMsgCount = 0;
#ifdef ITALC_BUILD_WIN32
CXEventLog *Logger::winEventLog = NULL;
#endif

Logger::Logger( const QString &appName ) :
	m_appName( appName ),
	m_logFile( NULL )
{
	instance = this;

	int ll = ItalcCore::config->logLevel();
	logLevel = qBound( LogLevelMin, static_cast<LogLevel>( ll ), LogLevelMax );
	initLogFile();

	qInstallMsgHandler( qtMsgHandler );

#ifdef ITALC_BUILD_WIN32
	if( ItalcCore::config->logToWindowsEventLog() )
	{
		winEventLog = new CXEventLog( appName.toUtf8().constData() );
	}
#endif

	QString user = LocalSystem::User::loggedOnUser().name();

	if( QCoreApplication::instance() )
	{
		// log current application start up
		LogStream() << "Startup for user" << user
					<< "with arguments" << QCoreApplication::arguments();
	}
	else
	{
		LogStream() << "Startup for user" << user
					<< "without QCoreApplication instance";
	}
}




Logger::~Logger()
{
	LogStream() << "Shutdown";

	instance = NULL;

	delete m_logFile;
}




void Logger::initLogFile()
{
	QString logPath = LocalSystem::Path::expand( ItalcCore::config->logFileDirectory() );

	if( !QDir( logPath ).exists() )
	{
		if( QDir( QDir::rootPath() ).mkdir( logPath ) )
		{
			QFile::setPermissions( logPath,
						QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
						QFile::ReadUser | QFile::WriteUser | QFile::ExeUser |
						QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup |
						QFile::ReadOther | QFile::WriteOther | QFile::ExeOther );
		}
	}

	logPath = logPath + QDir::separator();
	m_logFile = new QFile( logPath + QString( "%1.log" ).arg( m_appName ) );
	m_logFile->open( QFile::WriteOnly | QFile::Append | QFile::Unbuffered );
	m_logFile->setPermissions( QFile::ReadOwner | QFile::WriteOwner );
}




QString Logger::formatMessage( LogLevel ll, const QString &msg )
{
#ifdef ITALC_BUILD_WIN32
	static const char *linebreak = "\r\n";
#else
	static const char *linebreak = "\n";
#endif

	QString msgType;
	switch( ll )
	{
		case LogLevelDebug: msgType = "DEBUG"; break;
		case LogLevelInfo: msgType = "INFO"; break;
		case LogLevelWarning: msgType = "WARN"; break;
		case LogLevelError: msgType = "ERR"; break;
		case LogLevelCritical: msgType = "CRIT"; break;
		default: break;
	}

	return QString( "%1: [%2] %3%4" ).
				arg( QDateTime::currentDateTime().toString() ).
				arg( msgType ).
				arg( msg.trimmed() ).
				arg( linebreak );
}




void Logger::qtMsgHandler( QtMsgType msgType, const char *msg )
{
	LogLevel ll = LogLevelDefault;

	switch( msgType )
	{
		case QtDebugMsg: ll = LogLevelDebug; break;
		case QtWarningMsg: ll = LogLevelWarning; break;
		case QtCriticalMsg: ll = LogLevelError; break;
		case QtFatalMsg: ll = LogLevelCritical; break;
		default:
			break;
	}

	log( ll, msg );
}




void Logger::log( LogLevel ll, const QString &msg )
{
	if( instance != NULL && logLevel >= ll )
	{
		QMutexLocker l( &logMutex );
		if( msg == lastMsg && ll == lastMsgLevel )
		{
			++lastMsgCount;
		}
		else
		{
			if( lastMsgCount )
			{
				instance->outputMessage( formatMessage( lastMsgLevel, "---" ) );
				instance->outputMessage( formatMessage( lastMsgLevel,
				QString( "Last message repeated %1 times" ).
					arg( lastMsgCount ) ) );
				instance->outputMessage( formatMessage( lastMsgLevel, "---" ) );
				lastMsgCount = 0;
			}
			instance->outputMessage( formatMessage( ll, msg ) );
#ifdef ITALC_BUILD_WIN32
			WORD wtype = -1;
			switch( ll )
			{
				case LogLevelCritical:
				case LogLevelError: wtype = EVENTLOG_ERROR_TYPE; break;
				case LogLevelWarning: wtype = EVENTLOG_WARNING_TYPE; break;
				//case LogLevelInfo: wtype = EVENTLOG_INFORMATION_TYPE; break;
				default:
					break;
			}
			if( winEventLog != NULL && wtype > 0 )
			{
				winEventLog->Write( wtype, msg.toUtf8().constData() );
			}
#endif
			lastMsg = msg;
			lastMsgLevel = ll;
		}
	}
}




void Logger::log( LogLevel ll, const char *format, ... )
{
	va_list args;
	va_start( args, format );

	QString message;
	message.vsprintf( format, args );

	va_end(args);

	log( ll, message );
}




void Logger::outputMessage( const QString &msg )
{
	QMutexLocker l( &logMutex );

	if( m_logFile )
	{
		m_logFile->write( msg.toUtf8() );
		m_logFile->flush();
	}

	if( !ItalcCore::config || ItalcCore::config->logToStdErr() )
	{
		fprintf( stderr, "%s", msg.toUtf8().constData() );
		fflush( stderr );
	}
}


