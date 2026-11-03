/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "GreeterEnvironment.h"
#include "login-helper/LoginSession.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>
#include <unistd.h>

class GreeterEnvironmentTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void logindDisplayOverridesStaleEnvironment();
	void defaultAuthorityIsMadeExplicit();
	void unsafeAuthorityIsNotUsed();
	void nativeWaylandIsPreserved();
	void userSessionIsUnchanged();
	void lightDmGreeterWithUserClassUsesLogindDisplay();
};

static LoginSession greeter()
{
	LoginSession session;
	session.sessionClass = QStringLiteral("greeter");
	session.type = QStringLiteral("x11");
	session.display = QStringLiteral(":7");
	session.uid = uint(::getuid());
	return session;
}

void GreeterEnvironmentTest::logindDisplayOverridesStaleEnvironment()
{
	QProcessEnvironment environment;
	environment.insert(QStringLiteral("DISPLAY"), QStringLiteral(":99"));
	environment.insert(QStringLiteral("WAYLAND_DISPLAY"), QStringLiteral("wayland-1"));
	const auto result = GreeterEnvironment::prepare(greeter(), environment);
	QCOMPARE(result.value(QStringLiteral("DISPLAY")), QStringLiteral(":7"));
	QCOMPARE(result.value(QStringLiteral("XDG_SESSION_TYPE")), QStringLiteral("x11"));
	QVERIFY(!result.contains(QStringLiteral("WAYLAND_DISPLAY")));

	// LightDM PAM can omit Type even though logind knows the X display.
	auto session = greeter();
	session.type = QStringLiteral("unspecified");
	environment.remove(QStringLiteral("WAYLAND_DISPLAY"));
	QCOMPARE(GreeterEnvironment::prepare(session, environment).value(QStringLiteral("DISPLAY")), QStringLiteral(":7"));
	environment.insert(QStringLiteral("WAYLAND_DISPLAY"), QStringLiteral("wayland-1"));
	environment.insert(QStringLiteral("XDG_SESSION_TYPE"), QStringLiteral("x11"));
	const auto explicitX11 = GreeterEnvironment::prepare(session, environment);
	QCOMPARE(explicitX11.value(QStringLiteral("DISPLAY")), QStringLiteral(":7"));
	QVERIFY(!explicitX11.contains(QStringLiteral("WAYLAND_DISPLAY")));
}

void GreeterEnvironmentTest::defaultAuthorityIsMadeExplicit()
{
	QTemporaryDir home;
	QVERIFY(home.isValid());
	const auto authority = home.filePath(QStringLiteral(".Xauthority"));
	QFile file(authority);
	QVERIFY(file.open(QIODevice::WriteOnly));
	file.close();
	QVERIFY(file.setPermissions(QFile::ReadOwner | QFile::WriteOwner));
	QProcessEnvironment environment;
	environment.insert(QStringLiteral("HOME"), home.path());
	QCOMPARE(GreeterEnvironment::prepare(greeter(), environment).value(QStringLiteral("XAUTHORITY")), authority);

	// An explicit cookie takes precedence over the home-directory default.
	const auto explicitAuthority = home.filePath(QStringLiteral("cookie"));
	QVERIFY(QFile::copy(authority, explicitAuthority));
	environment.insert(QStringLiteral("XAUTHORITY"), explicitAuthority);
	QCOMPARE(GreeterEnvironment::prepare(greeter(), environment).value(QStringLiteral("XAUTHORITY")), explicitAuthority);
}

void GreeterEnvironmentTest::unsafeAuthorityIsNotUsed()
{
	QTemporaryDir home;
	QVERIFY(home.isValid());
	const auto authority = home.filePath(QStringLiteral(".Xauthority"));
	QFile file(authority);
	QVERIFY(file.open(QIODevice::WriteOnly));
	file.close();
	QVERIFY(file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::WriteGroup));
	QProcessEnvironment environment;
	environment.insert(QStringLiteral("HOME"), home.path());
	environment.insert(QStringLiteral("XAUTHORITY"), authority);
	QVERIFY(!GreeterEnvironment::prepare(greeter(), environment).contains(QStringLiteral("XAUTHORITY")));
}

void GreeterEnvironmentTest::nativeWaylandIsPreserved()
{
	auto session = greeter();
	session.type = QStringLiteral("wayland");
	QProcessEnvironment environment;
	environment.insert(QStringLiteral("WAYLAND_DISPLAY"), QStringLiteral("wayland-0"));
	environment.insert(QStringLiteral("DISPLAY"), QStringLiteral(":7")); // Xwayland is not the greeter framebuffer.
	QVERIFY(GreeterEnvironment::prepare(session, environment) == environment);
}

void GreeterEnvironmentTest::userSessionIsUnchanged()
{
	auto session = greeter();
	session.sessionClass = QStringLiteral("user");
	QProcessEnvironment environment;
	environment.insert(QStringLiteral("DISPLAY"), QStringLiteral(":99"));
	QVERIFY(GreeterEnvironment::prepare(session, environment) == environment);
}

void GreeterEnvironmentTest::lightDmGreeterWithUserClassUsesLogindDisplay()
{
	auto session = greeter();
	session.sessionClass = QStringLiteral("user");
	session.service = QStringLiteral("lightdm-greeter");
	session.state = QStringLiteral("active");
	session.seat = QStringLiteral("seat0");
	session.type = QStringLiteral("unspecified");
	const auto result = GreeterEnvironment::prepare(session, QProcessEnvironment());
	QCOMPARE(result.value(QStringLiteral("DISPLAY")), QStringLiteral(":7"));
	QCOMPARE(result.value(QStringLiteral("XDG_SESSION_TYPE")), QStringLiteral("x11"));
}

QTEST_GUILESS_MAIN(GreeterEnvironmentTest)
#include "GreeterEnvironmentTest.moc"
