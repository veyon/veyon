/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "MasterWorkspace.h"
#include <algorithm>
#include <QAbstractButton>
#include <QAction>
#include <QButtonGroup>
#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QPainter>
#include <QSlider>
#include <QSplitter>
#include <QStyle>
#include <QStylePainter>
#include <QStyleOptionToolButton>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include "ClassroomSidebar.h"
#include "ComputerControlListModel.h"
#include "ComputerMonitoringModel.h"
#include "ComputerMonitoringWidget.h"
#include "DesktopIcons.h"
#include "FeatureManager.h"
#include "UserConfig.h"
#include "VeyonConfiguration.h"
#include "VeyonMaster.h"

namespace
{
class ClassroomMenuButton final : public QToolButton
{
public:
	ClassroomMenuButton(QWidget* parent, bool chevron) : QToolButton(parent)
	{
		setProperty("dropdownIndicator", chevron);
		setFixedHeight(28);
		setIconSize(QSize(16, 16));
	}

protected:
	void paintEvent(QPaintEvent*) override
	{
		QStylePainter painter(this);
		QStyleOptionToolButton option;
		initStyleOption(&option);
		option.features.setFlag(QStyleOptionToolButton::HasMenu, false);
		option.features.setFlag(QStyleOptionToolButton::MenuButtonPopup, false);
		option.subControls = QStyle::SC_ToolButton;
		painter.drawComplexControl(QStyle::CC_ToolButton, option);
		if (property("dropdownIndicator").toBool())
		{
			DesktopIcons::icon(QStringLiteral("chevron-down")).paint(&painter,
				QRect(width() - 20, (height() - 12) / 2, 12, 12), Qt::AlignCenter,
				isEnabled() ? QIcon::Normal : QIcon::Disabled);
		}
	}
};

QString featureIcon(const QString& name)
{
	static const QHash<QString, QString> icons {
		{QStringLiteral("Demo"), QStringLiteral("cast")}, {QStringLiteral("ScreenLock"), QStringLiteral("lock")},
		{QStringLiteral("TextMessage"), QStringLiteral("message-square-text")}, {QStringLiteral("OpenWebsite"), QStringLiteral("globe")},
		{QStringLiteral("StartApp"), QStringLiteral("app-window")}, {QStringLiteral("DistributeFiles"), QStringLiteral("files")},
		{QStringLiteral("FileCollect"), QStringLiteral("download")}, {QStringLiteral("PowerOn"), QStringLiteral("power")},
		{QStringLiteral("PowerDown"), QStringLiteral("power-off")}, {QStringLiteral("Reboot"), QStringLiteral("rotate-ccw")},
		{QStringLiteral("RemoteView"), QStringLiteral("eye")}, {QStringLiteral("RemoteControl"), QStringLiteral("mouse-pointer-2")},
		{QStringLiteral("Screenshot"), QStringLiteral("camera")}, {QStringLiteral("Logon"), QStringLiteral("log-in")},
		{QStringLiteral("Logoff"), QStringLiteral("log-out")}, {QStringLiteral("MonitoringMode"), QStringLiteral("monitor")}
	};
	return icons.value(name, QStringLiteral("settings"));
}
Feature findFeature(const VeyonMaster& master, const QString& name)
{
	for (const auto& feature : master.allFeatures()) if (feature.name() == name) return feature;
	return Feature();
}
}

MasterWorkspace::MasterWorkspace(VeyonMaster& master, ComputerMonitoringWidget* monitoring,
	QSplitter* splitter, QLineEdit* search, QWidget* parent) : QWidget(parent), m_master(master), m_monitoring(monitoring)
{
	setObjectName(QStringLiteral("masterWorkspace"));
	auto root = new QHBoxLayout(this);
	root->setContentsMargins(0, 0, 0, 0);
	root->setSpacing(0);
	auto sidebar = new ClassroomSidebar(master, search, this);
	root->addWidget(sidebar);
	auto center = new QWidget(this);
	center->setObjectName(QStringLiteral("classroomCenter"));
	center->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
	auto content = new QVBoxLayout(center);
	content->setContentsMargins(0, 0, 0, 0);
	content->setSpacing(0);
	root->addWidget(center, 1);
	connect(sidebar, &ClassroomSidebar::roomChanged, this, [this](const QString& name) {
		m_monitoring->clearSelection();
		m_scopeAll->setChecked(true);
		window()->setWindowTitle(tr("Veyon Master — %1").arg(name));
		updateOverview();
	});

	auto toolbar = new QWidget(center);
	toolbar->setObjectName(QStringLiteral("classroomCommands"));
	auto commands = new QHBoxLayout(toolbar);
	commands->setContentsMargins(10, 7, 10, 7);
	commands->setSpacing(2);
	auto targetLabel = new QLabel(tr("TARGET"), toolbar);
	targetLabel->setObjectName(QStringLiteral("classroomTargetLabel"));
	auto targetFont = targetLabel->font();
	targetFont.setLetterSpacing(QFont::AbsoluteSpacing, 1);
	targetLabel->setFont(targetFont);
	commands->addWidget(targetLabel);
	commands->addSpacing(6);
	m_scopeAll = new QToolButton(toolbar);
	m_scopeAll->setObjectName(QStringLiteral("scopeAllButton"));
	m_scopeSelected = new QToolButton(toolbar);
	m_scopeSelected->setObjectName(QStringLiteral("scopeSelectedButton"));
	auto scopes = new QButtonGroup(toolbar);
	for (auto button : {m_scopeAll, m_scopeSelected})
	{
		button->setCheckable(true);
		button->setProperty("segment", true);
		button->setFixedHeight(28);
		scopes->addButton(button);
		commands->addWidget(button);
	}
	m_scopeAll->setChecked(true);
	m_scopeSelected->setEnabled(false);
	connect(scopes, &QButtonGroup::buttonToggled, this, [this](QAbstractButton*, bool checked) { if (checked) reloadFeatures(); });
	auto separator = new QFrame(toolbar);
	separator->setFrameShape(QFrame::VLine);
	separator->setFixedHeight(22);
	commands->addSpacing(6);
	commands->addWidget(separator);
	commands->addSpacing(6);
	m_demo = featureButton(toolbar, QStringLiteral("Demo"), QStringLiteral("cast"), tr("Demo"));
	auto demoMenu = new QMenu(m_demo);
	connect(demoMenu, &QMenu::aboutToShow, this, [this, demoMenu]() {
		demoMenu->clear();
		addFeature(demoMenu, findFeature(m_master, QStringLiteral("Demo")), false, true);
	});
	m_demo->setMenu(demoMenu);
	m_demo->setPopupMode(QToolButton::InstantPopup);
	commands->addWidget(m_demo);
	m_lock = featureButton(toolbar, QStringLiteral("ScreenLock"), QStringLiteral("lock"), tr("Lock"));
	commands->addWidget(m_lock);
	m_message = featureButton(toolbar, QStringLiteral("TextMessage"), QStringLiteral("message-square-text"), tr("Message"));
	commands->addWidget(m_message);
	for (const auto& name : {QStringLiteral("OpenWebsite"), QStringLiteral("StartApp"), QStringLiteral("DistributeFiles")})
		commands->addWidget(featureButton(toolbar, name, featureIcon(name)));
	commands->addStretch();
	auto power = new ClassroomMenuButton(toolbar, true);
	power->setObjectName(QStringLiteral("powerMenuButton"));
	power->setProperty("bordered", true);
	power->setText(tr("Power"));
	power->setIcon(DesktopIcons::icon(QStringLiteral("power")));
	power->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	power->setPopupMode(QToolButton::InstantPopup);
	auto powerMenu = new QMenu(power);
	connect(powerMenu, &QMenu::aboutToShow, this, [this, powerMenu]() {
		populateFeatures(powerMenu, {QStringLiteral("PowerOn"), QStringLiteral("Reboot"), QStringLiteral("PowerDown")});
	});
	power->setMenu(powerMenu);
	commands->addWidget(power);
	auto more = new ClassroomMenuButton(toolbar, false);
	more->setObjectName(QStringLiteral("moreActionsButton"));
	more->setFixedWidth(30);
	more->setIcon(DesktopIcons::icon(QStringLiteral("ellipsis")));
	more->setToolTip(tr("More actions and view options"));
	more->setPopupMode(QToolButton::InstantPopup);
	auto moreMenu = new QMenu(more);
	connect(moreMenu, &QMenu::aboutToShow, this, [this, moreMenu]() { populateMore(moreMenu); });
	more->setMenu(moreMenu);
	commands->addWidget(more);
	content->addWidget(toolbar);

	m_banner = new QWidget(center);
	m_banner->setObjectName(QStringLiteral("classroomModeBanner"));
	auto bannerLayout = new QHBoxLayout(m_banner);
	bannerLayout->setContentsMargins(12, 5, 12, 5);
	m_bannerDescription = new QLabel(m_banner);
	m_bannerDescription->setTextFormat(Qt::PlainText);
	m_bannerDescription->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	bannerLayout->addWidget(m_bannerDescription, 1);
	m_bannerAction = new QToolButton(m_banner);
	bannerLayout->addWidget(m_bannerAction);
	connect(m_bannerAction, &QToolButton::clicked, this, [this]() {
		const auto action = m_bannerAction->property("bannerAction").toString();
		const auto feature = findFeature(m_master, action == QStringLiteral("unlock") ? QStringLiteral("ScreenLock") : QStringLiteral("Demo"));
		const auto computers = m_master.allComputerControlInterfaces();
		VeyonCore::featureManager().stopFeature(m_master, feature, computers);
		for (const auto& subFeature : m_master.subFeatures(feature.uid()))
			VeyonCore::featureManager().stopFeature(m_master, subFeature, computers);
	});
	content->addWidget(m_banner);
	m_banner->hide();
	content->addWidget(splitter, 1);
	m_empty = new QLabel(monitoring);
	m_empty->setObjectName(QStringLiteral("emptyMonitoringLabel"));
	m_empty->setAlignment(Qt::AlignCenter);
	m_empty->setWordWrap(true);
	m_empty->setAttribute(Qt::WA_TransparentForMouseEvents);
	auto emptyLayout = new QVBoxLayout(monitoring);
	emptyLayout->addWidget(m_empty);

	auto footer = new QWidget(center);
	footer->setObjectName(QStringLiteral("classroomStatus"));
	footer->setFixedHeight(28);
	auto statusLayout = new QHBoxLayout(footer);
	statusLayout->setContentsMargins(12, 2, 12, 2);
	statusLayout->setSpacing(8);
	m_summary = new QLabel(footer);
	m_summary->setObjectName(QStringLiteral("workspaceSummary"));
	m_summary->setTextFormat(Qt::PlainText);
	m_summary->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	auto statusParts = new QWidget(footer);
	statusParts->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	auto partsLayout = new QHBoxLayout(statusParts);
	partsLayout->setContentsMargins(0, 0, 0, 0);
	partsLayout->setSpacing(5);
	const QStringList statusIcons {QStringLiteral("monitor"), QStringLiteral("circle-check"), QStringLiteral("power-off"), QStringLiteral("key-round")};
	for (int part = 0; part < statusIcons.size(); ++part)
	{
		if (part) partsLayout->addSpacing(7);
		auto icon = new QLabel(statusParts);
		auto pixmap = DesktopIcons::icon(statusIcons[part]).pixmap(12, 12);
		QPainter tint(&pixmap);
		tint.setCompositionMode(QPainter::CompositionMode_SourceIn);
		tint.fillRect(pixmap.rect(), part == 1 ? QColor(QStringLiteral("#438358")) :
			part == 3 ? palette().color(QPalette::Highlight) : palette().color(QPalette::PlaceholderText));
		tint.end();
		icon->setPixmap(pixmap);
		partsLayout->addWidget(icon);
		auto value = part == 0 ? m_summary : new QLabel(statusParts);
		if (part) m_statusValues[part - 1] = value;
		partsLayout->addWidget(value);
	}
	partsLayout->addStretch();
	statusLayout->addWidget(statusParts, 1);
	auto viewGroup = new QButtonGroup(footer);
	auto grid = new QToolButton(footer);
	grid->setObjectName(QStringLiteral("gridViewButton"));
	grid->setIcon(DesktopIcons::icon(QStringLiteral("layout-grid")));
	grid->setToolTip(tr("Grid"));
	auto list = new QToolButton(footer);
	list->setObjectName(QStringLiteral("listViewButton"));
	list->setIcon(DesktopIcons::icon(QStringLiteral("list")));
	list->setToolTip(tr("List"));
	for (auto button : {grid, list})
	{
		button->setCheckable(true);
		button->setProperty("segment", true);
		button->setFixedSize(27, 22);
		button->setIconSize(QSize(13, 13));
		viewGroup->addButton(button);
		statusLayout->addWidget(button);
	}
	grid->setChecked(true);
	connect(list, &QToolButton::toggled, monitoring, &ComputerMonitoringWidget::setListMode);
	for (const auto& icon : {QStringLiteral("zoom-out"), QStringLiteral("zoom-in")})
	{
		if (icon == QStringLiteral("zoom-in"))
		{
			auto slider = window()->findChild<QSlider*>(QStringLiteral("gridSizeSlider"));
			if (slider) { slider->setFixedWidth(90); statusLayout->addWidget(slider); }
		}
		auto label = new QLabel(footer);
		label->setPixmap(DesktopIcons::icon(icon).pixmap(14, 14));
		statusLayout->addWidget(label);
	}
	content->addWidget(footer);

	m_details = new QWidget(this);
	m_details->setObjectName(QStringLiteral("computerDetails"));
	m_details->setFixedWidth(264);
	m_details->setAutoFillBackground(true);
	m_details->setBackgroundRole(QPalette::AlternateBase);
	root->addWidget(m_details);
	auto details = new QVBoxLayout(m_details);
	details->setContentsMargins(12, 10, 12, 12);
	details->setSpacing(10);
	auto detailHeader = new QHBoxLayout;
	m_detailsTitle = new QLabel(m_details);
	m_detailsTitle->setObjectName(QStringLiteral("detailsTitle"));
	m_detailsTitle->setTextFormat(Qt::PlainText);
	m_detailsTitle->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	detailHeader->addWidget(m_detailsTitle, 1);
	auto close = new QToolButton(m_details);
	close->setIcon(DesktopIcons::icon(QStringLiteral("x")));
	close->setToolTip(tr("Clear selection"));
	connect(close, &QToolButton::clicked, monitoring, &QAbstractItemView::clearSelection);
	detailHeader->addWidget(close);
	details->addLayout(detailHeader);
	m_preview = new QLabel(m_details);
	m_preview->setObjectName(QStringLiteral("detailsPreview"));
	m_preview->setFixedSize(240, 150);
	m_preview->setAlignment(Qt::AlignCenter);
	details->addWidget(m_preview);
	m_detailState = new QLabel(m_details);
	m_detailState->setObjectName(QStringLiteral("detailState"));
	details->addWidget(m_detailState);
	auto information = new QGridLayout;
	information->setHorizontalSpacing(0);
	information->setVerticalSpacing(6);
	information->setColumnMinimumWidth(0, 96);
	information->setColumnStretch(1, 1);
	const QStringList keys {tr("User"), tr("Address"), tr("System"), tr("Session since")};
	for (int row = 0; row < keys.size(); ++row)
	{
		auto key = new QLabel(keys[row], m_details);
		key->setObjectName(QStringLiteral("detailMetadataKey"));
		auto colors = key->palette();
		colors.setColor(QPalette::WindowText, palette().color(QPalette::PlaceholderText));
		key->setPalette(colors);
		information->addWidget(key, row, 0, Qt::AlignTop);
		m_detailValues[row] = new QLabel(m_details);
		m_detailValues[row]->setObjectName(QStringLiteral("detailMetadataValue"));
		m_detailValues[row]->setTextFormat(Qt::PlainText);
		m_detailValues[row]->setWordWrap(true);
		m_detailValues[row]->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
		m_detailValues[row]->setTextInteractionFlags(Qt::TextSelectableByMouse);
		information->addWidget(m_detailValues[row], row, 1);
	}
	details->addLayout(information);
	auto detailSeparator = new QFrame(m_details);
	detailSeparator->setObjectName(QStringLiteral("detailSeparator"));
	detailSeparator->setFrameShape(QFrame::HLine);
	details->addWidget(detailSeparator);
	auto remoteButtons = new QHBoxLayout;
	m_view = featureButton(m_details, QStringLiteral("RemoteView"), QStringLiteral("eye"), tr("View"));
	m_view->setObjectName(QStringLiteral("detailViewButton"));
	m_view->setProperty("primary", true);
	auto viewIcon = DesktopIcons::icon(QStringLiteral("eye")).pixmap(QSize(16, 16), devicePixelRatioF());
	QPainter iconPainter(&viewIcon);
	iconPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
	iconPainter.fillRect(viewIcon.rect(), palette().color(QPalette::Base));
	iconPainter.end();
	m_view->setIcon(QIcon(viewIcon));
	m_control = featureButton(m_details, QStringLiteral("RemoteControl"), QStringLiteral("mouse-pointer-2"), tr("Control"));
	m_control->setObjectName(QStringLiteral("detailControlButton"));
	m_control->setProperty("bordered", true);
	remoteButtons->addWidget(m_view);
	remoteButtons->addWidget(m_control);
	details->addLayout(remoteButtons);
	auto detailActionsLayout = new QVBoxLayout;
	detailActionsLayout->setSpacing(0);
	details->addLayout(detailActionsLayout);
	const QList<QPair<QString, QString>> detailActions {
		{QStringLiteral("TextMessage"), tr("Message…")}, {QStringLiteral("ScreenLock"), tr("Lock")},
		{QStringLiteral("Reboot"), tr("Restart…")}, {QStringLiteral("PowerDown"), tr("Shut down…")}
	};
	for (const auto& action : detailActions)
	{
		auto button = featureButton(m_details, action.first, featureIcon(action.first), action.second);
		button->setProperty("dangerous", action.first == QStringLiteral("PowerDown"));
		button->setFixedHeight(30);
		button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		detailActionsLayout->addWidget(button);
	}
	auto share = featureButton(m_details, QStringLiteral("ShareUserScreenFullScreen"), QStringLiteral("monitor-up"), tr("Show this screen to everyone"));
	share->setFixedHeight(30);
	disconnect(share, &QToolButton::clicked, this, nullptr);
	connect(share, &QToolButton::clicked, this, [this]() {
		const auto selected = m_monitoring->selectedComputerControlInterfaces();
		if (selected.size() == 1 && selected.first()->state() == ComputerControlInterface::State::Connected)
			m_monitoring->runFeatureOnComputers(findFeature(m_master, QStringLiteral("ShareUserScreenFullScreen")), m_master.allComputerControlInterfaces(), false);
	});
	detailActionsLayout->insertWidget(2, share);
	auto actions = new QToolButton(m_details);
	actions->setText(tr("All actions"));
	actions->setIcon(DesktopIcons::icon(QStringLiteral("ellipsis")));
	actions->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	connect(actions, &QToolButton::clicked, this, [monitoring, actions]() {
		monitoring->showContextMenu(actions->mapToGlobal(QPoint(0, actions->height())));
	});
	actions->setFixedHeight(30);
	detailActionsLayout->addWidget(actions);
	details->addStretch();
	m_details->hide();
	for (auto button : m_details->findChildren<QToolButton*>())
	{
		button->style()->unpolish(button);
		button->style()->polish(button);
	}

	auto clear = new QAction(this);
	clear->setShortcut(QKeySequence(Qt::Key_Escape));
	addAction(clear);
	connect(clear, &QAction::triggered, monitoring, &QAbstractItemView::clearSelection);
	auto focusSearch = new QAction(this);
	focusSearch->setShortcut(QKeySequence::Find);
	addAction(focusSearch);
	connect(focusSearch, &QAction::triggered, search, [search]() { search->setFocus(); search->selectAll(); });
	for (const auto& feature : master.allFeatures())
	{
		if (!feature.shortcut().isEmpty())
		{
			auto shortcut = new QAction(this);
			shortcut->setShortcut(feature.shortcut());
			addAction(shortcut);
			connect(shortcut, &QAction::triggered, this, [this, feature]() { runFeature(feature.name()); });
		}
	}
	connect(monitoring->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
		if (m_monitoring->selectionModel()->hasSelection()) m_scopeSelected->setChecked(true);
		updateDetails();
	});
	const auto update = [this]() { updateOverview(); };
	for (auto model : {static_cast<QAbstractItemModel*>(&master.computerControlListModel()), static_cast<QAbstractItemModel*>(master.computerMonitoringModel())})
	{
		connect(model, &QAbstractItemModel::rowsInserted, this, update);
		connect(model, &QAbstractItemModel::rowsRemoved, this, update);
		connect(model, &QAbstractItemModel::modelReset, this, update);
	}
	connect(&master.computerControlListModel(), &ComputerControlListModel::stateChanged, this, update);
	connect(&master.computerControlListModel(), &ComputerControlListModel::activeFeaturesChanged, this, update);
	connect(master.computerMonitoringModel(), &QAbstractItemModel::dataChanged, this, [this](const QModelIndex& first, const QModelIndex& last) {
		const auto indices = m_monitoring->selectionModel()->selectedIndexes();
		if (indices.size() == 1 && indices.first().row() >= first.row() && indices.first().row() <= last.row()) updateDetails();
	});
	connect(search, &QLineEdit::textChanged, this, update);
	QTimer::singleShot(0, this, update);
}

ComputerControlInterfaceList MasterWorkspace::actionTargets() const
{
	return m_scopeSelected->isChecked() ? m_monitoring->selectedComputerControlInterfaces() : m_master.allComputerControlInterfaces();
}

QToolButton* MasterWorkspace::featureButton(QWidget* parent, const QString& name, const QString& icon, const QString& label)
{
	QToolButton* button = name == QStringLiteral("Demo") ? new ClassroomMenuButton(parent, true) : new QToolButton(parent);
	button->setObjectName(name + QStringLiteral("Button"));
	button->setIcon(DesktopIcons::icon(icon));
	button->setIconSize(QSize(16, 16));
	button->setText(label);
	button->setToolTip(findFeature(m_master, name).displayName());
	button->setToolButtonStyle(label.isEmpty() ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon);
	button->setVisible(findFeature(m_master, name).isValid());
	connect(button, &QToolButton::clicked, this, [this, name, parent]() { runFeature(name, parent == m_details); });
	return button;
}

void MasterWorkspace::runFeature(const QString& name, bool selectedOnly)
{
	const auto targets = selectedOnly ? m_monitoring->selectedComputerControlInterfaces() : actionTargets();
	const auto feature = findFeature(m_master, name);
	if (targets.isEmpty() || !feature.isValid()) return;
	m_monitoring->runFeatureOnComputers(feature, targets, feature.parentUid().isNull());
	reloadFeatures();
}

void MasterWorkspace::addFeature(QMenu* menu, const Feature& feature, bool selectedOnly, bool flat)
{
	if (feature.testFlag(Feature::Flag::Meta)) return;
	const auto targets = selectedOnly ? m_monitoring->selectedComputerControlInterfaces() : actionTargets();
	const auto subFeatures = m_master.subFeatures(feature.uid());
	if (!subFeatures.isEmpty())
	{
		auto submenu = flat ? menu : menu->addMenu(DesktopIcons::icon(featureIcon(feature.name())), feature.displayName());
		if (feature.testFlag(Feature::Flag::Mode))
		{
			auto stop = submenu->addAction(tr("Stop"));
			bool active = false;
			for (const auto& computer : targets)
				for (const auto& related : VeyonCore::featureManager().relatedFeatures(feature.uid()))
					active |= computer->activeFeatures().contains(related.uid());
			stop->setEnabled(active);
			connect(stop, &QAction::triggered, this, [this, feature, targets]() {
				VeyonCore::featureManager().stopFeature(m_master, feature, targets);
				for (const auto& related : m_master.subFeatures(feature.uid()))
					VeyonCore::featureManager().stopFeature(m_master, related, targets);
			});
			submenu->addSeparator();
		}
		for (const auto& subFeature : subFeatures) addFeature(submenu, subFeature, selectedOnly);
		return;
	}
	auto action = menu->addAction(DesktopIcons::icon(featureIcon(feature.name())), feature.displayName());
	action->setObjectName(feature.uid().toString());
	action->setToolTip(feature.description());
	action->setEnabled(!targets.isEmpty());
	if (feature.testFlag(Feature::Flag::Option))
	{
		action->setCheckable(true);
		action->setChecked(feature.testFlag(Feature::Flag::Checked));
	}
	connect(action, &QAction::triggered, this, [this, feature, targets]() {
		m_monitoring->runFeatureOnComputers(feature, targets, feature.parentUid().isNull());
		reloadFeatures();
	});
}

void MasterWorkspace::populateFeatures(QMenu* menu, const QStringList& names, bool selectedOnly)
{
	menu->clear();
	for (const auto& name : names)
	{
		const auto feature = findFeature(m_master, name);
		if (feature.isValid()) addFeature(menu, feature, selectedOnly);
	}
}

void MasterWorkspace::populateMore(QMenu* menu)
{
	menu->clear();
	auto features = menu->addMenu(DesktopIcons::icon(QStringLiteral("settings")), tr("All features"));
	for (const auto& feature : m_master.features()) addFeature(features, feature, false);
	menu->addSeparator();
	const QStringList controlNames {QStringLiteral("filterPoweredOnComputersButton"), QStringLiteral("filterComputersWithLoggedOnUsersButton"),
		QStringLiteral("autoAdjustComputerIconSizeButton"), QStringLiteral("useCustomComputerPositionsButton"), QStringLiteral("alignComputersButton"),
		QStringLiteral("computerSelectPanelButton"), QStringLiteral("screenshotManagementPanelButton"), QStringLiteral("slideshowPanelButton"),
		QStringLiteral("spotlightPanelButton"), QStringLiteral("aboutButton")};
	for (const auto& name : controlNames)
	{
		auto button = window()->findChild<QToolButton*>(name);
		if (!button) continue;
		auto action = menu->addAction(DesktopIcons::icon(QStringLiteral("settings")), button->text().isEmpty() ? button->toolTip().section(QLatin1Char('\n'), 0, 0) : button->text());
		action->setEnabled(button->isEnabled());
		action->setCheckable(button->isCheckable());
		action->setChecked(button->isChecked());
		connect(action, &QAction::triggered, button, &QToolButton::click);
		if (button->menu())
		{
			auto submenu = menu->addMenu(button->toolTip().section(QLatin1Char('\n'), 0, 0));
			for (auto original : button->menu()->actions()) submenu->addAction(original);
		}
	}
}

void MasterWorkspace::reloadFeatures()
{
	if (!m_lock) return;
	bool allLocked = !actionTargets().isEmpty();
	const auto lock = findFeature(m_master, QStringLiteral("ScreenLock"));
	for (const auto& computer : actionTargets()) allLocked &= computer->activeFeatures().contains(lock.uid());
	m_lock->setText(allLocked ? tr("Unlock") : tr("Lock"));
	m_lock->setIcon(DesktopIcons::icon(allLocked ? QStringLiteral("lock-open") : QStringLiteral("lock")));
	if (auto detailLock = m_details->findChild<QToolButton*>(QStringLiteral("ScreenLockButton")))
	{
		const auto selected = m_monitoring->selectedComputerControlInterfaces();
		const auto selectedLocked = !selected.isEmpty() && std::all_of(selected.cbegin(), selected.cend(), [&lock](const auto& computer) {
			return computer->activeFeatures().contains(lock.uid());
		});
		detailLock->setText(selectedLocked ? tr("Unlock") : tr("Lock"));
		detailLock->setIcon(DesktopIcons::icon(selectedLocked ? QStringLiteral("lock-open") : QStringLiteral("lock")));
	}
	int demonstrating = 0, locked = 0;
	const auto demo = findFeature(m_master, QStringLiteral("Demo"));
	for (const auto& computer : m_master.allComputerControlInterfaces())
	{
		bool active = computer->activeFeatures().contains(demo.uid());
		for (const auto& related : m_master.subFeatures(demo.uid()))
			active |= computer->activeFeatures().contains(related.uid()) ||
				computer->activeFeatures().contains(VeyonCore::featureManager().metaFeatureUid(related.uid()));
		if (active) ++demonstrating;
		if (computer->activeFeatures().contains(lock.uid())) ++locked;
	}
	m_banner->setVisible(demonstrating > 0 || locked > 0);
	const auto description = demonstrating > 0 ? tr("Screen demonstration · %1 computers").arg(demonstrating) : tr("Computers locked: %1").arg(locked);
	const auto action = demonstrating > 0 ? QStringLiteral("stop") : QStringLiteral("unlock");
	const auto actionLabel = demonstrating > 0 ? tr("Stop demonstration") : tr("Unlock all");
	m_bannerDescription->setText(description);
	m_bannerDescription->setToolTip(description);
	m_bannerAction->setText(actionLabel);
	m_bannerAction->setProperty("bannerAction", action);
	m_demo->setToolButtonStyle(m_details && m_details->isVisible() ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon);
	m_message->setToolButtonStyle(m_details && m_details->isVisible() ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon);
}

void MasterWorkspace::updateOverview()
{
	int online = 0, offline = 0, errors = 0;
	for (const auto& computer : m_master.allComputerControlInterfaces())
	{
		if (computer->state() == ComputerControlInterface::State::Connected) ++online;
		else if (computer->state() == ComputerControlInterface::State::HostOffline || computer->state() == ComputerControlInterface::State::Disconnected || computer->state() == ComputerControlInterface::State::None) ++offline;
		else if (computer->state() != ComputerControlInterface::State::Connecting) ++errors;
	}
	m_summary->setText(tr("%1 computers").arg(m_master.allComputerControlInterfaces().size()));
	m_statusValues[0]->setText(tr("%1 online").arg(online));
	m_statusValues[1]->setText(tr("%1 offline").arg(offline));
	m_statusValues[2]->setText(tr("%1 errors").arg(errors));
	m_summary->parentWidget()->setToolTip(tr("%1 computers    %2 online    %3 offline    %4 errors")
		.arg(m_master.allComputerControlInterfaces().size()).arg(online).arg(offline).arg(errors));
	m_scopeAll->setText(tr("Whole class · %1").arg(m_master.allComputerControlInterfaces().size()));
	m_empty->setVisible(m_monitoring->model()->rowCount() == 0);
	m_empty->setText(m_master.allComputerControlInterfaces().isEmpty() ? tr("Choose a classroom to begin monitoring.") : tr("No computers match the current filters."));
	updateDetails();
}

void MasterWorkspace::updateDetails()
{
	const auto indices = m_monitoring->selectionModel()->selectedIndexes();
	m_scopeSelected->setText(tr("Selected · %1").arg(indices.size()));
	m_scopeSelected->setEnabled(!indices.isEmpty());
	if (indices.isEmpty()) m_scopeAll->setChecked(true);
	const auto withDetails = indices.size() == 1;
	findChild<QLabel*>(QStringLiteral("classroomTargetLabel"))->setVisible(!withDetails);
	m_demo->setToolButtonStyle(withDetails ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon);
	m_message->setToolButtonStyle(withDetails ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon);
	m_details->setVisible(withDetails);
	if (indices.size() == 1)
	{
		const auto index = indices.first();
		const auto computer = index.data(ComputerControlListModel::ControlInterfaceRole).value<ComputerControlInterface::Pointer>();
		if (computer)
		{
			m_detailsTitle->setText(computer->computerName());
			const auto connected = computer->state() == ComputerControlInterface::State::Connected;
			const auto image = connected && computer->hasValidFramebuffer() ? index.data(Qt::DecorationRole).value<QImage>() : QImage{};
			m_preview->setPixmap(image.isNull() ? DesktopIcons::icon(QStringLiteral("monitor")).pixmap(32, 32) :
				QPixmap::fromImage(image).scaled(240, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
			m_detailState->setText(QStringLiteral("● ") + (connected ? tr("Online") : tr("Unavailable")));
			auto stateColors = m_detailState->palette();
			stateColors.setColor(QPalette::WindowText, connected ? QColor(QStringLiteral("#438358")) : palette().color(QPalette::PlaceholderText));
			m_detailState->setPalette(stateColors);
			const auto uptime = computer->sessionInfo().uptime;
			const auto since = uptime > 0 ? QDateTime::currentDateTime().addSecs(-uptime).toString(QStringLiteral("HH:mm")) : QStringLiteral("—");
			const QStringList values {computer->userFullName().isEmpty() ? computer->userLoginName() : computer->userFullName(),
				computer->computer().hostName(), QStringLiteral("—"), since};
			for (int row = 0; row < values.size(); ++row)
			{
				m_detailValues[row]->setText(values[row].isEmpty() ? QStringLiteral("—") : values[row]);
				m_detailValues[row]->setToolTip(values[row]);
			}
			m_view->setEnabled(connected);
			m_control->setEnabled(connected);
			m_details->findChild<QToolButton*>(QStringLiteral("ShareUserScreenFullScreenButton"))->setEnabled(connected);
		}
		else m_details->hide();
	}
	reloadFeatures();
}
