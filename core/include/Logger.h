/*
 * Logger.h - a global clas for easily logging messages to log files
 *
 * Copyright (c) 2010-2017 Tobias Junghans <tobydox@users.sf.net>
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
class CXEventLog;

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

	Logger( const QString &appName, VeyonConfiguration* config );
	~Logger();

	static void log( LogLevel ll, const QString &msg );
	static void log( LogLevel ll, const char *format, ... );


private:
	void initLogFile( VeyonConfiguration* config );
	void openLogFile();
	void closeLogFile();
	void clearLogFile();
	void rotateLogFile();
	void outputMessage( const QString &msg );

	static QString formatMessage( LogLevel ll, const QString &msg );
	static void qtMsgHandler( QtMsgType msgType, const QMessageLogContext &, const QString& msg );

	static LogLevel logLevel;
	static Logger *instance;
	static QMutex logMutex;

	static LogLevel lastMsgLevel;
	static QString lastMsg;
	static int lastMsgCount;

	QString m_appName;

#ifdef VEYON_BUILD_WIN32
	static CXEventLog *winEventLog;
#endif

	QFile *m_logFile;
	int m_logFileSizeLimit;
	int m_logFileRotationCount;

} ;

#define ilog(ll, msg) Logger::log(Logger::LogLevel##ll, msg )
#define ilogf(ll, format, ...) Logger::log(Logger::LogLevel##ll, format, __VA_ARGS__)
#define ilog_failed(what) ilogf( Warning, "%s: %s failed", __PRETTY_FUNCTION__, what )
#define ilog_failedf(what, format, ...) ilogf( Warning, "%s: %s failed: " format, __PRETTY_FUNCTION__, what, __VA_ARGS__ )

#endif
