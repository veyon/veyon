// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-FileCopyrightText: 2026 Ilya Mukhamadeev <nicourced@altlinux.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusVirtualObject>
#include <QElapsedTimer>
#include <QMap>
#include <QMutex>
#include <QThread>
#include <QUuid>
#include <QtTest>

#include "LinuxDesktopIntegration.h"
#include "LinuxSessionControl.h"

#include <memory>

using Result = LinuxSessionControl::Result;

Q_DECLARE_METATYPE(LinuxSessionControl::Action)

// Stands in for one D-Bus: records every method call and answers it with the configured reply for
// "<service> <member>" (accept or a D-Bus error), or with ServiceUnknown like a bus without that service.
// org.freedesktop.DBus.NameHasOwner is answered from the configured owners and recorded separately.
class FakeBus
{
public:
	static constexpr auto Accept = QDBusError::NoError;

	void setReply(const QString& service, const QString& member, QDBusError::ErrorType reply)
	{
		setReply(key(service, member), reply);
	}

	// key as returned by calls()
	void setReply(const QString& callKey, QDBusError::ErrorType reply)
	{
		m_replies[callKey] = reply == Accept ? QString() : QDBusError::errorString(reply);
	}

	void setErrorReply(const QString& service, const QString& member, const QString& errorName)
	{
		m_replies[key(service, member)] = errorName;
	}

	// reply for calls without configured reply (default: ServiceUnknown)
	void setDefaultReply(QDBusError::ErrorType reply)
	{
		m_defaultErrorName = QDBusError::errorString(reply);
	}

	void addOwner(const QString& serviceName)
	{
		m_owners.append(serviceName);
	}

	void setOwnerQueriesFail(bool fail)
	{
		m_ownerQueriesFail = fail;
	}

	const QStringList& ownerQueries() const
	{
		return m_ownerQueries;
	}

	void setInvalidReply(const QString& service, const QString& member)
	{
		m_invalidReplies.append(key(service, member));
	}

	QDBusMessage call(const QDBusMessage& methodCall)
	{
		if (methodCall.service() == QLatin1String("org.freedesktop.DBus") &&
			methodCall.member() == QLatin1String("NameHasOwner"))
		{
			const auto serviceName = methodCall.arguments().value(0).toString();
			m_ownerQueries.append(serviceName);
			if (m_ownerQueriesFail)
			{
				return methodCall.createErrorReply(QDBusError::AccessDenied, QStringLiteral("error from fake bus"));
			}
			return methodCall.createReply(m_owners.contains(serviceName));
		}

		m_calls.append(methodCall);
		m_callOrder->append(methodCall.service());

		const auto callKey = key(methodCall.service(), methodCall.member());
		if (m_invalidReplies.contains(callKey))
		{
			return {};
		}

		const auto errorName = m_replies.value(callKey, m_defaultErrorName);
		if (errorName.isEmpty())
		{
			return methodCall.createReply();
		}

		return methodCall.createErrorReply(errorName, QStringLiteral("error from fake bus"));
	}

	LinuxSessionControl::DBusCall callFunction()
	{
		return [this](const QDBusMessage& methodCall) {
			return call(methodCall);
		};
	}

	// "<service> <member>" of all calls
	QStringList calls() const
	{
		QStringList calls;
		for (const auto& methodCall : m_calls)
		{
			calls.append(key(methodCall.service(), methodCall.member()));
		}
		return calls;
	}

	const QList<QDBusMessage>& methodCalls() const
	{
		return m_calls;
	}

	void setCallOrder(QStringList* callOrder)
	{
		m_callOrder = callOrder;
	}

	static QString key(const QString& service, const QString& member)
	{
		return service + QLatin1Char(' ') + member;
	}

private:
	// error name, empty for accept
	QMap<QString, QString> m_replies;
	QString m_defaultErrorName{QDBusError::errorString(QDBusError::ServiceUnknown)};
	QStringList m_owners;
	QStringList m_ownerQueries;
	bool m_ownerQueriesFail{false};
	QStringList m_invalidReplies;
	QList<QDBusMessage> m_calls;
	QStringList m_ownCallOrder;
	QStringList* m_callOrder{&m_ownCallOrder};
};



// Owns all well-known names of the desktop session managers on a private session bus (dbus-run-session)
// with a separate connection in its own thread, so that the blocking calls of the code under test are
// answered. Prevents activation of any installed service; nothing touches the system bus.
class FakeDesktopServices : public QDBusVirtualObject
{
	Q_OBJECT
public:
	enum class Mode {
		Accept,
		UnknownMethod,
		NoReply,
	};

	explicit FakeDesktopServices(const QMap<QString, Mode>& modes) :
		m_modes(modes)
	{
	}

	QString introspect(const QString& path) const override
	{
		Q_UNUSED(path)
		return {};
	}

	bool handleMessage(const QDBusMessage& message, const QDBusConnection& connection) override
	{
		const auto mode = m_modes.value(message.path(), Mode::Accept);
		{
			QMutexLocker locker(&m_mutex);
			m_calls.append(message.path() + QLatin1Char(' ') + message.member() +
						   QLatin1Char('(') + message.signature() + QLatin1Char(')') +
						   argumentsToString(message.arguments()));
		}

		switch (mode)
		{
		case Mode::Accept:
			connection.send(message.createReply());
			break;
		case Mode::UnknownMethod:
			connection.send(message.createErrorReply(QDBusError::UnknownMethod, QStringLiteral("no such method")));
			break;
		case Mode::NoReply:
			// keep the caller waiting until its timeout
			message.setDelayedReply(true);
			break;
		}

		return true;
	}

	QStringList calls() const
	{
		QMutexLocker locker(&m_mutex);
		return m_calls;
	}

private:
	static QString argumentsToString(const QVariantList& arguments)
	{
		QStringList values;
		for (const auto& argument : arguments)
		{
			values.append(argument.toString());
		}
		return values.join(QLatin1Char(','));
	}

	const QMap<QString, Mode> m_modes;
	mutable QMutex m_mutex;
	QStringList m_calls;
};



class PrivateSessionBusPeer
{
public:
	static inline const QStringList AllServiceNames{
		QStringLiteral("org.kde.Shutdown"),
		QStringLiteral("org.kde.ksmserver"),
		QStringLiteral("org.gnome.SessionManager"),
	};
	static inline const QStringList ObjectPaths{
		QStringLiteral("/Shutdown"),
		QStringLiteral("/KSMServer"),
		QStringLiteral("/org/gnome/SessionManager"),
	};

	explicit PrivateSessionBusPeer(const QMap<QString, FakeDesktopServices::Mode>& modes,
								   const QStringList& serviceNames = AllServiceNames) :
		m_serviceNames(serviceNames),
		m_connectionName(QUuid::createUuid().toString()),
		m_connection(QDBusConnection::connectToBus(QDBusConnection::SessionBus, m_connectionName)),
		m_services(new FakeDesktopServices(modes))
	{
		m_services->moveToThread(&m_thread);
		QObject::connect(&m_thread, &QThread::finished, m_services, &QObject::deleteLater);
		m_thread.start();

		m_valid = m_connection.isConnected();
		for (const auto& serviceName : m_serviceNames)
		{
			m_valid = m_valid && m_connection.registerService(serviceName);
		}
		for (const auto& objectPath : ObjectPaths)
		{
			m_valid = m_valid && m_connection.registerVirtualObject(objectPath, m_services);
		}
	}

	~PrivateSessionBusPeer()
	{
		for (const auto& objectPath : ObjectPaths)
		{
			m_connection.unregisterObject(objectPath);
		}
		for (const auto& serviceName : m_serviceNames)
		{
			m_connection.unregisterService(serviceName);
		}
		m_thread.quit();
		m_thread.wait();
		QDBusConnection::disconnectFromBus(m_connectionName);
	}

	Q_DISABLE_COPY_MOVE(PrivateSessionBusPeer)

	bool isValid() const
	{
		return m_valid;
	}

	QStringList calls() const
	{
		return m_services->calls();
	}

private:
	const QStringList m_serviceNames;
	const QString m_connectionName;
	QDBusConnection m_connection;
	QThread m_thread;
	FakeDesktopServices* m_services;
	bool m_valid{false};
};



class LinuxSessionControlTest : public QObject
{
	Q_OBJECT
private:
	static constexpr auto KdeShutdown = "org.kde.Shutdown";
	static constexpr auto KsmServer = "org.kde.ksmserver";
	static constexpr auto Gnome = "org.gnome.SessionManager";
	static constexpr auto Mate = "org.mate.SessionManager";
	static constexpr auto PowerManagement = "org.freedesktop.PowerManagement";
	static constexpr auto Logind = "org.freedesktop.login1";
	static constexpr auto ConsoleKit = "org.freedesktop.ConsoleKit";

	static QString call(const char* service, const char* member)
	{
		return FakeBus::key(QLatin1String(service), QLatin1String(member));
	}

	// D-Bus signature of the arguments of an unsent method call (QDBusMessage::signature() is only
	// known after marshalling)
	static QString signature(const QDBusMessage& methodCall)
	{
		QString signature;
		for (const auto& argument : methodCall.arguments())
		{
			switch (argument.userType())
			{
			case QMetaType::Bool: signature += QLatin1Char('b'); break;
			case QMetaType::Int: signature += QLatin1Char('i'); break;
			case QMetaType::UInt: signature += QLatin1Char('u'); break;
			default: signature += QLatin1Char('?'); break;
			}
		}
		return signature;
	}

	static QProcessEnvironment environment(const QString& currentDesktop)
	{
		QProcessEnvironment environment;
		environment.insert(QStringLiteral("XDG_CURRENT_DESKTOP"), currentDesktop);
		return environment;
	}

	static QProcessEnvironment kdeEnvironment()
	{
		return environment(QStringLiteral("KDE"));
	}

	static QProcessEnvironment xfceEnvironment()
	{
		return environment(QStringLiteral("XFCE"));
	}

	struct Fixture
	{
		explicit Fixture(const QProcessEnvironment& environment = kdeEnvironment()) :
			control(backend(), withSessionIds(environment))
		{
			systemBus.setCallOrder(&callOrder);
			sessionBus.setCallOrder(&callOrder);
			// logind and ConsoleKit refuse by default, so that the desktop managers are reached
			systemBus.setReply(QLatin1String(Logind), QStringLiteral("Reboot"), QDBusError::AccessDenied);
			systemBus.setReply(QLatin1String(Logind), QStringLiteral("PowerOff"), QDBusError::AccessDenied);
		}

		Q_DISABLE_COPY_MOVE(Fixture)

		LinuxSessionControl::Backend backend()
		{
			return {systemBus.callFunction(), sessionBus.callFunction(),
					[this]() {
				callOrder.append(QStringLiteral("prepare"));
			},
					[this]() {
				callOrder.append(QStringLiteral("logout-commands"));
			}};
		}

		static QProcessEnvironment withSessionIds(QProcessEnvironment environment)
		{
			environment.insert(QStringLiteral("XDG_SESSION_ID"), QStringLiteral("c7"));
			environment.insert(QStringLiteral("XDG_SESSION_COOKIE"), QStringLiteral("cookie"));
			return environment;
		}

		void setSessionReply(const char* service, const char* member, QDBusError::ErrorType reply)
		{
			sessionBus.setReply(QLatin1String(service), QLatin1String(member), reply);
		}

		Result request(LinuxSessionControl::Action action) const
		{
			switch (action)
			{
			case LinuxSessionControl::Action::Logout: return control.requestLogout();
			case LinuxSessionControl::Action::Reboot: return control.requestReboot();
			case LinuxSessionControl::Action::PowerOff: return control.requestPowerOff();
			}
			return Result::Unavailable;
		}

		FakeBus systemBus;
		FakeBus sessionBus;
		QStringList callOrder;
		LinuxSessionControl control;
	};

	static void addActionColumns()
	{
		QTest::addColumn<LinuxSessionControl::Action>("action");
		QTest::addColumn<QString>("kdeMethod");
		QTest::addColumn<int>("ksmServerShutdownType");
		QTest::addColumn<QStringList>("otherDesktopCalls");

		using KDE = LinuxDesktopIntegration::KDE;

		QTest::newRow("logout") << LinuxSessionControl::Action::Logout
								<< QStringLiteral("logout") << int(KDE::ShutdownTypeLogout)
								<< QStringList{call(Gnome, "Logout"), call(Mate, "Logout")};
		QTest::newRow("reboot") << LinuxSessionControl::Action::Reboot
								<< QStringLiteral("logoutAndReboot") << int(KDE::ShutdownTypeReboot)
								<< QStringList{call(Gnome, "RequestReboot"), call(Mate, "RequestReboot"),
											   call(PowerManagement, "Reboot")};
		QTest::newRow("power-off") << LinuxSessionControl::Action::PowerOff
								   << QStringLiteral("logoutAndShutdown") << int(KDE::ShutdownTypeHalt)
								   << QStringList{call(Gnome, "RequestShutdown"), call(Mate, "RequestShutdown"),
												  call(PowerManagement, "Shutdown")};
	}

private Q_SLOTS:
	void logindAcceptedSendsNoFurtherRequest_data()
	{
		QTest::addColumn<bool>("reboot");
		QTest::newRow("reboot") << true;
		QTest::newRow("power-off") << false;
	}

	void logindAcceptedSendsNoFurtherRequest()
	{
		QFETCH(bool, reboot);

		Fixture fixture;
		const auto method = reboot ? QStringLiteral("Reboot") : QStringLiteral("PowerOff");
		fixture.systemBus.setReply(QLatin1String(Logind), method, FakeBus::Accept);

		QCOMPARE(reboot ? fixture.control.requestReboot() : fixture.control.requestPowerOff(), Result::Accepted);
		QCOMPARE(fixture.systemBus.calls(), QStringList{FakeBus::key(QLatin1String(Logind), method)});

		// interactive = false, no polkit prompt
		const auto& logindCall = fixture.systemBus.methodCalls().constFirst();
		QCOMPARE(logindCall.path(), QStringLiteral("/org/freedesktop/login1"));
		QCOMPARE(logindCall.interface(), QStringLiteral("org.freedesktop.login1.Manager"));
		QCOMPARE(signature(logindCall), QStringLiteral("b"));
		QCOMPARE(logindCall.arguments(), QVariantList{false});

		QVERIFY(fixture.sessionBus.calls().isEmpty());
		QVERIFY(fixture.callOrder.contains(QStringLiteral("prepare")) == false);
	}

	void consoleKitAcceptedSendsNoDesktopRequest_data()
	{
		QTest::addColumn<bool>("reboot");
		QTest::addColumn<QString>("method");
		QTest::newRow("reboot") << true << QStringLiteral("Restart");
		QTest::newRow("power-off") << false << QStringLiteral("Stop");
	}

	void consoleKitAcceptedSendsNoDesktopRequest()
	{
		QFETCH(bool, reboot);
		QFETCH(QString, method);

		Fixture fixture;
		fixture.systemBus.setReply(QLatin1String(ConsoleKit), method, FakeBus::Accept);

		QCOMPARE(reboot ? fixture.control.requestReboot() : fixture.control.requestPowerOff(), Result::Accepted);
		QCOMPARE(fixture.systemBus.calls(),
				 QStringList({call(Logind, reboot ? "Reboot" : "PowerOff"), FakeBus::key(QLatin1String(ConsoleKit), method)}));
		const auto& consoleKitCall = fixture.systemBus.methodCalls().constLast();
		QCOMPARE(consoleKitCall.path(), QStringLiteral("/org/freedesktop/ConsoleKit/Manager"));
		QCOMPARE(consoleKitCall.interface(), QStringLiteral("org.freedesktop.ConsoleKit.Manager"));
		QVERIFY(consoleKitCall.arguments().isEmpty());

		QVERIFY(fixture.sessionBus.calls().isEmpty());
		QVERIFY(fixture.callOrder.contains(QStringLiteral("prepare")) == false);
	}

	void systemManagerFailuresFallBackToDesktop_data()
	{
		QTest::addColumn<int>("logindError");
		QTest::addColumn<int>("consoleKitError");
		QTest::newRow("denied-unknown") << int(QDBusError::AccessDenied) << int(QDBusError::ServiceUnknown);
		QTest::newRow("denied-denied") << int(QDBusError::AccessDenied) << int(QDBusError::AccessDenied);
		QTest::newRow("no-reply-failed") << int(QDBusError::NoReply) << int(QDBusError::Failed);
		QTest::newRow("unknown-unknown") << int(QDBusError::ServiceUnknown) << int(QDBusError::ServiceUnknown);
	}

	void systemManagerFailuresFallBackToDesktop()
	{
		QFETCH(int, logindError);
		QFETCH(int, consoleKitError);

		Fixture fixture;
		fixture.systemBus.setReply(QLatin1String(Logind), QStringLiteral("Reboot"), QDBusError::ErrorType(logindError));
		fixture.systemBus.setReply(QLatin1String(ConsoleKit), QStringLiteral("Restart"), QDBusError::ErrorType(consoleKitError));
		fixture.setSessionReply(KdeShutdown, "logoutAndReboot", FakeBus::Accept);

		QCOMPARE(fixture.control.requestReboot(), Result::Accepted);
		// session bus access is prepared once, after the system managers and before the first desktop call
		const QStringList expectedCallOrder{
			QLatin1String(Logind),
			QLatin1String(ConsoleKit),
			QStringLiteral("prepare"),
			QLatin1String(KdeShutdown),
		};
		QCOMPARE(fixture.callOrder, expectedCallOrder);
	}

	void modernKdeMethodsHaveNoArguments_data()
	{
		addActionColumns();
	}

	void modernKdeMethodsHaveNoArguments()
	{
		QFETCH(LinuxSessionControl::Action, action);
		QFETCH(QString, kdeMethod);

		Fixture fixture;
		fixture.sessionBus.setReply(QLatin1String(KdeShutdown), kdeMethod, FakeBus::Accept);

		QCOMPARE(fixture.request(action), Result::Accepted);
		QCOMPARE(fixture.sessionBus.calls(), QStringList{FakeBus::key(QLatin1String(KdeShutdown), kdeMethod)});

		const auto& modernCall = fixture.sessionBus.methodCalls().constFirst();
		QCOMPARE(modernCall.path(), QStringLiteral("/Shutdown"));
		QCOMPARE(modernCall.interface(), QStringLiteral("org.kde.Shutdown"));
		QVERIFY(modernCall.arguments().isEmpty());
	}

	void unavailableModernKdeFallsBackToKsmServer_data()
	{
		QTest::addColumn<QString>("modernError");
		QTest::addColumn<LinuxSessionControl::Action>("action");
		QTest::addColumn<QString>("kdeMethod");
		QTest::addColumn<int>("ksmServerShutdownType");

		using KDE = LinuxDesktopIntegration::KDE;

		const auto errorName = [](QDBusError::ErrorType error) {
			return QDBusError::errorString(error);
		};
		for (const auto& error : {std::pair{"service-unknown", errorName(QDBusError::ServiceUnknown)},
								  std::pair{"unknown-object", errorName(QDBusError::UnknownObject)},
								  std::pair{"unknown-interface", errorName(QDBusError::UnknownInterface)},
								  std::pair{"unknown-method", errorName(QDBusError::UnknownMethod)},
								  std::pair{"disconnected", errorName(QDBusError::Disconnected)},
								  std::pair{"name-has-no-owner", QStringLiteral("org.freedesktop.DBus.Error.NameHasNoOwner")},
								  std::pair{"spawn-child-exited", QStringLiteral("org.freedesktop.DBus.Error.Spawn.ChildExited")},
								  std::pair{"spawn-exec-failed", QStringLiteral("org.freedesktop.DBus.Error.Spawn.ExecFailed")},
								  std::pair{"no-such-unit", QStringLiteral("org.freedesktop.systemd1.NoSuchUnit")}})
		{
			using Action = LinuxSessionControl::Action;
			for (const auto& action : {std::tuple{"logout", Action::Logout, "logout", KDE::ShutdownTypeLogout},
									   std::tuple{"reboot", Action::Reboot, "logoutAndReboot", KDE::ShutdownTypeReboot},
									   std::tuple{"power-off", Action::PowerOff, "logoutAndShutdown", KDE::ShutdownTypeHalt}})
			{
				QTest::addRow("%s-%s", error.first, std::get<0>(action))
					<< error.second << std::get<1>(action)
					<< QString::fromLatin1(std::get<2>(action)) << int(std::get<3>(action));
			}
		}
	}

	void unavailableModernKdeFallsBackToKsmServer()
	{
		QFETCH(QString, modernError);
		QFETCH(LinuxSessionControl::Action, action);
		QFETCH(QString, kdeMethod);
		QFETCH(int, ksmServerShutdownType);

		Fixture fixture;
		fixture.sessionBus.setErrorReply(QLatin1String(KdeShutdown), kdeMethod, modernError);
		fixture.setSessionReply(KsmServer, "logout", FakeBus::Accept);

		QCOMPARE(fixture.request(action), Result::Accepted);
		QCOMPARE(fixture.sessionBus.calls(),
				 QStringList({FakeBus::key(QLatin1String(KdeShutdown), kdeMethod), call(KsmServer, "logout")}));

		using KDE = LinuxDesktopIntegration::KDE;
		const auto& legacyCall = fixture.sessionBus.methodCalls().constLast();
		QCOMPARE(legacyCall.path(), QStringLiteral("/KSMServer"));
		QCOMPARE(legacyCall.interface(), QStringLiteral("org.kde.KSMServerInterface"));
		QCOMPARE(signature(legacyCall), QStringLiteral("iii"));
		QCOMPARE(legacyCall.arguments(),
				 QVariantList({int(KDE::ShutdownConfirmNo), ksmServerShutdownType, int(KDE::ShutdownModeForceNow)}));
	}

	void failedModernKdeIsNotRetried_data()
	{
		QTest::addColumn<int>("error");
		QTest::addColumn<LinuxSessionControl::Action>("action");
		QTest::addColumn<QString>("kdeMethod");

		for (const auto& error : {std::pair{"access-denied", QDBusError::AccessDenied},
								  std::pair{"no-reply", QDBusError::NoReply},
								  std::pair{"timeout", QDBusError::Timeout},
								  std::pair{"timed-out", QDBusError::TimedOut},
								  std::pair{"failed", QDBusError::Failed},
								  std::pair{"not-supported", QDBusError::NotSupported}})
		{
			for (const auto& action : {std::pair{LinuxSessionControl::Action::Logout, "logout"},
									   std::pair{LinuxSessionControl::Action::Reboot, "logoutAndReboot"},
									   std::pair{LinuxSessionControl::Action::PowerOff, "logoutAndShutdown"}})
			{
				QTest::addRow("%s-%s", error.first, action.second)
					<< int(error.second) << action.first << QString::fromLatin1(action.second);
			}
		}
	}

	void failedModernKdeIsNotRetried()
	{
		QFETCH(int, error);
		QFETCH(LinuxSessionControl::Action, action);
		QFETCH(QString, kdeMethod);

		Fixture fixture;
		fixture.sessionBus.setReply(QLatin1String(KdeShutdown), kdeMethod, QDBusError::ErrorType(error));
		fixture.setSessionReply(KsmServer, "logout", FakeBus::Accept);
		fixture.setSessionReply(Gnome, "Logout", FakeBus::Accept);
		fixture.setSessionReply(Gnome, "RequestReboot", FakeBus::Accept);
		fixture.setSessionReply(Gnome, "RequestShutdown", FakeBus::Accept);

		// neither the legacy KDE interface nor any other desktop manager gets a second request
		QCOMPARE(fixture.request(action), Result::Failed);
		QCOMPARE(fixture.sessionBus.calls(), QStringList{FakeBus::key(QLatin1String(KdeShutdown), kdeMethod)});
	}

	void invalidReplyIsNotRetried()
	{
		Fixture fixture;
		fixture.sessionBus.setInvalidReply(QLatin1String(KdeShutdown), QStringLiteral("logoutAndReboot"));

		QCOMPARE(fixture.control.requestReboot(), Result::Failed);
		QCOMPARE(fixture.sessionBus.calls(), QStringList{call(KdeShutdown, "logoutAndReboot")});
	}

	void failedKsmServerIsNotRetried_data()
	{
		addActionColumns();
	}

	void failedKsmServerIsNotRetried()
	{
		QFETCH(LinuxSessionControl::Action, action);
		QFETCH(QString, kdeMethod);
		QFETCH(QStringList, otherDesktopCalls);

		Fixture fixture;
		fixture.setSessionReply(KsmServer, "logout", QDBusError::AccessDenied);
		for (const auto& otherCall : otherDesktopCalls)
		{
			fixture.sessionBus.setReply(otherCall, FakeBus::Accept);
		}

		QCOMPARE(fixture.request(action), Result::Failed);
		QCOMPARE(fixture.sessionBus.calls(),
				 QStringList({FakeBus::key(QLatin1String(KdeShutdown), kdeMethod), call(KsmServer, "logout")}));
	}

	void unavailableKdeFallsBackToOtherDesktops_data()
	{
		addActionColumns();
	}

	void unavailableKdeFallsBackToOtherDesktops()
	{
		QFETCH(LinuxSessionControl::Action, action);
		QFETCH(QString, kdeMethod);
		QFETCH(QStringList, otherDesktopCalls);

		Fixture fixture;
		const auto& gnomeCall = otherDesktopCalls.constFirst();
		fixture.sessionBus.setReply(gnomeCall, FakeBus::Accept);

		QCOMPARE(fixture.request(action), Result::Accepted);
		QCOMPARE(fixture.sessionBus.calls(),
				 QStringList({FakeBus::key(QLatin1String(KdeShutdown), kdeMethod), call(KsmServer, "logout"), gnomeCall}));
	}

	void otherDesktopsAreAskedInOrderUntilOneAccepts_data()
	{
		addActionColumns();
	}

	void otherDesktopsAreAskedInOrderUntilOneAccepts()
	{
		QFETCH(LinuxSessionControl::Action, action);
		QFETCH(QStringList, otherDesktopCalls);

		for (int acceptingIndex = 0; acceptingIndex < otherDesktopCalls.size(); ++acceptingIndex)
		{
			Fixture fixture(xfceEnvironment());
			const auto& acceptingCall = otherDesktopCalls[acceptingIndex];
			fixture.sessionBus.setReply(acceptingCall, FakeBus::Accept);

			QCOMPARE(fixture.request(action), Result::Accepted);
			QCOMPARE(fixture.sessionBus.calls(), otherDesktopCalls.mid(0, acceptingIndex + 1));
		}
	}

	void failedOtherDesktopIsNotRetried_data()
	{
		addActionColumns();
	}

	void failedOtherDesktopIsNotRetried()
	{
		QFETCH(LinuxSessionControl::Action, action);
		QFETCH(QStringList, otherDesktopCalls);

		for (int failingIndex = 0; failingIndex < otherDesktopCalls.size(); ++failingIndex)
		{
			Fixture fixture(xfceEnvironment());
			for (int index = 0; index < otherDesktopCalls.size(); ++index)
			{
				const auto& otherCall = otherDesktopCalls[index];
				fixture.sessionBus.setReply(otherCall, index == failingIndex ? QDBusError::NoReply : FakeBus::Accept);
			}
			// all managers before the failing one are unavailable
			for (int index = 0; index < failingIndex; ++index)
			{
				const auto& otherCall = otherDesktopCalls[index];
				fixture.sessionBus.setReply(otherCall, QDBusError::UnknownObject);
			}

			QCOMPARE(fixture.request(action), Result::Failed);
			QCOMPARE(fixture.sessionBus.calls(), otherDesktopCalls.mid(0, failingIndex + 1));
		}
	}

	void noDesktopAvailable_data()
	{
		addActionColumns();
	}

	void noDesktopAvailable()
	{
		QFETCH(LinuxSessionControl::Action, action);
		QFETCH(QString, kdeMethod);
		QFETCH(QStringList, otherDesktopCalls);

		Fixture fixture;

		QCOMPARE(fixture.request(action), Result::Unavailable);
		QCOMPARE(fixture.sessionBus.calls(),
				 QStringList({FakeBus::key(QLatin1String(KdeShutdown), kdeMethod), call(KsmServer, "logout")}) + otherDesktopCalls);
	}

	void nonKdeSessionSkipsKdeServices_data()
	{
		addActionColumns();
	}

	void nonKdeSessionSkipsKdeServices()
	{
		QFETCH(LinuxSessionControl::Action, action);
		QFETCH(QStringList, otherDesktopCalls);

		// an installed, activatable org.kde.Shutdown must not take over another desktop
		Fixture fixture(xfceEnvironment());
		fixture.setSessionReply(KdeShutdown, "logout", FakeBus::Accept);
		fixture.setSessionReply(KdeShutdown, "logoutAndReboot", FakeBus::Accept);
		fixture.setSessionReply(KdeShutdown, "logoutAndShutdown", FakeBus::Accept);
		fixture.setSessionReply(KsmServer, "logout", FakeBus::Accept);

		QCOMPARE(fixture.request(action), Result::Unavailable);
		QCOMPARE(fixture.sessionBus.calls(), otherDesktopCalls);
		QCOMPARE(fixture.sessionBus.ownerQueries(), QStringList({QLatin1String(KsmServer), QLatin1String(KdeShutdown)}));
	}

	void kdeSessionDetection_data()
	{
		QTest::addColumn<QString>("currentDesktop");
		QTest::addColumn<QString>("sessionDesktop");
		QTest::addColumn<QStringList>("owners");
		QTest::addColumn<bool>("ownerQueriesFail");
		QTest::addColumn<QStringList>("expectedOwnerQueries");
		QTest::addColumn<bool>("kde");

		const QStringList bothQueries{QLatin1String(KsmServer), QLatin1String(KdeShutdown)};

		QTest::newRow("environment") << QStringLiteral("KDE") << QString() << QStringList{} << false
									 << QStringList{} << true;
		// LinuxServiceCore rewrites XDG_CURRENT_DESKTOP=KDE to X-Generic (#817)
		QTest::newRow("x-generic-with-session-desktop") << QStringLiteral("X-Generic") << QStringLiteral("KDE")
														<< QStringList{} << false << QStringList{} << true;
		QTest::newRow("x-generic-with-ksmserver") << QStringLiteral("X-Generic") << QString()
												  << QStringList{QLatin1String(KsmServer)} << false
												  << QStringList{QLatin1String(KsmServer)} << true;
		QTest::newRow("x-generic-with-shutdown") << QStringLiteral("X-Generic") << QString()
												 << QStringList{QLatin1String(KdeShutdown)} << false << bothQueries << true;
		QTest::newRow("x-generic-without-kde") << QStringLiteral("X-Generic") << QString() << QStringList{} << false
											   << bothQueries << false;
		QTest::newRow("owner-queries-fail") << QStringLiteral("X-Generic") << QString()
											<< QStringList{QLatin1String(KsmServer)} << true << bothQueries << false;
	}

	void kdeSessionDetection()
	{
		QFETCH(QString, currentDesktop);
		QFETCH(QString, sessionDesktop);
		QFETCH(QStringList, owners);
		QFETCH(bool, ownerQueriesFail);
		QFETCH(QStringList, expectedOwnerQueries);
		QFETCH(bool, kde);

		auto environment = this->environment(currentDesktop);
		environment.insert(QStringLiteral("XDG_SESSION_DESKTOP"), sessionDesktop);

		Fixture fixture(environment);
		for (const auto& owner : owners)
		{
			fixture.sessionBus.addOwner(owner);
		}
		fixture.sessionBus.setOwnerQueriesFail(ownerQueriesFail);

		QCOMPARE(fixture.control.requestReboot(), Result::Unavailable);
		QCOMPARE(fixture.sessionBus.calls().contains(call(KdeShutdown, "logoutAndReboot")), kde);
		QCOMPARE(fixture.sessionBus.calls().contains(call(KsmServer, "logout")), kde);
		QCOMPARE(fixture.sessionBus.ownerQueries(), expectedOwnerQueries);
	}

	void desktopMethodArguments()
	{
		Fixture fixture(xfceEnvironment());

		QCOMPARE(fixture.control.requestLogout(), Result::Unavailable);
		QCOMPARE(fixture.control.requestReboot(), Result::Unavailable);
		QCOMPARE(fixture.control.requestPowerOff(), Result::Unavailable);

		const auto& methodCalls = fixture.sessionBus.methodCalls();
		QCOMPARE(methodCalls.size(), 8);

		// logout without confirmation
		QCOMPARE(methodCalls[0].path(), QStringLiteral("/org/gnome/SessionManager"));
		QCOMPARE(methodCalls[0].interface(), QStringLiteral("org.gnome.SessionManager"));
		QCOMPARE(signature(methodCalls[0]), QStringLiteral("u"));
		QCOMPARE(methodCalls[0].arguments(), QVariantList{uint(LinuxDesktopIntegration::Gnome::GSM_MANAGER_LOGOUT_MODE_FORCE)});
		QCOMPARE(methodCalls[1].path(), QStringLiteral("/org/mate/SessionManager"));
		QCOMPARE(methodCalls[1].interface(), QStringLiteral("org.mate.SessionManager"));
		QCOMPARE(signature(methodCalls[1]), QStringLiteral("u"));
		QCOMPARE(methodCalls[1].arguments(), QVariantList{uint(LinuxDesktopIntegration::Mate::GSM_LOGOUT_MODE_FORCE)});

		for (int index = 2; index < methodCalls.size(); ++index)
		{
			QVERIFY(methodCalls[index].arguments().isEmpty());
		}
		QCOMPARE(methodCalls[4].path(), QStringLiteral("/org/freedesktop/PowerManagement"));
		QCOMPARE(methodCalls[4].interface(), QStringLiteral("org.freedesktop.PowerManagement"));
	}

	void acceptedLogoutEndsLogout()
	{
		Fixture fixture;
		fixture.setSessionReply(KdeShutdown, "logout", FakeBus::Accept);

		QCOMPARE(fixture.control.requestLogout(), Result::Accepted);
		QVERIFY(fixture.systemBus.calls().isEmpty());
		QCOMPARE(fixture.callOrder, QStringList({QStringLiteral("prepare"), QLatin1String(KdeShutdown)}));
	}

	void logoutFallbacks_data()
	{
		QTest::addColumn<int>("gnomeReply");
		QTest::addColumn<int>("defaultReply");
		QTest::addColumn<int>("result");
		QTest::addColumn<QStringList>("expectedCallOrder");

		const auto prepare = QStringLiteral("prepare");
		const auto logoutCommands = QStringLiteral("logout-commands");

		// no desktop session manager: Xfce/LXDE commands, then terminate the session
		QTest::newRow("unavailable") << int(QDBusError::ServiceUnknown) << int(QDBusError::ServiceUnknown)
									 << int(Result::Unavailable)
									 << QStringList{prepare, QLatin1String(Gnome), QLatin1String(Mate), logoutCommands,
													QLatin1String(Logind), QLatin1String(ConsoleKit)};
		// no session bus at all counts as unavailable
		QTest::newRow("disconnected") << int(QDBusError::Disconnected) << int(QDBusError::Disconnected)
									  << int(Result::Unavailable)
									  << QStringList{prepare, QLatin1String(Gnome), QLatin1String(Mate), logoutCommands,
													 QLatin1String(Logind), QLatin1String(ConsoleKit)};
		// a desktop session manager refused or did not answer: no other desktop's commands, but
		// terminate the session
		QTest::newRow("refused") << int(QDBusError::AccessDenied) << int(QDBusError::ServiceUnknown)
								 << int(Result::Failed)
								 << QStringList{prepare, QLatin1String(Gnome), QLatin1String(Logind), QLatin1String(ConsoleKit)};
		QTest::newRow("timeout") << int(QDBusError::NoReply) << int(QDBusError::ServiceUnknown)
								 << int(Result::Failed)
								 << QStringList{prepare, QLatin1String(Gnome), QLatin1String(Logind), QLatin1String(ConsoleKit)};
		QTest::newRow("accepted") << int(FakeBus::Accept) << int(QDBusError::ServiceUnknown)
								  << int(Result::Accepted) << QStringList{prepare, QLatin1String(Gnome)};
	}

	void logoutFallbacks()
	{
		QFETCH(int, gnomeReply);
		QFETCH(int, defaultReply);
		QFETCH(int, result);
		QFETCH(QStringList, expectedCallOrder);

		Fixture fixture(xfceEnvironment());
		fixture.sessionBus.setDefaultReply(QDBusError::ErrorType(defaultReply));
		fixture.setSessionReply(Gnome, "Logout", QDBusError::ErrorType(gnomeReply));

		QCOMPARE(fixture.control.requestLogout(), Result(result));
		QCOMPARE(fixture.callOrder, expectedCallOrder);

		if (Result(result) != Result::Accepted)
		{
			const auto& systemCalls = fixture.systemBus.methodCalls();
			QCOMPARE(fixture.systemBus.calls(),
					 QStringList({call(Logind, "TerminateSession"), call(ConsoleKit, "CloseSession")}));
			QCOMPARE(systemCalls[0].path(), QStringLiteral("/org/freedesktop/login1"));
			QCOMPARE(systemCalls[0].interface(), QStringLiteral("org.freedesktop.login1.Manager"));
			QCOMPARE(systemCalls[0].arguments(), QVariantList{QStringLiteral("c7")});
			QCOMPARE(systemCalls[1].path(), QStringLiteral("/org/freedesktop/ConsoleKit/Manager"));
			QCOMPARE(systemCalls[1].interface(), QStringLiteral("org.freedesktop.ConsoleKit.Manager"));
			QCOMPARE(systemCalls[1].arguments(), QVariantList{QStringLiteral("cookie")});
		}
	}

	void terminateSessionReplies_data()
	{
		QTest::addColumn<int>("terminateReply");
		QTest::addColumn<int>("closeReply");
		QTest::newRow("accepted") << int(FakeBus::Accept) << int(FakeBus::Accept);
		QTest::newRow("unavailable") << int(QDBusError::ServiceUnknown) << int(QDBusError::ServiceUnknown);
		QTest::newRow("refused") << int(QDBusError::AccessDenied) << int(QDBusError::AccessDenied);
	}

	void terminateSessionReplies()
	{
		QFETCH(int, terminateReply);
		QFETCH(int, closeReply);

		Fixture fixture(xfceEnvironment());
		fixture.systemBus.setReply(QLatin1String(Logind), QStringLiteral("TerminateSession"), QDBusError::ErrorType(terminateReply));
		fixture.systemBus.setReply(QLatin1String(ConsoleKit), QStringLiteral("CloseSession"), QDBusError::ErrorType(closeReply));

		// both are always sent, ConsoleKit as a last resort
		QCOMPARE(fixture.control.requestLogout(), Result::Unavailable);
		QCOMPARE(fixture.systemBus.calls(), QStringList({call(Logind, "TerminateSession"), call(ConsoleKit, "CloseSession")}));
	}

	void unavailableLogindIsNotFatal()
	{
		Fixture fixture;
		fixture.systemBus.setReply(QLatin1String(Logind), QStringLiteral("Reboot"), QDBusError::ServiceUnknown);
		fixture.setSessionReply(KdeShutdown, "logoutAndReboot", FakeBus::Accept);

		QCOMPARE(fixture.control.requestReboot(), Result::Accepted);
		QCOMPARE(fixture.systemBus.calls(), QStringList({call(Logind, "Reboot"), call(ConsoleKit, "Restart")}));
	}

	void isUnavailable_data()
	{
		QTest::addColumn<QString>("errorName");
		QTest::addColumn<bool>("unavailable");

		const auto addRow = [](const char* name, QDBusError::ErrorType error, bool unavailable) {
			QTest::newRow(name) << QDBusError::errorString(error) << unavailable;
		};
		addRow("service-unknown", QDBusError::ServiceUnknown, true);
		addRow("unknown-object", QDBusError::UnknownObject, true);
		addRow("unknown-interface", QDBusError::UnknownInterface, true);
		addRow("unknown-method", QDBusError::UnknownMethod, true);
		addRow("disconnected", QDBusError::Disconnected, true);
		addRow("access-denied", QDBusError::AccessDenied, false);
		addRow("no-reply", QDBusError::NoReply, false);
		addRow("timeout", QDBusError::Timeout, false);
		addRow("not-supported", QDBusError::NotSupported, false);
		addRow("failed", QDBusError::Failed, false);

		QTest::newRow("name-has-no-owner") << QStringLiteral("org.freedesktop.DBus.Error.NameHasNoOwner") << true;
		QTest::newRow("spawn-child-exited") << QStringLiteral("org.freedesktop.DBus.Error.Spawn.ChildExited") << true;
		QTest::newRow("spawn-exec-failed") << QStringLiteral("org.freedesktop.DBus.Error.Spawn.ExecFailed") << true;
		QTest::newRow("spawn-service-not-found") << QStringLiteral("org.freedesktop.DBus.Error.Spawn.ServiceNotFound")
												 << true;
		QTest::newRow("systemd-no-such-unit") << QStringLiteral("org.freedesktop.systemd1.NoSuchUnit") << true;
		QTest::newRow("spawn-without-dot") << QStringLiteral("org.freedesktop.DBus.Error.SpawnSomething") << false;
		QTest::newRow("other") << QStringLiteral("org.example.Error") << false;
	}

	void isUnavailable()
	{
		QFETCH(QString, errorName);
		QFETCH(bool, unavailable);

		const auto methodCall = QDBusMessage::createMethodCall(QStringLiteral("org.example.Service"), QStringLiteral("/"),
															   QStringLiteral("org.example.Interface"), QStringLiteral("Method"));
		QCOMPARE(LinuxSessionControl::isUnavailable(methodCall.createErrorReply(errorName, QStringLiteral("test"))),
				 unavailable);
		QCOMPARE(LinuxSessionControl::isUnavailable(methodCall.createReply()), false);
		QCOMPARE(LinuxSessionControl::isUnavailable(QDBusMessage()), false);
	}

	void isKdeEnvironment_data()
	{
		QTest::addColumn<QString>("currentDesktop");
		QTest::addColumn<QString>("sessionDesktop");
		QTest::addColumn<QString>("desktopSession");
		QTest::addColumn<QString>("fullSession");
		QTest::addColumn<bool>("kde");

		QTest::newRow("kde") << QStringLiteral("KDE") << QString() << QString() << QString() << true;
		QTest::newRow("desktop-list") << QStringLiteral("KDE:Plasma") << QString() << QString() << QString() << true;
		QTest::newRow("list-with-other-first") << QStringLiteral("ubuntu:plasma") << QString() << QString() << QString() << true;
		QTest::newRow("whitespace") << QStringLiteral(" KDE ") << QString() << QString() << QString() << true;
		QTest::newRow("session-wayland") << QString() << QStringLiteral("plasmawayland") << QString() << QString() << true;
		QTest::newRow("session-x11") << QString() << QString() << QStringLiteral("plasmax11") << QString() << true;
		QTest::newRow("session-dashed-x11") << QString() << QString() << QStringLiteral("plasma-x11") << QString() << true;
		QTest::newRow("session-dashed-wayland") << QString() << QStringLiteral("plasma-wayland") << QString() << QString() << true;
		QTest::newRow("session-plasma") << QString() << QString() << QStringLiteral("plasma") << QString() << true;
		QTest::newRow("full-session-true") << QString() << QString() << QString() << QStringLiteral("true") << true;
		QTest::newRow("full-session-TRUE") << QString() << QString() << QString() << QStringLiteral("TRUE") << true;
		QTest::newRow("full-session-1") << QString() << QString() << QString() << QStringLiteral("1") << true;
		QTest::newRow("full-session-0") << QString() << QString() << QString() << QStringLiteral("0") << false;
		// every variable is a positive hint (XDG_CURRENT_DESKTOP may have been rewritten, see #817)
		QTest::newRow("x-generic-session-desktop") << QStringLiteral("X-Generic") << QStringLiteral("KDE")
												   << QString() << QString() << true;
		QTest::newRow("x-generic-desktop-session") << QStringLiteral("X-Generic") << QString()
												   << QStringLiteral("plasma") << QString() << true;
		QTest::newRow("x-generic-full-session") << QStringLiteral("X-Generic") << QString() << QString()
												<< QStringLiteral("true") << true;
		QTest::newRow("x-generic") << QStringLiteral("X-Generic") << QString() << QString() << QString() << false;
		QTest::newRow("desktop-session-path") << QString() << QString()
											  << QStringLiteral("/usr/share/xsessions/plasma.desktop") << QString() << true;
		QTest::newRow("plasma6") << QString() << QStringLiteral("plasma6") << QString() << QString() << true;
		QTest::newRow("gnome") << QStringLiteral("ubuntu:GNOME") << QStringLiteral("ubuntu") << QStringLiteral("ubuntu")
							   << QString() << false;
		QTest::newRow("xfce") << QStringLiteral("XFCE") << QStringLiteral("xfce") << QStringLiteral("xfce")
							  << QStringLiteral("0") << false;
		QTest::newRow("only-separators") << QStringLiteral("::") << QString() << QString() << QString() << false;
		QTest::newRow("kde-prefix-only") << QStringLiteral("kdesomething") << QString() << QString() << QString() << false;
		QTest::newRow("unknown") << QString() << QString() << QString() << QString() << false;
	}

	void isKdeEnvironment()
	{
		QFETCH(QString, currentDesktop);
		QFETCH(QString, sessionDesktop);
		QFETCH(QString, desktopSession);
		QFETCH(QString, fullSession);
		QFETCH(bool, kde);

		QProcessEnvironment environment;
		environment.insert(QStringLiteral("XDG_CURRENT_DESKTOP"), currentDesktop);
		environment.insert(QStringLiteral("XDG_SESSION_DESKTOP"), sessionDesktop);
		environment.insert(QStringLiteral("DESKTOP_SESSION"), desktopSession);
		environment.insert(QStringLiteral("KDE_FULL_SESSION"), fullSession);

		QCOMPARE(LinuxSessionControl::isKdeEnvironment(environment), kde);
		QCOMPARE(LinuxSessionControl::isKdeEnvironment(QProcessEnvironment()), false);
	}

	// the real session bus call against fake services on a private bus (see CMakeLists.txt)
	void sessionBusCalls_data()
	{
		QTest::addColumn<bool>("modernUnknown");
		QTest::addColumn<bool>("legacyUnknown");
		QTest::addColumn<QStringList>("expectedCalls");

		QTest::newRow("modern") << false << false << QStringList{QStringLiteral("/Shutdown logoutAndReboot()")};
		QTest::newRow("legacy") << true << false
								<< QStringList{QStringLiteral("/Shutdown logoutAndReboot()"),
											   QStringLiteral("/KSMServer logout(iii)0,1,2")};
		QTest::newRow("gnome") << true << true
							   << QStringList{QStringLiteral("/Shutdown logoutAndReboot()"),
											  QStringLiteral("/KSMServer logout(iii)0,1,2"),
											  QStringLiteral("/org/gnome/SessionManager RequestReboot()")};
	}

	void sessionBusCalls()
	{
		QFETCH(bool, modernUnknown);
		QFETCH(bool, legacyUnknown);
		QFETCH(QStringList, expectedCalls);

		requirePrivateSessionBus();

		QMap<QString, FakeDesktopServices::Mode> modes;
		if (modernUnknown)
		{
			modes[QStringLiteral("/Shutdown")] = FakeDesktopServices::Mode::UnknownMethod;
		}
		if (legacyUnknown)
		{
			modes[QStringLiteral("/KSMServer")] = FakeDesktopServices::Mode::UnknownMethod;
		}

		PrivateSessionBusPeer peer(modes);
		QVERIFY(peer.isValid());

		const auto control = sessionBusControl(kdeEnvironment());
		QCOMPARE(control.requestReboot(), Result::Accepted);
		QCOMPARE(peer.calls(), expectedCalls);
	}

	void sessionBusLogoutArguments()
	{
		requirePrivateSessionBus();

		PrivateSessionBusPeer peer({}, {QStringLiteral("org.gnome.SessionManager")});
		QVERIFY(peer.isValid());

		const auto control = sessionBusControl(xfceEnvironment());
		QCOMPARE(control.requestLogout(), Result::Accepted);
		QCOMPARE(peer.calls(), QStringList{QStringLiteral("/org/gnome/SessionManager Logout(u)2")});
	}

	void sessionBusKdeDetection()
	{
		requirePrivateSessionBus();

		// only ksmserver runs, XDG_CURRENT_DESKTOP was rewritten (#817): org.kde.Shutdown is not available
		// on the private bus, so the legacy interface is used
		PrivateSessionBusPeer peer({}, {QStringLiteral("org.kde.ksmserver")});
		QVERIFY(peer.isValid());

		const auto control = sessionBusControl(environment(QStringLiteral("X-Generic")));
		QCOMPARE(control.requestReboot(), Result::Accepted);
		QCOMPARE(peer.calls(), QStringList{QStringLiteral("/KSMServer logout(iii)0,1,2")});
	}

	void sessionBusTimeoutIsNotRetried()
	{
		requirePrivateSessionBus();

		PrivateSessionBusPeer peer({{QStringLiteral("/Shutdown"), FakeDesktopServices::Mode::NoReply}});
		QVERIFY(peer.isValid());

		QElapsedTimer timer;
		timer.start();

		const auto control = sessionBusControl(kdeEnvironment(), ShortTimeout);
		QCOMPARE(control.requestReboot(), Result::Failed);
		QVERIFY(timer.elapsed() >= ShortTimeout / 2);
		QVERIFY(timer.elapsed() < LinuxSessionControl::SessionBusCallTimeout);
		QCOMPARE(peer.calls(), QStringList{QStringLiteral("/Shutdown logoutAndReboot()")});
	}

private:
	static void requirePrivateSessionBus()
	{
		// never register fake services on a real desktop session bus
		if (qEnvironmentVariableIsSet("VEYON_TEST_PRIVATE_SESSION_BUS") == false)
		{
			QSKIP("requires a private session bus (run through ctest with dbus-run-session)");
		}
	}

	static constexpr int ShortTimeout = 200;

	static LinuxSessionControl sessionBusControl(const QProcessEnvironment& environment,
												 int timeout = LinuxSessionControl::SessionBusCallTimeout)
	{
		// logind and ConsoleKit are faked and refuse, the session bus is real
		const auto systemBus = std::make_shared<FakeBus>();
		const auto callSystemBus = [systemBus](const QDBusMessage& methodCall) {
			return systemBus->call(methodCall);
		};
		const auto callSessionBus = [timeout](const QDBusMessage& methodCall) {
			return LinuxSessionControl::callSessionBus(methodCall, timeout);
		};

		return LinuxSessionControl({callSystemBus, callSessionBus, []() {}, []() {}}, environment);
	}
};


QTEST_GUILESS_MAIN(LinuxSessionControlTest)
#include "LinuxSessionControlTest.moc"
