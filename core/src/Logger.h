/*
 * Logger.h - a global clas for easily logging messages to log files
 *
 * Copyright (c) 2010-2021 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include <QMutex>
#include <QTextStream>

#include "VeyonCore.h"

class QFile;

// clazy:excludeall=rule-of-three

class VEYON_CORE_EXPORT Logger
{
	Q_GADGET
public:
	enum class LogLevel
	{
		Nothing,
		Critical,
		Error,
		Warning,
		Info,
		Debug,
		NumLogLevels,
		Min = Nothing+1,
		Max = NumLogLevels-1,
		Default = Warning
	};
	Q_ENUM(LogLevel)

	static constexpr int DefaultFileSizeLimit = 100;
	static constexpr int DefaultFileRotationCount = 10;
	static constexpr int MaximumMessageSize = 1000;
	static constexpr const char* DefaultLogFileDirectory = "$TEMP";

	explicit Logger( const QString &appName );
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
