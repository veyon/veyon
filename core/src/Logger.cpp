/*
 * Logger.cpp - a global clas for easily logging messages to log files
 *
 * Copyright (c) 2010-2020 Tobias Junghans <tobydox@veyon.io>
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
#include <QDateTime>
#include <QDir>
#include <QFile>

#include "VeyonConfiguration.h"
#include "Filesystem.h"
#include "Logger.h"
#include "PlatformCoreFunctions.h"

QAtomicPointer<Logger> Logger::s_instance = nullptr;
QMutex Logger::s_instanceMutex;


Logger::Logger( const QString &appName ) :
	m_appName( QStringLiteral( "Veyon" ) + appName )
{
	s_instanceMutex.lock();

	Q_ASSERT(s_instance == nullptr);

	s_instance = this;
	s_instanceMutex.unlock();

	auto configuredLogLevel = VeyonCore::config().logLevel();
	if( qEnvironmentVariableIsSet( logLevelEnvironmentVariable() ) )
	{
		configuredLogLevel = static_cast<LogLevel>( qEnvironmentVariableIntValue( logLevelEnvironmentVariable() ) );
	}

	m_logLevel = qBound( LogLevel::Min, configuredLogLevel, LogLevel::Max );
	m_logToSystem = VeyonCore::config().logToSystem();

	if( m_logLevel > LogLevel::Nothing )
	{
		initLogFile();
	}

	qInstallMessageHandler( qtMsgHandler );

	VeyonCore::platform().coreFunctions().initNativeLoggingSystem( appName );

	if( QCoreApplication::instance() )
	{
		// log current application start up
		vDebug() << "Startup with arguments" << QCoreApplication::arguments();
	}
	else
	{
		vDebug() << "Startup without QCoreApplication instance";
	}
}




Logger::~Logger()
{
	vDebug() << "Shutdown";

	QMutexLocker l( &m_logMutex );

	qInstallMessageHandler(nullptr);

	s_instanceMutex.lock();
	s_instance = nullptr;
	s_instanceMutex.unlock();

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
		static constexpr auto BytesPerKB = 1024;
		m_logFileSizeLimit = VeyonCore::config().logFileSizeLimit() * BytesPerKB * BytesPerKB;
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

	for( auto it = rotatedLogFiles.crbegin(), end = rotatedLogFiles.crend(); it != end; ++it )
	{
		bool numberOk = false;
		int logFileIndex = it->section( QLatin1Char('.'), -1 ).toInt( &numberOk );
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




QString Logger::formatMessage( LogLevel ll, const QString& message )
{
	QString messageType;
	switch( ll )
	{
	case LogLevel::Debug: messageType = QStringLiteral( "DEBUG" ); break;
	case LogLevel::Info: messageType = QStringLiteral( "INFO" ); break;
	case LogLevel::Warning: messageType = QStringLiteral( "WARN" ); break;
	case LogLevel::Error: messageType = QStringLiteral( "ERR" ); break;
	case LogLevel::Critical: messageType = QStringLiteral( "CRIT" ); break;
	default: break;
	}

	return QStringLiteral( "%1.%2: [%3] %4\n" ).arg(
				QDateTime::currentDateTime().toString( Qt::ISODate ),
				QDateTime::currentDateTime().toString( QStringLiteral( "zzz" ) ),
				messageType,
				message.trimmed() );
}




void Logger::qtMsgHandler( QtMsgType messageType, const QMessageLogContext& context, const QString& message )
{
	QMutexLocker instanceLocker( &s_instanceMutex );

	const auto instance = s_instance.loadAcquire();

	if( instance == nullptr )
	{
		return;
	}

	LogLevel logLevel = LogLevel::Default;

	switch( messageType )
	{
	case QtDebugMsg: logLevel = LogLevel::Debug; break;
	case QtInfoMsg: logLevel = LogLevel::Info; break;
	case QtWarningMsg: logLevel = LogLevel::Warning; break;
	case QtCriticalMsg: logLevel = LogLevel::Error; break;
	case QtFatalMsg: logLevel = LogLevel::Critical; break;
	}

	if( context.category && strcmp(context.category, "default") != 0 )
	{
		instance->log( logLevel, QStringLiteral( "[%1] " ).arg(QLatin1String(context.category)) + message );
	}
	else
	{
		instance->log( logLevel, message );
	}
}




void Logger::log( LogLevel logLevel, const QString& message )
{
	if( m_logLevel >= logLevel )
	{
		QMutexLocker l( &m_logMutex );
		if( message == m_lastMessage && logLevel == m_lastMessageLevel )
		{
			++m_lastMessageCount;
		}
		else
		{
			if( m_lastMessageCount )
			{
				outputMessage( formatMessage( m_lastMessageLevel, QStringLiteral( "---" ) ) );
				outputMessage( formatMessage( m_lastMessageLevel, QStringLiteral( "Last message repeated %1 times" ).arg( m_lastMessageCount ) ) );
				outputMessage( formatMessage( m_lastMessageLevel, QStringLiteral( "---" ) ) );
				m_lastMessageCount = 0;
			}
			outputMessage( formatMessage( logLevel, message ) );

			if( m_logToSystem )
			{
				VeyonCore::platform().coreFunctions().writeToNativeLoggingSystem( message, logLevel );
			}

			m_lastMessage = message;
			m_lastMessageLevel = logLevel;
		}
	}
}



void Logger::outputMessage( const QString& message )
{
	if( m_logFile )
	{
		m_logFile->write( message.toUtf8() );
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
		fprintf( stderr, "%s", message.toUtf8().constData() );
		fflush( stderr );
	}
}
