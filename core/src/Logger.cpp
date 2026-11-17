// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>

#include "Filesystem.h"
#include "Logger.h"
#include "PlatformCoreFunctions.h"
#include "PlatformFilesystemFunctions.h"
#include "VeyonConfiguration.h"

QAtomicPointer<Logger> Logger::s_instance = nullptr;
QMutex Logger::s_instanceMutex;


Logger::Logger(const QString& appName) :
	m_logLevel(LogLevel::Default),
	m_logMutex(),
	m_appName(QStringLiteral("Veyon") + appName),
	m_logFile(nullptr),
	m_logFileSizeLimit(-1),
	m_logFileRotationCount(-1)
{
	s_instanceMutex.lock();

	Q_ASSERT(s_instance == nullptr);

	s_instance = this;
	s_instanceMutex.unlock();

	m_logToSystem = VeyonCore::config().logToSystem();
	m_logToStdErr = VeyonCore::config().logToStdErr();

	auto configuredLogLevel = VeyonCore::config().logLevel();
	if (qEnvironmentVariableIsSet(logLevelEnvironmentVariable()))
	{
		configuredLogLevel = logLevelFromString(qEnvironmentVariable(logLevelEnvironmentVariable()));
	}

	m_logLevel = qBound(LogLevel::Min, configuredLogLevel, LogLevel::Max);

	// before the log file so that a reason for not using it reaches the system log
	VeyonCore::platform().coreFunctions().initNativeLoggingSystem(appName);

	if (m_logLevel > LogLevel::Nothing)
	{
		initLogFile();
	}

	qInstallMessageHandler(qtMsgHandler);

	if (QCoreApplication::instance())
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

	qInstallMessageHandler(nullptr);

	s_instanceMutex.lock();
	s_instance = nullptr;
	s_instanceMutex.unlock();

	QMutexLocker l(&m_logMutex);
	delete m_logFile;
}



Logger::LogLevel Logger::logLevelFromString(const QString& logLevelString)
{
	if (logLevelString.startsWith(QLatin1String("debug")))
	{
		return LogLevel::Debug;
	}

	if (logLevelString.startsWith(QLatin1String("info")))
	{
		return LogLevel::Info;
	}

	if (logLevelString.startsWith(QLatin1String("warn")))
	{
		return LogLevel::Warning;
	}

	if (logLevelString.startsWith(QLatin1String("err")))
	{
		return LogLevel::Error;
	}

	if (logLevelString.startsWith(QLatin1String("crit")))
	{
		return LogLevel::Critical;
	}

	return LogLevel(logLevelString.toUInt());
}



void Logger::initLogFile()
{
	// the platform decides by the privileges of the process: protected system directory or a
	// per-user directory below the configured one
	const auto logFileDirectory = VeyonCore::platform().filesystemFunctions().prepareLogFileDirectory(
		VeyonCore::filesystem().expandPath(VeyonCore::config().logFileDirectory()));

	if (logFileDirectory.path.isEmpty())
	{
		reportLogFileError(logFileDirectory.error);
		return;
	}

	m_logFilePath = logFileDirectory.path + QDir::separator() + QStringLiteral("%1.log").arg(m_appName);
	m_logFile = new QFile(m_logFilePath);

	openLogFile();

	if (VeyonCore::config().logFileSizeLimitEnabled())
	{
		m_logFileSizeLimit = VeyonCore::config().logFileSizeLimit() * 1024 * 1024;
	}

	if (VeyonCore::config().logFileRotationEnabled())
	{
		m_logFileRotationCount = VeyonCore::config().logFileRotationCount();
	}
}



void Logger::openLogFile()
{
	if (m_logFile == nullptr)
	{
		return;
	}

	m_logFile->setFileName(m_logFilePath);

	if (VeyonCore::platform().filesystemFunctions().openFileSafely(
			m_logFile,
			QFile::WriteOnly | QFile::Append | QFile::Unbuffered | QFile::Text,
			QFile::ReadOwner | QFile::WriteOwner) == false)
	{
		// openFileSafely() refuses symlinks, files of other users and non-regular files
		reportLogFileError(QStringLiteral("%1 could not be opened safely (symlink, owned by another user, "
										  "not a regular file or not accessible)")
							   .arg(m_logFilePath));
		m_logFile->close();
		delete m_logFile;
		m_logFile = nullptr;
	}
}



void Logger::reportLogFileError(const QString& error)
{
	if (m_logFileErrorReported)
	{
		return;
	}

	m_logFileErrorReported = true;

	// written directly (stderr and Unix: syslog) as this may happen while a message is being logged
	const auto message = QStringLiteral("%1: file logging disabled: %2").arg(m_appName, error);
	logEarlyCritical(message.toUtf8().constData());

	if (m_logToSystem)
	{
		VeyonCore::platform().coreFunctions().writeToNativeLoggingSystem(message, LogLevel::Critical);
	}
}



void Logger::closeLogFile()
{
	if (m_logFile)
	{
		m_logFile->close();
	}
}



void Logger::clearLogFile()
{
	closeLogFile();
	if (m_logFile)
	{
		QFile::remove(m_logFilePath);
	}
	openLogFile();
}



void Logger::rotateLogFile()
{
	if (m_logFileRotationCount < 1 || m_logFile == nullptr)
	{
		return;
	}

	closeLogFile();

	const QFileInfo logFileInfo(m_logFilePath);
	const QStringList logFileFilter({logFileInfo.fileName() + QStringLiteral(".*")});

	auto rotatedLogFiles = logFileInfo.dir().entryList(logFileFilter, QDir::NoFilter, QDir::Name);

	while (rotatedLogFiles.isEmpty() == false &&
		   rotatedLogFiles.count() >= m_logFileRotationCount)
	{
		logFileInfo.dir().remove(rotatedLogFiles.takeLast());
	}

	for (auto it = rotatedLogFiles.crbegin(), end = rotatedLogFiles.crend(); it != end; ++it)
	{
		bool numberOk = false;
		int logFileIndex = it->section(QLatin1Char('.'), -1).toInt(&numberOk);
		if (numberOk)
		{
			const auto oldFileName = QString(QStringLiteral("%1.%2")).arg(m_logFilePath).arg(logFileIndex);
			const auto newFileName = QString(QStringLiteral("%1.%2")).arg(m_logFilePath).arg(logFileIndex + 1);
			QFile::rename(oldFileName, newFileName);
		}
		else
		{
			// remove stale log file
			logFileInfo.dir().remove(*it);
		}
	}

	QFile::rename(m_logFilePath, m_logFilePath + QStringLiteral(".0"));

	openLogFile();
}



QString Logger::formatMessage(LogLevel ll, const QString& message)
{
	QString messageType;
	switch (ll)
	{
	case LogLevel::Debug: messageType = QStringLiteral("DEBUG"); break;
	case LogLevel::Info: messageType = QStringLiteral("INFO"); break;
	case LogLevel::Warning: messageType = QStringLiteral("WARN"); break;
	case LogLevel::Error: messageType = QStringLiteral("ERR"); break;
	case LogLevel::Critical: messageType = QStringLiteral("CRIT"); break;
	default: break;
	}

	return QStringLiteral("%1.%2: [%3] %4\n").arg(QDateTime::currentDateTime().toString(Qt::ISODate), QDateTime::currentDateTime().toString(QStringLiteral("zzz")), messageType, message.trimmed());
}



void Logger::qtMsgHandler(QtMsgType messageType, const QMessageLogContext& context, const QString& message)
{
	QMutexLocker instanceLocker(&s_instanceMutex);

	const auto instance = s_instance.loadAcquire();

	if (instance == nullptr || message.size() > MaximumMessageSize)
	{
		return;
	}

	LogLevel logLevel = LogLevel::Default;

	switch (messageType)
	{
	case QtDebugMsg: logLevel = LogLevel::Debug; break;
	case QtInfoMsg: logLevel = LogLevel::Info; break;
	case QtWarningMsg: logLevel = LogLevel::Warning; break;
	case QtCriticalMsg: logLevel = LogLevel::Error; break;
	case QtFatalMsg: logLevel = LogLevel::Critical; break;
	}

	if (context.category && strcmp(context.category, "default") != 0)
	{
		instance->log(logLevel, QStringLiteral("[%1] ").arg(QLatin1String(context.category)) + message);
	}
	else
	{
		instance->log(logLevel, message);
	}
}



void Logger::log(LogLevel logLevel, const QString& message)
{
	if (m_logLevel >= logLevel)
	{
		QMutexLocker l(&m_logMutex);
		if (message == m_lastMessage && logLevel == m_lastMessageLevel)
		{
			++m_lastMessageCount;
		}
		else
		{
			if (m_lastMessageCount)
			{
				outputMessage(formatMessage(m_lastMessageLevel, QStringLiteral("---")));
				outputMessage(formatMessage(m_lastMessageLevel, QStringLiteral("Last message repeated %1 times").arg(m_lastMessageCount)));
				outputMessage(formatMessage(m_lastMessageLevel, QStringLiteral("---")));
				m_lastMessageCount = 0;
			}
			outputMessage(formatMessage(logLevel, message));

			if (m_logToSystem)
			{
				VeyonCore::platform().coreFunctions().writeToNativeLoggingSystem(message, logLevel);
			}

			m_lastMessage = message;
			m_lastMessageLevel = logLevel;
		}
	}
}



void Logger::outputMessage(const QString& message)
{
	if (m_logFile)
	{
		m_logFile->write(message.toUtf8());
		m_logFile->flush();

		if (m_logFileSizeLimit > 0 &&
			m_logFile->size() > m_logFileSizeLimit)
		{
			if (m_logFileRotationCount > 0)
			{
				rotateLogFile();
			}
			else
			{
				clearLogFile();
			}
		}
	}

	if (m_logToStdErr)
	{
		fprintf(stderr, "%s", message.toUtf8().constData());
		fflush(stderr);
	}
}
