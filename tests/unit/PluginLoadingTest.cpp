// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QPluginLoader>
#include <QtTest>

#include "PluginManager.h"

// plugins with unresolved symbols (e.g. built for an older Veyon version) must be refused at load time
// instead of terminating the process with a symbol lookup error once a missing function is called
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

	void validPluginLoads()
	{
		QPluginLoader pluginLoader(QStringLiteral(VALID_TEST_PLUGIN_FILE_PATH));
		QVERIFY(PluginManager::loadPluginInstance(pluginLoader) != nullptr);
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

	void validPluginStillLoadsAfterIncompatiblePlugin()
	{
		QPluginLoader pluginLoader(QStringLiteral(VALID_TEST_PLUGIN_FILE_PATH));
		QVERIFY(PluginManager::loadPluginInstance(pluginLoader) != nullptr);
		QCOMPARE(messageCount(QStringLiteral("is incompatible with this Veyon version")), 0);
	}
};

QTEST_GUILESS_MAIN(PluginLoadingTest)
#include "PluginLoadingTest.moc"
