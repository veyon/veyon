// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFile>
#include "Configuration/Object.h"
#include "Configuration/Property.h"
#include "Configuration/PropertyModel.h"
#include "Configuration/Proxy.h"
#include "Configuration/JsonStore.h"
#include "VeyonConfiguration.h"

class ConfigurationAspectTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void replacementNotifiesOnce()
	{
		Configuration::Object object;
		QSignalSpy changed(&object, &Configuration::Object::configurationChanged);
		object.replaceData({{QStringLiteral("Root"), 42}});
		QCOMPARE(changed.count(), 1);
		object.replaceData(object.data());
		QCOMPARE(changed.count(), 1);
		object.clear();
		QCOMPARE(changed.count(), 2);
	}

	void invalidEditorValueDoesNotChangeConfiguration()
	{
		Configuration::Object object;
		Configuration::TypedProperty<int> port(&object, QStringLiteral("Port"), QStringLiteral("Network"), 1234, Configuration::Property::Flag::Standard);
		Configuration::Property::UiMetadata metadata;
		metadata.label = QStringLiteral("Port");
		metadata.minimum = 1;
		metadata.maximum = 65535;
		port.setUiMetadata(metadata);
		QString error;
		QVERIFY(!port.setEditorValue(65536, &error));
		QVERIFY(!error.isEmpty());
		QCOMPARE(port.value(), 1234);
		QVERIFY(!port.setEditorValue(12.5, &error));
		QVERIFY(!port.setEditorValue(true, &error));
		QVERIFY(!port.setEditorValue(QStringLiteral("not a number"), &error));
		QVERIFY(port.setEditorValue(11100, &error));
		QVERIFY(error.isEmpty());
		QCOMPARE(port.value(), 11100);
	}

	void passwordTypeDoesNotFollowItsStringDefault()
	{
		Configuration::Object object;
		Configuration::TypedProperty<Configuration::Password> password(&object, QStringLiteral("Password"), QStringLiteral("Test"), QString(), Configuration::Property::Flag::Standard);
		QCOMPARE(password.valueType(), QMetaType::fromType<Configuration::Password>());
		QCOMPARE(password.editor(), Configuration::Property::Editor::Password);
		QString error;
		QVERIFY(!password.setEditorValue(42, &error));
		QVERIFY(!error.isEmpty());
		QVERIFY(object.data().isEmpty());
	}

	void unsetPasswordDoesNotRequireCryptoCore()
	{
		QVERIFY(!VeyonCore::instance());
		QVERIFY(Configuration::Password{}.plainText().toByteArray().isEmpty());
		Configuration::Object object;
		Configuration::TypedProperty<Configuration::Password> password(&object,
			QStringLiteral("Password"), QStringLiteral("Test"), QString(), Configuration::Property::Flag::Standard);
		QVERIFY(password.editorValue().toString().isEmpty());
		QVERIFY(object.data().isEmpty());
	}

	void invalidJsonDoesNotMutateDestination()
	{
		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		const auto path = directory.filePath(QStringLiteral("broken.json"));
		QFile file(path);
		QVERIFY(file.open(QIODevice::WriteOnly));
		file.write("{not json}");
		file.close();
		Configuration::Object object;
		object.replaceData({{QStringLiteral("existing"), 42}});
		const auto before = object.data();
		Configuration::JsonStore store(Configuration::Store::Scope::System, path);
		QString error;
		QVERIFY(!store.loadChecked(&object, &error));
		QVERIFY(!error.isEmpty());
		QCOMPARE(object.data(), before);
	}

	void failedExportReportsError()
	{
		QTemporaryDir directory;
		Configuration::Object object;
		Configuration::JsonStore store(Configuration::Store::Scope::System, directory.path());
		QString error;
		QVERIFY(!store.flushChecked(&object, &error));
		QVERIFY(!error.isEmpty());
	}

	void modelUpdatesAfterRevert()
	{
		VeyonConfiguration configuration(nullptr);
		Configuration::PropertyModel model(&configuration);
		QVERIFY(model.setValue(QStringLiteral("Master/ComputerMonitoringThumbnailSpacing"), 15));
		QCOMPARE(configuration.computerMonitoringThumbnailSpacing(), 15);
		configuration.replaceData({});
		QCOMPARE(model.value(QStringLiteral("Master/ComputerMonitoringThumbnailSpacing")).toInt(), 5);
		QVERIFY(!model.setValue(QStringLiteral("Core/ApplicationVersion"), 0));
	}

	void enumerationRejectsUnknownValues()
	{
		VeyonConfiguration configuration(nullptr);
		QString error;
		QVERIFY(!configuration.uiColorSchemeProperty().setEditorValue(27, &error));
		QVERIFY(!configuration.uiColorSchemeProperty().setEditorValue(true, &error));
		QCOMPARE(configuration.uiColorScheme(), VeyonCore::UiColorScheme::System);
		QVERIFY(configuration.uiColorSchemeProperty().setEditorValue(2, &error));
		QCOMPARE(configuration.uiColorScheme(), VeyonCore::UiColorScheme::Dark);
	}

	void metadataCoversEveryGlobalProperty()
	{
		VeyonConfiguration configuration(nullptr);
		const auto properties = configuration.findChildren<Configuration::Property*>();
		QVERIFY(!properties.isEmpty());
		for (const auto property : properties)
			QVERIFY2(!property->uiMetadata().label.isEmpty(), qPrintable(property->absoluteKey()));
	}

	void jsonRoundTripPreservesUnknownPluginKeys()
	{
		QTemporaryDir directory;
		Configuration::Object source;
		source.setValue(QStringLiteral("Option"), QStringLiteral("future value"), QStringLiteral("FuturePlugin"));
		source.setValue(QStringLiteral("List"), QJsonArray{1, 2, 3}, QStringLiteral("FuturePlugin"));
		Configuration::JsonStore store(Configuration::Store::Scope::System, directory.filePath(QStringLiteral("config.json")));
		QString error;
		QVERIFY2(store.flushChecked(&source, &error), qPrintable(error));
		Configuration::Object restored;
		QVERIFY2(store.loadChecked(&restored, &error), qPrintable(error));
		QCOMPARE(restored.data(), source.data());
	}

	void dependenciesFollowReplacedDraft()
	{
		VeyonConfiguration configuration(nullptr);
		QVERIFY(!configuration.maximumSessionCountProperty().isEnabled());
		configuration.setMultiSessionModeEnabled(true);
		QVERIFY(configuration.maximumSessionCountProperty().isEnabled());
		configuration.replaceData({});
		QVERIFY(!configuration.maximumSessionCountProperty().isEnabled());
	}

	void modelSearchMatchesLabelsAndKeys()
	{
		VeyonConfiguration configuration(nullptr);
		Configuration::PropertyModel model(&configuration);
		model.setPageId(QStringLiteral("monitoring"));
		model.setSearch(QStringLiteral("thumbnail spacing"));
		QCOMPARE(model.rowCount(), 1);
		QCOMPARE(model.data(model.index(0), Configuration::PropertyModel::KeyRole).toString(),
			QStringLiteral("Master/ComputerMonitoringThumbnailSpacing"));
		model.setSearch(QStringLiteral("ComputerMonitoringThumbnailSpacing"));
		QCOMPARE(model.rowCount(), 1);
		model.setSearch(QStringLiteral("a setting that does not exist"));
		QCOMPARE(model.rowCount(), 0);
	}

	void destroyedProxyDisablesDeferredProperties()
	{
		Configuration::Object object;
		auto proxy = new Configuration::Proxy(&object);
		QPointer<Configuration::Proxy> proxyGuard(proxy);
		QPointer<Configuration::Property> property = new Configuration::TypedProperty<bool>(proxy,
			QStringLiteral("Enabled"), QStringLiteral("Plugin"), false, Configuration::Property::Flag::Standard);
		bool evaluatedAfterOwnerDestruction = false;
		Configuration::Property::UiMetadata metadata;
		metadata.label = QStringLiteral("Enable plugin");
		metadata.enabled = metadata.visible = [&]() {
			evaluatedAfterOwnerDestruction |= proxyGuard.isNull();
			return true;
		};
		property->setUiMetadata(metadata);
		Configuration::PropertyModel model(&object);
		QCOMPARE(model.rowCount(), 1);
		delete proxy;
		QVERIFY(property);
		QVERIFY(!property->isEnabled());
		QVERIFY(!property->isVisible());
		QVERIFY(!evaluatedAfterOwnerDestruction);
		QCOMPARE(model.rowCount(), 0);
		QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
		QVERIFY(property.isNull());
	}
};

QTEST_GUILESS_MAIN(ConfigurationAspectTest)
#include "ConfigurationAspectTest.moc"
