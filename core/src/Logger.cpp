/*
 * Logger.cpp - a global clas for easily logging messages to log files
 *
 * Copyright (c) 2010-2018 Tobias Junghans <tobydox@users.sf.net>
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

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>

#include "VeyonConfiguration.h"
#include "Filesystem.h"
#include "Logger.h"
#include "PlatformCoreFunctions.h"

Logger::LogLevel Logger::logLevel = Logger::LogLevelDefault;
Logger *Logger::instance = nullptr;
QMutex Logger::logMutex( QMutex::Recursive );
Logger::LogLevel Logger::lastMsgLevel = Logger::LogLevelNothing;
QString Logger::lastMsg;
int Logger::lastMsgCount = 0;
bool Logger::logToSystem = false;

Logger::Logger( const QString &appName ) :
	m_appName( QStringLiteral( "Veyon" ) + appName ),
	m_logFile( nullptr ),
	m_logFileSizeLimit( -1 ),
	m_logFileRotationCount( -1 )
{
	instance = this;

	auto configuredLogLevel = VeyonCore::config().logLevel();
	if( qEnvironmentVariableIsSet( logLevelEnvironmentVariable() ) )
	{
		configuredLogLevel = qgetenv( logLevelEnvironmentVariable() ).toInt();
	}

	logLevel = qBound( LogLevelMin, static_cast<LogLevel>( configuredLogLevel ), LogLevelMax );
	logToSystem = VeyonCore::config().logToSystem();

	initLogFile();

	qInstallMessageHandler( qtMsgHandler );

	VeyonCore::platform().coreFunctions().initNativeLoggingSystem( appName );

	if( QCoreApplication::instance() )
	{
		// log current application start up
		qDebug() << "Startup with arguments" << QCoreApplication::arguments();
	}
	else
	{
		qDebug() << "Startup without QCoreApplication instance";
	}
}




Logger::~Logger()
{
	qDebug( "Shutdown" );

	QMutexLocker l( &logMutex );

	qInstallMessageHandler(nullptr);

	instance = nullptr;

	delete m_logFile;
}




void Logger::initLogFile()
{
	QString logPath = VeyonCore::filesystem().expandPath( VeyonCore::config().logFileDirectory() );

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
	m_logFile = new QFile( logPath + QString( QStringLiteral( "%1.log" ) ).arg( m_appName ) );

	openLogFile();

	if( VeyonCore::config().logFileSizeLimitEnabled() )
	{
		m_logFileSizeLimit = VeyonCore::config().logFileSizeLimit() * 1024 * 1024;
	}

	if( VeyonCore::config().logFileRotationEnabled() )
	{
		m_logFileRotationCount = VeyonCore::config().logFileRotationCount();
	}
}



void Logger::openLogFile()
{
	m_logFile->open( QFile::WriteOnly | QFile::Append | QFile::Unbuffered | QFile::Text );
	m_logFile->setPermissions( QFile::ReadOwner | QFile::WriteOwner );
}



void Logger::closeLogFile()
{
	m_logFile->close();
}



void Logger::clearLogFile()
{
	closeLogFile();
	m_logFile->remove();
	openLogFile();
}



void Logger::rotateLogFile()
{
	if( m_logFileRotationCount < 1 )
	{
		return;
	}

	closeLogFile();

	const QFileInfo logFileInfo( *m_logFile );
	const QStringList logFileFilter( { logFileInfo.fileName() + QStringLiteral( ".*" ) } );

	auto rotatedLogFiles = logFileInfo.dir().entryList( logFileFilter, QDir::NoFilter, QDir::Name );

	while( rotatedLogFiles.isEmpty() == false &&
		   rotatedLogFiles.count() >= m_logFileRotationCount )
	{
		logFileInfo.dir().remove( rotatedLogFiles.takeLast() );
	}

#if QT_VERSION < 0x050600
#warning Building compat code for unsupported version of Qt
	typedef std::reverse_iterator<QStringList::const_iterator> QStringListReverseIterator;
	for( auto it = QStringListReverseIterator(rotatedLogFiles.cend()),
				end = QStringListReverseIterator(rotatedLogFiles.cbegin()); it != end; ++it )
#else
	for( auto it = rotatedLogFiles.crbegin(), end = rotatedLogFiles.crend(); it != end; ++it )
#endif
	{
		bool numberOk = false;
		int logFileIndex = it->section( '.', -1 ).toInt( &numberOk );
		if( numberOk )
		{
			const auto oldFileName = QString( QStringLiteral( "%1.%2" ) ).arg( m_logFile->fileName() ).arg( logFileIndex );
			const auto newFileName = QString( QStringLiteral( "%1.%2" ) ).arg( m_logFile->fileName() ).arg( logFileIndex + 1 );
			QFile::rename( oldFileName, newFileName );
		}
		else
		{
			// remove stale log file
			logFileInfo.dir().remove( *it );
		}
	}

	QFile::rename( m_logFile->fileName(), m_logFile->fileName() + QStringLiteral( ".0" ) );

	openLogFile();
}




QString Logger::formatMessage( LogLevel ll, const QString &msg )
{
	QString msgType;
	switch( ll )
	{
		case LogLevelDebug: msgType = QStringLiteral( "DEBUG" ); break;
		case LogLevelInfo: msgType = QStringLiteral( "INFO" ); break;
		case LogLevelWarning: msgType = QStringLiteral( "WARN" ); break;
		case LogLevelError: msgType = QStringLiteral( "ERR" ); break;
		case LogLevelCritical: msgType = QStringLiteral( "CRIT" ); break;
		default: break;
	}

	return QString( QStringLiteral( "%1.%2: [%3] %4\n" ) ).arg(
				QDateTime::currentDateTime().toString( Qt::ISODate ),
				QDateTime::currentDateTime().toString( QStringLiteral( "zzz" ) ),
				msgType,
				msg.trimmed() );
}




void Logger::qtMsgHandler( QtMsgType msgType, const QMessageLogContext& context, const QString& msg )
{
	LogLevel ll = LogLevelDefault;

	switch( msgType )
	{
		case QtDebugMsg: ll = LogLevelDebug; break;
		case QtInfoMsg: ll = LogLevelInfo; break;
		case QtWarningMsg: ll = LogLevelWarning; break;
		case QtCriticalMsg: ll = LogLevelError; break;
		case QtFatalMsg: ll = LogLevelCritical; break;
		default:
			break;
	}

	if( context.category && strcmp(context.category, "default") != 0 )
	{
		log( ll, QString( QStringLiteral( "[%1] " ) ).arg(context.category) + msg );
	}
	else
	{
		log( ll, msg );
	}
}




void Logger::log( LogLevel ll, const QString &msg )
{
	if( instance != nullptr && logLevel >= ll )
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
				instance->outputMessage( formatMessage( lastMsgLevel, QStringLiteral( "---" ) ) );
				instance->outputMessage( formatMessage( lastMsgLevel, QString( QStringLiteral( "Last message repeated %1 times" ) ).arg( lastMsgCount ) ) );
				instance->outputMessage( formatMessage( lastMsgLevel, QStringLiteral( "---" ) ) );
				lastMsgCount = 0;
			}
			instance->outputMessage( formatMessage( ll, msg ) );

			if( logToSystem )
			{
				VeyonCore::platform().coreFunctions().writeToNativeLoggingSystem( msg, ll );
			}

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

		if( m_logFileSizeLimit > 0 &&
				m_logFile->size() > m_logFileSizeLimit )
		{
			if( m_logFileRotationCount > 0 )
			{
				rotateLogFile();
			}
			else
			{
				clearLogFile();
			}
		}
	}

	if( VeyonCore::config().logToStdErr() )
	{
		fprintf( stderr, "%s", msg.toUtf8().constData() );
		fflush( stderr );
	}
}
