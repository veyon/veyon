// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

// LinuxUserFunctions::setUserEnvironment(): HOME, USER, LOGNAME and SHELL of the server environment
// are taken from the password database instead of the merged session environment

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

#include "LinuxUserFunctions.h"

#include <pwd.h>
#include <unistd.h>

class LinuxUserFunctionsTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void setUserEnvironment()
	{
		const auto pwEntry = getpwuid(getuid());
		QVERIFY(pwEntry != nullptr);

		const auto home = QDir::cleanPath(QString::fromUtf8(pwEntry->pw_dir));
		const auto userName = QString::fromUtf8(pwEntry->pw_name);

		QProcessEnvironment environment;
		environment.insert(QStringLiteral("HOME"), QStringLiteral("/"));
		environment.insert(QStringLiteral("USER"), QStringLiteral("root-helper"));
		environment.insert(QStringLiteral("LOGNAME"), QStringLiteral("root-helper"));
		environment.insert(QStringLiteral("XDG_CONFIG_HOME"), QStringLiteral("//.config"));
		environment.insert(QStringLiteral("XDG_CACHE_HOME"), home + QStringLiteral("/../other/.cache"));
		environment.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/.local/share"));
		environment.insert(QStringLiteral("XDG_RUNTIME_DIR"), QStringLiteral("/run/user/1000"));

		QVERIFY(LinuxUserFunctions::setUserEnvironment(environment, getuid()));

		QCOMPARE(environment.value(QStringLiteral("HOME")), home);
		QCOMPARE(environment.value(QStringLiteral("USER")), userName);
		QCOMPARE(environment.value(QStringLiteral("LOGNAME")), userName);
		QCOMPARE(environment.contains(QStringLiteral("XDG_DATA_HOME")), home != QLatin1String("/"));

		if (home != QLatin1String("/"))
		{
			QVERIFY(environment.contains(QStringLiteral("XDG_CONFIG_HOME")) == false);
			QVERIFY(environment.contains(QStringLiteral("XDG_CACHE_HOME")) == false);
			QCOMPARE(environment.value(QStringLiteral("XDG_DATA_HOME")), home + QStringLiteral("/.local/share"));
		}

		// directories outside the home directory are kept if owned by the user
		QTemporaryDir userDirectory;
		QVERIFY(userDirectory.isValid());
		QVERIFY(QFileInfo(userDirectory.path()).ownerId() == getuid());

		// the home directory may also be reached through a symlink
		QTemporaryDir linkDirectory;
		QVERIFY(linkDirectory.isValid());
		const auto homeLink = linkDirectory.filePath(QStringLiteral("home"));
		QVERIFY(QFile::link(home, homeLink));

		environment.insert(QStringLiteral("XDG_CACHE_HOME"), userDirectory.path());
		environment.insert(QStringLiteral("XDG_STATE_HOME"), homeLink);
		environment.insert(QStringLiteral("XDG_CONFIG_HOME"), QStringLiteral(".config"));

		QVERIFY(LinuxUserFunctions::setUserEnvironment(environment, getuid()));

		QCOMPARE(environment.value(QStringLiteral("XDG_CACHE_HOME")), userDirectory.path());
		QCOMPARE(environment.value(QStringLiteral("XDG_STATE_HOME")), homeLink);
		QVERIFY(environment.contains(QStringLiteral("XDG_CONFIG_HOME")) == false);

		// other variables are left untouched
		QCOMPARE(environment.value(QStringLiteral("XDG_RUNTIME_DIR")), QStringLiteral("/run/user/1000"));
	}

	void setUserEnvironmentInvalidUser()
	{
		QProcessEnvironment environment;
		environment.insert(QStringLiteral("HOME"), QStringLiteral("/"));

		QVERIFY(LinuxUserFunctions::setUserEnvironment(environment, LinuxUserFunctions::InvalidUserId - 1) == false);
		QCOMPARE(environment.value(QStringLiteral("HOME")), QStringLiteral("/"));
	}
};

QTEST_GUILESS_MAIN(LinuxUserFunctionsTest)
#include "LinuxUserFunctionsTest.moc"
