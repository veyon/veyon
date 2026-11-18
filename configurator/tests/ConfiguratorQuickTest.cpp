// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>
#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QScopeGuard>
#include <QSettings>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QWidget>
#include <QValidator>
#include "ConfiguratorController.h"
#include "Configuration/LocalStore.h"
#include "QuickUi.h"

namespace
{
QQuickItem* visualItem(QQuickItem* item, const QString& name)
{
	if (!item) return nullptr;
	if (item->objectName() == name) return item;
	for (auto child : item->childItems())
		if (auto found = visualItem(child, name)) return found;
	return nullptr;
}

QQuickItem* actionWithText(QQuickItem* item, const QString& text)
{
	if (!item) return nullptr;
	if (item->inherits("QQuickAbstractButton") && item->isVisible() && item->property("text").toString() == text) return item;
	for (auto child : item->childItems())
		if (auto found = actionWithText(child, text)) return found;
	return nullptr;
}

bool hasEnabledSaveAction(QQuickItem* item)
{
	if (!item) return false;
	const auto text = item->property("text").toString();
	if (item->inherits("QQuickAbstractButton") && item->isVisible() && item->isEnabled() &&
		(text == QCoreApplication::translate("Configurator", "Apply") ||
		 text == QCoreApplication::translate("Configurator", "Save"))) return true;
	for (auto child : item->childItems())
		if (hasEnabledSaveAction(child)) return true;
	return false;
}
}

class EditorFixture : public QObject
{
	Q_OBJECT
	Q_PROPERTY(Configuration::PropertyModel* properties MEMBER model CONSTANT)
public:
	Configuration::PropertyModel* model;
};

class ConfiguratorQuickTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void initTestCase()
	{
#ifndef Q_OS_WIN
		QVERIFY(m_configurationRoot.isValid());
		QSettings::setPath(QSettings::NativeFormat, QSettings::SystemScope, m_configurationRoot.filePath(QStringLiteral("system")));
		QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_configurationRoot.filePath(QStringLiteral("user")));
#endif
		m_core = std::make_unique<VeyonCore>(qApp, VeyonCore::Component::Configurator, QStringLiteral("ConfiguratorQuickTest"));
#ifndef Q_OS_WIN
		Configuration::LocalStore store(Configuration::Store::Scope::System);
		std::unique_ptr<QSettings> settings(store.createSettingsObject());
		QVERIFY(settings->fileName().startsWith(m_configurationRoot.path() + QLatin1Char('/')));
		QString error;
		QVERIFY2(store.flushChecked(&VeyonCore::config(), &error), qPrintable(error));
#endif
	}
	void nativeWindowLoadsAndDraftEditsStayLocal()
	{
		const auto before = VeyonCore::config().data();
		ConfiguratorController controller;
		std::unique_ptr<QQmlApplicationEngine> engine(QuickUi::createEngine());
		engine->rootContext()->setContextProperty(QStringLiteral("configurator"), &controller);
		QStringList errors;
		connect(engine.get(), &QQmlEngine::warnings, this, [&errors](const QList<QQmlError>& warnings) {
			for (const auto& warning : warnings) errors.append(warning.toString());
		});
		engine->load(QUrl(QStringLiteral("qrc:/configurator/Configurator.qml")));
		QVERIFY2(!engine->rootObjects().isEmpty(), qPrintable(errors.join(QLatin1Char('\n'))));
		auto window = qobject_cast<QQuickWindow*>(engine->rootObjects().first());
		QVERIFY(window);
		controller.setAdvanced(true);
		const auto loginSession = controller.properties()->propertyInfo(QStringLiteral("Linux/UserLoginSession"));
		if (!loginSession.isEmpty())
		{
			QVERIFY(!loginSession.value(QStringLiteral("label")).toString().isEmpty());
			QCOMPARE(loginSession.value(QStringLiteral("editor")).toString(), QStringLiteral("text"));
		}
		for (const auto& page : controller.pages())
		{
			QVERIFY(QMetaObject::invokeMethod(window, "selectPage", Q_ARG(QVariant, page)));
			QCoreApplication::processEvents();
		}
		QVERIFY2(errors.isEmpty(), qPrintable(errors.join(QLatin1Char('\n'))));
		for (auto widget : QApplication::topLevelWidgets()) QVERIFY(!widget->isVisible());
		QVERIFY(controller.properties()->setValue(QStringLiteral("Master/ComputerMonitoringThumbnailSpacing"), 14));
		QVERIFY(controller.dirty()); QCOMPARE(VeyonCore::config().data(), before);
		controller.revert(); QVERIFY(!controller.dirty()); QCOMPARE(VeyonCore::config().data(), before);
	}
	void initialAndUnchangedConfigurationCanBeSaved_data()
	{
		QTest::addColumn<bool>("emptyStore");
		QTest::newRow("initialized-but-unsaved") << true;
		QTest::newRow("unchanged-configured-defaults") << false;
	}
	void initialAndUnchangedConfigurationCanBeSaved()
	{
#ifdef Q_OS_WIN
		QSKIP("This store-isolation fixture uses native file settings, not the Windows registry");
#else
		QFETCH(bool, emptyStore);
		Configuration::LocalStore store(Configuration::Store::Scope::System);
		Configuration::Object stored;
		QString error;
		QVERIFY2(store.loadChecked(&stored, &error), qPrintable(error));
		if (emptyStore) QVERIFY2(store.clearChecked(&error), qPrintable(error));
		ConfiguratorController controller;
		std::unique_ptr<QQmlApplicationEngine> engine(QuickUi::createEngine());
		engine->rootContext()->setContextProperty(QStringLiteral("configurator"), &controller);
		engine->load(QUrl(QStringLiteral("qrc:/configurator/Configurator.qml")));
		// Restore the isolated store before any UI assertion can return from the test.
		QVERIFY2(store.flushChecked(&stored, &error), qPrintable(error));
		QVERIFY(!engine->rootObjects().isEmpty());
		auto window = qobject_cast<QQuickWindow*>(engine->rootObjects().first());
		QVERIFY(window);
		QCoreApplication::processEvents();
		if (!hasEnabledSaveAction(window->contentItem()))
		{
			auto more = actionWithText(window->contentItem(), QCoreApplication::translate("Configurator", "More"));
			QVERIFY(more);
			QVERIFY(QMetaObject::invokeMethod(more, "clicked"));
			QCoreApplication::processEvents();
		}
		QTRY_VERIFY2(hasEnabledSaveAction(window->contentItem()),
			"Saving initial/default configuration must not require an unrelated edit; provide an enabled Apply or explicit Save action");
		// Deliberately do not activate Apply: it can change a real systemd service's start mode.
#endif
	}
	void unchangedConfigurationExportsWithoutChangingTheStore()
	{
#ifdef Q_OS_WIN
		QSKIP("This store-isolation fixture uses native file settings, not the Windows registry");
#else
		Configuration::LocalStore store(Configuration::Store::Scope::System);
		Configuration::Object before, after;
		QString error;
		QVERIFY2(store.loadChecked(&before, &error), qPrintable(error));
		ConfiguratorController controller;
		QVERIFY(!controller.dirty());
		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		const auto path = directory.filePath(QStringLiteral("unchanged.json"));
		QVERIFY2(controller.exportConfiguration(path), qPrintable(controller.error()));
		VeyonConfiguration exported(nullptr);
		Configuration::JsonStore json(Configuration::Store::Scope::System, path);
		QVERIFY2(json.loadChecked(&exported, &error), qPrintable(error));
		QCOMPARE(exported.veyonServerPort(), 11100);
		QCOMPARE(exported.computerMonitoringThumbnailSpacing(), 5);
		QCOMPARE(exported.installationID(), VeyonCore::config().installationID());
		QVERIFY2(store.loadChecked(&after, &error), qPrintable(error));
		QCOMPARE(after.data(), before.data());
		QVERIFY(!controller.dirty());
#endif
	}
	void sessionModesRemainMutuallyExclusive_data()
	{
		QTest::addColumn<QString>("firstMode");
		QTest::addColumn<QString>("secondMode");
		QTest::newRow("multi-to-active") << QStringLiteral("Service/MultiSession") << QStringLiteral("Service/ActiveSession");
		QTest::newRow("active-to-multi") << QStringLiteral("Service/ActiveSession") << QStringLiteral("Service/MultiSession");
	}
	void sessionModesRemainMutuallyExclusive()
	{
		QFETCH(QString, firstMode);
		QFETCH(QString, secondMode);
		const auto live = VeyonCore::config().data();
		ConfiguratorController controller;
		auto model = controller.properties();
		QVERIFY(model->setValue(QStringLiteral("Service/ActiveSession"), false));
		QVERIFY(model->setValue(QStringLiteral("Service/MultiSession"), false));
		QVERIFY(model->setValue(firstMode, true));
		QVERIFY(model->setValue(secondMode, true));
		QVERIFY(model->value(secondMode).toBool());
		QVERIFY2(!model->value(firstMode).toBool(), "Selecting a session mode must clear the previously selected mode");
		QVERIFY(model->setValue(secondMode, false));
		QVERIFY(!model->value(QStringLiteral("Service/ActiveSession")).toBool());
		QVERIFY(!model->value(QStringLiteral("Service/MultiSession")).toBool());
		QCOMPARE(VeyonCore::config().data(), live);
	}
	void importingBothSessionFlagsPreservesStoredValues()
	{
		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		QFile file(directory.filePath(QStringLiteral("session-modes.json")));
		QVERIFY(file.open(QIODevice::WriteOnly));
		const QByteArray json = R"({"Service":{"ActiveSession":true,"MultiSession":true}})";
		QCOMPARE(file.write(json), qint64(json.size()));
		file.close();
		ConfiguratorController controller;
		QVERIFY(controller.importConfiguration(file.fileName()));
		QVERIFY(controller.properties()->value(QStringLiteral("Service/ActiveSession")).toBool());
		QVERIFY(controller.properties()->value(QStringLiteral("Service/MultiSession")).toBool());
	}
	void ldapLocationNameIsEditableOnlyForGroupsAndContainers_data()
	{
		QTest::addColumn<int>("mode");
		QTest::addColumn<bool>("editable");
		QTest::newRow("groups") << 0 << true;
		QTest::newRow("containers") << 1 << true;
		QTest::newRow("attribute") << 2 << false;
	}
	void ldapLocationNameIsEditableOnlyForGroupsAndContainers()
	{
		QFETCH(int, mode);
		QFETCH(bool, editable);
		const auto live = VeyonCore::config().data();
		ConfiguratorController controller;
		auto ldap = controller.plugins().value(QStringLiteral("ldap")).value<QObject*>();
		QVERIFY2(ldap, "Run this test with the LDAP plugin installed in Veyon's plugin search path");
		QVERIFY(QMetaObject::invokeMethod(ldap, "setLocationMode", Q_ARG(int, mode)));
		auto model = controller.properties();
		const auto key = QStringLiteral("LDAP/LocationNameAttribute");
		const auto info = model->propertyInfo(key);
		QVERIFY(!info.isEmpty());
		QCOMPARE(info.value(QStringLiteral("enabled")).toBool(), editable);
		const auto before = model->value(key);
		QCOMPARE(model->setValue(key, QStringLiteral("parityLocationName")), editable);
		QCOMPARE(model->value(key), editable ? QVariant(QStringLiteral("parityLocationName")) : before);
		QCOMPARE(VeyonCore::config().data(), live);
	}
	void dependentSettingsRequireTheirEnablingOption_data()
	{
		QTest::addColumn<QString>("enabler");
		QTest::addColumn<QString>("dependent");
		QTest::newRow("log-rotation") << QStringLiteral("Logging/LogFileSizeLimitEnabled") << QStringLiteral("Logging/LogFileRotationEnabled");
		QTest::newRow("hidden-locations") << QStringLiteral("Master/ShowCurrentLocationOnly") << QStringLiteral("Master/AllowAddingHiddenLocations");
	}
	void dependentSettingsRequireTheirEnablingOption()
	{
		QFETCH(QString, enabler);
		QFETCH(QString, dependent);
		ConfiguratorController controller;
		auto model = controller.properties();
		QVERIFY(model->setValue(enabler, false));
		QVERIFY2(!model->propertyInfo(dependent).value(QStringLiteral("enabled")).toBool(),
			"The dependent option must be disabled while its prerequisite is disabled");
		QVERIFY(!model->setValue(dependent, true));
		QVERIFY(model->setValue(enabler, true));
		QVERIFY(model->propertyInfo(dependent).value(QStringLiteral("enabled")).toBool());
		QVERIFY(model->setValue(dependent, true));
		QVERIFY(model->setValue(enabler, false));
		QVERIFY(model->value(dependent).toBool());
		QVERIFY(!model->setValue(dependent, false));
	}
	void keyAuthenticationTestsEffectivePrivateKey_data()
	{
		QTest::addColumn<QByteArray>("selectedKey");
		QTest::addColumn<bool>("success");
		QTest::newRow("autodetected-private-only") << QByteArray() << true;
		QTest::newRow("selected-private-only") << QByteArray("teacher") << true;
		QTest::newRow("selected-missing-key") << QByteArray("missing") << false;
		QTest::newRow("invalid-selected-name") << QByteArray("../teacher") << false;
	}
	void keyAuthenticationTestsEffectivePrivateKey()
	{
		QFETCH(QByteArray, selectedKey);
		QFETCH(bool, success);
		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		const auto privateDirectory = directory.filePath(QStringLiteral("private"));
		const auto keyDirectory = directory.filePath(QStringLiteral("private/teacher"));
		QVERIFY(QDir().mkpath(keyDirectory));
		const auto keyPath = QDir(keyDirectory).filePath(QStringLiteral("key"));
		QVERIFY(QFile::copy(QStringLiteral(":/core/default-pkey.pem"), keyPath));
		QVERIFY(!CryptoCore::PrivateKey(keyPath).isNull());
		const auto previousKey = qgetenv("VEYON_AUTH_KEY_NAME");
		const auto previousKeyWasSet = qEnvironmentVariableIsSet("VEYON_AUTH_KEY_NAME");
		const auto restoreEnvironment = qScopeGuard([previousKey, previousKeyWasSet]() {
			if (previousKeyWasSet) qputenv("VEYON_AUTH_KEY_NAME", previousKey);
			else qunsetenv("VEYON_AUTH_KEY_NAME");
		});
		qputenv("VEYON_AUTH_KEY_NAME", selectedKey);
		const auto live = VeyonCore::config().data();
		ConfiguratorController controller;
		QVERIFY(controller.properties()->setValue(QStringLiteral("Authentication/Method"), 1));
		QVERIFY(controller.properties()->setValue(QStringLiteral("Authentication/PrivateKeyBaseDir"), privateDirectory));
		QVERIFY(controller.properties()->setValue(QStringLiteral("Authentication/PublicKeyBaseDir"), directory.filePath(QStringLiteral("absent-public-directory"))));
		QCOMPARE(controller.testAuthentication(QString(), QString()), success);
		std::unique_ptr<QQmlApplicationEngine> engine(QuickUi::createEngine());
		engine->rootContext()->setContextProperty(QStringLiteral("configurator"), &controller);
		engine->load(QUrl(QStringLiteral("qrc:/configurator/Configurator.qml")));
		QVERIFY(!engine->rootObjects().isEmpty());
		auto window = qobject_cast<QQuickWindow*>(engine->rootObjects().first());
		QVERIFY(window);
		for (const auto& page : controller.pages())
			if (page.toMap().value(QStringLiteral("id")) == QStringLiteral("authentication"))
				QVERIFY(QMetaObject::invokeMethod(window, "selectPage", Q_ARG(QVariant, page)));
		QCoreApplication::processEvents();
		auto testAction = actionWithText(window->contentItem(), QCoreApplication::translate("KeysPage", "Test authentication"));
		QVERIFY(testAction);
		QObject* results = nullptr;
		for (auto object : window->findChildren<QObject*>())
			if (object->metaObject()->indexOfProperty("authResult") >= 0) { results = object; break; }
		QVERIFY(results);
		QVERIFY(QMetaObject::invokeMethod(testAction, "clicked"));
		QCOMPARE(results->property("authResult").toString(),
			success ? QCoreApplication::translate("KeysPage", "Authentication succeeded.") : QString());
		QCOMPARE(VeyonCore::config().data(), live);
		QVERIFY(QFileInfo::exists(keyPath));
	}
	void keyAccessGroupInputChangesActualFilePermissions()
	{
#ifdef Q_OS_WIN
		QSKIP("This test asserts Unix group read/write permissions; Windows requires an ACL fixture");
#else
		QTemporaryDir directory;
		QVERIFY(directory.isValid());
		const auto privateDirectory = directory.filePath(QStringLiteral("private"));
		const auto keyDirectory = directory.filePath(QStringLiteral("private/teacher"));
		QVERIFY(QDir().mkpath(keyDirectory));
		const auto keyPath = QDir(keyDirectory).filePath(QStringLiteral("key"));
		QVERIFY(QFile::copy(QStringLiteral(":/core/default-pkey.pem"), keyPath));
		const auto group = QFileInfo(keyPath).group();
		QVERIFY(!group.isEmpty());
		QVERIFY(QFile::setPermissions(keyPath, QFile::ReadOwner | QFile::WriteOwner | QFile::WriteGroup | QFile::ExeGroup));
		QFile key(keyPath);
		QVERIFY(key.open(QIODevice::ReadOnly));
		const auto originalContents = key.readAll();
		key.close();
		ConfiguratorController controller;
		QVERIFY(controller.properties()->setValue(QStringLiteral("Authentication/PrivateKeyBaseDir"), privateDirectory));
		auto keys = controller.plugins().value(QStringLiteral("keys")).value<QObject*>();
		QVERIFY2(keys, "Run this test with the authentication-keys plugin installed");
		bool success = false;
		QVERIFY(QMetaObject::invokeMethod(keys, "setAccessGroup", Q_RETURN_ARG(bool, success),
			Q_ARG(QString, QStringLiteral("teacher/private")), Q_ARG(QString, group)));
		QVERIFY2(success, qPrintable(keys->property("message").toString()));
		const QFileInfo after(keyPath);
		QCOMPARE(after.group(), group);
		QVERIFY(after.permissions().testFlag(QFile::ReadGroup));
		QVERIFY(!after.permissions().testFlag(QFile::WriteGroup));
		QVERIFY(!after.permissions().testFlag(QFile::ExeGroup));
		QVERIFY(key.open(QIODevice::ReadOnly));
		QVERIFY(key.readAll() == originalContents);
#endif
	}
	void passwordFocusDoesNotRewriteCiphertext()
	{
		Configuration::Object object;
		Configuration::TypedProperty<Configuration::Password> password(&object, QStringLiteral("Password"), QStringLiteral("Test"), QString(), Configuration::Property::Flag::Standard);
		Configuration::Property::UiMetadata metadata; metadata.pageId = QStringLiteral("test"); metadata.label = QStringLiteral("Password");
		password.setUiMetadata(metadata);
		password.setValue(Configuration::Password::fromPlainText(QByteArray("old secret")));
		const auto encrypted = password.variantValue();
		Configuration::PropertyModel model(&object);
		EditorFixture fixture; fixture.model = &model;
		std::unique_ptr<QQmlApplicationEngine> engine(QuickUi::createEngine());
		engine->rootContext()->setContextProperty(QStringLiteral("configurator"), &fixture);
		QQmlComponent component(engine.get(), QUrl(QStringLiteral("qrc:/configurator/PropertyRow.qml")));
		std::unique_ptr<QObject> row(component.createWithInitialProperties({{QStringLiteral("model"), model.propertyInfo(QStringLiteral("Test/Password"))}, {QStringLiteral("index"), 0}, {QStringLiteral("editorKind"), QStringLiteral("password")}}));
		QVERIFY2(row, qPrintable(component.errorString()));
		QCoreApplication::processEvents();
		auto editor = row->findChild<QObject*>(QStringLiteral("propertyTextEditor"));
		QVERIFY(editor);
		QCOMPARE(editor->property("text").toString(), QStringLiteral("old secret"));
		QVERIFY(QMetaObject::invokeMethod(editor, "editingFinished"));
		QCOMPARE(password.variantValue(), encrypted);
		editor->setProperty("text", QStringLiteral("new secret"));
		QVERIFY(QMetaObject::invokeMethod(editor, "editingFinished"));
		QCOMPARE(password.value().plainText().toByteArray(), QByteArray("new secret"));
		QVERIFY(password.variantValue() != encrypted);
		QVERIFY(password.variantValue().toString() != QStringLiteral("new secret"));
	}
	void integerUnitsAreEditableInline_data()
	{
		QTest::addColumn<QString>("suffix");
		QTest::newRow("seconds") << QStringLiteral("s");
		QTest::newRow("milliseconds") << QStringLiteral("ms");
		QTest::newRow("percent") << QStringLiteral("%");
		QTest::newRow("literal-suffix") << QStringLiteral("m.s");
		QTest::newRow("unitless") << QString();
	}
	void integerUnitsAreEditableInline()
	{
		QFETCH(QString, suffix);
		Configuration::Object object;
		Configuration::TypedProperty<int> value(&object, QStringLiteral("Interval"), QStringLiteral("Test"), 1000, Configuration::Property::Flag::Standard);
		Configuration::Property::UiMetadata metadata;
		metadata.pageId = QStringLiteral("test"); metadata.label = QStringLiteral("Interval");
		metadata.editor = Configuration::Property::Editor::Integer; metadata.suffix = suffix;
		metadata.minimum = -100; metadata.maximum = 5000;
		value.setUiMetadata(metadata);
		Configuration::PropertyModel model(&object);
		EditorFixture fixture; fixture.model = &model;
		std::unique_ptr<QQmlApplicationEngine> engine(QuickUi::createEngine());
		engine->rootContext()->setContextProperty(QStringLiteral("configurator"), &fixture);
		QQmlComponent component(engine.get(), QUrl(QStringLiteral("qrc:/configurator/PropertyRow.qml")));
		std::unique_ptr<QObject> row(component.createWithInitialProperties({{QStringLiteral("model"), model.propertyInfo(QStringLiteral("Test/Interval"))}, {QStringLiteral("index"), 0}, {QStringLiteral("editorKind"), QStringLiteral("integer")}}));
		QVERIFY2(row, qPrintable(component.errorString()));
		QCoreApplication::processEvents();
		QObject* spin = nullptr;
		for (auto child : row->findChildren<QObject*>())
			if (child->inherits("QQuickSpinBox")) { spin = child; break; }
		QVERIFY(spin);
		for (const auto& locale : {QLocale(QLocale::English, QLocale::UnitedStates), QLocale(QLocale::Russian)})
		{
			spin->setProperty("locale", QVariant::fromValue(locale));
			QQmlExpression format(qmlContext(spin), spin, QStringLiteral("textFromValue(1000, locale)"));
			const auto formatted = format.evaluate().toString();
			QVERIFY2(!format.hasError(), qPrintable(format.error().toString()));
			QCOMPARE(formatted, locale.toString(1000) + (suffix.isEmpty() ? QString() : QLatin1Char(' ') + suffix));
			auto validator = spin->property("validator").value<QValidator*>();
			QVERIFY(validator);
			for (auto text : QStringList{formatted, QStringLiteral("250"), QStringLiteral("-25") + (suffix.isEmpty() ? QString() : QLatin1Char(' ') + suffix)})
			{
				int position = text.size();
				QCOMPARE(validator->validate(text, position), QValidator::Acceptable);
				QQmlExpression parse(qmlContext(spin), spin, QStringLiteral("valueFromText('%1', locale)").arg(text));
				const auto expected = text == formatted ? 1000 : text.startsWith(QLatin1Char('-')) ? -25 : 250;
				QCOMPARE(parse.evaluate().toInt(), expected);
				QVERIFY2(!parse.hasError(), qPrintable(parse.error().toString()));
			}
			QString invalid = QStringLiteral("250 invalid"); int position = invalid.size();
			QCOMPARE(validator->validate(invalid, position), QValidator::Invalid);
		}
	}
	void closingFlushesAuthorizedGroupsBeforeCheckingDirty()
	{
		const auto before = VeyonCore::config().data();
		VeyonCore::config().setAccessRestrictedToUserGroups(true);
		VeyonCore::config().setAccessControlRulesProcessingEnabled(false);
		ConfiguratorController controller;
		VeyonCore::config().replaceData(before);
		std::unique_ptr<QQmlApplicationEngine> engine(QuickUi::createEngine());
		engine->rootContext()->setContextProperty(QStringLiteral("configurator"), &controller);
		engine->load(QUrl(QStringLiteral("qrc:/configurator/Configurator.qml")));
		QVERIFY(!engine->rootObjects().isEmpty());
		auto window = qobject_cast<QQuickWindow*>(engine->rootObjects().first());
		QVERIFY(window);
		for (const auto& page : controller.pages())
			if (page.toMap().value(QStringLiteral("id")) == QStringLiteral("access"))
				QVERIFY(QMetaObject::invokeMethod(window, "selectPage", Q_ARG(QVariant, page)));
		QCOMPARE(controller.accessMode(), 1);
		QQuickItem* editor = nullptr;
		QTRY_VERIFY((editor = visualItem(window->contentItem(), QStringLiteral("authorizedGroupsEditor"))));
		QTRY_VERIFY(editor->isVisible());
		window->requestActivate(); editor->forceActiveFocus();
		QTRY_VERIFY(editor->hasActiveFocus());
		editor->setProperty("text", QStringLiteral("draft-only-group"));
		QVERIFY(!controller.dirty());
		QVERIFY(window->property("editorPending").toBool());
		QVERIFY(!window->close());
		QVERIFY(controller.dirty());
		QCOMPARE(controller.authorizedGroups(), QStringList{QStringLiteral("draft-only-group")});
		QCOMPARE(VeyonCore::config().data(), before);
	}
	void cleanupTestCase() { m_core.reset(); }
private:
	QTemporaryDir m_configurationRoot;
	std::unique_ptr<VeyonCore> m_core;
};

int main(int argc, char** argv)
{
	VeyonCore::setupApplicationParameters();
	QApplication app(argc, argv);
	app.setQuitOnLastWindowClosed(false);
	ConfiguratorQuickTest test;
	return QTest::qExec(&test, argc, argv);
}
#include "ConfiguratorQuickTest.moc"
