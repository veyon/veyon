// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>
#include <QApplication>
#include <QStandardItemModel>
#include <QTemporaryDir>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlError>
#include <QQmlProperty>
#include <QJSValue>
#include <QPalette>
#include <QPointer>
#include <QRegularExpression>
#include <QFile>
#include <QJsonDocument>
#include <QTimer>
#include <QPainter>
#include <cmath>
#include "MainWindow.h"
#include "VeyonMaster.h"
#include "ComputerControlListModel.h"
#include "ComputerMonitoringModel.h"
#include "ComputerManager.h"
#include "CheckableItemProxyModel.h"
#include "NetworkObjectModel.h"
#include "DesktopIcons.h"
#include "QuickUi.h"
#include "QuickFramebufferItem.h"
#include "QuickKeyEventTracker.h"
#include "UserConfig.h"
#include "VeyonConfiguration.h"

namespace {
ComputerControlInterface::Pointer computer(const QString& name)
{
	return ComputerControlInterface::Pointer::create(Computer(QUuid::createUuid(),name,QStringLiteral("192.0.2.1")));
}
void append(QStandardItemModel& source,const ComputerControlInterface::Pointer& computer)
{
	auto item=new QStandardItem(computer->computerName());
	item->setData(computer->computerName(),ComputerControlListModel::SearchRole);
	item->setData(QVariant::fromValue(computer),ComputerControlListModel::ControlInterfaceRole);
	item->setData(computer->computer().networkObjectUid(),ComputerControlListModel::UidRole);
	item->setData(computer->userLoginName(),ComputerControlListModel::UserLoginNameRole);
	item->setData(QVariant::fromValue(computer->state()),ComputerControlListModel::StateRole);
	source.appendRow(item);
}
struct SourceFixture {
	QStandardItemModel source;
	ComputerMonitoringModel* proxy;
	QAbstractItemModel* original;
	explicit SourceFixture(VeyonMaster& master):proxy(master.computerMonitoringModel()),original(proxy->sourceModel()){proxy->setSourceModel(&source);}
	~SourceFixture(){proxy->setSourceModel(original);}
};
// Each persistence test uses a real, isolated UserConfig store. In particular,
// constructing/destroying Master must not rewrite legacy fractional grid units.
struct UserConfigurationFixture {
	QTemporaryDir home;
	QString previous = VeyonCore::config().userConfigurationDirectory();
	UserConfigurationFixture() { VeyonCore::config().setUserConfigurationDirectory(home.path()); }
	~UserConfigurationFixture() { VeyonCore::config().setUserConfigurationDirectory(previous); }
};
QJsonObject savedPosition(const UserConfig& config, const QString& uid)
{
	for (const auto& value : config.computerPositions())
		if (value.toObject().value(QStringLiteral("uid")).toString() == uid) return value.toObject();
	return {};
}
QQuickItem* visualItem(QQuickItem* parent, const QString& name)
{
	if (!parent) return nullptr;
	if (parent->objectName() == name) return parent;
	for (auto child : parent->childItems()) if (auto found = visualItem(child, name)) return found;
	return nullptr;
}
QQuickItem* visibleItemWithProperty(QQuickItem* parent, const char* name, const QVariant& value)
{
	if (!parent) return nullptr;
	if (parent->isVisible() && parent->property(name) == value) return parent;
	for (auto child : parent->childItems())
		if (auto found = visibleItemWithProperty(child, name, value)) return found;
	return nullptr;
}
// Drive the production file/import dialogs without depending on a native picker.
struct PositionDialogResponder {
	QTimer poll, deadline;
	QString path;
	bool choosePixels = false;
	bool coordinatePrompt = false;
	bool cancelFile = false;
	bool cancelCoordinates = false;
	bool errorShown = false;
	bool unexpectedDialog = false;
	bool timedOut = false;
	explicit PositionDialogResponder(QString filePath, bool pixels = false) : path(std::move(filePath)), choosePixels(pixels)
	{
		QObject::connect(&poll, &QTimer::timeout, &poll, [this]() { respond(false); });
		QObject::connect(&deadline, &QTimer::timeout, &deadline, [this]() { timedOut = true; respond(true); });
		poll.start(10);deadline.setSingleShot(true);deadline.start(5000);
	}
	void respond(bool cancel)
	{
		for (auto window : QGuiApplication::allWindows()) {
			auto context = QQmlEngine::contextForObject(window);
			if (!context) continue;
			auto request = qobject_cast<QuickDialogRequest*>(context->contextProperty(QStringLiteral("dialogRequest")).value<QObject*>());
			if (!request || request->finished()) continue;
			if (cancel) {request->reject();continue;}
			if (request->kind() == QStringLiteral("file")) {
				if (cancelFile) request->reject();
				else request->accept({{QStringLiteral("path"), path}});
			} else if (request->kind() == QStringLiteral("input") && request->data().value(QStringLiteral("choices")).toStringList().size() == 2) {
				auto quickWindow = qobject_cast<QQuickWindow*>(window);
				if (!quickWindow) continue;
				auto choice = visualItem(quickWindow->contentItem(), QStringLiteral("dialogChoice"));
				auto accept = visualItem(quickWindow->contentItem(), QStringLiteral("dialogAccept"));
				if (!choice || !accept) continue;
				coordinatePrompt = true;
				if (cancelCoordinates) {request->reject();continue;}
				choice->setProperty("currentIndex", choosePixels ? 1 : 0);
				QMetaObject::invokeMethod(accept, "clicked");
			} else if (request->kind() == QStringLiteral("error") || request->kind() == QStringLiteral("information")) {
				errorShown = true;request->reject();
			} else {unexpectedDialog = true;request->reject();}
		}
	}
};
double luminance(const QColor& color)
{
	auto channel=[](double v){return v<=0.04045?v/12.92:std::pow((v+0.055)/1.055,2.4);};
	return 0.2126*channel(color.redF())+0.7152*channel(color.greenF())+0.0722*channel(color.blueF());
}
double contrast(const QColor& first,const QColor& second)
{
	const auto a=luminance(first), b=luminance(second);
	return (std::max(a,b)+0.05)/(std::min(a,b)+0.05);
}
}
class MasterWorkspaceTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void initTestCase()
	{
		m_core=std::make_unique<VeyonCore>(qApp,VeyonCore::Component::Master,QStringLiteral("WorkspaceTest"));
		VeyonCore::config().setUserConfigurationDirectory(m_home.path());
	}
	void init() { m_qmlWarnings.clear(); }
	void cleanup()
	{
		QVERIFY2(m_qmlWarnings.isEmpty(), qPrintable(m_qmlWarnings.join(QLatin1Char('\n'))));
	}
	void genuineQuickWindowAndResponsiveViews()
	{
		VeyonMaster master(m_core.get());observe(master);
		QVERIFY(master.mainWindow()==nullptr);
		auto window=qobject_cast<QQuickWindow*>(master.mainQmlWindow());
		QVERIFY(window);
		auto controller=master.findChild<MainWindow*>();QVERIFY(controller);
		window->resize(1280,748);window->show();
		QTRY_VERIFY(window->findChild<QQuickItem*>(QStringLiteral("classroomSidebar")));
		auto sidebar=window->findChild<QQuickItem*>(QStringLiteral("classroomSidebar"));
		QCOMPARE(sidebar->width(),216.0);
		auto details=window->findChild<QQuickItem*>(QStringLiteral("computerDetails"));QVERIFY(details);
		auto grid=window->findChild<QQuickItem*>(QStringLiteral("computerMonitoringView"));QVERIFY(grid);
		SourceFixture fixture(master);
		for(int i=0;i<24;++i)append(fixture.source,computer(QStringLiteral("PC-%1").arg(i)));
		QTRY_COMPARE(grid->property("count").toInt(),24);
		auto cardsDoNotOverlap = [grid]() {
			auto content = grid->property("contentItem").value<QQuickItem*>();
			if (!content) return false;
			QList<QRectF> rectangles;
			for (auto item : content->childItems()) {
				if (item->objectName() != QStringLiteral("computerCard")) continue;
				const auto rectangle = item->mapRectToItem(grid, item->boundingRect());
				if (!rectangle.intersects(grid->boundingRect())) continue;
				for (const auto& previous : rectangles) if (previous.intersects(rectangle)) return false;
				rectangles.append(rectangle);
			}
			return rectangles.size() > 1;
		};
		QTRY_VERIFY(cardsDoNotOverlap());
		for (int refresh = 0; refresh < 4; ++refresh) {
			Q_EMIT fixture.source.dataChanged(fixture.source.index(0, 0), fixture.source.index(23, 0));
			controller->select(controller->computerIdAt(refresh));
			controller->setSearch(QString{});
			QTRY_VERIFY(cardsDoNotOverlap());
		}
		for(int width:{1024,1280,1440}) {
			window->resize(width,748);QTest::qWait(20);
			const auto rectangle=details->mapRectToScene(details->boundingRect());
			QVERIFY(rectangle.right()<=window->width()+1);
			QVERIFY(rectangle.bottom()<=window->height()+1);
			QVERIFY(grid->width()>0);
			const auto cell=grid->property("cellWidth").toDouble();QVERIFY(cell>=180);
			const auto columns=std::max(1,int(std::floor(grid->width()/cell+0.001)));
			QVERIFY(std::abs(columns*cell-grid->width())<1);
			QTRY_VERIFY(cardsDoNotOverlap());
		}
		window->setProperty("listMode",true);
		QTRY_COMPARE(grid->property("cellHeight").toInt(),70);
		QCOMPARE(grid->property("cellWidth").toDouble(),grid->width());
		controller->selectAll(true);
		QTRY_VERIFY(cardsDoNotOverlap());
	}
	void realCardClicksSelectComputers()
	{
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		auto window = qobject_cast<QQuickWindow*>(master.mainQmlWindow());QVERIFY(window);
		window->resize(1440, 900);window->show();
		SourceFixture fixture(master);
		for (int index = 0; index < 3; ++index) append(fixture.source, computer(QStringLiteral("Click PC %1").arg(index)));
		auto grid = window->findChild<QQuickItem*>(QStringLiteral("computerMonitoringView"));QVERIFY(grid);
		QTRY_COMPARE(grid->property("count").toInt(), 3);
		QTRY_VERIFY(window->isExposed());
		auto content = grid->property("contentItem").value<QQuickItem*>();QVERIFY(content);
		auto cardAt = [content](int row) -> QQuickItem* {
			for (auto item : content->childItems())
				if (item->objectName() == QStringLiteral("computerCard") && item->property("index").toInt() == row) return item;
			return nullptr;
		};
		QTRY_VERIFY(cardAt(0) && cardAt(1));
		QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, cardAt(0)->mapToScene(QPointF(20, 20)).toPoint());
		QCOMPARE(controller->selectedCount(), 1);
		QVERIFY(controller->isSelected(controller->computerIdAt(0)));
		QVERIFY(controller->selectedScope());
		QCOMPARE(grid->property("currentIndex").toInt(), 0);
		QTest::mouseClick(window, Qt::LeftButton, Qt::ControlModifier, cardAt(1)->mapToScene(QPointF(20, 20)).toPoint());
		QCOMPARE(controller->selectedCount(), 2);
		QVERIFY(controller->isSelected(controller->computerIdAt(1)));
		QCOMPARE(grid->property("currentIndex").toInt(), 1);
		QVERIFY2(m_qmlWarnings.isEmpty(), qPrintable(m_qmlWarnings.join(QLatin1Char('\n'))));
	}
	void childWindowShortcutsDoNotReachMaster_data()
	{
		QTest::addColumn<QString>("action");
		for (const auto action : {"select-destination", "select-source", "escape", "fullscreen", "feature"})
			QTest::newRow(action) << QString::fromLatin1(action);
	}
	void childWindowShortcutsDoNotReachMaster()
	{
		QFETCH(QString, action);
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		auto window = qobject_cast<QQuickWindow*>(master.mainQmlWindow());QVERIFY(window);
		window->show();window->requestActivate();QTRY_VERIFY(window->isActive());
		SourceFixture fixture(master);
		auto first = computer(QStringLiteral("PC-01"));append(fixture.source, first);
		append(fixture.source, computer(QStringLiteral("PC-02")));
		append(fixture.source, computer(QStringLiteral("PC-03")));
		const auto selectedId = first->computer().networkObjectUid().toString();
		controller->select(selectedId);QCOMPARE(controller->selectedCount(), 1);
		const auto originalVisibility = window->visibility();
		const auto lock = controller->feature(QStringLiteral("ScreenLock"));
		QVERIFY(!lock.value(QStringLiteral("uid")).toString().isEmpty());
		if (action == QStringLiteral("feature")) {
			// Exercise the production feature Shortcut delegate with an extension-supplied shortcut.
			// Bundled features currently leave their optional shortcut empty.
			QObject* shortcut = nullptr;
			for (auto object : window->findChildren<QObject*>()) {
				if (!QByteArray(object->metaObject()->className()).contains("Shortcut")) continue;
				const auto value = object->property("modelData");
				auto data = value.metaType() == QMetaType::fromType<QJSValue>()
					? value.value<QJSValue>().toVariant().toMap() : value.toMap();
				if (data.value(QStringLiteral("name")) != QStringLiteral("ScreenLock")) continue;
				data.insert(QStringLiteral("shortcut"), QStringLiteral("Ctrl+Shift+L"));
				QVERIFY(object->setProperty("modelData", data));shortcut = object;break;
			}
			QVERIFY(shortcut);QTRY_VERIFY(shortcut->property("enabled").toBool());
		}
		const auto collect = controller->feature(QStringLiteral("FileCollect")).value(QStringLiteral("uid")).toString();
		QVERIFY(!collect.isEmpty());controller->trigger(collect);
		QPointer<QQuickWindow> child;
		for (auto candidate : QGuiApplication::allWindows()) {
			auto quick = qobject_cast<QQuickWindow*>(candidate);
			if (quick && visualItem(quick->contentItem(), QStringLiteral("collectDestination"))) child = quick;
		}
		QVERIFY(child);
		struct CloseChild {
			QPointer<QQuickWindow> window;
			~CloseChild() { if (window) window->close();QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); }
		} closeChild{child};
		QCOMPARE(child->transientParent(), window);
		child->requestActivate();QTRY_COMPARE(QGuiApplication::focusWindow(), child.data());
		auto field = visualItem(child->contentItem(), action == QStringLiteral("select-source")
			? QStringLiteral("collectSource") : QStringLiteral("collectDestination"));
		QVERIFY(field);
		const auto text = QStringLiteral("/tmp/parity collection destination");
		field->setProperty("text", text);field->setProperty("cursorPosition", text.size());
		QTest::mouseClick(child, Qt::LeftButton, Qt::NoModifier, field->mapToScene(field->boundingRect().center()).toPoint());
		QTRY_VERIFY(field->hasActiveFocus());
		if (action.startsWith(QStringLiteral("select-"))) QTest::keyClick(child, Qt::Key_A, Qt::ControlModifier);
		else if (action == QStringLiteral("escape")) QTest::keyClick(child, Qt::Key_Escape);
		else if (action == QStringLiteral("fullscreen")) QTest::keyClick(child, Qt::Key_F11);
		else QTest::keyClick(child, Qt::Key_L, Qt::ControlModifier | Qt::ShiftModifier);
		QCOMPARE(controller->selectedCount(), 1);
		QVERIFY(controller->isSelected(selectedId));QVERIFY(controller->selectedScope());
		QCOMPARE(window->visibility(), originalVisibility);
		QVERIFY(first->designatedModeFeature().isNull());
		if (action.startsWith(QStringLiteral("select-"))) QCOMPARE(field->property("selectedText").toString(), text);
	}
	void searchSelectAllSelectsTextOnly()
	{
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		auto window = qobject_cast<QQuickWindow*>(master.mainQmlWindow());QVERIFY(window);
		SourceFixture fixture(master);
		for (int i = 0; i < 3; ++i) append(fixture.source, computer(QStringLiteral("Search PC %1").arg(i)));
		const auto selectedId = controller->computerIdAt(0);controller->select(selectedId);
		auto search = visualItem(window->contentItem(), QStringLiteral("computerSearch"));QVERIFY(search);
		search->setProperty("text", QStringLiteral("Search PC"));
		window->show();window->requestActivate();QTRY_VERIFY(window->isActive());
		QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, search->mapToScene(search->boundingRect().center()).toPoint());
		QTRY_VERIFY(search->hasActiveFocus());
		QTest::keyClick(window, Qt::Key_A, Qt::ControlModifier);
		QCOMPARE(search->property("selectedText").toString(), QStringLiteral("Search PC"));
		QCOMPARE(controller->selectedCount(), 1);QVERIFY(controller->isSelected(selectedId));
		// The same shortcut still selects computers when focus returns to the monitoring view.
		auto grid = visualItem(window->contentItem(), QStringLiteral("computerMonitoringView"));QVERIFY(grid);
		grid->forceActiveFocus();QTRY_VERIFY(grid->hasActiveFocus());
		QTest::keyClick(window, Qt::Key_A, Qt::ControlModifier);
		QCOMPARE(controller->selectedCount(), 3);
	}
	void duplicateRoomNamesAreResolvedByUid()
	{
		VeyonMaster master(m_core.get());observe(master);auto controller=master.findChild<MainWindow*>();QVERIFY(controller);
		auto tree=qobject_cast<CheckableItemProxyModel*>(master.computerManager().computerTreeModel());QVERIFY(tree);
		QStandardItemModel source;auto original=tree->sourceModel();
		struct Restore {CheckableItemProxyModel* tree;QAbstractItemModel* source;~Restore(){tree->setSourceModel(source);}} restore{tree,original};
		QStringList ids;
		for(int i=0;i<2;++i){auto item=new QStandardItem(QStringLiteral("Same room"));auto id=QUuid::createUuid();ids.append(id.toString());item->setData(id,NetworkObjectModel::UidRole);item->setData(QVariant::fromValue(NetworkObject::Type::Location),NetworkObjectModel::TypeRole);source.appendRow(item);}
		tree->setSourceModel(&source);
		auto window = qobject_cast<QQuickWindow*>(master.mainQmlWindow());QVERIFY(window);
		window->show();window->setProperty("panel", QStringLiteral("screenshots"));
		QTRY_VERIFY(window->isExposed());
		const auto roomName = QStringLiteral("room-") + ids.at(1);
		QTRY_VERIFY(visualItem(window->contentItem(), roomName));
		auto room = visualItem(window->contentItem(), roomName);
		controller->setSelectedScope(true);
		QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, room->mapToScene(room->boundingRect().center()).toPoint());
		QCOMPARE(window->property("panel").toString(), QStringLiteral("computers"));
		QVERIFY(!controller->selectedScope());
		QCOMPARE(controller->rooms().size(),2);
		QVERIFY(!controller->rooms().at(0).toMap().value(QStringLiteral("current")).toBool());
		QVERIFY(controller->rooms().at(1).toMap().value(QStringLiteral("current")).toBool());
		QCOMPARE(tree->data(tree->index(0,0),Qt::CheckStateRole).toInt(),int(Qt::Unchecked));
		QCOMPARE(tree->data(tree->index(1,0),Qt::CheckStateRole).toInt(),int(Qt::Checked));
	}
	void selectionSearchAndModelReset()
	{
		VeyonMaster master(m_core.get());observe(master);auto controller=master.findChild<MainWindow*>();QVERIFY(controller);
		SourceFixture fixture(master);
		auto first=computer(QStringLiteral("PC-01")),second=computer(QStringLiteral("PC-02"));
		append(fixture.source,first);append(fixture.source,second);
		controller->selectAll(true);QCOMPARE(controller->selectedCount(),2);
		controller->setSearch(QStringLiteral("PC-02"));
		QCOMPARE(controller->computers()->rowCount(),1);QCOMPARE(controller->selectedCount(),1);
		QCOMPARE(controller->selectedComputerControlInterfaces().first(),second);
		controller->setSearch({});controller->select(first->computer().networkObjectUid().toString());
		QCOMPARE(controller->details().value(QStringLiteral("name")).toString(),first->computerName());
		fixture.source.clear();QCOMPARE(controller->selectedCount(),0);QVERIFY(controller->selectedComputerControlInterfaces().isEmpty());
	}
	void selectingComputersDefaultsCommandsToSelection_data()
	{
		QTest::addColumn<QString>("operation");
		for (const auto& operation : {"single", "additive", "range", "all"})
			QTest::newRow(operation) << QString::fromLatin1(operation);
	}
	void selectingComputersDefaultsCommandsToSelection()
	{
		QFETCH(QString, operation);
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		SourceFixture fixture(master);
		auto first = computer(QStringLiteral("PC-01")), second = computer(QStringLiteral("PC-02"));
		append(fixture.source, first);append(fixture.source, second);
		const auto firstId = first->computer().networkObjectUid().toString();
		const auto secondId = second->computer().networkObjectUid().toString();
		controller->setSelectedScope(false);
		if (operation == QStringLiteral("all")) controller->selectAll(true);
		else {
			controller->select(firstId);
			if (operation == QStringLiteral("additive")) controller->select(secondId, true);
			if (operation == QStringLiteral("range")) controller->select(secondId, false, true);
		}
		QVERIFY(controller->selectedScope());
		// Exercise real feature routing, not just a button's checked property.
		const auto lock = controller->feature(QStringLiteral("ScreenLock")).value(QStringLiteral("uid")).toString();
		QVERIFY(!lock.isEmpty());
		controller->trigger(lock);
		QCOMPARE(first->designatedModeFeature(), QUuid(lock));
		if (operation == QStringLiteral("single")) QVERIFY(second->designatedModeFeature().isNull());
		else QCOMPARE(second->designatedModeFeature(), QUuid(lock));
	}
	void losingLastSelectionRestoresWholeClass_data()
	{
		QTest::addColumn<QString>("operation");
		for (const auto& operation : {"clear", "toggle", "search", "remove", "reset"})
			QTest::newRow(operation) << QString::fromLatin1(operation);
	}
	void losingLastSelectionRestoresWholeClass()
	{
		QFETCH(QString, operation);
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		SourceFixture fixture(master);
		auto target = computer(QStringLiteral("PC-01"));append(fixture.source, target);
		const auto id = target->computer().networkObjectUid().toString();
		controller->select(id);controller->setSelectedScope(true);
		if (operation == QStringLiteral("clear")) controller->selectAll(false);
		else if (operation == QStringLiteral("toggle")) controller->select(id, true);
		else if (operation == QStringLiteral("search")) controller->setSearch(QStringLiteral("no matching host"));
		else if (operation == QStringLiteral("remove")) fixture.source.removeRow(0);
		else fixture.source.clear();
		QCOMPARE(controller->selectedCount(), 0);
		QVERIFY(!controller->selectedScope());
	}
	void explicitWholeClassScopeSurvivesNonSelectionUpdates()
	{
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		SourceFixture fixture(master);
		auto target = computer(QStringLiteral("PC-01"));append(fixture.source, target);
		controller->select(target->computer().networkObjectUid().toString());
		controller->setSelectedScope(false);
		Q_EMIT fixture.source.dataChanged(fixture.source.index(0, 0), fixture.source.index(0, 0));
		controller->setSearch(QStringLiteral("PC"));
		QCOMPARE(controller->selectedCount(), 1);
		QVERIFY(!controller->selectedScope());
	}
	void openMenuRetainsTargetsAfterModelReset()
	{
		VeyonMaster master(m_core.get());observe(master);auto controller=master.findChild<MainWindow*>();QVERIFY(controller);
		SourceFixture fixture(master);auto target=computer(QStringLiteral("PC-01"));append(fixture.source,target);
		controller->selectAll(true);const auto token=controller->captureTargets(true);
		fixture.source.clear();QCOMPARE(controller->selectedCount(),0);
		const auto id=controller->feature(QStringLiteral("ScreenLock")).value(QStringLiteral("uid")).toString();QVERIFY(!id.isEmpty());
		controller->triggerSnapshot(id,token);
		QCOMPARE(target->designatedModeFeature(),QUuid(id));
		controller->releaseTargets(token);
	}
	void globalStopReachesUnselectedRecipients_data()
	{
		QTest::addColumn<bool>("filterRecipients");
		QTest::newRow("selected-presenter") << false;
		QTest::newRow("recipient-hidden-by-search") << true;
	}
	void globalStopReachesUnselectedRecipients()
	{
		QFETCH(bool, filterRecipients);
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		auto window = qobject_cast<QQuickWindow*>(master.mainQmlWindow());QVERIFY(window);
		window->resize(1440, 900);window->show();
		auto tree = qobject_cast<CheckableItemProxyModel*>(master.computerManager().computerTreeModel());QVERIFY(tree);
		QStandardItemModel source;
		struct Restore { CheckableItemProxyModel* tree; QAbstractItemModel* original;
			~Restore() { tree->setSourceModel(original); }
		} restore{tree, tree->sourceModel()};
		auto entry = [](const QString& name, NetworkObject::Type type) {
			auto item = new QStandardItem(name);
			item->setData(name, NetworkObjectModel::NameRole);
			item->setData(QUuid::createUuid(), NetworkObjectModel::UidRole);
			item->setData(QVariant::fromValue(type), NetworkObjectModel::TypeRole);
			if (type == NetworkObject::Type::Host) item->setData(QStringLiteral("127.0.0.1:1"), NetworkObjectModel::HostAddressRole);
			return item;
		};
		auto room = entry(QStringLiteral("Presentation room"), NetworkObject::Type::Location);
		auto presenterEntry = entry(QStringLiteral("Presenter S1"), NetworkObject::Type::Host);
		auto recipientEntry = entry(QStringLiteral("Recipient S2"), NetworkObject::Type::Host);
		const auto presenterId = presenterEntry->data(NetworkObjectModel::UidRole).toUuid().toString();
		room->appendRow(presenterEntry);room->appendRow(recipientEntry);source.appendRow(room);
		tree->setSourceModel(&source);controller->activateRoom(room->data(NetworkObjectModel::UidRole).toUuid().toString());
		QTRY_COMPARE(master.allComputerControlInterfaces().size(), 2);
		ComputerControlInterface::Pointer recipient;
		for (const auto& computer : master.allComputerControlInterfaces())
			if (computer->computer().networkObjectUid().toString() != presenterId) recipient = computer;
		QVERIFY(recipient);
		const auto share = QUuid(controller->feature(QStringLiteral("ShareUserScreenFullScreen")).value(QStringLiteral("uid")).toString());
		const auto demo = QUuid(controller->feature(QStringLiteral("Demo")).value(QStringLiteral("uid")).toString());
		QVERIFY(!share.isNull());QVERIFY(!demo.isNull());
		// Model the received demo state without starting a network demo server.
		recipient->setDesignatedModeFeature(share);recipient->setActiveFeatures({demo});
		controller->select(presenterId);QVERIFY(controller->selectedScope());
		if (filterRecipients) controller->setSearch(QStringLiteral("Presenter S1"));
		QCOMPARE(controller->visibleCount(), filterRecipients ? 1 : 2);
		QVERIFY(!controller->activeSummary().isEmpty());
		QTRY_VERIFY(visibleItemWithProperty(window->contentItem(), "text", QStringLiteral("Stop")));
		auto stop = visibleItemWithProperty(window->contentItem(), "text", QStringLiteral("Stop"));
		QVERIFY(QMetaObject::invokeMethod(stop, "clicked"));
		QVERIFY2(recipient->designatedModeFeature().isNull(), "Global Stop must cancel the unselected recipient's demo mode");
		QCOMPARE(controller->selectedCount(), 1);QVERIFY(controller->isSelected(presenterId));QVERIFY(controller->selectedScope());
	}
	void mixedModeTargetsConvergeBeforeTogglingOff()
	{
		VeyonMaster master(m_core.get());observe(master);auto controller=master.findChild<MainWindow*>();QVERIFY(controller);
		Feature mode;for(const auto& feature:master.allFeatures())if(feature.name()==QStringLiteral("ScreenLock"))mode=feature;
		QVERIFY(mode.isValid());auto first=computer(QStringLiteral("PC-01")),second=computer(QStringLiteral("PC-02"));
		first->setActiveFeatures({mode.uid()});controller->runFeatureOnComputers(mode,{first,second});
		QCOMPARE(first->designatedModeFeature(),mode.uid());QCOMPARE(second->designatedModeFeature(),mode.uid());
		second->setActiveFeatures({mode.uid()});controller->runFeatureOnComputers(mode,{first,second},{},false);
		QCOMPARE(first->designatedModeFeature(),mode.uid());QCOMPARE(second->designatedModeFeature(),mode.uid());
		controller->runFeatureOnComputers(mode,{first,second});
		QVERIFY(first->designatedModeFeature().isNull());QVERIFY(second->designatedModeFeature().isNull());
	}
	void customPositionsSurviveViewChanges()
	{
		VeyonMaster master(m_core.get());observe(master);auto controller=master.findChild<MainWindow*>();QVERIFY(controller);
		SourceFixture fixture(master);auto target=computer(QStringLiteral("PC-01"));append(fixture.source,target);
		const auto id=target->computer().networkObjectUid().toString();
		controller->setPosition(id,240,170);controller->setCustomPositions(true);
		QCOMPARE(controller->position(id,0),QPoint(240,170));
		controller->setCustomPositions(false);master.mainQmlWindow()->setProperty("listMode",true);
		controller->setCustomPositions(true);QCOMPARE(controller->position(id,0),QPoint(240,170));
		QCOMPARE(master.userConfig().computerPositions().first().toObject().value(QStringLiteral("uid")).toString(),id);
	}
	void legacyGridPositionsKeepFractionsAcrossStartupAndShutdown()
	{
		UserConfigurationFixture configuration;QVERIFY(configuration.home.isValid());
		const auto id = QUuid::createUuid().toString();
		{
			UserConfig seed(QStringLiteral("VeyonMaster"));
			seed.setMonitoringScreenSize(200);
			seed.setUseCustomComputerPositions(true);
			seed.setComputerPositions({QJsonObject{{QStringLiteral("uid"), id}, {QStringLiteral("x"), 1.5}, {QStringLiteral("y"), 2.25}}});
			seed.flushStore();
		}
		{
			VeyonMaster master(m_core.get());observe(master);
			auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
			// The saved values are desk-grid units, not a pixel offset near (0,0).
			const auto pixels = controller->position(id, 0);
			QVERIFY(pixels.x() > controller->cardWidth());
			QVERIFY(pixels.y() > controller->cardWidth());
		}
		UserConfig restored(QStringLiteral("VeyonMaster"));
		const auto position = savedPosition(restored, id);
		QVERIFY(!position.isEmpty());
		QCOMPARE(position.value(QStringLiteral("x")).toDouble(), 1.5);
		QCOMPARE(position.value(QStringLiteral("y")).toDouble(), 2.25);
	}
	void customDeskPositionsScaleWithThumbnailSize()
	{
		UserConfigurationFixture configuration;QVERIFY(configuration.home.isValid());
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		const auto id = QUuid::createUuid().toString();
		controller->setCustomPositions(true);controller->setCardWidth(200);
		controller->setPosition(id, 480, 340);
		QCOMPARE(controller->position(id, 0), QPoint(480, 340));
		const auto before = savedPosition(master.userConfig(), id);
		controller->setCardWidth(400);
		const auto enlarged = controller->position(id, 0);
		QVERIFY(enlarged.x() > 480);
		QVERIFY(enlarged.y() > 340);
		QCOMPARE(savedPosition(master.userConfig(), id), before);
		controller->setCardWidth(200);
		QCOMPARE(controller->position(id, 0), QPoint(480, 340));
	}
	void alignSnapsExistingDeskPositionsWithoutReordering()
	{
		UserConfigurationFixture configuration;QVERIFY(configuration.home.isValid());
		auto first = computer(QStringLiteral("PC-01")), second = computer(QStringLiteral("PC-02"));
		const auto firstId = first->computer().networkObjectUid().toString();
		const auto secondId = second->computer().networkObjectUid().toString();
		{
			UserConfig seed(QStringLiteral("VeyonMaster"));
			seed.setUseCustomComputerPositions(true);
			seed.setComputerPositions({
				QJsonObject{{QStringLiteral("uid"), firstId}, {QStringLiteral("x"), 2.2}, {QStringLiteral("y"), 1.1}},
				QJsonObject{{QStringLiteral("uid"), secondId}, {QStringLiteral("x"), 0.2}, {QStringLiteral("y"), 3.4}}
			});
			seed.flushStore();
		}
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		SourceFixture fixture(master);append(fixture.source, first);append(fixture.source, second);
		controller->alignPositions();
		const auto firstPosition = savedPosition(master.userConfig(), firstId);
		const auto secondPosition = savedPosition(master.userConfig(), secondId);
		QCOMPARE(firstPosition.value(QStringLiteral("x")).toDouble(), 2.0);
		QCOMPARE(firstPosition.value(QStringLiteral("y")).toDouble(), 1.0);
		QCOMPARE(secondPosition.value(QStringLiteral("x")).toDouble(), 0.0);
		QCOMPARE(secondPosition.value(QStringLiteral("y")).toDouble(), 3.0);
		QVERIFY(controller->position(firstId, 0).x() > controller->position(secondId, 1).x());
		QVERIFY(controller->position(firstId, 0).y() < controller->position(secondId, 1).y());
	}
	void markedPositionStoresUseDeclaredCoordinates_data()
	{
		QTest::addColumn<QString>("format");
		QTest::addColumn<double>("storedX");QTest::addColumn<double>("storedY");
		QTest::addColumn<QPoint>("pixels");
		QTest::newRow("grid-v1") << QStringLiteral("grid-v1") << 1.5 << 2.25 << QPoint(324, 422);
		QTest::newRow("screen-pixels-v1") << QStringLiteral("screen-pixels-v1") << 480.0 << 340.0 << QPoint(480, 340);
	}
	void markedPositionStoresUseDeclaredCoordinates()
	{
		QFETCH(QString, format);QFETCH(double, storedX);QFETCH(double, storedY);QFETCH(QPoint, pixels);
		UserConfigurationFixture configuration;QVERIFY(configuration.home.isValid());
		const auto id = QUuid::createUuid().toString();
		{
			UserConfig seed(QStringLiteral("VeyonMaster"));seed.setMonitoringScreenSize(200);
			seed.setValue(QStringLiteral("ComputerPositionsFormat"), format, QStringLiteral("UI"));
			seed.setComputerPositions({QJsonObject{{QStringLiteral("uid"), id}, {QStringLiteral("x"), storedX}, {QStringLiteral("y"), storedY}}});
			seed.flushStore();
		}
		{
			VeyonMaster master(m_core.get());observe(master);
			auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
			QCOMPARE(controller->position(id, 0), pixels);
		}
		UserConfig restored(QStringLiteral("VeyonMaster"));
		QCOMPARE(restored.value(QStringLiteral("ComputerPositionsFormat"), QStringLiteral("UI"), {}).toString(), QStringLiteral("grid-v1"));
		const auto saved = savedPosition(restored, id);
		if (format == QStringLiteral("grid-v1")) {
			QCOMPARE(saved.value(QStringLiteral("x")).toDouble(), 1.5);
			QCOMPARE(saved.value(QStringLiteral("y")).toDouble(), 2.25);
		} else {
			QVERIFY(qAbs(saved.value(QStringLiteral("x")).toDouble() - 20. / 9.) < 0.000001);
			QVERIFY(qAbs(saved.value(QStringLiteral("y")).toDouble() - 136. / 75.) < 0.000001);
		}
	}
	void positionExportsDeclareCoordinateFormat()
	{
		UserConfigurationFixture configuration;QVERIFY(configuration.home.isValid());
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		const auto id = QUuid::createUuid().toString();controller->setCardWidth(200);controller->setPosition(id, 480, 340);
		const auto path = configuration.home.filePath(QStringLiteral("positions-export.json"));
		PositionDialogResponder dialogs(path);
		controller->positionsFile(true);
		QVERIFY(!dialogs.timedOut);QVERIFY(!dialogs.unexpectedDialog);QVERIFY(!dialogs.coordinatePrompt);
		QFile file(path);QVERIFY(file.open(QIODevice::ReadOnly));
		const auto ui = QJsonDocument::fromJson(file.readAll()).object().value(QStringLiteral("UI")).toObject();
		QCOMPARE(ui.value(QStringLiteral("ComputerPositionsFormat")).toString(), QStringLiteral("grid-v1"));
		const auto positions = ui.value(QStringLiteral("ComputerPositions")).toObject().value(QStringLiteral("JsonStoreArray")).toArray();
		QCOMPARE(positions.size(), 1);
		QCOMPARE(positions.first().toObject().value(QStringLiteral("uid")).toString(), id);
	}
	void positionImportsRecoverPixelsOnlyWithExplicitChoice_data()
	{
		QTest::addColumn<QString>("format");QTest::addColumn<bool>("choosePixels");QTest::addColumn<bool>("prompt");
		QTest::newRow("unmarked-grid") << QString{} << false << true;
		QTest::newRow("unmarked-recover-pixels") << QString{} << true << true;
		QTest::newRow("marked-grid") << QStringLiteral("grid-v1") << false << false;
		QTest::newRow("marked-pixels") << QStringLiteral("screen-pixels-v1") << false << false;
	}
	void positionImportsRecoverPixelsOnlyWithExplicitChoice()
	{
		QFETCH(QString, format);QFETCH(bool, choosePixels);QFETCH(bool, prompt);
		UserConfigurationFixture configuration;QVERIFY(configuration.home.isValid());
		const auto id = QUuid::createUuid().toString();
		const bool pixels = choosePixels || format == QStringLiteral("screen-pixels-v1");
		const QJsonArray positions{QJsonObject{{QStringLiteral("uid"), id}, {QStringLiteral("x"), pixels ? 480.0 : 1.5}, {QStringLiteral("y"), pixels ? 340.0 : 2.25}}};
		QJsonObject ui{{QStringLiteral("ComputerPositions"), QJsonObject{{QStringLiteral("JsonStoreArray"), positions}}}};
		if (!format.isEmpty()) ui.insert(QStringLiteral("ComputerPositionsFormat"), format);
		const auto path = configuration.home.filePath(QStringLiteral("positions-import.json"));
		QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));
		file.write(QJsonDocument(QJsonObject{{QStringLiteral("UI"), ui}}).toJson());file.close();
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);controller->setCardWidth(200);
		PositionDialogResponder dialogs(path, choosePixels);
		controller->positionsFile(false);
		QVERIFY(!dialogs.timedOut);QVERIFY(!dialogs.unexpectedDialog);QCOMPARE(dialogs.coordinatePrompt, prompt);
		QCOMPARE(controller->position(id, 0), pixels ? QPoint(480, 340) : QPoint(324, 422));
		QCOMPARE(master.userConfig().value(QStringLiteral("ComputerPositionsFormat"), QStringLiteral("UI"), {}).toString(), QStringLiteral("grid-v1"));
	}
	void failedPositionImportsPreserveCurrentLayout_data()
	{
		QTest::addColumn<QString>("failure");
		for (const auto failure : {"file-cancel", "coordinate-cancel", "invalid-json", "invalid-array", "unknown-format", "unreadable"})
			QTest::newRow(failure) << QString::fromLatin1(failure);
	}
	void failedPositionImportsPreserveCurrentLayout()
	{
		QFETCH(QString, failure);
		UserConfigurationFixture configuration;QVERIFY(configuration.home.isValid());
		const auto id = QUuid::createUuid().toString();
		const auto path = configuration.home.filePath(QStringLiteral("positions-failure.json"));
		if (failure != QStringLiteral("unreadable")) {
			QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));
			if (failure == QStringLiteral("invalid-json")) file.write("{invalid-json");
			else if (failure == QStringLiteral("invalid-array")) file.write("{\"UI\":{\"ComputerPositions\":{\"JsonStoreArray\":\"invalid\"}}}");
			else {
				QJsonObject ui{{QStringLiteral("ComputerPositions"), QJsonObject{{QStringLiteral("JsonStoreArray"), QJsonArray{}}}}};
				if (failure == QStringLiteral("unknown-format")) ui.insert(QStringLiteral("ComputerPositionsFormat"), QStringLiteral("unknown-v99"));
				file.write(QJsonDocument(QJsonObject{{QStringLiteral("UI"), ui}}).toJson());
			}
		}
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		controller->setPosition(id, 250, 175);controller->setCustomPositions(true);
		const auto before = master.userConfig().computerPositions();
		PositionDialogResponder dialogs(path);
		dialogs.cancelFile = failure == QStringLiteral("file-cancel");
		dialogs.cancelCoordinates = failure == QStringLiteral("coordinate-cancel");
		controller->positionsFile(false);
		QVERIFY(!dialogs.timedOut);QVERIFY(!dialogs.unexpectedDialog);
		if (failure == QStringLiteral("coordinate-cancel")) QVERIFY(dialogs.coordinatePrompt);
		else if (failure != QStringLiteral("file-cancel")) QVERIFY(dialogs.errorShown);
		QCOMPARE(master.userConfig().computerPositions(), before);
		QCOMPARE(controller->position(id, 0), QPoint(250, 175));
		QVERIFY(controller->customPositions());
	}
	void customLayoutSupportsKeyboardSelection()
	{
		UserConfigurationFixture configuration;QVERIFY(configuration.home.isValid());
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		auto window = qobject_cast<QQuickWindow*>(master.mainQmlWindow());QVERIFY(window);
		SourceFixture fixture(master);
		auto first = computer(QStringLiteral("Keyboard PC-01")), second = computer(QStringLiteral("Keyboard PC-02"));
		append(fixture.source, first);append(fixture.source, second);
		const auto firstId = first->computer().networkObjectUid().toString();
		const auto secondId = second->computer().networkObjectUid().toString();
		controller->setCardWidth(200);controller->setCustomPositions(true);
		controller->setPosition(firstId, 0, 0);controller->setPosition(secondId, 240, 0);
		window->resize(1440, 900);window->show();
		QTRY_VERIFY(window->isExposed());
		QTRY_VERIFY(visibleItemWithProperty(window->contentItem(), "computerUid", firstId));
		auto firstCard = visibleItemWithProperty(window->contentItem(), "computerUid", firstId);
		QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
			firstCard->mapToScene(firstCard->boundingRect().center()).toPoint());
		QCOMPARE(controller->selectedCount(), 1);QVERIFY(controller->isSelected(firstId));
		QTest::keyClick(window, Qt::Key_Right);
		QCOMPARE(controller->selectedCount(), 1);
		QVERIFY(controller->isSelected(secondId));
		QTest::keyClick(window, Qt::Key_Space);
		QCOMPARE(controller->selectedCount(), 0);
		QTest::keyClick(window, Qt::Key_Home);
		QCOMPARE(controller->selectedCount(), 1);QVERIFY(controller->isSelected(firstId));
	}
	void accessDenialDetailsDisappearWhenFailureStateEnds()
	{
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		auto window = qobject_cast<QQuickWindow*>(master.mainQmlWindow());QVERIFY(window);
		window->resize(1440, 900);window->show();
		SourceFixture fixture(master);
		auto target = computer(QStringLiteral("Recovering PC"));
		const auto reason = QStringLiteral("Accessing user not member of an authorized user group");
		target->setAccessControlFailed(reason);append(fixture.source, target);
		controller->select(target->computer().networkObjectUid().toString());
		QCOMPARE(controller->details().value(QStringLiteral("error")).toString(), reason);
		QTRY_VERIFY(visibleItemWithProperty(window->contentItem(), "text", reason));
		// stop() publicly leaves AccessControlFailed while retaining its diagnostic.
		// This checks UI state gating without bypassing CCI encapsulation; the live
		// denied-to-Connected recovery is covered separately on the multi-VM stand.
		target->stop();
		QCOMPARE(target->state(), ComputerControlInterface::State::Disconnected);
		QCOMPARE(target->accessControlDetails(), reason);
		fixture.source.setData(fixture.source.index(0, 0), QVariant::fromValue(target->state()), ComputerControlListModel::StateRole);
		QVERIFY(controller->details().value(QStringLiteral("error")).toString().isEmpty());
		QTRY_VERIFY(!visibleItemWithProperty(window->contentItem(), "text", reason));
		// A subsequent real denial must still be displayed.
		target->setAccessControlFailed(reason);
		fixture.source.setData(fixture.source.index(0, 0), QVariant::fromValue(target->state()), ComputerControlListModel::StateRole);
		QCOMPARE(controller->details().value(QStringLiteral("error")).toString(), reason);
		QTRY_VERIFY(visibleItemWithProperty(window->contentItem(), "text", reason));
	}
	void slideshowHasNoCurrentComputerWithoutValidScreens()
	{
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		SourceFixture fixture(master);
		auto disconnected = computer(QStringLiteral("Offline PC"));
		auto denied = computer(QStringLiteral("Denied PC"));
		denied->setAccessControlFailed(QStringLiteral("Fixture access denied"));
		append(fixture.source, disconnected);append(fixture.source, denied);
		QVERIFY(!disconnected->hasValidFramebuffer());QVERIFY(!denied->hasValidFramebuffer());
		controller->nextSlide(1);
		QVERIFY(controller->slideshowComputer() == nullptr);
		controller->nextSlide(-1);
		QVERIFY(controller->slideshowComputer() == nullptr);
	}
	void screenshotViewersKeepIndependentImages()
	{
		QTemporaryDir screenshots;QVERIFY(screenshots.isValid());
		const auto firstName = QStringLiteral("comparison-first.png");
		const auto secondName = QStringLiteral("comparison-second.png");
		const auto firstPath = screenshots.filePath(firstName), secondPath = screenshots.filePath(secondName);
		QImage image(100, 80, QImage::Format_RGB32);image.fill(Qt::red);QVERIFY(image.save(firstPath));
		image.fill(Qt::blue);QVERIFY(image.save(secondPath));
		VeyonMaster master(m_core.get());observe(master);
		auto window = qobject_cast<QQuickWindow*>(master.mainQmlWindow());QVERIFY(window);
		window->resize(1440, 900);window->show();window->setProperty("panel", QStringLiteral("screenshots"));
		window->setProperty("selectedScreenshot", QUrl::fromLocalFile(firstPath).toString());
		window->setProperty("selectedScreenshotName", firstName);
		QTRY_VERIFY(visibleItemWithProperty(window->contentItem(), "text", QStringLiteral("Open")));
		auto open = visibleItemWithProperty(window->contentItem(), "text", QStringLiteral("Open"));
		QVERIFY(QMetaObject::invokeMethod(open, "clicked"));
		auto viewer = [](const QString& title) -> QQuickWindow* {
			for (auto candidate : QGuiApplication::allWindows())
				if (candidate->isVisible() && candidate->title() == title) return qobject_cast<QQuickWindow*>(candidate);
			return nullptr;
		};
		QTRY_VERIFY(viewer(firstName));
		QPointer<QQuickWindow> firstViewer(viewer(firstName));
		window->setProperty("selectedScreenshot", QUrl::fromLocalFile(secondPath).toString());
		window->setProperty("selectedScreenshotName", secondName);
		QVERIFY(QMetaObject::invokeMethod(open, "clicked"));
		QTRY_VERIFY(viewer(secondName));
		QVERIFY(firstViewer);
		QCOMPARE(firstViewer->title(), firstName);
		QVERIFY(viewer(firstName) != viewer(secondName));
		QVERIFY(visibleItemWithProperty(firstViewer->contentItem(), "source", QUrl::fromLocalFile(firstPath)));
		QVERIFY(visibleItemWithProperty(viewer(secondName)->contentItem(), "source", QUrl::fromLocalFile(secondPath)));
	}
	void selectedComputerExposesSessionDuration()
	{
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		auto window = qobject_cast<QQuickWindow*>(master.mainQmlWindow());QVERIFY(window);
		SourceFixture fixture(master);
		auto target = computer(QStringLiteral("Session PC"));
		target->setUserInformation(QStringLiteral("student"), QStringLiteral("Student"));
		PlatformSessionFunctions::SessionInfo session;session.uptime = 3661;
		target->setSessionInfo(session);append(fixture.source, target);
		controller->select(target->computer().networkObjectUid().toString());
		window->resize(1440, 900);window->show();
		QTRY_VERIFY(visibleItemWithProperty(window->contentItem(), "text", QStringLiteral("01:01:01")));
		session.uptime = 3662;target->setSessionInfo(session);
		Q_EMIT fixture.source.dataChanged(fixture.source.index(0, 0), fixture.source.index(0, 0));
		QTRY_VERIFY(visibleItemWithProperty(window->contentItem(), "text", QStringLiteral("01:01:02")));
	}
	void searchDiscoversUncheckedDirectoryComputers_data()
	{
		QTest::addColumn<QString>("query");
		QTest::newRow("computer") << QStringLiteral("PC-B");
		QTest::newRow("user") << QStringLiteral("Student B");
		QTest::newRow("room") << QStringLiteral("Room B");
	}
	void searchDiscoversUncheckedDirectoryComputers()
	{
		QFETCH(QString, query);
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		auto tree = qobject_cast<CheckableItemProxyModel*>(master.computerManager().computerTreeModel());QVERIFY(tree);
		QStandardItemModel source;
		struct Restore { CheckableItemProxyModel* tree; QAbstractItemModel* original;
			~Restore() { tree->setSourceModel(original); }
		} restore{tree, tree->sourceModel()};
		QString secondRoom, secondHost;
		for (const auto& suffix : {QStringLiteral("A"), QStringLiteral("B")}) {
			auto room = new QStandardItem(QStringLiteral("Room ") + suffix);
			const auto roomId = QUuid::createUuid(), hostId = QUuid::createUuid();
			room->setData(roomId, NetworkObjectModel::UidRole);
			room->setData(QVariant::fromValue(NetworkObject::Type::Location), NetworkObjectModel::TypeRole);
			auto host = new QStandardItem(QStringLiteral("PC-") + suffix);
			host->setData(hostId, NetworkObjectModel::UidRole);
			host->setData(QVariant::fromValue(NetworkObject::Type::Host), NetworkObjectModel::TypeRole);
			room->appendRow({host, new QStandardItem(QStringLiteral("Student ") + suffix), new QStandardItem(QStringLiteral("00:10:00"))});
			source.appendRow({room, new QStandardItem, new QStandardItem});
			if (suffix == QStringLiteral("B")) {secondRoom = roomId.toString();secondHost = hostId.toString();}
		}
		tree->setSourceModel(&source);
		QCOMPARE(controller->rooms().size(), 2);
		controller->setSearch(query);
		QCOMPARE(controller->rooms().size(), 1);
		const auto match = controller->rooms().first().toMap();
		QCOMPARE(match.value(QStringLiteral("uid")).toString(), secondRoom);
		const auto hosts = match.value(QStringLiteral("hosts")).toList();
		QCOMPARE(hosts.size(), 1);
		QCOMPARE(hosts.first().toMap().value(QStringLiteral("uid")).toString(), secondHost);
		QVERIFY(!hosts.first().toMap().value(QStringLiteral("checked")).toBool());
		controller->setSearch({});
		QCOMPARE(controller->rooms().size(), 2);
	}
	void oversizedGridImportCannotOverflowAfterZoom()
	{
		UserConfigurationFixture configuration;QVERIFY(configuration.home.isValid());
		const auto existingId = QUuid::createUuid().toString(), importedId = QUuid::createUuid().toString();
		const auto path = configuration.home.filePath(QStringLiteral("oversized-grid.json"));
		const QJsonArray positions{QJsonObject{{QStringLiteral("uid"), importedId}, {QStringLiteral("x"), 5000000.0}, {QStringLiteral("y"), 0.0}}};
		const QJsonObject ui{{QStringLiteral("ComputerPositionsFormat"), QStringLiteral("grid-v1")},
			{QStringLiteral("ComputerPositions"), QJsonObject{{QStringLiteral("JsonStoreArray"), positions}}}};
		QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));
		file.write(QJsonDocument(QJsonObject{{QStringLiteral("UI"), ui}}).toJson());file.close();
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		controller->setCardWidth(190);controller->setPosition(existingId, 250, 175);
		const auto before = master.userConfig().computerPositions();
		PositionDialogResponder dialogs(path);controller->positionsFile(false);
		// Five million columns fit int at width190, but overflow after a supported zoom to640.
		controller->setCardWidth(640);
		QVERIFY(!dialogs.timedOut);QVERIFY(dialogs.errorShown);
		QCOMPARE(master.userConfig().computerPositions(), before);
		QVERIFY(controller->position(existingId, 0).x() > 250);
	}
	void untouchedUnmarkedPositionsRemainRecoverableAfterShutdown()
	{
		UserConfigurationFixture configuration;QVERIFY(configuration.home.isValid());
		const auto id = QUuid::createUuid().toString();
		{
			UserConfig seed(QStringLiteral("VeyonMaster"));
			seed.setComputerPositions({QJsonObject{{QStringLiteral("uid"), id}, {QStringLiteral("x"), 480.0}, {QStringLiteral("y"), 340.0}}});
			seed.flushStore();
		}
		{
			VeyonMaster master(m_core.get());observe(master);
			// Merely opening/closing cannot decide whether an unmarked file came
			// from the legacy grid UI or the interim pixel-coordinate QML UI.
		}
		UserConfig restored(QStringLiteral("VeyonMaster"));
		QVERIFY(restored.value(QStringLiteral("ComputerPositionsFormat"), QStringLiteral("UI"), {}).toString().isEmpty());
		const auto position = savedPosition(restored, id);
		QCOMPARE(position.value(QStringLiteral("x")).toDouble(), 480.0);
		QCOMPARE(position.value(QStringLiteral("y")).toDouble(), 340.0);
	}
	void roomSearchKeepsNewlyIncludedComputersVisible()
	{
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		auto tree = qobject_cast<CheckableItemProxyModel*>(master.computerManager().computerTreeModel());QVERIFY(tree);
		QStandardItemModel source;
		struct Restore { CheckableItemProxyModel* tree; QAbstractItemModel* original;
			~Restore() { tree->setSourceModel(original); }
		} restore{tree, tree->sourceModel()};
		const auto roomId = QUuid::createUuid(), hostId = QUuid::createUuid();
		auto room = new QStandardItem(QStringLiteral("Art Lab"));
		room->setData(QStringLiteral("Art Lab"), NetworkObjectModel::NameRole);
		room->setData(roomId, NetworkObjectModel::UidRole);
		room->setData(QVariant::fromValue(NetworkObject::Type::Location), NetworkObjectModel::TypeRole);
		auto host = new QStandardItem(QStringLiteral("Station"));
		host->setData(QStringLiteral("Station"), NetworkObjectModel::NameRole);
		host->setData(hostId, NetworkObjectModel::UidRole);
		host->setData(QVariant::fromValue(NetworkObject::Type::Host), NetworkObjectModel::TypeRole);
		host->setData(QStringLiteral("127.0.0.1:1"), NetworkObjectModel::HostAddressRole);
		room->appendRow(host);source.appendRow(room);tree->setSourceModel(&source);
		controller->setSearch(QStringLiteral("Art Lab"));
		QCOMPARE(controller->rooms().size(), 1);
		controller->activateRoom(roomId.toString());
		QTRY_COMPARE(master.computerControlListModel().rowCount(), 1);
		const auto computer = master.computerControlListModel().computerControlInterface(master.computerControlListModel().index(0, 0));
		QVERIFY(computer);QCOMPARE(computer->computer().location(), QStringLiteral("Art Lab"));
		QCOMPARE(controller->computers()->rowCount(), 1);
		QCOMPARE(controller->computerIdAt(0), hostId.toString());
	}
	void desktopGroupDirectoryContainersRemainReachable()
	{
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		auto tree = qobject_cast<CheckableItemProxyModel*>(master.computerManager().computerTreeModel());QVERIFY(tree);
		QStandardItemModel source;
		struct Restore { CheckableItemProxyModel* tree; QAbstractItemModel* original;
			~Restore() { tree->setSourceModel(original); }
		} restore{tree, tree->sourceModel()};
		auto entry = [](const QString& name, NetworkObject::Type type) {
			auto item = new QStandardItem(name);
			item->setData(name, NetworkObjectModel::NameRole);
			item->setData(QUuid::createUuid(), NetworkObjectModel::UidRole);
			item->setData(QVariant::fromValue(type), NetworkObjectModel::TypeRole);
			if (type == NetworkObject::Type::Host) item->setData(QStringLiteral("127.0.0.1:1"), NetworkObjectModel::HostAddressRole);
			return item;
		};
		auto group = entry(QStringLiteral("Desktop group"), NetworkObject::Type::DesktopGroup);
		auto directHost = entry(QStringLiteral("Direct desktop"), NetworkObject::Type::Host);
		auto location = entry(QStringLiteral("Nested room"), NetworkObject::Type::Location);
		auto nestedHost = entry(QStringLiteral("Nested desktop"), NetworkObject::Type::Host);
		const auto groupId = group->data(NetworkObjectModel::UidRole).toUuid().toString();
		const auto locationId = location->data(NetworkObjectModel::UidRole).toUuid().toString();
		group->appendRow(directHost);location->appendRow(nestedHost);group->appendRow(location);
		source.appendRow(group);tree->setSourceModel(&source);
		QCOMPARE(controller->rooms().size(), 2);
		QCOMPARE(controller->rooms().first().toMap().value(QStringLiteral("uid")).toString(), groupId);
		QCOMPARE(controller->rooms().first().toMap().value(QStringLiteral("count")).toInt(), 2);
		QCOMPARE(controller->rooms().first().toMap().value(QStringLiteral("hosts")).toList().size(), 1);
		controller->activateRoom(groupId);
		QTRY_COMPARE(master.computerControlListModel().rowCount(), 2);
		controller->setSearch(QStringLiteral("Nested desktop"));
		QCOMPARE(controller->rooms().size(), 2);
		QCOMPARE(controller->rooms().last().toMap().value(QStringLiteral("uid")).toString(), locationId);
		QCOMPARE(controller->rooms().last().toMap().value(QStringLiteral("depth")).toInt(), 1);
		QCOMPARE(controller->computers()->rowCount(), 1);
	}
	void framebufferOwnsComputerAcrossModelReset()
	{
		auto target=computer(QStringLiteral("PC-01"));QWeakPointer<ComputerControlInterface> weak(target);
		QuickFramebufferItem item;item.setComputer(target.data());target.clear();
		QVERIFY(!weak.isNull());QVERIFY(item.computer());
		item.setComputer(nullptr);QVERIFY(weak.isNull());
		ComputerControlInterface local(Computer(QUuid::createUuid(),QStringLiteral("local"),QStringLiteral("127.0.0.1")));
		{QuickFramebufferItem stackView;stackView.setComputer(&local);QCOMPARE(stackView.computer(),&local);}
	}
	void framebufferLiveLeaseRestoresMonitoring()
	{
		auto target=computer(QStringLiteral("PC-01"));target->setUpdateMode(ComputerControlInterface::UpdateMode::Monitoring);
		QuickFramebufferItem first,second;first.setComputer(target.data());second.setComputer(target.data());
		first.setLive(true);second.setLive(true);QCOMPARE(target->updateMode(),ComputerControlInterface::UpdateMode::Live);
		first.setLive(false);QCOMPARE(target->updateMode(),ComputerControlInterface::UpdateMode::Live);
		second.setLive(false);QCOMPARE(target->updateMode(),ComputerControlInterface::UpdateMode::Monitoring);
	}
	void remoteKeysReleaseTheirOriginalSymbol()
	{
		QuickKeyEventTracker tracker;
		struct SentKey { QEvent::Type type; int key; quint32 symbol; QString text; bool repeat; };
		QList<SentKey> sent;
		auto send = [&sent](QKeyEvent* event) {
			sent.append({event->type(), event->key(), event->nativeVirtualKey(), event->text(), event->isAutoRepeat()});
		};
		QKeyEvent slashDown(QEvent::KeyPress, Qt::Key_Slash, Qt::NoModifier, 61, 0x2f, 0, QStringLiteral("/"));
		QKeyEvent remappedUp(QEvent::KeyRelease, Qt::Key_Question, Qt::NoModifier, 61, 0x3f, 0, QStringLiteral("?"));
		tracker.forward(&slashDown, send);
		tracker.forward(&remappedUp, send);
		QCOMPARE(sent.size(), 2);
		QCOMPARE(sent.at(1).type, QEvent::KeyRelease);
		QCOMPARE(sent.at(1).key, int(Qt::Key_Slash));
		QCOMPARE(sent.at(1).symbol, quint32(0x2f));
		QCOMPARE(sent.at(1).text, QStringLiteral("/"));
		tracker.releaseAll(send);
		QCOMPARE(sent.size(), 2);

		sent.clear();
		tracker.forward(&slashDown, send);
		QKeyEvent repeatUp(QEvent::KeyRelease, Qt::Key_Question, Qt::NoModifier, 61, 0x3f, 0, QStringLiteral("?"), true);
		QKeyEvent repeatDown(QEvent::KeyPress, Qt::Key_Question, Qt::NoModifier, 61, 0x3f, 0, QStringLiteral("?"), true);
		tracker.forward(&repeatUp, send);
		tracker.forward(&repeatDown, send);
		tracker.releaseAll(send);
		QCOMPARE(sent.size(), 4);
		for (const auto& key : sent) QCOMPARE(key.symbol, quint32(0x2f));
		QVERIFY(sent.at(1).repeat && sent.at(2).repeat);
		QCOMPARE(sent.last().type, QEvent::KeyRelease);
		QVERIFY(!sent.last().repeat);
		tracker.releaseAll(send);
		QCOMPARE(sent.size(), 4);

		sent.clear();
		QKeyEvent shiftDown(QEvent::KeyPress, Qt::Key_Shift, Qt::ShiftModifier, 50, 0xffe1, 0);
		QKeyEvent letterDown(QEvent::KeyPress, Qt::Key_A, Qt::ShiftModifier, 0, 0x41, 0, QStringLiteral("A"));
		tracker.forward(&shiftDown, send);
		tracker.forward(&letterDown, send);
		tracker.forward(&slashDown, send);
		tracker.releaseAll(send);
		QCOMPARE(sent.size(), 6);
		QCOMPARE(sent.at(3).symbol, quint32(0x2f));
		QCOMPARE(sent.at(4).symbol, quint32(0x41));
		QCOMPARE(sent.at(5).symbol, quint32(0xffe1));
		for (int index = 3; index < sent.size(); ++index) QCOMPARE(sent.at(index).type, QEvent::KeyRelease);
		tracker.forward(&letterDown, send);
		QKeyEvent letterUp(QEvent::KeyRelease, Qt::Key_A, Qt::NoModifier, 0, 0x61, 0, QStringLiteral("a"));
		tracker.forward(&letterUp, send);
		QCOMPARE(sent.last().symbol, quint32(0x41));
		tracker.releaseAll(send);
		QCOMPARE(sent.size(), 8);
	}
	void themeHasReadableTextInBothSchemes()
	{
		for(auto scheme:{VeyonCore::UiColorScheme::Light,VeyonCore::UiColorScheme::Dark}) {
			VeyonCore::config().setUiColorScheme(scheme);
			std::unique_ptr<QQmlApplicationEngine> engine(QuickUi::createEngine());
			connect(engine.get(), &QQmlEngine::warnings, this, [this](const QList<QQmlError>& warnings) {
				for (const auto& warning : warnings) m_qmlWarnings.append(warning.toString());
			});
			QQmlComponent component(engine.get());
			component.setData("import QtQuick\nimport \"qrc:/quick\" as Ui\nQtObject { property color foreground: Ui.Theme.text; property color disabled: Ui.Theme.muted; property color background: Ui.Theme.surface; property color accent: Ui.Theme.accent; property color onAccent: Ui.Theme.onAccent }",QUrl());
			std::unique_ptr<QObject> theme(component.create());QVERIFY2(theme,qPrintable(component.errorString()));
			const auto background=theme->property("background").value<QColor>();
			QVERIFY(contrast(theme->property("foreground").value<QColor>(),background)>=4.5);
			QVERIFY(contrast(theme->property("disabled").value<QColor>(),background)>=4.5);
			QVERIFY(contrast(theme->property("onAccent").value<QColor>(),theme->property("accent").value<QColor>())>=3);
		}
	}
	void forcedThemeOverridesDesktopControlPalette()
	{
		struct RestorePalette { QPalette value; ~RestorePalette() { qApp->setPalette(value); } } restore{qApp->palette()};
		QPalette desktop = restore.value;
		desktop.setColor(QPalette::Text, Qt::magenta);
		desktop.setColor(QPalette::Base, Qt::yellow);
		qApp->setPalette(desktop);
		for (auto scheme : {VeyonCore::UiColorScheme::Light, VeyonCore::UiColorScheme::Dark}) {
			VeyonCore::config().setUiColorScheme(scheme);
			VeyonMaster master(m_core.get());observe(master);
			auto window = master.mainQmlWindow();QVERIFY(window);
			auto search = window->findChild<QObject*>(QStringLiteral("computerSearch"));QVERIFY(search);
			const bool dark = scheme == VeyonCore::UiColorScheme::Dark;
			QCOMPARE(QQmlProperty::read(search, QStringLiteral("palette.text")).value<QColor>(), QColor(dark ? QStringLiteral("#edebea") : QStringLiteral("#201e1d")));
			QCOMPARE(QQmlProperty::read(search, QStringLiteral("palette.base")).value<QColor>(), QColor(dark ? QStringLiteral("#22201f") : QStringLiteral("#eae9e9")));
			QCOMPARE(QQmlProperty::read(search, QStringLiteral("palette.placeholderText")).value<QColor>(), QColor(dark ? QStringLiteral("#bab6b6") : QStringLiteral("#605d5d")));
			search->setProperty("enabled", false);
			const auto disabledText = QQmlProperty::read(search, QStringLiteral("palette.text")).value<QColor>();
			const auto disabledBase = QQmlProperty::read(search, QStringLiteral("palette.base")).value<QColor>();
			QVERIFY(contrast(disabledText, disabledBase) >= 4.5);
			QCOMPARE(window->property("font").value<QFont>().pixelSize(), 13);
		}
	}
	void remotePluginLoadsActualQuickTabHostWithoutWarnings()
	{
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		Feature remote;
		for (const auto& feature : master.allFeatures()) if (feature.name() == QStringLiteral("RemoteView")) remote = feature;
		QVERIFY(remote.isValid());
		auto target = computer(QStringLiteral("Remote UI test"));
		controller->runFeatureOnComputers(remote, {target}, {target});
		QObject* workspace = nullptr;
		for (auto object : qApp->children()) if (QByteArray(object->metaObject()->className()) == QByteArrayLiteral("RemoteAccessWorkspace")) workspace = object;
		QVERIFY(workspace);
		m_qmlWarnings.append(workspace->property("qmlWarnings").toStringList());
		auto engine = workspace->findChild<QQmlApplicationEngine*>();QVERIFY(engine);
		connect(engine, &QQmlEngine::warnings, this, [this](const QList<QQmlError>& warnings) {
			for (const auto& warning : warnings) m_qmlWarnings.append(warning.toString());
		});
		QCOMPARE(engine->rootObjects().size(), 1);
		auto window = qobject_cast<QQuickWindow*>(engine->rootObjects().first());QVERIFY(window);
		QCOMPARE(window->objectName(), QStringLiteral("remoteAccessWindow"));
		QCOMPARE(workspace->property("sessions").toList().size(), 1);
		QCOMPARE(window->property("font").value<QFont>().pixelSize(), 13);
		QPointer<QObject> lifetime(workspace);
		QVERIFY(window->close());
		QTRY_VERIFY(lifetime.isNull());
	}
	void remoteScreenViewportsUseFramebufferCoordinates_data()
	{
		QTest::addColumn<QRect>("otherScreen");
		QTest::addColumn<QRect>("primaryViewport");
		QTest::addColumn<QRect>("otherViewport");
		QTest::newRow("left") << QRect(-1920, 0, 1920, 1080)
			<< QRect(1920, 0, 1920, 1080) << QRect(0, 0, 1920, 1080);
		QTest::newRow("above") << QRect(0, -1200, 1920, 1200)
			<< QRect(0, 1200, 1920, 1080) << QRect(0, 0, 1920, 1200);
		QTest::newRow("right") << QRect(1920, 0, 1280, 1024)
			<< QRect(0, 0, 1920, 1080) << QRect(1920, 0, 1280, 1024);
	}
	void remoteScreenViewportsUseFramebufferCoordinates()
	{
		QFETCH(QRect, otherScreen);
		QFETCH(QRect, primaryViewport);
		QFETCH(QRect, otherViewport);
		VeyonMaster master(m_core.get());observe(master);
		auto controller = master.findChild<MainWindow*>();QVERIFY(controller);
		Feature remote;
		for (const auto& feature : master.allFeatures())
			if (feature.name() == QStringLiteral("RemoteView")) remote = feature;
		QVERIFY(remote.isValid());
		auto target = computer(QStringLiteral("Remote screen origins"));
		target->setScreens({
			{0, QStringLiteral("Primary"), QRect(0, 0, 1920, 1080)},
			{1, QStringLiteral("Other"), otherScreen}
		});
		controller->runFeatureOnComputers(remote, {target}, {target});
		QObject* workspace = nullptr;
		for (auto object : qApp->children())
			if (QByteArray(object->metaObject()->className()) == QByteArrayLiteral("RemoteAccessWorkspace")) workspace = object;
		QVERIFY(workspace);
		struct CloseWorkspace {
			QPointer<QObject> workspace;
			~CloseWorkspace() {
				if (workspace) QMetaObject::invokeMethod(workspace, "closeAll", Qt::DirectConnection, Q_ARG(bool, true));
				QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
			}
		} close{workspace};
		m_qmlWarnings.append(workspace->property("qmlWarnings").toStringList());
		auto engine = workspace->findChild<QQmlApplicationEngine*>();QVERIFY(engine);
		connect(engine, &QQmlEngine::warnings, this, [this](const QList<QQmlError>& warnings) {
			for (const auto& warning : warnings) m_qmlWarnings.append(warning.toString());
		});
		auto session = workspace->property("currentSession").value<QObject*>();QVERIFY(session);
		QVERIFY(session->property("viewport").toRect().isEmpty());
		QVERIFY(QMetaObject::invokeMethod(session, "selectScreen", Q_ARG(int, 0)));
		QCOMPARE(session->property("viewport").toRect(), primaryViewport);
		QVERIFY(QMetaObject::invokeMethod(session, "selectScreen", Q_ARG(int, 1)));
		QCOMPARE(session->property("viewport").toRect(), otherViewport);
		QVERIFY(QMetaObject::invokeMethod(session, "selectScreen", Q_ARG(int, -1)));
		QVERIFY(session->property("viewport").toRect().isEmpty());
	}
	void iconGlyphsStayInsideTheirCanvas()
	{
		QFile file(QStringLiteral(VEYON_TEST_SOURCE_DIR "/core/src/DesktopIcons.cpp"));QVERIFY(file.open(QIODevice::ReadOnly));
		const auto source=QString::fromUtf8(file.readAll());
		QRegularExpression expression(QStringLiteral("\\{ \\\"([^\\\"]+)\\\", 0x[0-9a-f]+ \\}"));
		auto matches=expression.globalMatch(source);int count=0;
		while(matches.hasNext()) {
			const auto name=matches.next().captured(1);++count;
			for(int scale:{1,2}) {
				QImage image(QSize(48,48)*scale,QImage::Format_ARGB32_Premultiplied);image.setDevicePixelRatio(scale);image.fill(Qt::transparent);
				{QPainter painter(&image);DesktopIcons::icon(name).paint(&painter,QRect(16,16,16,16));}
				QRect ink;for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)if(qAlpha(image.pixel(x,y))>32)ink|=QRect(x,y,1,1);
				QVERIFY2(!ink.isEmpty(),qPrintable(name));
				QVERIFY2(QRect(16*scale,16*scale,16*scale,16*scale).contains(ink),qPrintable(name));
			}
		}
		QVERIFY(count>1000);
		for(auto name:{QStringLiteral("chevron-down"),QStringLiteral("ellipsis")})for(int size:{12,16})for(int scale:{1,2}) {
			const auto image=DesktopIcons::icon(name).pixmap(QSize(size,size),scale).toImage();QRect ink;
			for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)if(qAlpha(image.pixel(x,y))>32)ink|=QRect(x,y,1,1);
			const auto offset=QRectF(ink).center()-QRectF(image.rect()).center();
			QVERIFY2(qAbs(offset.x())<=scale*.5&&qAbs(offset.y())<=scale*.5,qPrintable(name));
		}
	}
	void cleanupTestCase(){m_core.reset();}
private:
	void observe(VeyonMaster& master)
	{
		auto controller = master.findChild<MainWindow*>();
		QVERIFY(controller);
		m_qmlWarnings.append(controller->qmlWarnings());
		connect(controller, &MainWindow::qmlWarning, this, [this](const QString& warning) {
			m_qmlWarnings.append(warning);
		});
	}
	QStringList m_qmlWarnings;
	QTemporaryDir m_home;
	std::unique_ptr<VeyonCore> m_core;
};
int main(int argc,char** argv)
{
	VeyonCore::setupApplicationParameters();QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);
	MasterWorkspaceTest test;return QTest::qExec(&test,argc,argv);
}
#include "MasterWorkspaceTest.moc"
