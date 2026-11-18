// SPDX-License-Identifier: GPL-2.0-or-later
#include <QBitArray>
#include <QtTest>
#include <QApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQuickImageProvider>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QTemporaryDir>
#include <QTextDocument>
#include <memory>
#include <QTemporaryFile>
#include <QTimer>
#include <QScopeGuard>
#include "FileTransferPlugin.h"
#include "FileTransferController.h"
#include "FileTransferDialog.h"
#include "FileCollectController.h"
#include "FileCollectDialog.h"
#include "QuickUi.h"
#include "QuickFramebufferItem.h"
#include "VeyonConfiguration.h"

namespace
{
QObject* visualField(QQuickItem* item, const QString& name)
{
	if (!item) return nullptr;
	if (item->objectName() == name) return item;
	for (auto child : item->childItems())
		if (auto found = visualField(child, name)) return found;
	return nullptr;
}
QObject* windowField(QQuickWindow* window, const char* name)
{
	if (!window) return nullptr;
	const auto objectName = QString::fromLatin1(name);
	if (auto child = window->findChild<QObject*>(objectName)) return child;
	return visualField(window->contentItem(), objectName);
}
struct DialogScene
{
	QStringList warnings;
	std::unique_ptr<QQmlApplicationEngine> engine{QuickUi::createEngine()};
	QuickDialogRequest* request = nullptr;
	QQuickWindow* window = nullptr;

	DialogScene(const QString& kind, const QVariantMap& data = {})
	{
		QObject::connect(engine.get(), &QQmlEngine::warnings, engine.get(), [this](const QList<QQmlError>& errors) {
			for (const auto& error : errors) warnings.append(error.toString());
		});
		request = new QuickDialogRequest(kind, data, engine.get());
		engine->rootContext()->setContextProperty(QStringLiteral("dialogRequest"), request);
		engine->load(QUrl(QStringLiteral("qrc:/quick/GenericDialog.qml")));
		if (!engine->rootObjects().isEmpty())
			window = qobject_cast<QQuickWindow*>(engine->rootObjects().first());
	}
	QObject* field(const char* name) const
	{
		return windowField(window, name);
	}
	QString diagnostics() const { return warnings.join(QLatin1Char('\n')); }
};

bool noVisibleWidgets()
{
	for (const auto widget : QApplication::allWidgets())
		if (widget->isVisible()) return false;
	return true;
}

class BackgroundProvider : public QQuickImageProvider
{
public:
	BackgroundProvider() : QQuickImageProvider(Image) {}
	QImage requestImage(const QString&, QSize* size, const QSize&) override
	{
		QImage image(16, 16, QImage::Format_ARGB32_Premultiplied);
		image.fill(Qt::darkGray);
		if (size) *size = image.size();
		return image;
	}
};
}

class SmokeLockState : public QObject
{
	Q_OBJECT
	Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY modeChanged)
public:
	int mode() const { return m_mode; }
	void setMode(int value) { m_mode = value; Q_EMIT modeChanged(); }
Q_SIGNALS:
	void modeChanged();
private:
	int m_mode = 3;
};

class QuickDialogTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void initTestCase()
	{
		QVERIFY(m_home.isValid());
		m_core = std::make_unique<VeyonCore>(qApp, VeyonCore::Component::Master, QStringLiteral("QuickDialogTest"));
		VeyonCore::config().setUserConfigurationDirectory(m_home.path());
	}

	void dialogKindsLoadWithoutWarnings_data()
	{
		QTest::addColumn<QString>("kind");
		for (const auto kind : {"input", "password", "authenticate", "login", "message", "website",
			"application", "schedule", "access", "information", "error", "about", "notification", "busy", "countdown"})
			QTest::newRow(kind) << QString::fromLatin1(kind);
	}
	void dialogKindsLoadWithoutWarnings()
	{
		QFETCH(QString, kind);
		DialogScene scene(kind, {{QStringLiteral("title"), QStringLiteral("Runtime dialog test")},
			{QStringLiteral("text"), QStringLiteral("A message with <literal> characters.")},
			{QStringLiteral("seconds"), 60}});
		QVERIFY2(scene.window, qPrintable(scene.diagnostics()));
		QTRY_VERIFY(scene.window->isVisible());
		QTest::qWait(30);
		QVERIFY2(scene.warnings.isEmpty(), qPrintable(scene.diagnostics()));
		QVERIFY(noVisibleWidgets());
		QVERIFY(scene.field("dialogAccept"));
		if (kind == QStringLiteral("busy") || kind == QStringLiteral("countdown"))
		{
			QVERIFY(!scene.field("dialogAccept")->property("visible").toBool());
			QVERIFY(!scene.window->close());
			QVERIFY(!scene.request->finished());
		}
		else
		{
			QVERIFY(scene.window->close());
			QVERIFY(scene.request->finished());
			QVERIFY(!scene.request->result().value(QStringLiteral("accepted")).toBool());
		}
	}

	void checkboxesSharePrototypeAppearance()
	{
		const auto oldScheme = VeyonCore::config().uiColorScheme();
		const auto restore = qScopeGuard([oldScheme]() { VeyonCore::config().setUiColorScheme(oldScheme); });
		for (const auto scheme : {VeyonCore::UiColorScheme::Light, VeyonCore::UiColorScheme::Dark})
		{
			VeyonCore::config().setUiColorScheme(scheme);
			DialogScene scene(QStringLiteral("website"));
			QVERIFY2(scene.window, qPrintable(scene.diagnostics()));
			auto control = qobject_cast<QQuickItem*>(scene.field("dialogRemember"));
			QVERIFY(control);
			auto indicator = control->property("indicator").value<QQuickItem*>();
			QVERIFY(indicator);
			control->setProperty("checked", true);
			const auto accent = QColor(scheme == VeyonCore::UiColorScheme::Dark ? QStringLiteral("#ff563c") : QStringLiteral("#ec3013"));
			QCOMPARE(indicator->property("color").value<QColor>(), accent);
			QCOMPARE(indicator->width(), 18.0);
			QCOMPARE(indicator->height(), 18.0);
			auto mark = visualField(indicator, QStringLiteral("checkGlyph"));
			QVERIFY(mark);
			QCOMPARE(mark->property("color").value<QColor>(), QColor(Qt::white));
			QCOMPARE(mark->property("name").toString(), QStringLiteral("check"));
			QVERIFY(mark->property("visible").toBool());
			control->setProperty("tristate", true);
			control->setProperty("checkState", Qt::PartiallyChecked);
			QCOMPARE(mark->property("name").toString(), QStringLiteral("minus"));
			QCOMPARE(indicator->property("color").value<QColor>(), accent);
			control->setProperty("checkState", Qt::Unchecked);
			QVERIFY(!mark->property("visible").toBool());
			control->setProperty("enabled", false);
			QVERIFY(control->opacity() < 1.0);
			QVERIFY2(scene.warnings.isEmpty(), qPrintable(scene.diagnostics()));
		}
	}

	void inputEditorIsVisibleAndAcceptsKeyboard()
	{
		DialogScene scene(QStringLiteral("input"));
		QVERIFY2(scene.window, qPrintable(scene.diagnostics()));
		auto entry = qobject_cast<QQuickItem*>(scene.field("dialogEntry"));
		QVERIFY(entry);
		QTRY_VERIFY(entry->isVisible());
		QTRY_VERIFY(entry->width() > 100 && entry->height() >= 20);
		entry->forceActiveFocus();
		QTest::keyClick(scene.window, Qt::Key_A);
		QCOMPARE(entry->property("text").toString(), QStringLiteral("a"));
		scene.request->reject();
	}

	void editorValueAndIdempotentCompletion()
	{
		DialogScene scene(QStringLiteral("input"), {{QStringLiteral("value"), QStringLiteral("original")}});
		QVERIFY2(scene.window, qPrintable(scene.diagnostics()));
		auto entry = scene.field("dialogEntry");
		auto accept = scene.field("dialogAccept");
		QVERIFY(entry); QVERIFY(accept);
		QCOMPARE(entry->property("text").toString(), QStringLiteral("original"));
		QVERIFY(entry->setProperty("text", QStringLiteral("edited value")));
		QTRY_VERIFY(accept->property("enabled").toBool());
		QSignalSpy completed(scene.request, &QuickDialogRequest::completed);
		QVERIFY(QMetaObject::invokeMethod(accept, "clicked"));
		QTRY_VERIFY(scene.request->finished());
		QCOMPARE(scene.request->result().value(QStringLiteral("value")).toString(), QStringLiteral("edited value"));
		QVERIFY(scene.request->result().value(QStringLiteral("accepted")).toBool());
		scene.request->reject();
		scene.request->accept({{QStringLiteral("value"), QStringLiteral("must not replace")}});
		QCOMPARE(completed.count(), 1);
		QCOMPARE(scene.request->result().value(QStringLiteral("value")).toString(), QStringLiteral("edited value"));
		QVERIFY2(scene.warnings.isEmpty(), qPrintable(scene.diagnostics()));
	}

	void credentialFieldsRemainMasked_data()
	{
		QTest::addColumn<QString>("kind");
		for (const auto kind : {"password", "login", "authenticate"})
			QTest::newRow(kind) << QString::fromLatin1(kind);
	}
	void credentialFieldsRemainMasked()
	{
		QFETCH(QString, kind);
		DialogScene scene(kind);
		QVERIFY2(scene.window, qPrintable(scene.diagnostics()));
		auto username = scene.field("dialogUsername");
		auto password = scene.field("dialogPassword");
		auto accept = scene.field("dialogAccept");
		QVERIFY(username); QVERIFY(password); QVERIFY(accept);
		QVERIFY(!accept->property("enabled").toBool());
		QVERIFY(password->property("echoMode").toInt() != 0);
		username->setProperty("text", QStringLiteral("student"));
		password->setProperty("text", QStringLiteral("test secret"));
		QVERIFY(password->property("displayText").toString() != QStringLiteral("test secret"));
		QTRY_VERIFY(accept->property("enabled").toBool());
		if (kind == QStringLiteral("authenticate"))
		{
			QVERIFY(QMetaObject::invokeMethod(scene.field("dialogCancel"), "clicked"));
			QVERIFY(!scene.request->result().value(QStringLiteral("accepted")).toBool());
		}
		else
		{
			QVERIFY(QMetaObject::invokeMethod(accept, "clicked"));
			QCOMPARE(scene.request->result().value(QStringLiteral("username")).toString(), QStringLiteral("student"));
			QCOMPARE(scene.request->result().value(QStringLiteral("password")).toString(), QStringLiteral("test secret"));
		}
		QVERIFY(scene.request->finished());
		QVERIFY2(scene.warnings.isEmpty(), qPrintable(scene.diagnostics()));
	}

	void applicationPresetsRequireNamesAndReturnValues()
	{
		DialogScene scene(QStringLiteral("application"));
		QVERIFY2(scene.window, qPrintable(scene.diagnostics()));
		auto entry = scene.field("dialogEntry");
		auto remember = scene.field("dialogRemember");
		auto name = scene.field("dialogPresetName");
		auto accept = scene.field("dialogAccept");
		QVERIFY(entry); QVERIFY(remember); QVERIFY(name); QVERIFY(accept);
		entry->setProperty("text", QStringLiteral("/usr/bin/editor --new-window"));
		remember->setProperty("checked", true);
		QTRY_VERIFY(!accept->property("enabled").toBool());
		name->setProperty("text", QStringLiteral("Editor"));
		QTRY_VERIFY(accept->property("enabled").toBool());
		QVERIFY(QMetaObject::invokeMethod(accept, "clicked"));
		QCOMPARE(scene.request->result().value(QStringLiteral("value")).toString(), QStringLiteral("/usr/bin/editor --new-window"));
		QCOMPARE(scene.request->result().value(QStringLiteral("presetName")).toString(), QStringLiteral("Editor"));
		QVERIFY(scene.request->result().value(QStringLiteral("remember")).toBool());
		QVERIFY2(scene.warnings.isEmpty(), qPrintable(scene.diagnostics()));
	}

	void websiteValidationAllowsCorrection()
	{
		DialogScene scene(QStringLiteral("website"));
		QVERIFY2(scene.window, qPrintable(scene.diagnostics()));
		auto entry = scene.field("dialogEntry");
		auto accept = scene.field("dialogAccept");
		QVERIFY(entry); QVERIFY(accept);
		entry->setProperty("text", QStringLiteral("http://["));
		QSignalSpy error(scene.request, &QuickDialogRequest::errorChanged);
		QVERIFY(QMetaObject::invokeMethod(accept, "clicked"));
		QVERIFY(!scene.request->finished());
		QVERIFY(!scene.request->errorString().isEmpty());
		QCOMPARE(error.count(), 1);
		entry->setProperty("text", QStringLiteral("https://example.org/lesson"));
		QVERIFY(QMetaObject::invokeMethod(accept, "clicked"));
		QVERIFY(scene.request->finished());
		QCOMPARE(scene.request->result().value(QStringLiteral("value")).toString(), QStringLiteral("https://example.org/lesson"));
		QVERIFY2(scene.warnings.isEmpty(), qPrintable(scene.diagnostics()));
	}

	void messageRichTextAndTitleRoundTrip()
	{
		DialogScene scene(QStringLiteral("message"), {
			{QStringLiteral("value"), QStringLiteral("<p><b>Lesson</b></p><p>Second line</p>")},
			{QStringLiteral("messageTitle"), QStringLiteral("Assignment")}});
		QVERIFY2(scene.window, qPrintable(scene.diagnostics()));
		QVERIFY(QMetaObject::invokeMethod(scene.field("dialogAccept"), "clicked"));
		QVERIFY(scene.request->finished());
		QTextDocument document;
		document.setHtml(scene.request->result().value(QStringLiteral("value")).toString());
		QVERIFY(document.toPlainText().contains(QStringLiteral("Lesson")));
		QVERIFY(document.toPlainText().contains(QStringLiteral("Second line")));
		QVERIFY(scene.request->result().value(QStringLiteral("value")).toString().contains(QStringLiteral("font-weight")));
		QCOMPARE(scene.request->result().value(QStringLiteral("title")).toString(), QStringLiteral("Assignment"));
		QVERIFY2(scene.warnings.isEmpty(), qPrintable(scene.diagnostics()));
	}

	void scheduledSecondsAndAccessChoices()
	{
		DialogScene schedule(QStringLiteral("schedule"));
		QVERIFY2(schedule.window, qPrintable(schedule.diagnostics()));
		auto minutes = schedule.field("dialogMinutes");
		auto seconds = schedule.field("dialogSeconds");
		auto accept = schedule.field("dialogAccept");
		QVERIFY(minutes); QVERIFY(seconds); QVERIFY(accept);
		QVERIFY(!accept->property("enabled").toBool());
		minutes->setProperty("value", 2); seconds->setProperty("value", 7);
		QTRY_VERIFY(accept->property("enabled").toBool());
		QVERIFY(QMetaObject::invokeMethod(accept, "clicked"));
		QCOMPARE(schedule.request->result().value(QStringLiteral("seconds")).toInt(), 127);
		QVERIFY2(schedule.warnings.isEmpty(), qPrintable(schedule.diagnostics()));

		DialogScene access(QStringLiteral("access"), {{QStringLiteral("buttons"), QVariantList{
			QVariantMap{{QStringLiteral("text"), QStringLiteral("Never this session")}, {QStringLiteral("value"), QStringLiteral("never")}},
			QVariantMap{{QStringLiteral("text"), QStringLiteral("Always this session")}, {QStringLiteral("value"), QStringLiteral("always")}}}}});
		QVERIFY2(access.window, qPrintable(access.diagnostics()));
		QVERIFY(access.window->flags().testFlag(Qt::WindowStaysOnTopHint));
		auto always = access.field("dialogChoice_always");
		QVERIFY(always);
		QVERIFY(QMetaObject::invokeMethod(always, "clicked"));
		QVERIFY(access.request->result().value(QStringLiteral("accepted")).toBool());
		QCOMPARE(access.request->result().value(QStringLiteral("choice")).toString(), QStringLiteral("always"));
		QVERIFY2(access.warnings.isEmpty(), qPrintable(access.diagnostics()));
	}

	void countdownCompletesWithoutManualAcceptance_data()
	{
		QTest::addColumn<int>("seconds");
		QTest::newRow("negative") << -1;
		QTest::newRow("zero") << 0;
		QTest::newRow("positive") << 1;
	}
	void countdownCompletesWithoutManualAcceptance()
	{
		QFETCH(int, seconds);
		DialogScene scene(QStringLiteral("countdown"), {{QStringLiteral("seconds"), seconds}});
		QVERIFY2(scene.window, qPrintable(scene.diagnostics()));
		QSignalSpy completed(scene.request, &QuickDialogRequest::completed);
		QTRY_VERIFY_WITH_TIMEOUT(scene.request->finished(), 2500);
		QCOMPARE(completed.count(), 1);
		QVERIFY(scene.request->result().value(QStringLiteral("accepted")).toBool());
		QVERIFY2(scene.warnings.isEmpty(), qPrintable(scene.diagnostics()));
	}

	void lockAndDemoScenesDoNotNeedInputLockingControllers()
	{
		QStringList warnings;
		std::unique_ptr<QQmlApplicationEngine> engine(QuickUi::createEngine());
		connect(engine.get(), &QQmlEngine::warnings, engine.get(), [&warnings](const QList<QQmlError>& errors) {
			for (const auto& error : errors) warnings.append(error.toString());
		});
		SmokeLockState lockState;
		engine->addImageProvider(QStringLiteral("lockBackground"), new BackgroundProvider);
		engine->rootContext()->setContextProperty(QStringLiteral("lockState"), &lockState);
		engine->rootContext()->setContextProperty(QStringLiteral("demoConnection"), QVariant::fromValue<QObject*>(nullptr));
		engine->rootContext()->setContextProperty(QStringLiteral("demoViewport"), QRect(0, 0, 640, 480));
		engine->load(QUrl(QStringLiteral("qrc:/quick/LockWindow.qml")));
		QCOMPARE(engine->rootObjects().size(), 1);
		auto lock = qobject_cast<QQuickWindow*>(engine->rootObjects().first());
		QVERIFY2(lock, qPrintable(warnings.join(QLatin1Char('\n'))));
		lock->resize(800, 600);
		lock->show();
		for (int mode = 0; mode < 4; ++mode)
		{
			lockState.setMode(mode);
			QTest::qWait(20);
			QVERIFY(noVisibleWidgets());
			QVERIFY(lock->isVisible());
			QCOMPARE(lock->color().alpha(), 255);
			if (mode == 2) QCOMPARE(lock->color(), QColor(Qt::black));
		}
		auto lockFrame = lock->findChild<QuickFramebufferItem*>();
		QVERIFY(lockFrame); QVERIFY(lockFrame->viewOnly()); QVERIFY(!lockFrame->computer());
		QVERIFY(!lockFrame->live());
		engine->load(QUrl(QStringLiteral("qrc:/quick/DemoWindow.qml")));
		QCOMPARE(engine->rootObjects().size(), 2);
		auto demo = qobject_cast<QQuickWindow*>(engine->rootObjects().last());
		QVERIFY(demo);
		auto demoFrame = demo->findChild<QuickFramebufferItem*>();
		QVERIFY(demoFrame); QVERIFY(demoFrame->viewOnly()); QVERIFY(!demoFrame->computer());
		QVERIFY(demoFrame->live());
		QTRY_VERIFY(demo->isVisible());
		QTest::qWait(30);
		QVERIFY(noVisibleWidgets());
		QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QLatin1Char('\n'))));
		engine.reset();
	}

	void transferFacadeLoadsAndReceivesRealControllerErrors()
	{
		FileTransferPlugin plugin;
		FileTransferController controller(&plugin);
		QTemporaryFile source(m_home.filePath(QStringLiteral("lesson-XXXXXX.txt")));
		QVERIFY(source.open());
		const auto sourcePath = source.fileName();
		source.close();
		QVERIFY(QFile::remove(sourcePath));
		controller.setFiles({sourcePath});
		auto recipient = ComputerControlInterface::Pointer::create(
			Computer(QUuid::createUuid(), QStringLiteral("Classroom PC"), QStringLiteral("192.0.2.1")));
		controller.setInterfaces({recipient});
		FileTransferDialog dialog(&controller, nullptr, {recipient});
		dialog.open();
		auto engine = dialog.findChild<QQmlApplicationEngine*>();
		QVERIFY(engine); QCOMPARE(engine->rootObjects().size(), 1);
		auto window = qobject_cast<QQuickWindow*>(engine->rootObjects().first());
		QVERIFY(window);
		QStringList warnings;
		connect(engine, &QQmlEngine::warnings, &dialog, [&warnings](const QList<QQmlError>& errors) {
			for (const auto& error : errors) warnings.append(error.toString());
		});
		QTRY_VERIFY(window->isVisible());
		QTest::qWait(30);
		QVERIFY(noVisibleWidgets());
		auto files = windowField(window, "transferFiles");
		auto targets = windowField(window, "transferTargets");
		auto start = windowField(window, "transferStart");
		auto cancel = windowField(window, "transferCancel");
		QVERIFY(files); QVERIFY(targets); QVERIFY(start); QVERIFY(cancel);
		QCOMPARE(files->property("count").toInt(), 1);
		QCOMPARE(targets->property("count").toInt(), 1);
		auto fileName = qobject_cast<QQuickItem*>(windowField(window, "transferFileName"));
		QVERIFY(fileName);
		auto hover = fileName->findChild<QObject*>(QStringLiteral("transferFileHover"));
		QVERIFY(hover);
		QTest::mouseMove(window, fileName->mapToScene(fileName->boundingRect().center()).toPoint());
		QTRY_VERIFY(hover->property("hovered").toBool());
		QSignalSpy changed(&dialog, &FileTransferDialog::changed);
		QVERIFY(QMetaObject::invokeMethod(start, "clicked"));
		QTRY_VERIFY(dialog.completed());
		QVERIFY(dialog.errors().contains(sourcePath));
		QVERIFY(changed.count() > 0);
		auto errors = windowField(window, "transferErrors");
		QVERIFY(errors);
		QCOMPARE(errors->property("text").toString(), dialog.errors());
		QVERIFY(!controller.isRunning());
		QFile replacement(sourcePath); QVERIFY(replacement.open(QIODevice::WriteOnly));
		QVERIFY(replacement.write("lesson") > 0); replacement.close();
		QVERIFY(QMetaObject::invokeMethod(start, "clicked"));
		QVERIFY(controller.isRunning());
		QVERIFY(QMetaObject::invokeMethod(cancel, "clicked"));
		QVERIFY(!controller.isRunning());
		QVERIFY(dialog.errors().isEmpty());
		QSignalSpy finished(&dialog, &FileTransferDialog::finished);
		QVERIFY(window->close());
		QCOMPARE(finished.count(), 1);
		QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QLatin1Char('\n'))));
	}

	void collectionFacadeLoadsAndReportsNoEligibleTargets()
	{
		FileTransferPlugin plugin;
		plugin.configuration().setCollectionDirectory(FileCollectController::CollectionDirectory::DestinationDirectory);
		FileCollectController controller(&plugin);
		controller.setCollectSourceDirectory(QStringLiteral("Documents"));
		controller.setFilePattern(QStringLiteral("*.odt"));
		controller.setDestinationDirectory(m_home.path());
		FileCollectDialog dialog(&controller);
		dialog.open();
		auto engine = dialog.findChild<QQmlApplicationEngine*>();
		QVERIFY(engine); QCOMPARE(engine->rootObjects().size(), 1);
		auto window = qobject_cast<QQuickWindow*>(engine->rootObjects().first());
		QVERIFY(window);
		QStringList warnings;
		connect(engine, &QQmlEngine::warnings, &dialog, [&warnings](const QList<QQmlError>& errors) {
			for (const auto& error : errors) warnings.append(error.toString());
		});
		QTRY_VERIFY(window->isVisible());
		QTest::qWait(30);
		QVERIFY(noVisibleWidgets());
		auto source = windowField(window, "collectSource");
		auto pattern = windowField(window, "collectPattern");
		auto subfolders = windowField(window, "collectSubfolders");
		auto destination = windowField(window, "collectDestination");
		auto start = windowField(window, "collectStart");
		QVERIFY(source); QVERIFY(pattern); QVERIFY(subfolders); QVERIFY(destination); QVERIFY(start);
		QCOMPARE(source->property("text").toString(), QStringLiteral("Documents"));
		QCOMPARE(pattern->property("text").toString(), QStringLiteral("*.odt"));
		QCOMPARE(destination->property("text").toString(), m_home.path());
		source->setProperty("text", QStringLiteral("Homework"));
		pattern->setProperty("text", QStringLiteral("*.pdf"));
		subfolders->setProperty("currentIndex", 2);
		bool errorShown = false;
		QTimer::singleShot(0, &dialog, [&errorShown]() {
			for (auto candidate : QGuiApplication::topLevelWindows())
			{
				if (candidate->property("kind").toString() == QStringLiteral("error"))
				{
					errorShown = qobject_cast<QQuickWindow*>(candidate) && candidate->isVisible();
					candidate->close();
				}
			}
		});
		QVERIFY(QMetaObject::invokeMethod(start, "clicked"));
		QVERIFY(errorShown);
		QVERIFY(!controller.isRunning());
		QCOMPARE(controller.collectSourceDirectory(), QStringLiteral("Homework"));
		QCOMPARE(controller.filePattern(), QStringLiteral("*.pdf"));
		QSignalSpy finished(&dialog, &FileCollectDialog::finished);
		QVERIFY(window->close());
		QCOMPARE(finished.count(), 1);
		QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QLatin1Char('\n'))));
	}

	void cleanupTestCase() { m_core.reset(); }
private:
	QTemporaryDir m_home;
	std::unique_ptr<VeyonCore> m_core;
};

int main(int argc, char** argv)
{
	VeyonCore::setupApplicationParameters();
	QApplication app(argc, argv);
	app.setQuitOnLastWindowClosed(false);
	QuickDialogTest test;
	return QTest::qExec(&test, argc, argv);
}
#include "QuickDialogTest.moc"
