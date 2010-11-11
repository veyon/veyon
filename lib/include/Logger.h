/*
 * Logger.h - a global clas for easily logging messages to log files
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

#ifndef _LOGGER_H
#define _LOGGER_H

#include <italcconfig.h>

#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QMutex>

class QFile;
class CXEventLog;

class Logger
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

	static void log( LogLevel ll, const QString &msg );
	static void log( LogLevel ll, const char *format, ... );


private:
	void initLogFile();
	void outputMessage( const QString &msg );

	static QString formatMessage( LogLevel ll, const QString &msg );
	static void qtMsgHandler( QtMsgType msgType, const char *msg );

	static LogLevel logLevel;
	static Logger *instance;
	static QMutex logMutex;

	static LogLevel lastMsgLevel;
	static QString lastMsg;
	static int lastMsgCount;

	QString m_appName;

#ifdef ITALC_BUILD_WIN32
	static CXEventLog *winEventLog;
#endif

	QFile *m_logFile;

} ;

#define ilog(ll, msg) Logger::log(Logger::LogLevel##ll, msg )
#define ilogf(ll, format, ...) Logger::log(Logger::LogLevel##ll, format, __VA_ARGS__)
#define ilog_failed(what) ilogf( Warning, "%s: %s failed", __PRETTY_FUNCTION__, what )
#define ilog_failedf(what, format, ...) ilogf( Warning, "%s: %s failed: " format, __PRETTY_FUNCTION__, what, __VA_ARGS__ )


// helper class for easily streaming Qt datatypes into logfiles
class LogStream : public QTextStream
{
public:
	LogStream( Logger::LogLevel ll = Logger::LogLevelInfo ) :
		QTextStream(),
		m_logLevel( ll ),
		m_out()
	{
		setString( &m_out );
	}

	~LogStream()
	{
		flush();
		Logger::log( m_logLevel, m_out );
	}

	// allows to stream advanced data types such as lists, maps etc. by
	// using QDebug's stream operator
	template<class QDEBUG_STREAMABLE>
	LogStream &operator<<( const QDEBUG_STREAMABLE &s )
	{
		QDebug( &m_out ) << s;
		return *this;
	}


private:
	Logger::LogLevel m_logLevel;
	QString m_out;

} ;

#endif
