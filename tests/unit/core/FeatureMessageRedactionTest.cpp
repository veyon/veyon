// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "ComputerControlInterface.h"
#include "FeatureManager.h"
#include "FeatureMessage.h"
#include "Logger.h"

// 05-tls.md §7a: values of sensitive feature arguments never reach the log, neither through the
// FeatureMessage debug operator nor through the FeatureManager log paths; features unknown to the
// local FeatureManager are logged with argument keys only
class FeatureMessageRedactionTest : public QObject
{
	Q_OBJECT
private:
	// argument indices of the plugins (UserSessionControlPlugin::Argument, DemoFeaturePlugin::Argument)
	enum class UserLoginArgument {
		Username,
		Password,
	};

	enum class DemoArgument {
		DemoAccessToken,
		VncServerPort,
		VncServerPassword,
		DemoServerHost,
		DemoServerPort,
	};

	// DemoFeaturePlugin::FeatureCommand
	enum class DemoCommand {
		StartDemoServer,
		StopDemoServer,
		StartDemoClient,
	};

	static constexpr auto UsernameMarker = "UsernameMarker-2b7c";
	static constexpr auto PasswordMarker = "PasswordMarker-7f3a";
	static constexpr auto TokenMarker = "TokenMarker-91c2";
	static constexpr auto VncPasswordMarker = "VncPasswordMarker-5e0d";
	static constexpr auto HostMarker = "HostMarker-c41e";

	static inline QStringList* s_logMessages = nullptr;

	QTemporaryDir m_configurationDirectory;
	VeyonCore* m_core = nullptr;
	QStringList m_logMessages;
	QtMessageHandler m_previousMessageHandler = nullptr;

	FeatureMessage m_userLoginMessage;
	FeatureMessage m_startDemoServerMessage;
	FeatureMessage m_startDemoClientMessage;

	static void captureMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
	{
		Q_UNUSED(type)
		Q_UNUSED(context)
		s_logMessages->append(message);
	}

	static Feature::Uid featureUid(const QString& name)
	{
		for (const auto& feature : VeyonCore::featureManager().features())
		{
			if (feature.name() == name)
			{
				return feature.uid();
			}
		}
		return {};
	}

	static QString debugString(const FeatureMessage& message)
	{
		QString output;
		QDebug(&output) << message;
		return output;
	}

	static QStringList keysOf(int index, const QString& name)
	{
		return {QString::number(index), name};
	}

	void createMessages()
	{
		m_userLoginMessage = FeatureMessage{featureUid(QStringLiteral("UserLogin"))};
		m_userLoginMessage.addArgument(UserLoginArgument::Username, QString::fromLatin1(UsernameMarker))
			.addArgument(UserLoginArgument::Password, QByteArray(PasswordMarker));

		m_startDemoServerMessage = FeatureMessage{featureUid(QStringLiteral("DemoServer")), DemoCommand::StartDemoServer};
		m_startDemoServerMessage.addArgument(DemoArgument::DemoAccessToken, QByteArray(TokenMarker))
			.addArgument(DemoArgument::VncServerPort, 11234)
			.addArgument(DemoArgument::VncServerPassword, QByteArray(VncPasswordMarker));

		m_startDemoClientMessage = FeatureMessage{featureUid(QStringLiteral("FullScreenDemo")), DemoCommand::StartDemoClient};
		m_startDemoClientMessage.addArgument(DemoArgument::DemoAccessToken, QByteArray(TokenMarker))
			.addArgument(DemoArgument::DemoServerHost, QString::fromLatin1(HostMarker))
			.addArgument(DemoArgument::DemoServerPort, 11400);
	}

	void verifyRedacted(const QString& output, const QStringList& secrets)
	{
		for (const auto& secret : secrets)
		{
			QVERIFY2(output.contains(secret) == false, qPrintable(output));
			// neither encoded
			QVERIFY2(output.contains(QString::fromLatin1(secret.toLatin1().toBase64())) == false, qPrintable(output));
		}
		QVERIFY2(output.contains(QLatin1String("<redacted>")), qPrintable(output));
	}

private Q_SLOTS:
	void initTestCase()
	{
		QVERIFY(m_configurationDirectory.isValid());
		// never read or write a real configuration
		QSettings::setPath(QSettings::NativeFormat, QSettings::SystemScope, m_configurationDirectory.filePath(QStringLiteral("system")));
		QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_configurationDirectory.filePath(QStringLiteral("user")));
		qputenv(Logger::logLevelEnvironmentVariable(), "debug");

		m_core = new VeyonCore(QCoreApplication::instance(), VeyonCore::Component::CLI, QStringLiteral("RedactionTest"),
							   VeyonCore::InitializationFlags(VeyonCore::InitializationFlag::Plugins) |
								   VeyonCore::InitializationFlag::Managers | VeyonCore::InitializationFlag::Features |
								   VeyonCore::InitializationFlag::LegacyCrypto |
								   VeyonCore::InitializationFlag::AuthenticationCredentials);
		QVERIFY(VeyonCore::isDebugging());

		for (const auto& name : {"UserLogin", "DemoServer", "FullScreenDemo", "WindowDemo", "ClipboardExchange", "DistributeFiles"})
		{
			QVERIFY2(featureUid(QLatin1String(name)).isNull() == false, name);
		}

		createMessages();

		s_logMessages = &m_logMessages;
		m_previousMessageHandler = qInstallMessageHandler(captureMessage);
	}

	void cleanupTestCase()
	{
		qInstallMessageHandler(m_previousMessageHandler);
		s_logMessages = nullptr;
		delete m_core;
		m_core = nullptr;
	}

	void init()
	{
		m_logMessages.clear();
	}

	void redactedFormat_data()
	{
		QTest::addColumn<QVariantMap>("arguments");
		QTest::addColumn<QStringList>("sensitiveKeys");
		QTest::addColumn<QString>("expected");

		QTest::newRow("empty") << QVariantMap{} << QStringList{} << QStringLiteral("{}");
		QTest::newRow("no sensitive keys") << QVariantMap{{QStringLiteral("0"), QStringLiteral("alice")}, {QStringLiteral("1"), 42}}
										   << QStringList{} << QStringLiteral(R"({0="alice", 1=42})");
		QTest::newRow("sensitive key") << QVariantMap{{QStringLiteral("0"), QStringLiteral("alice")}, {QStringLiteral("1"), QStringLiteral("secret")}}
									   << QStringList{QStringLiteral("1")} << QStringLiteral(R"({0="alice", 1=<redacted>})");
		QTest::newRow("case-insensitive") << QVariantMap{{QStringLiteral("Password"), QStringLiteral("secret")}}
										  << QStringList{QStringLiteral("password")} << QStringLiteral("{Password=<redacted>}");
		QTest::newRow("sensitive key not present") << QVariantMap{{QStringLiteral("username"), QStringLiteral("alice")}}
												   << QStringList{QStringLiteral("password")} << QStringLiteral(R"({username="alice"})");
		QTest::newRow("container value") << QVariantMap{{QStringLiteral("list"), QVariantList{1, QStringLiteral("a")}},
														{QStringLiteral("map"), QVariantMap{{QStringLiteral("k"), true}}}}
										 << QStringList{} << QStringLiteral(R"({list=[1,"a"], map={"k":true}})");
		QTest::newRow("sensitive container") << QVariantMap{{QStringLiteral("map"), QVariantMap{{QStringLiteral("k"), QStringLiteral("secret")}}}}
											 << QStringList{QStringLiteral("map")} << QStringLiteral("{map=<redacted>}");
		QTest::newRow("keys only") << QVariantMap{{QStringLiteral("0"), QStringLiteral("a")}, {QStringLiteral("1"), QStringLiteral("b")}}
								   << QStringList{QStringLiteral("0"), QStringLiteral("1")} << QStringLiteral("{0=<redacted>, 1=<redacted>}");
	}

	void redactedFormat()
	{
		QFETCH(QVariantMap, arguments);
		QFETCH(QStringList, sensitiveKeys);
		QFETCH(QString, expected);

		QCOMPARE(Logger::redacted(arguments, sensitiveKeys), expected);
	}

	void sensitiveArgumentRegistrations_data()
	{
		QTest::addColumn<QString>("featureName");
		QTest::addColumn<QStringList>("sensitiveArguments");

		// UserSessionControlPlugin::Argument::Password
		QTest::newRow("UserLogin") << QStringLiteral("UserLogin") << keysOf(1, QStringLiteral("password"));
		QTest::newRow("UserLogoff") << QStringLiteral("UserLogoff") << QStringList{};
		// DemoFeaturePlugin::Argument::DemoAccessToken, VncServerPassword for all demo features
		const auto demoArguments = keysOf(0, QStringLiteral("demoAccessToken")) + keysOf(2, QStringLiteral("vncServerPassword"));
		for (const auto& name : {"Demo", "DemoServer", "FullScreenDemo", "WindowDemo", "ShareOwnScreenFullScreen",
								 "ShareOwnScreenWindow", "ShareUserScreenFullScreen", "ShareUserScreenWindow"})
		{
			QTest::newRow(name) << QString::fromLatin1(name) << demoArguments;
		}
		// RemoteAccessFeaturePlugin::Argument::ClipboardText, ClipboardImage
		QTest::newRow("ClipboardExchange") << QStringLiteral("ClipboardExchange")
										   << keysOf(1, QStringLiteral("clipboardText")) + keysOf(2, QStringLiteral("clipboardImage"));
		QTest::newRow("RemoteControl") << QStringLiteral("RemoteControl") << QStringList{};
		// FileTransferPlugin::Argument::DataChunk
		QTest::newRow("DistributeFiles") << QStringLiteral("DistributeFiles") << keysOf(2, QStringLiteral("dataChunk"));
		QTest::newRow("FileCollect") << QStringLiteral("FileCollect") << keysOf(2, QStringLiteral("dataChunk"));
		QTest::newRow("TextMessage") << QStringLiteral("TextMessage") << QStringList{};
	}

	void sensitiveArgumentRegistrations()
	{
		QFETCH(QString, featureName);
		QFETCH(QStringList, sensitiveArguments);

		const auto uid = featureUid(featureName);
		if (uid.isNull())
		{
			QSKIP("feature not available in this build");
		}

		QCOMPARE(VeyonCore::featureManager().sensitiveArguments(uid), sensitiveArguments);
	}

	void userLoginMessage_redacted()
	{
		const auto output = debugString(m_userLoginMessage);

		verifyRedacted(output, {QString::fromLatin1(PasswordMarker)});
		QVERIFY2(output.contains(QLatin1String(UsernameMarker)), qPrintable(output));
		QVERIFY2(output.contains(QLatin1String("UserLogin")), qPrintable(output));
	}

	void demoMessages_redacted()
	{
		const auto serverOutput = debugString(m_startDemoServerMessage);
		verifyRedacted(serverOutput, {QString::fromLatin1(TokenMarker), QString::fromLatin1(VncPasswordMarker)});
		QVERIFY2(serverOutput.contains(QLatin1String("11234")), qPrintable(serverOutput));

		const auto clientOutput = debugString(m_startDemoClientMessage);
		verifyRedacted(clientOutput, {QString::fromLatin1(TokenMarker)});
		QVERIFY2(clientOutput.contains(QLatin1String(HostMarker)), qPrintable(clientOutput));
	}

	void unknownFeature_keysOnly()
	{
		FeatureMessage message{Feature::Uid::createUuid()};
		message.addArgument(0, QString::fromLatin1(UsernameMarker)).addArgument(7, QByteArray(PasswordMarker));

		const auto output = debugString(message);

		verifyRedacted(output, {QString::fromLatin1(UsernameMarker), QString::fromLatin1(PasswordMarker)});
		QVERIFY2(output.contains(QLatin1String("0=<redacted>")), qPrintable(output));
		QVERIFY2(output.contains(QLatin1String("7=<redacted>")), qPrintable(output));
	}

	void featureManagerPaths_redacted()
	{
		const auto computerControlInterface = ComputerControlInterface::Pointer::create(Computer{});

		// master path (messages received from a server)
		for (const auto* message : {&m_userLoginMessage, &m_startDemoServerMessage, &m_startDemoClientMessage})
		{
			VeyonCore::featureManager().handleFeatureMessage(computerControlInterface, *message);
		}

		// controlFeature() argument maps use camel-case names; no interfaces, so nothing is sent
		VeyonCore::featureManager().controlFeature(featureUid(QStringLiteral("UserLogin")),
												   FeatureProviderInterface::Operation::Start,
												   {{QStringLiteral("username"), QString::fromLatin1(UsernameMarker)},
													{QStringLiteral("password"), QByteArray(PasswordMarker)}},
												   {});
		VeyonCore::featureManager().controlFeature(Feature::Uid::createUuid(),
												   FeatureProviderInterface::Operation::Start,
												   {{QStringLiteral("password"), QByteArray(PasswordMarker)}}, {});

		const auto output = m_logMessages.join(QLatin1Char('\n'));
		QVERIFY2(m_logMessages.size() >= 5, qPrintable(output));
		verifyRedacted(output, {QString::fromLatin1(PasswordMarker), QString::fromLatin1(TokenMarker), QString::fromLatin1(VncPasswordMarker)});
		QVERIFY2(output.contains(QLatin1String(UsernameMarker)), qPrintable(output));
		QVERIFY2(output.contains(QLatin1String("password=<redacted>")), qPrintable(output));
	}
};


int main(int argc, char** argv)
{
	// plugins may create widgets
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QApplication app(argc, argv);
	FeatureMessageRedactionTest test;
	return QTest::qExec(&test, argc, argv);
}

#include "FeatureMessageRedactionTest.moc"
