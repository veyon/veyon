// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QPluginLoader>
#include <QtTest>

#include "PluginManager.h"

// plugins with unresolved symbols (e.g. built for an older Veyon version) must be refused at load time
// instead of terminating the process with a symbol lookup error once a missing function is called;
// plugins built for any other Veyon version must be refused before they are instantiated
class PluginLoadingTest : public QObject
{
	Q_OBJECT
private:
	static QStringList& messages()
	{
		static QStringList list;
		return list;
	}

	static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
	{
		Q_UNUSED(context)
		if (type == QtCriticalMsg || type == QtWarningMsg)
		{
			messages().append(message);
		}
	}

	static qsizetype messageCount(const QString& text)
	{
		return messages().filter(text).size();
	}

	static bool isInstantiated(const char* pluginName)
	{
		const QByteArray variableName = QByteArrayLiteral("VEYON_TEST_PLUGIN_INSTANTIATED_") + pluginName;
		return qEnvironmentVariableIsSet(variableName.constData());
	}

	void checkRefusedBeforeInstantiation(const QString& filePath, const char* pluginName, const QString& builtFor)
	{
		QPluginLoader pluginLoader(filePath);
		QCOMPARE(PluginManager::loadPluginInstance(pluginLoader), nullptr);
		QVERIFY(isInstantiated(pluginName) == false);
		QVERIFY(pluginLoader.isLoaded() == false);

		const auto message = messages().filter(QStringLiteral("is incompatible with this Veyon version"));
		QCOMPARE(message.size(), 1);
		QVERIFY(message.first().contains(filePath));
		QVERIFY(message.first().contains(builtFor));
		QVERIFY(message.first().contains(QStringLiteral("Veyon %1").arg(PluginManager::buildVersion())));
		QVERIFY(PluginManager::incompatiblePluginFiles().contains(filePath));

		// further attempts do not log again and still do not instantiate the plugin
		QPluginLoader anotherPluginLoader(filePath);
		QCOMPARE(PluginManager::loadPluginInstance(anotherPluginLoader), nullptr);
		QVERIFY(isInstantiated(pluginName) == false);
		QCOMPARE(messageCount(QStringLiteral("is incompatible with this Veyon version")), 1);
	}

	QtMessageHandler m_previousMessageHandler = nullptr;

private Q_SLOTS:
	void init()
	{
		messages().clear();
		m_previousMessageHandler = qInstallMessageHandler(messageHandler);
	}

	void cleanup()
	{
		qInstallMessageHandler(m_previousMessageHandler);
	}

	void buildVersionIsMajorMinorPatch()
	{
		const auto version = QVersionNumber::fromString(PluginManager::buildVersion());
		QCOMPARE(version.segmentCount(), 3);
		QCOMPARE(version.toString(), PluginManager::buildVersion());
	}

	void validPluginLoads()
	{
		QVERIFY(isInstantiated("valid") == false);
		QPluginLoader pluginLoader(QStringLiteral(VALID_TEST_PLUGIN_FILE_PATH));
		QVERIFY(PluginManager::loadPluginInstance(pluginLoader) != nullptr);
		QVERIFY(isInstantiated("valid"));
		QVERIFY(pluginLoader.loadHints().testFlag(QLibrary::ResolveAllSymbolsHint));
		QVERIFY(messages().isEmpty());
		QVERIFY(PluginManager::incompatiblePluginFiles().isEmpty());
	}

	void incompatiblePluginIsRefused()
	{
		const auto filePath = QStringLiteral(INCOMPATIBLE_TEST_PLUGIN_FILE_PATH);

		QPluginLoader pluginLoader(filePath);
		QCOMPARE(PluginManager::loadPluginInstance(pluginLoader), nullptr);
		QVERIFY(pluginLoader.isLoaded() == false);
		QVERIFY(pluginLoader.errorString().contains(QStringLiteral("veyonMissingSymbolForTest")));

		const auto message = messages().filter(QStringLiteral("is incompatible with this Veyon version"));
		QCOMPARE(message.size(), 1);
		QVERIFY(message.first().contains(filePath));
		QVERIFY(message.first().contains(QStringLiteral("veyonMissingSymbolForTest")));
		QCOMPARE(PluginManager::incompatiblePluginFiles(), QStringList{filePath});

		// further attempts (e.g. loadPlatformPlugins() and loadPlugins()) do not log again
		QPluginLoader anotherPluginLoader(filePath);
		QCOMPARE(PluginManager::loadPluginInstance(anotherPluginLoader), nullptr);
		QCOMPARE(messageCount(QStringLiteral("is incompatible with this Veyon version")), 1);
		QCOMPARE(PluginManager::incompatiblePluginFiles(), QStringList{filePath});
	}

	void nonPluginLibraryIsNotReported()
	{
		QPluginLoader pluginLoader(QStringLiteral(NON_PLUGIN_LIBRARY_FILE_PATH));
		QCOMPARE(PluginManager::loadPluginInstance(pluginLoader), nullptr);
		QCOMPARE(messageCount(QStringLiteral("is incompatible with this Veyon version")), 0);
		QVERIFY(PluginManager::incompatiblePluginFiles().contains(pluginLoader.fileName()) == false);
	}

	void pluginForOtherVersionIsRefused()
	{
		checkRefusedBeforeInstantiation(QStringLiteral(OTHER_VERSION_TEST_PLUGIN_FILE_PATH), "other-version",
										QStringLiteral("built for Veyon " OTHER_VERSION_TEST_PLUGIN_VERSION));
	}

	void pluginWithoutBuildVersionIsRefused()
	{
		checkRefusedBeforeInstantiation(QStringLiteral(UNVERSIONED_TEST_PLUGIN_FILE_PATH), "unversioned",
										QStringLiteral("built for an unknown Veyon version"));
	}

	void validPluginStillLoadsAfterIncompatiblePlugin()
	{
		QPluginLoader pluginLoader(QStringLiteral(VALID_TEST_PLUGIN_FILE_PATH));
		QVERIFY(PluginManager::loadPluginInstance(pluginLoader) != nullptr);
		QCOMPARE(messageCount(QStringLiteral("is incompatible with this Veyon version")), 0);
		QCOMPARE(PluginManager::incompatiblePluginFiles(),
				 QStringList({QStringLiteral(INCOMPATIBLE_TEST_PLUGIN_FILE_PATH),
							  QStringLiteral(OTHER_VERSION_TEST_PLUGIN_FILE_PATH),
							  QStringLiteral(UNVERSIONED_TEST_PLUGIN_FILE_PATH)}));
	}
};

QTEST_GUILESS_MAIN(PluginLoadingTest)
#include "PluginLoadingTest.moc"
