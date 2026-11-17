// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QSettings>
#include <QtTest>

#include "FeatureManager.h"
#include "IpcEndpointCredentials.h"
#include "NetworkObjectDirectoryManager.h"
#include "PlatformPluginInterface.h"
#include "PluginManager.h"
#include "UserGroupsBackendManager.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"

// minimal core of veyon-capture-helper (02-ipc.md §1a.5): with minimalInitialization() the dumpable
// helper loads only the platform plugin, never changes the configuration and never connects to the
// service IPC; the behaviour follows the initialization flags only, never the component
class CaptureHelperCoreTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void initTestCase()
	{
		VeyonCore::setupApplicationParameters();
		m_core = createCore(VeyonCore::minimalInitialization());
	}

	void cleanupTestCase()
	{
		delete m_core;
	}

	void minimalFlags()
	{
		QVERIFY(VeyonCore::initializationFlags() == VeyonCore::minimalInitialization());
		QVERIFY(VeyonCore::minimalInitialization() == VeyonCore::InitializationFlags());

		// capabilities are never part of a preset
		QVERIFY(VeyonCore::defaultInitialization().testFlag(VeyonCore::InitializationFlag::ServiceIpc) == false);
		QVERIFY(VeyonCore::defaultInitialization().testFlag(VeyonCore::InitializationFlag::SecretStore) == false);
		QVERIFY(VeyonCore::defaultInitialization().testFlag(VeyonCore::InitializationFlag::WritableConfiguration));
	}

	void nothingElseInitialized()
	{
		QVERIFY(VeyonCore::instance()->findChild<UserGroupsBackendManager*>() == nullptr);
		QVERIFY(VeyonCore::instance()->findChild<NetworkObjectDirectoryManager*>() == nullptr);
		QVERIFY(VeyonCore::instance()->findChild<FeatureManager*>() == nullptr);
	}

	void onlyPlatformPluginLoaded()
	{
		const auto& pluginObjects = VeyonCore::pluginManager().pluginObjects();
		QVERIFY(pluginObjects.isEmpty() == false);
		for (auto* pluginObject : pluginObjects)
		{
			QVERIFY2(qobject_cast<PlatformPluginInterface*>(pluginObject) != nullptr,
					 pluginObject->metaObject()->className());
		}
	}

	void configurationNotModified()
	{
		// no upgrade() and no generated installation ID: the in-memory value is what is stored
		QSettings systemSettings(QSettings::SystemScope, QStringLiteral("Veyon Solutions"), QStringLiteral("Veyon"));
		QCOMPARE(VeyonCore::config().installationID(),
				 systemSettings.value(QStringLiteral("Core/InstallationID")).toString());
	}

	void serviceIpcRefused()
	{
		auto credentials = IpcEndpointCredentials::generate(IpcProtocol::ServerEndpointPrefix, 0);
		QVERIFY(credentials.has_value());

		// refused before any connection attempt (the endpoint wouldn't exist here anyway)
		s_messages.clear();
		const auto previousHandler = qInstallMessageHandler(&CaptureHelperCoreTest::collectMessage);
		const auto connected = m_core->connectToServiceIpc(std::move(*credentials));
		qInstallMessageHandler(previousHandler);

		QVERIFY(connected == false);
		QVERIFY(VeyonCore::serviceIpcClient() == nullptr);
		QVERIFY2(s_messages.join(QLatin1Char('\n')).contains(QStringLiteral("refusing service IPC")),
				 qPrintable(s_messages.join(QLatin1Char('\n'))));
	}

	void serviceIpcFollowsFlagNotComponent()
	{
		// same component with the ServiceIpc flag: not refused by the core (the connection attempt
		// itself fails as there is no service endpoint here)
		delete m_core;
		m_core = createCore(VeyonCore::minimalInitialization() | VeyonCore::InitializationFlag::ServiceIpc);

		auto credentials = IpcEndpointCredentials::generate(IpcProtocol::ServerEndpointPrefix, 0);
		QVERIFY(credentials.has_value());

		s_messages.clear();
		const auto previousHandler = qInstallMessageHandler(&CaptureHelperCoreTest::collectMessage);
		const auto connected = m_core->connectToServiceIpc(std::move(*credentials));
		qInstallMessageHandler(previousHandler);

		QVERIFY(connected == false);
		QVERIFY(VeyonCore::serviceIpcClient() == nullptr);
		QVERIFY2(s_messages.join(QLatin1Char('\n')).contains(QStringLiteral("refusing service IPC")) == false,
				 qPrintable(s_messages.join(QLatin1Char('\n'))));
	}

private:
	static VeyonCore* createCore(VeyonCore::InitializationFlags flags)
	{
		return new VeyonCore(QCoreApplication::instance(), VeyonCore::Component::CaptureHelper,
							 QStringLiteral("CaptureHelperCoreTest"), flags);
	}

	static void collectMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
	{
		Q_UNUSED(type)
		Q_UNUSED(context)
		s_messages.append(message);
	}

	static QStringList s_messages; // clazy:exclude=non-pod-global-static

	VeyonCore* m_core = nullptr;
};

QStringList CaptureHelperCoreTest::s_messages;


QTEST_GUILESS_MAIN(CaptureHelperCoreTest)
#include "CaptureHelperCoreTest.moc"
