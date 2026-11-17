// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

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
	enum class LogLevel {
		Nothing,
		Critical,
		Error,
		Warning,
		Info,
		Debug,
		NumLogLevels,
		Min = Nothing + 1,
		Max = NumLogLevels - 1,
		Default = Warning
	};
	Q_ENUM(LogLevel)

	static constexpr int DefaultFileSizeLimit = 100;
	static constexpr int DefaultFileRotationCount = 10;
	static constexpr int MaximumMessageSize = 16384;
	static constexpr const char* DefaultLogFileDirectory = "%TEMP%";

	explicit Logger(const QString& appName);
	~Logger();

	static const char* logLevelEnvironmentVariable()
	{
		return "VEYON_LOG_LEVEL";
	}

	LogLevel logLevel() const
	{
		return m_logLevel;
	}

	static LogLevel logLevelFromString(const QString& logLevelString);

	// writes a line to stderr and the system log (Unix: syslog/journal, Windows: debugger output)
	// without any Qt logging, for failures before any Logger instance or QCoreApplication exists
	static void logEarlyCritical(const char* message);

private:
	void initLogFile();
	void openLogFile();
	void closeLogFile();
	void clearLogFile();
	void rotateLogFile();

	void log(LogLevel logLevel, const QString& message);
	void outputMessage(const QString& message);

	static QString formatMessage(LogLevel ll, const QString& msg);
	static void qtMsgHandler(QtMsgType msgType, const QMessageLogContext&, const QString& msg);

	static QAtomicPointer<Logger> s_instance;
	static QMutex s_instanceMutex;

	LogLevel m_logLevel;
	QMutex m_logMutex;

	LogLevel m_lastMessageLevel{LogLevel::Nothing};
	QString m_lastMessage{};
	int m_lastMessageCount{0};
	bool m_logToStdErr{false};
	bool m_logToSystem{false};

	QString m_appName;

	QFile* m_logFile;
	// QFile::fileName() is not reliable after opening a file from a descriptor (openFileSafely())
	QString m_logFilePath;
	int m_logFileSizeLimit;
	int m_logFileRotationCount;
};
