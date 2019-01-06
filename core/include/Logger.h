/*
 * Logger.h - a global clas for easily logging messages to log files
 *
 * Copyright (c) 2010-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef VEYONCORE_LOGGER_H
#define VEYONCORE_LOGGER_H

#include <QMutex>
#include <QTextStream>

#include "VeyonCore.h"

class QFile;

// clazy:excludeall=rule-of-three

class VEYON_CORE_EXPORT Logger
{
public:
	enum LogLevels
	{
		LogLevelNothing,
		LogLevelCritical,
		LogLevelError,
		LogLevelWarning,
		LogLevelInfo,
		LogLevelDebug,
		NumLogLevels,
		LogLevelMin = LogLevelNothing+1,
		LogLevelMax = NumLogLevels-1,
		LogLevelDefault = LogLevelInfo
	} ;

	typedef LogLevels LogLevel;

	Logger( const QString &appName );
	~Logger();

	static const char* logLevelEnvironmentVariable()
	{
		return "VEYON_LOG_LEVEL";
	}

	LogLevel logLevel() const
	{
		return m_logLevel;
	}


private:
	void initLogFile();
	void openLogFile();
	void closeLogFile();
	void clearLogFile();
	void rotateLogFile();

	void log( LogLevel logLevel, const QString& message );
	void outputMessage( const QString& message );

	static QString formatMessage( LogLevel ll, const QString &msg );
	static void qtMsgHandler( QtMsgType msgType, const QMessageLogContext &, const QString& msg );

	static QAtomicPointer<Logger> s_instance;
	static QMutex s_instanceMutex;

	LogLevel m_logLevel;
	QMutex m_logMutex;

	LogLevel m_lastMessageLevel;
	QString m_lastMessage;
	int m_lastMessageCount;
	bool m_logToSystem;

	QString m_appName;

	QFile *m_logFile;
	int m_logFileSizeLimit;
	int m_logFileRotationCount;

} ;

#endif
