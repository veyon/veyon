// SPDX-FileCopyrightText: 2004-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later
#include "MainWindow.h"
#include "AboutDialog.h"
#include "VeyonMaster.h"
#include "ComputerControlListModel.h"
#include "ComputerMonitoringModel.h"
#include "ComputerManager.h"
#include "UserConfig.h"
#include "QuickUi.h"
#include "FeatureManager.h"
#include "PluginManager.h"
#include "NetworkObjectModel.h"
#include "NetworkObjectDirectory.h"
#include "NetworkObjectDirectoryManager.h"
#include "VeyonConfiguration.h"
#include "AuthenticationCredentials.h"
#include "AccessControlProvider.h"
#include "PlatformUserFunctions.h"
#include "BuiltinFeatures.h"
#include "Filesystem.h"
#include "Screenshot.h"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QWindow>
#include <QGuiApplication>
#include <QHostAddress>
#include <QDesktopServices>
#include <QFileSystemWatcher>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QFileInfo>
#include <QTime>
#include <QJsonDocument>
#include <QScopedValueRollback>
#include <algorithm>
#include <functional>
#include <cmath>
#include <limits>

namespace {
constexpr int maximumCardWidth = 640;
QSizeF computerPositionGridSize(int width) { return {width + 16., width * 9. / 16. + 75.}; }
const QString gridPositionsFormat = QStringLiteral("grid-v1");
const QString pixelPositionsFormat = QStringLiteral("screen-pixels-v1");
QModelIndex findUid(QAbstractItemModel* model, const QString& uid, QModelIndex parent = {})
{
	if (model->canFetchMore(parent)) model->fetchMore(parent);
	for (int row = 0; row < model->rowCount(parent); ++row) {
		const auto index = model->index(row,0,parent);
		if (index.data(NetworkObjectModel::UidRole).toUuid().toString() == uid) return index;
		if (const auto child = findUid(model, uid, index); child.isValid()) return child;
	}
	return {};
}
bool active(const ComputerControlInterface::Pointer& computer, const Feature& feature)
{
	if (computer->activeFeatures().contains(feature.uid())) return true;
	const auto meta = VeyonCore::featureManager().metaFeatureUid(feature.uid());
	if (!meta.isNull() && computer->activeFeatures().contains(meta)) return true;
	for (const auto& child : VeyonCore::featureManager().relatedFeatures(feature.uid()))
		if (child.parentUid() == feature.uid() && computer->activeFeatures().contains(child.uid())) return true;
	return false;
}
QVariantMap featureMap(const Feature& feature)
{
	return {{QStringLiteral("uid"),feature.uid().toString()},{QStringLiteral("name"),feature.name()},{QStringLiteral("title"),feature.displayName()},{QStringLiteral("description"),feature.description()},{QStringLiteral("icon"),feature.iconUrl()},{QStringLiteral("checked"),feature.testFlag(Feature::Flag::Checked)},{QStringLiteral("parent"),feature.parentUid().toString()},{QStringLiteral("shortcut"),feature.shortcut().toString()}};
}
QString localPath(const QVariantMap& result)
{
	auto path = result.value(QStringLiteral("path")).toString();
	if (path.isEmpty()) path = result.value(QStringLiteral("value")).toString();
	const QUrl url(path);
	return url.isLocalFile() ? url.toLocalFile() : path;
}
}
MainWindow::MainWindow(VeyonMaster& master) : QObject(&master), m_master(master), m_model(new MasterComputerModel(*this,this)), m_engine(QuickUi::createEngine(this))
{
	connect(qGuiApp, &QGuiApplication::focusWindowChanged, this, &MainWindow::shortcutContextActiveChanged);
	connect(m_engine, &QQmlEngine::warnings, this, [this](const QList<QQmlError>& warnings) {
		for (const auto& warning : warnings) {
			const auto text = warning.toString();
			m_qmlWarnings.append(text);
			Q_EMIT qmlWarning(text);
		}
	});
	m_model->setSourceModel(master.computerMonitoringModel());
	auto tree = master.computerManager().computerTreeModel();
	connect(tree, &QAbstractItemModel::modelReset, this, &MainWindow::rebuildRooms);
	connect(tree, &QAbstractItemModel::rowsInserted, this, &MainWindow::rebuildRooms);
	connect(tree, &QAbstractItemModel::rowsRemoved, this, &MainWindow::rebuildRooms);
	connect(tree, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex&,const QModelIndex&,const QList<int>& roles) { if (roles.isEmpty() || roles.contains(Qt::CheckStateRole) || roles.contains(Qt::DisplayRole)) rebuildRooms(); });
	connect(master.computerMonitoringModel(), &QAbstractItemModel::dataChanged, this, [this](const QModelIndex& first,const QModelIndex& last,const QList<int>& roles) {
		Q_EMIT m_model->dataChanged(m_model->index(first.row(),0),m_model->index(last.row(),0));
		bool selected=false;
		for(int row=first.row();row<=last.row();++row)selected |= m_selected.contains(m_model->data(m_model->index(row,0),MasterComputerModel::ComputerUid).toString());
		if(selected || roles.isEmpty() || roles.contains(Qt::DisplayRole) || roles.contains(Qt::ToolTipRole))Q_EMIT changed();
		if (!slideshowTarget(m_slide)) nextSlide(1);
	});
	connect(master.computerMonitoringModel(), &QAbstractItemModel::modelReset, this, &MainWindow::refresh);
	connect(master.computerMonitoringModel(), &QAbstractItemModel::rowsInserted, this, &MainWindow::refresh);
	connect(master.computerMonitoringModel(), &QAbstractItemModel::rowsRemoved, this, &MainWindow::refresh);
	m_positionsLoaded = decodePositions(master.userConfig().computerPositions(), master.userConfig().computerPositionsFormat(), m_positions);
	m_positionsFormatKnown = m_positionsLoaded && !master.userConfig().computerPositionsFormat().isEmpty();
	master.computerMonitoringModel()->setStateFilter(master.userConfig().filterPoweredOnComputers() ? ComputerControlInterface::State::Connected : ComputerControlInterface::State::None);
	master.computerMonitoringModel()->setFilterNonEmptyUserLoginNames(master.userConfig().filterComputersWithLoggedOnUsers());
	connect(&m_slideshowTimer,&QTimer::timeout,this,[this]() { nextSlide(1); });
	m_slideshowTimer.setInterval(std::max(1000, master.userConfig().slideshowDuration()));
	connect(&VeyonCore::filesystem(), &Filesystem::screenshotDirectoryModified, this, &MainWindow::screenshotsChanged);
	auto watcher = new QFileSystemWatcher(this);
	watcher->addPath(VeyonCore::filesystem().screenshotDirectoryPath());
	connect(watcher,&QFileSystemWatcher::directoryChanged,this,&MainWindow::screenshotsChanged);
	rebuildRooms();
	m_engine->rootContext()->setContextProperty(QStringLiteral("masterUi"),this);
	m_engine->load(QUrl(QStringLiteral("qrc:/master/qml/Main.qml")));
	if (!m_engine->rootObjects().isEmpty()) m_window = qobject_cast<QWindow*>(m_engine->rootObjects().first());
	Q_EMIT shortcutContextActiveChanged();
	if (m_window) {
		if(master.userConfig().windowState()==QStringLiteral("maximized"))m_window->setWindowState(Qt::WindowMaximized);
		else if(master.userConfig().windowState()==QStringLiteral("fullscreen"))m_window->setWindowState(Qt::WindowFullScreen);
		const auto geometry = QJsonDocument::fromJson(master.userConfig().windowGeometry().toUtf8()).object();
		if (!geometry.isEmpty()) m_window->setGeometry(geometry.value(QStringLiteral("x")).toInt(),geometry.value(QStringLiteral("y")).toInt(),std::max(900,geometry.value(QStringLiteral("width")).toInt()),std::max(600,geometry.value(QStringLiteral("height")).toInt()));
	}
}
MainWindow::~MainWindow() { savePositions(); delete m_engine; }
QWindow* MainWindow::window() const { return m_window.data(); }
bool MainWindow::shortcutContextActive() const
{
	// QWindow::isActive() also includes focused transient children. Their keys
	// must not select computers or invoke actions in the underlying Master.
	return m_window && QGuiApplication::focusWindow() == m_window.data();
}
void MainWindow::show() { if (m_window) m_window->show(); }
bool MainWindow::initAuthentication()
{
	if (VeyonCore::instance()->initAuthentication()) return true;
	if (VeyonCore::config().authenticationMethod() == VeyonCore::AuthenticationMethod::KeyFileAuthentication)
		QuickUi::execDialog(QStringLiteral("information"),{{QStringLiteral("title"),tr("Authentication impossible")},{QStringLiteral("text"),tr("No valid authentication key files were found. Configure authentication using Veyon Configurator.")}});
	return false;
}
bool MainWindow::initAccessControl()
{
	if (!VeyonCore::config().accessControlForMasterEnabled()) return true;
	const auto user = VeyonCore::authenticationCredentials().hasCredentials(AuthenticationCredentials::Type::UserLogon) ? VeyonCore::authenticationCredentials().logonUsername() : VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::LoginName);
	if (VeyonCore::builtinFeatures().accessControlProvider().checkAccess(user,QHostAddress(QHostAddress::LocalHost).toString(),{}).access != AccessControlProvider::Access::Deny) return true;
	QuickUi::execDialog(QStringLiteral("information"),{{QStringLiteral("title"),tr("Access denied")},{QStringLiteral("text"),tr("Your account is not allowed to access computers. Contact your system administrator.")}});
			return false;
}
void MainWindow::reloadSubFeatures() { Q_EMIT featuresChanged(); }
QAbstractItemModel* MainWindow::computers() const { return m_model; }
QString MainWindow::uid(const ComputerControlInterface::Pointer& computer) const
{
	const auto& model = m_master.computerControlListModel();
	const auto row = model.computerControlInterfaces().indexOf(computer);
	return row >= 0 ? model.data(model.index(row,0),ComputerControlListModel::UidRole).toUuid().toString() : computer->computer().networkObjectUid().toString();
}
ComputerControlInterfaceList MainWindow::selectedComputerControlInterfaces() const
{
	if (m_dispatching) return m_dispatchSelection;
	ComputerControlInterfaceList result;
	for (int row=0;row<m_model->rowCount();++row) {
		const auto index=m_model->index(row,0);
		if(m_selected.contains(m_model->data(index,MasterComputerModel::ComputerUid).toString())) {
			const auto computer=m_model->data(index,ComputerControlListModel::ControlInterfaceRole).value<ComputerControlInterface::Pointer>();
			if(computer)result.append(computer);
		}
	}
	return result;
}
ComputerControlInterfaceList MainWindow::actionTargets(bool selectedOnly) const { return selectedOnly || m_selectedScope ? selectedComputerControlInterfaces() : m_master.allComputerControlInterfaces(); }
int MainWindow::totalCount() const { return m_master.allComputerControlInterfaces().size(); }
int MainWindow::onlineCount() const { return std::count_if(m_master.allComputerControlInterfaces().cbegin(),m_master.allComputerControlInterfaces().cend(),[](const auto& c) { return c->state() == ComputerControlInterface::State::Connected; }); }
void MainWindow::refresh()
{
	QSet<QString> available;
	for(int row=0;row<m_model->rowCount();++row)available.insert(m_model->data(m_model->index(row,0),MasterComputerModel::ComputerUid).toString());
	m_selected.intersect(available);
	// Keep an explicit Whole class choice while its selection still exists.
	if (m_selected.isEmpty()) m_selectedScope = false;
	if (!slideshowTarget(m_slide)) nextSlide(1);
	m_model->refreshSelection();
	Q_EMIT changed(); Q_EMIT slideshowChanged();
}
void MainWindow::rebuildRooms()
{
	if(m_rebuildingRooms)return;
	QScopedValueRollback<bool> rebuilding(m_rebuildingRooms,true);
	QVariantList values;
	auto tree = m_master.computerManager().computerTreeModel();
	std::function<int(QModelIndex)> count = [&](QModelIndex parent) {
		if (tree->canFetchMore(parent)) tree->fetchMore(parent);
		int total = 0;
		for (int row = 0; row < tree->rowCount(parent); ++row) {
			auto index = tree->index(row,0,parent);
			total += index.data(NetworkObjectModel::TypeRole).value<NetworkObject::Type>() == NetworkObject::Type::Host ? 1 : count(index);
		}
		return total;
	};
	const auto matches = [&](const QModelIndex& index) {
		if (m_directorySearch.pattern().isEmpty()) return true;
		for (int column = 0; column < tree->columnCount(index.parent()); ++column)
			if (m_directorySearch.match(index.siblingAtColumn(column).data().toString()).hasMatch()) return true;
		return false;
	};
	QHash<QPersistentModelIndex, bool> matchingSubtrees;
	std::function<bool(QModelIndex)> subtreeMatches = [&](QModelIndex index) {
		const QPersistentModelIndex key(index);
		if (matchingSubtrees.contains(key)) return matchingSubtrees.value(key);
		bool found = matches(index);
		if (tree->canFetchMore(index)) tree->fetchMore(index);
		for (int row = 0; row < tree->rowCount(index); ++row)
			found = subtreeMatches(tree->index(row, 0, index)) || found;
		matchingSubtrees.insert(key, found);
		return found;
	};
	std::function<void(QModelIndex,int,bool)> append = [&](QModelIndex parent,int depth,bool ancestorMatches) {
		if (tree->canFetchMore(parent)) tree->fetchMore(parent);
		for (int row = 0; row < tree->rowCount(parent); ++row) {
			const auto index = tree->index(row,0,parent);
			if (!NetworkObject::isContainer(index.data(NetworkObjectModel::TypeRole).value<NetworkObject::Type>())) continue;
			const bool showAll = ancestorMatches || matches(index);
			if (!showAll && !subtreeMatches(index)) continue;
			const auto id = index.data(NetworkObjectModel::UidRole).toUuid().toString();
			const auto checked = index.data(Qt::CheckStateRole).toInt();
			const auto computers=count(index);
			QVariantList hosts;
			for (int hostRow = 0; hostRow < tree->rowCount(index); ++hostRow) {
				const auto host = tree->index(hostRow, 0, index);
				if (host.data(NetworkObjectModel::TypeRole).value<NetworkObject::Type>() == NetworkObject::Type::Host && (showAll || matches(host)))
					hosts.append(QVariantMap{{QStringLiteral("uid"), host.data(NetworkObjectModel::UidRole).toUuid().toString()}, {QStringLiteral("name"), host.data().toString()}, {QStringLiteral("checked"), host.data(Qt::CheckStateRole).toInt() != Qt::Unchecked}});
			}
			values.append(QVariantMap{{QStringLiteral("uid"),id},{QStringLiteral("name"),index.data().toString()},{QStringLiteral("depth"),depth},{QStringLiteral("count"),computers},{QStringLiteral("checked"),checked != Qt::Unchecked},{QStringLiteral("checkState"),checked},{QStringLiteral("hosts"),hosts},{QStringLiteral("searchMatch"),!m_directorySearch.pattern().isEmpty()},{QStringLiteral("current"),id == m_currentRoomUid}});
			append(index,depth+1,showAll);
	}
	};
	append({},0,false);
	m_rooms = values;
	if (m_currentRoom.isEmpty()) {
		QStringList checked;
		for (const auto& value : values) if (value.toMap().value(QStringLiteral("checked")).toBool()) checked.append(value.toMap().value(QStringLiteral("name")).toString());
		m_currentRoom = checked.isEmpty() ? tr("Classroom") : checked.join(QStringLiteral(", "));
	}
	Q_EMIT roomsChanged();
}
void MainWindow::activateRoom(const QString& id)
{
	if (m_dispatching) return;
	auto tree = m_master.computerManager().computerTreeModel();
	auto target = QPersistentModelIndex(findUid(tree,id));
	if (!target.isValid()) return;
	m_currentRoomUid = id; m_currentRoom = target.data().toString();
	for (int row=0;row<tree->rowCount();++row) tree->setData(tree->index(row,0),Qt::Unchecked,Qt::CheckStateRole);
	tree->setData(target,Qt::Checked,Qt::CheckStateRole);
	m_selected.clear(); rebuildRooms(); refresh();
}
void MainWindow::setRoomChecked(const QString& id,bool checked)
{
	if (m_dispatching) return;
	auto tree = m_master.computerManager().computerTreeModel();
	auto target = findUid(tree,id);
	if (target.isValid()) tree->setData(target,checked ? Qt::Checked : Qt::Unchecked,Qt::CheckStateRole);
	m_currentRoom.clear(); m_currentRoomUid.clear(); rebuildRooms(); refresh();
}
bool MainWindow::canAddRooms() const{return VeyonCore::config().showCurrentLocationOnly()&&VeyonCore::config().allowAddingHiddenLocations();}
bool MainWindow::expandRooms() const{return VeyonCore::config().expandLocations();}
void MainWindow::setComputerChecked(const QString& id,bool checked)
{
	if(m_dispatching)return;
	auto tree=m_master.computerManager().computerTreeModel();const auto target=findUid(tree,id);
	if(target.isValid())tree->setData(target,checked?Qt::Checked:Qt::Unchecked,Qt::CheckStateRole);
	refresh();
}
void MainWindow::addRoom()
{
	if(!canAddRooms())return;
	QStringList choices;auto model=m_master.computerManager().networkObjectModel();
	std::function<void(QModelIndex)> visit=[&](QModelIndex parent){if(model->canFetchMore(parent))model->fetchMore(parent);for(int row=0;row<model->rowCount(parent);++row){const auto index=model->index(row,0,parent);if(index.data(NetworkObjectModel::TypeRole).value<NetworkObject::Type>()==NetworkObject::Type::Location)choices.append(index.data().toString());visit(index);}};
	visit({});choices.removeDuplicates();choices.sort();
	const auto result=QuickUi::execDialog(QStringLiteral("input"),{{QStringLiteral("title"),tr("Add room")},{QStringLiteral("choices"),choices}},window());
	if(result.value(QStringLiteral("accepted")).toBool())m_master.computerManager().addLocation(result.value(QStringLiteral("value")).toString());
	rebuildRooms();
}
void MainWindow::removeRoom(){if(canAddRooms()&&!m_currentRoomUid.isEmpty()){m_master.computerManager().removeLocation(m_currentRoom);m_currentRoom.clear();m_currentRoomUid.clear();rebuildRooms();}}
void MainWindow::reloadComputers(){if(m_dispatching)return;if(auto directory=VeyonCore::networkObjectDirectoryManager().configuredDirectory())directory->update();m_master.computerControlListModel().reload();rebuildRooms();}
void MainWindow::select(const QString& id,bool additive,bool range)
{
	if (m_dispatching) return;
	if (range && !m_anchor.isEmpty()) {
		int start=-1,end=-1;
		for(int row=0;row<m_model->rowCount();++row) {
			const auto current=m_model->data(m_model->index(row,0),MasterComputerModel::ComputerUid).toString();
			if(current==m_anchor) start=row;
			if(current==id) end=row;
		}
		if(start>=0 && end>=0) { if(!additive)m_selected.clear(); for(int row=std::min(start,end);row<=std::max(start,end);++row)m_selected.insert(m_model->data(m_model->index(row,0),MasterComputerModel::ComputerUid).toString()); }
	} else {
		if(!additive)m_selected.clear();
		if(additive && m_selected.contains(id))m_selected.remove(id); else m_selected.insert(id);
		m_anchor=id;
	}
	m_selectedScope = !m_selected.isEmpty();
	m_model->refreshSelection(); Q_EMIT changed();
}
QString MainWindow::computerIdAt(int row) const{return m_model->data(m_model->index(row,0),MasterComputerModel::ComputerUid).toString();}
void MainWindow::selectAll(bool selected)
{
	if(m_dispatching)return;
	m_selected.clear();
	if(selected)for(int row=0;row<m_model->rowCount();++row)m_selected.insert(m_model->data(m_model->index(row,0),MasterComputerModel::ComputerUid).toString());
	m_selectedScope = !m_selected.isEmpty();
	m_model->refreshSelection(); Q_EMIT changed();
}
void MainWindow::setSelectedScope(bool selected) { m_selectedScope=selected; Q_EMIT changed(); }
void MainWindow::setSearch(const QString& search)
{
	if (m_dispatching) return;
	m_directorySearch = QRegularExpression(search, QRegularExpression::CaseInsensitiveOption);
	m_master.computerMonitoringModel()->setFilterRegularExpression(search);
	rebuildRooms();
	refresh();
}
int MainWindow::filterIndex() const
{
	return (m_master.userConfig().filterPoweredOnComputers()?1:0)+(m_master.userConfig().filterComputersWithLoggedOnUsers()?2:0);
}
void MainWindow::setFilter(int filter)
{
	const bool online=filter==1 || filter==3;
	const bool users=filter==2 || filter==3;
	m_master.computerMonitoringModel()->setStateFilter(online?ComputerControlInterface::State::Connected:ComputerControlInterface::State::None);
	m_master.computerMonitoringModel()->setFilterNonEmptyUserLoginNames(users);
	m_master.userConfig().setFilterPoweredOnComputers(online);m_master.userConfig().setFilterComputersWithLoggedOnUsers(users);refresh();
}
QVariantMap MainWindow::details() const
{
	const auto selected=selectedComputerControlInterfaces();
	if(selected.isEmpty())return {{QStringLiteral("thumbnail"),QImage{}}};
	const auto c=selected.first();
	const auto uptime = c->sessionInfo().uptime;
	QString sessionDuration;
	if (uptime > 0) {
		sessionDuration = QTime::fromMSecsSinceStartOfDay((uptime % 86400) * 1000).toString(QStringLiteral("hh:mm:ss"));
		if (uptime >= 86400) sessionDuration.prepend(tr("%n day(s), ", nullptr, uptime / 86400));
	}
	return {{QStringLiteral("name"),c->computerName()},{QStringLiteral("user"),c->userFullName().isEmpty()?c->userLoginName():c->userFullName()},{QStringLiteral("address"),c->computer().hostName()},{QStringLiteral("location"),c->computer().location()},{QStringLiteral("thumbnail"),QVariant::fromValue(m_master.computerControlListModel().data(m_master.computerControlListModel().index(m_master.allComputerControlInterfaces().indexOf(c),0),Qt::DecorationRole).value<QImage>())},{QStringLiteral("computer"),QVariant::fromValue<QObject*>(c.data())},{QStringLiteral("online"),c->state()==ComputerControlInterface::State::Connected},{QStringLiteral("error"),c->state()==ComputerControlInterface::State::AccessControlFailed?c->accessControlDetails():QString{}},{QStringLiteral("sessionDuration"),sessionDuration}};
}
QVariantList MainWindow::features() const
{
	QVariantList list;
	for(const auto& feature:m_master.features()) {
		auto value=featureMap(feature);QVariantList children;
		for(const auto& child:m_master.subFeatures(feature.uid()))children.append(featureMap(child));
		value.insert(QStringLiteral("children"),children);list.append(value);
	}
	return list;
}
QVariantMap MainWindow::feature(const QString& name) const
{
	for(const auto& feature:m_master.allFeatures())if(feature.name()==name)return featureMap(feature);
	return {};
}
int MainWindow::captureTargets(bool selectedOnly)
{
	const auto token=++m_nextInvocation;
	m_invocations.insert(token,{actionTargets(selectedOnly),selectedComputerControlInterfaces()});
	return token;
}
void MainWindow::releaseTargets(int token){m_invocations.remove(token);}
void MainWindow::trigger(const QString& id,bool selectedOnly)
{
	const auto token=captureTargets(selectedOnly);
	triggerSnapshot(id,token);
	releaseTargets(token);
}
void MainWindow::triggerSnapshot(const QString& id,int token)
{
	if(m_dispatching || !m_invocations.contains(token))return;
	Feature chosen;
	for(const auto& feature:m_master.allFeatures())if(feature.uid().toString()==id || feature.name()==id) {chosen=feature;break;}
	if(!chosen.isValid())return;
	const auto invocation=m_invocations.value(token);
	runFeatureOnComputers(chosen,invocation.targets,invocation.sources);
}
void MainWindow::runFeatureOnComputers(const Feature& chosen,const ComputerControlInterfaceList& targets,const ComputerControlInterfaceList& sources,bool toggle)
{
	if(m_dispatching || !chosen.isValid())return;
	if(chosen.name().startsWith(QStringLiteral("ShareUserScreen")) && sources.size()!=1) {
		QuickUi::execDialog(QStringLiteral("information"),{{QStringLiteral("title"),tr("Demonstration")},{QStringLiteral("text"),tr("Select exactly one student screen to share.")}},window());return;
	}
	const bool remote=chosen.name()==QStringLiteral("RemoteView") || chosen.name()==QStringLiteral("RemoteControl");
	if(targets.isEmpty() && !remote && !chosen.testFlag(Feature::Flag::Option))return;
	m_dispatchSelection=sources;
	QScopedValueRollback<bool> dispatch(m_dispatching,true);
	const bool alreadyActive=toggle && chosen.testFlag(Feature::Flag::Mode) && !targets.isEmpty() && std::all_of(targets.cbegin(),targets.cend(),[&](const auto& c){return active(c,chosen);});
	if(chosen.testFlag(Feature::Flag::Mode)) {
		for(const auto& feature:m_master.features())if(feature.testFlag(Feature::Flag::Mode) && (alreadyActive || feature!=chosen)) {
			VeyonCore::featureManager().stopFeature(m_master,feature,targets);
			for(const auto& child:m_master.subFeatures(feature.uid()))VeyonCore::featureManager().stopFeature(m_master,child,targets);
		}
	}
	if(!alreadyActive)VeyonCore::featureManager().startFeature(m_master,chosen,targets);
	m_dispatchSelection.clear();
	m_dispatching=false;
	Q_EMIT changed();Q_EMIT featuresChanged();
}
void MainWindow::removePreset(const QString& id)
{
	if(!QuickUi::execDialog(QStringLiteral("confirm"),{{QStringLiteral("title"),tr("Remove saved action")},{QStringLiteral("text"),tr("Remove this saved application or website?")}},window()).value(QStringLiteral("accepted")).toBool())return;
	for(auto plugin:VeyonCore::pluginManager().pluginObjects())
		if(plugin->metaObject()->indexOfMethod("removePresetFeature(QString)")>=0)
			QMetaObject::invokeMethod(plugin,"removePresetFeature",Qt::DirectConnection,Q_ARG(QString,id));
	reloadSubFeatures();
}
void MainWindow::doubleClick(const QString& id) { select(id); trigger(VeyonCore::config().computerDoubleClickFeature().toString(),true); }
void MainWindow::stopFeatures() { m_master.stopAllFeatures(m_master.allComputerControlInterfaces());Q_EMIT changed(); }
QString MainWindow::activeSummary() const
{
	QStringList names;
	for(const auto& feature:m_master.features())if(feature.testFlag(Feature::Flag::Mode) && feature.name()!=QStringLiteral("Monitoring"))
		if(std::any_of(m_master.allComputerControlInterfaces().cbegin(),m_master.allComputerControlInterfaces().cend(),[&](const auto& c){return active(c,feature);}))names.append(feature.displayName());
	return names.join(QStringLiteral(" · "));
}
bool MainWindow::customPositions() const{return m_master.userConfig().useCustomComputerPositions();}
void MainWindow::setCustomPositions(bool value){m_master.userConfig().setUseCustomComputerPositions(value);Q_EMIT changed();}
int MainWindow::cardWidth() const{return m_master.userConfig().monitoringScreenSize();}
void MainWindow::setCardWidth(int width)
{
	width = qBound(150, width, maximumCardWidth);
	if (cardWidth() == width) return;
	m_master.userConfig().setMonitoringScreenSize(width);
	m_master.computerControlListModel().updateComputerScreenSize();
	// Custom card positions are model roles and must rescale with the cards.
	m_model->refreshSelection();
	Q_EMIT changed();
}
bool MainWindow::autoFit() const{return VeyonCore::config().autoAdjustMonitoringIconSize() || m_master.userConfig().autoAdjustMonitoringIconSize();}
void MainWindow::setAutoFit(bool enabled){m_master.userConfig().setAutoAdjustMonitoringIconSize(enabled);Q_EMIT changed();}
void MainWindow::fitCards(int width,int height)
{
	if(!autoFit() || customPositions() || m_model->rowCount()==0 || width<1 || height<1)return;
	int best=150;
	for(int columns=1;columns<=m_model->rowCount();++columns){const auto size=qMin(maximumCardWidth,width/columns-16);const auto rows=(m_model->rowCount()+columns-1)/columns;if(size>=150 && rows*(size*9/16+72)<=height)best=std::max(best,size);}
	setCardWidth(best);
}
QSizeF MainWindow::positionGridSize() const
{
	return computerPositionGridSize(cardWidth());
}
QPoint MainWindow::position(const QString& id, int row) const
{
	const auto grid = positionGridSize();
	const auto point = m_positions.value(id, QPointF(row % 4, row / 4));
	return {qMax(0, qRound(point.x() * grid.width())), qMax(0, qRound(point.y() * grid.height()))};
}
void MainWindow::setPosition(const QString& id, int x, int y)
{
	const auto grid = positionGridSize();
	// Preserve the fractional grid-unit format used by QWidget seat layouts.
	m_positions.insert(id, {qMax(0, x) / grid.width(), qMax(0, y) / grid.height()});
	m_positionsLoaded = true;
	m_positionsFormatKnown = true;
	savePositions();
	m_model->refreshSelection();
}
void MainWindow::savePositions()
{
	// Do not overwrite unsupported or malformed startup data on ordinary shutdown.
	if (!m_positionsLoaded) return;
	QJsonArray values;
	for (auto it = m_positions.cbegin(); it != m_positions.cend(); ++it)
		values.append(QJsonObject{{QStringLiteral("uid"), it.key()}, {QStringLiteral("x"), it.value().x()}, {QStringLiteral("y"), it.value().y()}});
	m_master.userConfig().setComputerPositions(values);
	if (m_positionsFormatKnown) m_master.userConfig().setComputerPositionsFormat(gridPositionsFormat);
}
void MainWindow::alignPositions()
{
	m_positionsLoaded = true;
	m_positionsFormatKnown = true;
	for (int row = 0; row < m_model->rowCount(); ++row) {
		const auto id = m_model->data(m_model->index(row, 0), MasterComputerModel::ComputerUid).toString();
		const auto point = m_positions.value(id, QPointF(row % 4, row / 4));
		m_positions.insert(id, {qreal(qMax(0, qRound(point.x()))), qreal(qMax(0, qRound(point.y())))});
	}
	savePositions();
	m_model->refreshSelection();
}
bool MainWindow::decodePositions(const QJsonArray& values, const QString& format, QHash<QString,QPointF>& positions) const
{
	if (!format.isEmpty() && format != gridPositionsFormat && format != pixelPositionsFormat) return false;
	QHash<QString,QPointF> decoded;
	const auto grid = positionGridSize();
	const auto maximumGrid = computerPositionGridSize(maximumCardWidth);
	for (const auto& value : values) {
		if (!value.isObject()) return false;
		const auto object = value.toObject();
		const QUuid id(object.value(QStringLiteral("uid")).toString());
		const auto x = object.value(QStringLiteral("x")), y = object.value(QStringLiteral("y"));
		if (id.isNull() || decoded.contains(id.toString()) || !x.isDouble() || !y.isDouble()) return false;
		QPointF point(x.toDouble(), y.toDouble());
		if (format == pixelPositionsFormat) point = {point.x() / grid.width(), point.y() / grid.height()};
		if (!std::isfinite(point.x()) || !std::isfinite(point.y()) ||
			std::abs(point.x()) > std::numeric_limits<int>::max() / maximumGrid.width() ||
			std::abs(point.y()) > std::numeric_limits<int>::max() / maximumGrid.height()) return false;
		decoded.insert(id.toString(), point);
	}
	positions = std::move(decoded);
	return true;
}
void MainWindow::positionsFile(bool save)
{
	const auto result = QuickUi::execDialog(QStringLiteral("file"), {
		{QStringLiteral("title"), save ? tr("Save computer positions") : tr("Load computer positions")},
		{QStringLiteral("save"), save}, {QStringLiteral("filter"), tr("JSON files (*.json)")}}, window());
	if (!result.value(QStringLiteral("accepted")).toBool()) return;
	const auto showError = [this](const QString& text) {
		QuickUi::showError(tr("Computer positions"), text, window());
	};
	if (save) {
		if (!m_positionsLoaded) {showError(tr("The saved computer positions could not be read. Load a valid layout before saving."));return;}
		savePositions();
		// Keep the existing array nesting so older Veyon versions can read grid layouts.
		const QJsonObject ui{{QStringLiteral("ComputerPositionsFormat"), gridPositionsFormat},
			{QStringLiteral("ComputerPositions"), QJsonObject{{QStringLiteral("JsonStoreArray"), m_master.userConfig().computerPositions()}}}};
		const auto contents = QJsonDocument(QJsonObject{{QStringLiteral("UI"), ui}}).toJson();
		QSaveFile file(localPath(result));
		if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size() || !file.commit()) showError(file.errorString());
		return;
	}
	QFile file(localPath(result));
	if (!file.open(QIODevice::ReadOnly)) {showError(file.errorString());return;}
	QJsonParseError parseError;
	const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
	const auto ui = document.object().value(QStringLiteral("UI")).toObject();
	const auto values = ui.value(QStringLiteral("ComputerPositions")).toObject().value(QStringLiteral("JsonStoreArray"));
	const auto marker = ui.value(QStringLiteral("ComputerPositionsFormat"));
	auto format = marker.toString();
	QHash<QString,QPointF> decoded;
	if (parseError.error != QJsonParseError::NoError || !document.isObject() || !values.isArray() ||
		(!marker.isUndefined() && !marker.isString()) || !decodePositions(values.toArray(), format, decoded)) {
		showError(tr("The file does not contain valid computer positions in a supported format."));return;
	}
	if (format.isEmpty()) {
		const auto relative = tr("Relative to card size"), pixels = tr("Screen pixels");
		const auto choice = QuickUi::execDialog(QStringLiteral("input"), {
			{QStringLiteral("title"), tr("Position coordinates")},
			{QStringLiteral("text"), tr("Choose how to interpret the saved computer positions.")},
			{QStringLiteral("choices"), QStringList{relative, pixels}}}, window());
		if (!choice.value(QStringLiteral("accepted")).toBool()) return;
		format = choice.value(QStringLiteral("value")).toString() == pixels ? pixelPositionsFormat : gridPositionsFormat;
		if (!decodePositions(values.toArray(), format, decoded)) {showError(tr("The file contains invalid computer positions."));return;}
	}
	m_positions = std::move(decoded);
	m_positionsLoaded = true;
	m_positionsFormatKnown = true;
	savePositions();
	setCustomPositions(true);
	m_model->refreshSelection();
}
void MainWindow::exportComputers(){auto result=QuickUi::execDialog(QStringLiteral("file"),{{QStringLiteral("title"),tr("Export computers and users")},{QStringLiteral("save"),true}},window());if(result.value(QStringLiteral("accepted")).toBool()&&!m_master.computerManager().saveComputerAndUsersList(localPath(result)))QuickUi::execDialog(QStringLiteral("information"),{{QStringLiteral("title"),tr("File error")},{QStringLiteral("text"),tr("Could not save the computer list.")}},window());}
QVariantList MainWindow::screenshots() const
{
	QVariantList list;QDir directory(VeyonCore::filesystem().screenshotDirectoryPath());
	for(const auto& name:directory.entryList({QStringLiteral("*.png")},QDir::Files,QDir::Time)) {Screenshot screenshot(directory.filePath(name));list.append(QVariantMap{{QStringLiteral("name"),name},{QStringLiteral("url"),QUrl::fromLocalFile(directory.filePath(name))},{QStringLiteral("user"),screenshot.user()},{QStringLiteral("host"),screenshot.host()},{QStringLiteral("date"),QStringLiteral("%1 %2").arg(screenshot.date(), screenshot.time())}});}
	return list;
}
void MainWindow::deleteScreenshots(const QStringList& names)
{
	if(names.isEmpty())return;
	if(!QuickUi::execDialog(QStringLiteral("confirm"),{{QStringLiteral("title"),tr("Delete screenshots")},{QStringLiteral("text"),tr("Delete %1 selected screenshots?").arg(names.size())}},window()).value(QStringLiteral("accepted")).toBool())return;
	QDir directory(VeyonCore::filesystem().screenshotDirectoryPath());for(const auto& name:names)if(QFileInfo(name).fileName()==name)QFile::remove(directory.filePath(name));Q_EMIT screenshotsChanged();
}
QVariantList MainWindow::spotlight() const{QVariantList values;for(const auto& c:m_spotlight)values.append(QVariantMap{{QStringLiteral("name"),c->computerName()},{QStringLiteral("thumbnail"),QVariant::fromValue(m_master.computerControlListModel().data(m_master.computerControlListModel().index(m_master.allComputerControlInterfaces().indexOf(c),0),Qt::DecorationRole).value<QImage>())},{QStringLiteral("computer"),QVariant::fromValue<QObject*>(c.data())}});return values;}
void MainWindow::addSpotlight(){for(const auto& c:selectedComputerControlInterfaces())if(!m_spotlight.contains(c))m_spotlight.append(c);Q_EMIT spotlightChanged();}
void MainWindow::removeSpotlight(int index){if(index>=0&&index<m_spotlight.size())m_spotlight.removeAt(index);Q_EMIT spotlightChanged();}
bool MainWindow::spotlightRealtime() const{return m_master.userConfig().spotlightRealtime();}
void MainWindow::setSpotlightRealtime(bool value){m_master.userConfig().setSpotlightRealtime(value);Q_EMIT spotlightChanged();}
ComputerControlInterface::Pointer MainWindow::slideshowTarget(int row) const
{
	if (row < 0 || row >= m_model->rowCount()) return {};
	const auto computer = m_model->data(m_model->index(row, 0), ComputerControlListModel::ControlInterfaceRole).value<ComputerControlInterface::Pointer>();
	return computer && computer->state() == ComputerControlInterface::State::Connected && computer->hasValidFramebuffer() ? computer : ComputerControlInterface::Pointer{};
}
QObject* MainWindow::slideshowComputer() const { return slideshowTarget(m_slide).data(); }
void MainWindow::nextSlide(int delta)
{
	const auto count = m_model->rowCount();
	const auto step = delta < 0 ? -1 : 1;
	auto row = m_slide >= 0 && m_slide < count ? m_slide : (step > 0 ? -1 : 0);
	m_slide = -1;
	for (int visited = 0; visited < count; ++visited) {
		row = (row + step + count) % count;
		if (slideshowTarget(row)) { m_slide = row; break; }
	}
	Q_EMIT slideshowChanged();
}
void MainWindow::setSlideshowRunning(bool running)
{
	if (running) {
		if (!slideshowTarget(m_slide)) nextSlide(1);
		m_slideshowTimer.start();
	} else m_slideshowTimer.stop();
	Q_EMIT slideshowChanged();
}
int MainWindow::slideshowDuration() const{return m_slideshowTimer.interval();}
void MainWindow::setSlideshowDuration(int duration){m_slideshowTimer.setInterval(qBound(1000,duration,60000));m_master.userConfig().setSlideshowDuration(m_slideshowTimer.interval());Q_EMIT slideshowChanged();}
bool MainWindow::prepareClose()
{
	if(!activeSummary().isEmpty() && !QuickUi::execDialog(QStringLiteral("confirm"),{{QStringLiteral("title"),tr("Features active")},{QStringLiteral("text"),tr("Stop active features and close Veyon?")}},window()).value(QStringLiteral("accepted")).toBool())return false;
	m_master.stopAllFeatures(m_master.allComputerControlInterfaces());savePositions();
	if(m_window){m_master.userConfig().setWindowState(m_window->windowState()==Qt::WindowFullScreen?QStringLiteral("fullscreen"):m_window->windowState()==Qt::WindowMaximized?QStringLiteral("maximized"):QStringLiteral("normal"));const auto r=m_window->geometry();m_master.userConfig().setWindowGeometry(QString::fromUtf8(QJsonDocument(QJsonObject{{QStringLiteral("x"),r.x()},{QStringLiteral("y"),r.y()},{QStringLiteral("width"),r.width()},{QStringLiteral("height"),r.height()}}).toJson(QJsonDocument::Compact)));}
	return true;
}
void MainWindow::about(){AboutDialog(nullptr).exec();}
void MainWindow::documentation(){QDesktopServices::openUrl(QUrl(QStringLiteral("https://docs.veyon.io/")));}
