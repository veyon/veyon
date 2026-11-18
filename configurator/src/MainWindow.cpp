// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QActionGroup>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPalette>
#include <QProcess>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QStackedWidget>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "AboutDialog.h"
#include "Configuration/JsonStore.h"
#include "Configuration/UiMapping.h"
#include "ConfigurationManager.h"
#include "ConfigurationPagePluginInterface.h"
#include "DesktopIcons.h"
#include "MainWindow.h"
#include "NetworkObject.h"
#include "PluginManager.h"
#include "VeyonConfiguration.h"
#include "VeyonServiceControl.h"

#include "ui_MainWindow.h"

namespace
{
QPixmap statusPixmap(const QString& name, const QColor& color, int size)
{
	auto pixmap = DesktopIcons::icon(name).pixmap(size, size);
	QPainter painter(&pixmap);
	painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
	painter.fillRect(pixmap.rect(), color);
	return pixmap;
}

class NavigationDelegate final : public QStyledItemDelegate
{
public:
	using QStyledItemDelegate::QStyledItemDelegate;

protected:
	void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
	{
		QStyledItemDelegate::initStyleOption(option, index);
		const bool category = !index.parent().isValid();
		option->font.setPixelSize(category ? 11 : 12);
		option->font.setBold(category || option->state.testFlag(QStyle::State_Selected));
	}
};
}


MainWindow::MainWindow(QWidget* parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	m_configChanged(false)
{
	ui->setupUi(this);


	setWindowTitle(tr("Veyon Configurator %1").arg(VeyonCore::versionString()));

	loadConfigurationPagePlugins();

	// reset all widget's values to current configuration
	reset(true);

	// if local configuration is incomplete, re-enable the apply button
	if (VeyonConfiguration().data().size() < VeyonCore::config().data().size())
	{
		configurationChanged();
	}

	const auto pages = findChildren<ConfigurationPage*>();
	for (auto page : pages)
	{
		page->connectWidgetsToProperties();
		connect(page, &ConfigurationPage::widgetsChanged, this, &MainWindow::updateView);
	}

	buildNavigation();

	connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &MainWindow::resetOrApply);

	connect(ui->actionLoadSettings, &QAction::triggered, this, &MainWindow::loadSettingsFromFile);
	connect(ui->actionSaveSettings, &QAction::triggered, this, &MainWindow::saveSettingsToFile);

	auto viewModeGroup = new QActionGroup(this);
	viewModeGroup->addAction(ui->viewModeStandard);
	viewModeGroup->addAction(ui->viewModeAdvanced);
	viewModeGroup->setExclusive(true);
	ui->viewModeStandard->setChecked(true);

	connect(viewModeGroup, &QActionGroup::triggered, this, &MainWindow::updateView);

	connect(ui->actionAboutQt, &QAction::triggered, QApplication::instance(), &QApplication::aboutQt);

	connect(&VeyonCore::config(), &VeyonConfiguration::configurationChanged, this, &MainWindow::configurationChanged);

	connect(ui->configPages, &QStackedWidget::currentChanged, this, &MainWindow::updateSizes);

	resize(1100, 760);

	updateView();
}



MainWindow::~MainWindow()
{
	delete ui;
}



void MainWindow::reset(bool onlyUI)
{
	if (onlyUI == false)
	{
		VeyonCore::config().clear();
		VeyonCore::config().reloadFromStore();
	}

	const auto pages = findChildren<ConfigurationPage*>();
	for (auto page : pages)
	{
		page->resetWidgets();
	}

	ui->buttonBox->setEnabled(false);
	m_configChanged = false;
	updateStatus();
}



void MainWindow::apply()
{
	const auto showError = [this](const ConfigurationManager& configurationManager) {
		vCritical() << configurationManager.errorString().toUtf8().constData();

		QMessageBox::critical(this, tr("Veyon Configurator"), configurationManager.errorString());
	};

	ConfigurationManager configurationManager;
	if (configurationManager.saveConfiguration())
	{
		if (configurationManager.applyConfiguration() == false)
		{
			showError(configurationManager);
			return;
		}

		const auto pages = findChildren<ConfigurationPage*>();
		for (auto page : pages)
		{
			page->applyConfiguration();
		}

		ui->buttonBox->setEnabled(false);
		m_configChanged = false;
		updateStatus();
	}
	else
	{
		showError(configurationManager);
	}
}



void MainWindow::resizeEvent(QResizeEvent* event)
{
	QMainWindow::resizeEvent(event);

	updateSizes();
}



void MainWindow::configurationChanged()
{
	ui->buttonBox->setEnabled(true);
	m_configChanged = true;
	updateStatus();
}



void MainWindow::resetOrApply(QAbstractButton* btn)
{
	if (ui->buttonBox->standardButton(btn) & QDialogButtonBox::Apply)
	{
		apply();
	}
	else if (ui->buttonBox->standardButton(btn) & QDialogButtonBox::Reset)
	{
		reset();
	}
}



void MainWindow::loadSettingsFromFile()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load settings from file"),
													QDir::homePath(), tr("JSON files (*.json)"));
	if (!fileName.isEmpty())
	{
		// write current configuration to output file
		Configuration::JsonStore(Configuration::JsonStore::Scope::System, fileName).load(&VeyonCore::config());
		reset(true);
		configurationChanged(); // give user a chance to apply possible changes
	}
}



void MainWindow::saveSettingsToFile()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save settings to file"),
													QDir::homePath(), tr("JSON files (*.json)"));
	if (!fileName.isEmpty())
	{
		if (!fileName.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive))
		{
			fileName += QStringLiteral(".json");
		}

		bool configChangedPrevious = m_configChanged;

		// write current configuration to output file
		Configuration::JsonStore(Configuration::JsonStore::Scope::System, fileName).flush(&VeyonCore::config());

		m_configChanged = configChangedPrevious;
		ui->buttonBox->setEnabled(m_configChanged);
	}
}



void MainWindow::resetConfiguration()
{
	if (QMessageBox::warning(this, tr("Reset configuration"),
							 tr("Do you really want to reset the local configuration and revert "
								"all settings to their defaults?"),
							 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ==
		QMessageBox::Yes)
	{
		ConfigurationManager().clearConfiguration();
		reset(false);
	}
}



void MainWindow::aboutVeyon()
{
	AboutDialog(this).exec();
}



void MainWindow::updateSizes()
{
	const auto page = ui->configPages->currentWidget();
	if (!page) return;
	const auto margins = ui->verticalLayout->contentsMargins();
	const int width = qMax(0, ui->scrollArea->viewport()->width() - margins.left() - margins.right());
	const int pageHeight = qMax(page->minimumSizeHint().height(), page->heightForWidth(width));
	ui->configPages->setMinimumSize(0, pageHeight);
	const int height = qMax(pageHeight,
		ui->scrollArea->viewport()->height() - margins.top() - margins.bottom());
	ui->scrollAreaWidgetContents->setMinimumHeight(0);
	ui->scrollAreaWidgetContents->resize(ui->scrollArea->viewport()->width(), height + margins.top() + margins.bottom());
	ui->configPages->setGeometry(margins.left(), margins.top(), width, height);
}



void MainWindow::updateView()
{
	if (ui->viewModeAdvanced->isChecked())
	{
		switchToAdvancedView();
	}
	else
	{
		switchToStandardView();
	}

	if (m_navigation && !ui->viewModeAdvanced->isChecked() && m_navigation->currentItem() &&
		m_navigation->currentItem()->data(0, Qt::UserRole + 2).toBool())
	{
		m_navigation->setCurrentItem(m_navigation->topLevelItem(0)->child(0));
	}
	for (int i = 0; i < ui->configPages->count(); ++i)
		ui->configPages->widget(i)->setVisible(i == ui->configPages->currentIndex());
	filterPages();
	QTimer::singleShot(0, this, &MainWindow::updateSizes);
}



void MainWindow::switchToStandardView()
{
	const auto widgets = findChildren<QWidget*>();

	// hide widgets with advanced property flag
	for (auto widget : widgets)
	{
		const auto flags = Configuration::UiMapping::flags(widget);
		if (flags & Configuration::Property::Flag::Hidden)
		{
			widget->hide();
		}
		else if (flags & Configuration::Property::Flag::Advanced)
		{
			widget->hide();
		}
	}


}



void MainWindow::switchToAdvancedView()
{
	const auto widgets = findChildren<QWidget*>();
	for (auto widget : widgets)
	{
		const auto flags = Configuration::UiMapping::flags(widget);
		if (flags & Configuration::Property::Flag::Advanced && !(flags & Configuration::Property::Flag::Hidden))
		{
			widget->show();
		}
	}

}



void MainWindow::loadConfigurationPagePlugins()
{
	for (auto pluginObject : std::as_const(VeyonCore::pluginManager().pluginObjects()))
	{
		auto pluginInterface = qobject_cast<PluginInterface*>(pluginObject);
		auto configurationPagePluginInterface = qobject_cast<ConfigurationPagePluginInterface*>(pluginObject);

		if (pluginInterface && configurationPagePluginInterface)
		{
			auto page = configurationPagePluginInterface->createConfigurationPage();
			if (page)
			{
				ui->configPages->addWidget(page);

				auto item = new QListWidgetItem(page->windowIcon(), page->windowTitle());
				item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
				item->setData(Qt::UserRole, QVariant::fromValue(page));
				ui->pageSelector->addItem(item);
			}
		}
	}

	// adjust minimum size
	ui->pageSelector->setMinimumSize(ui->pageSelector->sizeHintForColumn(0) + 3 * ui->pageSelector->spacing(),
									 ui->pageSelector->minimumHeight());
}



void MainWindow::closeEvent(QCloseEvent* closeEvent)
{
	if (m_configChanged &&
		QMessageBox::question(this, tr("Unsaved settings"),
							  tr("There are unsaved settings. "
								 "Quit anyway?"),
							  QMessageBox::Yes | QMessageBox::No) !=
			QMessageBox::Yes)
	{
		closeEvent->ignore();
		return;
	}

	closeEvent->accept();
	QMainWindow::closeEvent(closeEvent);
}


void MainWindow::filterPages()
{
	if (!m_navigation) return;
	const auto query = m_pageSearch->text().trimmed();
	for (int groupIndex = 0; groupIndex < m_navigation->topLevelItemCount(); ++groupIndex)
	{
		auto group = m_navigation->topLevelItem(groupIndex);
		bool anyVisible = false;
		for (int i = 0; i < group->childCount(); ++i)
		{
			auto item = group->child(i);
			const auto advanced = item->data(0, Qt::UserRole + 2).toBool();
			const auto matches = item->text(0).contains(query, Qt::CaseInsensitive) ||
				item->data(0, Qt::UserRole + 1).toString().contains(query, Qt::CaseInsensitive) ||
				item->data(0, Qt::UserRole + 3).toString().contains(query, Qt::CaseInsensitive);
			item->setHidden(!matches || (query.isEmpty() && advanced && !ui->viewModeAdvanced->isChecked()));
			anyVisible |= !item->isHidden();
		}
		group->setHidden(!anyVisible);
	}
}


void MainWindow::selectNavigationPage(QTreeWidgetItem* item)
{
	if (!item || !item->parent()) return;
	if (item->data(0, Qt::UserRole + 2).toBool() && !ui->viewModeAdvanced->isChecked())
	{
		ui->viewModeAdvanced->setChecked(true);
		updateView();
	}
	ui->configPages->setCurrentWidget(item->data(0, Qt::UserRole).value<QWidget*>());
	m_heading->setText(item->text(0));
	m_description->setText(item->data(0, Qt::UserRole + 1).toString());
	updateStatus();
}


void MainWindow::updateStatus()
{
	const bool dark = palette().color(QPalette::Window).lightness() < 128;
	const QColor positive(dark ? QStringLiteral("#75c58c") : QStringLiteral("#288747"));
	const QColor warning(dark ? QStringLiteral("#e7b75f") : QStringLiteral("#a96700"));
	const auto neutral = palette().color(QPalette::Text);
	const auto muted = palette().color(QPalette::Disabled, QPalette::Text);
	if (m_status) m_status->setText(m_configChanged ? tr("Unsaved changes") : tr("All changes saved"));
	if (m_statusIndicator) m_statusIndicator->setPixmap(statusPixmap(m_configChanged ? QStringLiteral("triangle-alert") : QStringLiteral("circle-check"), m_configChanged ? warning : positive, 14));
	if (m_serviceStatus)
	{
		const auto indicator = [](QLabel* value, const QString& icon, const QColor& color) {
			if (auto symbol = value->property("overviewIndicator").value<QLabel*>())
				symbol->setPixmap(statusPixmap(icon, color, 18));
		};
		const bool running = VeyonServiceControl(this).isServiceRunning();
		m_serviceStatus->setText(running ? tr("Running") : tr("Stopped"));
		m_serviceAction->setText(running ? tr("Restart") : tr("Start service"));
		indicator(m_serviceStatus, running ? QStringLiteral("circle-check") : QStringLiteral("circle-x"), running ? positive : warning);
		m_authenticationStatus->setText(VeyonCore::config().authenticationMethod() == VeyonCore::AuthenticationMethod::KeyFileAuthentication ?
			tr("Key file authentication") : tr("Logon authentication"));
		int rooms = 0;
		int computers = 0;
		const auto objects = VeyonCore::config().value(QStringLiteral("NetworkObjects"), QStringLiteral("BuiltinDirectory"), {}).value<QJsonArray>();
		for (const auto& value : objects)
		{
			const NetworkObject object(value.toObject());
			rooms += object.type() == NetworkObject::Type::Location;
			computers += object.type() == NetworkObject::Type::Host;
		}
		m_directoryStatus->setText(tr("%1 rooms · %2 computers in the local directory").arg(rooms).arg(computers));
		indicator(m_directoryStatus, rooms || computers ? QStringLiteral("circle-check") : QStringLiteral("circle-dashed"), rooms || computers ? positive : muted);
		const auto ldapServer = VeyonCore::config().value(QStringLiteral("ServerHost"), QStringLiteral("LDAP"), {}).toString();
		m_ldapStatus->setText(ldapServer.isEmpty() ? tr("Not configured") : tr("Configured server: %1").arg(ldapServer));
		indicator(m_ldapStatus, ldapServer.isEmpty() ? QStringLiteral("circle-dashed") : QStringLiteral("network"), ldapServer.isEmpty() ? muted : neutral);
		m_accessStatus->setText(VeyonCore::config().isAccessRestrictedToUserGroups() || VeyonCore::config().isAccessControlRulesProcessingEnabled() ?
			tr("Access restrictions enabled") : tr("Access rules are not enabled"));
		const bool restricted = VeyonCore::config().isAccessRestrictedToUserGroups() || VeyonCore::config().isAccessControlRulesProcessingEnabled();
		indicator(m_accessStatus, restricted ? QStringLiteral("shield-check") : QStringLiteral("triangle-alert"), restricted ? positive : warning);
	}
}


void MainWindow::buildNavigation()
{
	disconnect(ui->pageSelector, nullptr, ui->configPages, nullptr);
	ui->pageSelector->hide();
	ui->gridLayout->removeWidget(ui->pageSelector);
	ui->line->hide();
	ui->menubar->hide();
	ui->gridLayout->setContentsMargins(0, 0, 0, 0);
	ui->gridLayout->setHorizontalSpacing(0);
	ui->gridLayout->setVerticalSpacing(0);
	auto sidebar = new QWidget(this);
	sidebar->setObjectName(QStringLiteral("ConfiguratorSidebar"));
	sidebar->setFixedWidth(236);
	auto sideLayout = new QVBoxLayout(sidebar);
	sideLayout->setContentsMargins(0, 12, 0, 12);
	sideLayout->setSpacing(12);
	auto searchContainer = new QWidget(sidebar);
	auto searchLayout = new QHBoxLayout(searchContainer);
	searchLayout->setContentsMargins(12, 0, 12, 0);
	m_pageSearch = new QLineEdit(sidebar);
	m_pageSearch->setPlaceholderText(tr("Search settings"));
	m_pageSearch->setClearButtonEnabled(true);
	m_pageSearch->setFixedHeight(30);
	m_pageSearch->addAction(DesktopIcons::icon(QStringLiteral("search")), QLineEdit::LeadingPosition);
	auto searchHint = new QLabel(QStringLiteral("Ctrl+F"), m_pageSearch);
	searchHint->setObjectName(QStringLiteral("ConfiguratorSearchShortcut"));
	searchHint->setAttribute(Qt::WA_TransparentForMouseEvents);
	auto hintLayout = new QHBoxLayout(m_pageSearch);
	hintLayout->setContentsMargins(0, 0, 8, 0);
	hintLayout->addStretch();
	hintLayout->addWidget(searchHint);
	m_pageSearch->setTextMargins(0, 0, 42, 0);
	connect(m_pageSearch, &QLineEdit::textChanged, searchHint, [this, searchHint](const QString& text) {
		searchHint->setVisible(text.isEmpty());
		m_pageSearch->setTextMargins(0, 0, text.isEmpty() ? 42 : 0, 0);
	});
	searchLayout->addWidget(m_pageSearch);
	sideLayout->addWidget(searchContainer);
	m_navigation = new QTreeWidget(sidebar);
	m_navigation->setObjectName(QStringLiteral("ConfiguratorNavigation"));
	m_navigation->setHeaderHidden(true);
	m_navigation->setItemDelegate(new NavigationDelegate(m_navigation));
	m_navigation->header()->setSectionResizeMode(QHeaderView::Stretch);
	m_navigation->setRootIsDecorated(false);
	m_navigation->setIndentation(0);
	m_navigation->setIconSize(QSize(16, 16));
	m_navigation->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	sideLayout->addWidget(m_navigation, 1);
	ui->gridLayout->addWidget(sidebar, 0, 0, 3, 1);

	auto heading = new QWidget(this);
	auto headingLayout = new QVBoxLayout(heading);
	headingLayout->setContentsMargins(28, 20, 28, 0);
	headingLayout->setSpacing(4);
	m_heading = new QLabel(heading);
	m_heading->setObjectName(QStringLiteral("ConfiguratorHeading"));
	auto font = m_heading->font();
	font.setPointSize(18);
	font.setBold(true);
	m_heading->setFont(font);
	m_description = new QLabel(heading);
	m_description->setObjectName(QStringLiteral("ConfiguratorDescription"));
	m_description->setWordWrap(true);
	m_description->setMaximumWidth(640);
	headingLayout->addWidget(m_heading);
	headingLayout->addWidget(m_description);
	headingLayout->addSpacing(6);
	auto headingDivider = new QWidget(heading);
	headingDivider->setObjectName(QStringLiteral("ConfiguratorHeadingDivider"));
	headingDivider->setAutoFillBackground(true);
	headingDivider->setBackgroundRole(QPalette::Mid);
	headingDivider->setFixedHeight(2);
	headingLayout->addWidget(headingDivider);
	ui->verticalLayout->setContentsMargins(28, 0, 28, 28);
	ui->verticalLayout->setEnabled(false);
	ui->configPages->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	ui->verticalLayout->setSizeConstraint(QLayout::SetNoConstraint);
	ui->configPages->layout()->setSizeConstraint(QLayout::SetNoConstraint);
	ui->scrollAreaWidgetContents->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	ui->gridLayout->removeWidget(ui->scrollArea);
	auto content = new QWidget(this);
	auto contentLayout = new QVBoxLayout(content);
	contentLayout->setContentsMargins(0, 0, 0, 0);
	contentLayout->setSpacing(0);
	contentLayout->addWidget(heading);
	contentLayout->addWidget(ui->scrollArea, 1);
	ui->scrollArea->setFrameShape(QFrame::NoFrame);
	ui->scrollArea->setWidgetResizable(false);
	connect(ui->scrollArea->verticalScrollBar(), &QScrollBar::rangeChanged,
		this, &MainWindow::updateSizes, Qt::QueuedConnection);
	ui->gridLayout->addWidget(content, 0, 1, 2, 1);

	auto bar = new QWidget(this);
	bar->setObjectName(QStringLiteral("ConfiguratorActionBar"));
	bar->setFixedHeight(46);
	auto barLayout = new QHBoxLayout(bar);
	barLayout->setContentsMargins(16, 0, 16, 0);
	m_status = new QLabel(bar);
	m_status->setObjectName(QStringLiteral("ConfiguratorStatus"));
	m_statusIndicator = new QLabel(bar);
	m_statusIndicator->setFixedSize(16, 16);
	barLayout->addWidget(m_statusIndicator);
	barLayout->addWidget(m_status);
	barLayout->addStretch();
	auto more = new QToolButton(bar);
	more->setText(tr("More"));
	more->setIcon(DesktopIcons::icon(QStringLiteral("ellipsis")));
	more->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	more->setPopupMode(QToolButton::InstantPopup);
	auto menu = new QMenu(more);
	ui->actionLoadSettings->setIcon(DesktopIcons::icon(QStringLiteral("download")));
	ui->actionSaveSettings->setIcon(DesktopIcons::icon(QStringLiteral("upload")));
	ui->actionResetConfiguration->setIcon(DesktopIcons::icon(QStringLiteral("rotate-ccw")));
	ui->actionAboutVeyon->setIcon(DesktopIcons::icon(QStringLiteral("info")));
	menu->addAction(ui->actionLoadSettings);
	menu->addAction(ui->actionSaveSettings);
	menu->addSeparator();
	menu->addAction(ui->viewModeStandard);
	menu->addAction(ui->viewModeAdvanced);
	menu->addSeparator();
	menu->addAction(ui->actionResetConfiguration);
	menu->addAction(ui->actionAboutVeyon);
	menu->addAction(ui->actionAboutQt);
	menu->addAction(ui->actionQuit);
	more->setMenu(menu);
	barLayout->addWidget(more);
	ui->gridLayout->removeWidget(ui->buttonBox);
	barLayout->addWidget(ui->buttonBox);
	ui->buttonBox->button(QDialogButtonBox::Reset)->setText(tr("Revert"));
	ui->buttonBox->button(QDialogButtonBox::Apply)->setIcon(DesktopIcons::icon(QStringLiteral("check")));
	ui->gridLayout->addWidget(bar, 2, 1);

	auto start = new QTreeWidgetItem(m_navigation, {tr("START")});
	auto connections = new QTreeWidgetItem(m_navigation, {tr("CONNECTIONS")});
	auto features = new QTreeWidgetItem(m_navigation, {tr("FEATURES")});
	auto system = new QTreeWidgetItem(m_navigation, {tr("SYSTEM")});
	for (auto group : {start, connections, features, system})
	{
		group->setFlags(Qt::ItemIsEnabled);
		group->setExpanded(true);
		group->setSizeHint(0, QSize(200, 30));
	}
	const auto addPage = [this](QTreeWidgetItem* group, QWidget* page, const QString& title,
		const QString& description, const QString& icon, bool advanced = false) {
		if (ui->configPages->indexOf(page) < 0) ui->configPages->addWidget(page);
		auto item = new QTreeWidgetItem(group, {title});
		item->setIcon(0, DesktopIcons::icon(icon));
		item->setToolTip(0, title);
		item->setData(0, Qt::UserRole, QVariant::fromValue(page));
		item->setData(0, Qt::UserRole + 1, description);
		item->setData(0, Qt::UserRole + 2, advanced);
		QStringList keywords;
		for (auto label : page->findChildren<QLabel*>()) keywords.append(label->text());
		for (auto group : page->findChildren<QGroupBox*>()) keywords.append(group->title());
		for (auto button : page->findChildren<QAbstractButton*>()) keywords.append(button->text());
		item->setData(0, Qt::UserRole + 3, keywords.join(QLatin1Char(' ')));
		item->setSizeHint(0, QSize(200, 30));
		return item;
	};
	const auto section = [this](QWidget* owner, const QString& name) {
		auto page = new QWidget;
		auto layout = new QVBoxLayout(page);
		layout->setContentsMargins(0, 0, 0, 0);
		if (auto group = owner->findChild<QGroupBox*>(name)) layout->addWidget(group);
		layout->addStretch();
		return page;
	};
	auto overview = new QWidget;
	auto overviewLayout = new QVBoxLayout(overview);
	overviewLayout->setContentsMargins(0, 0, 0, 0);
	overviewLayout->setSpacing(0);
	auto overviewItem = addPage(start, overview, tr("Overview"),
		tr("Check this computer's configuration and manage your classroom settings."), QStringLiteral("layout-dashboard"));
	auto computersPage = section(ui->generalConfigurationPage, QStringLiteral("groupBox_12"));
	auto computersItem = addPage(connections, computersPage, tr("Computers and rooms"),
		tr("Choose the source of computers and locations shown in Veyon Master."), QStringLiteral("monitor"));
	auto authenticationPage = section(ui->generalConfigurationPage, QStringLiteral("groupBox"));
	auto authItem = addPage(connections, authenticationPage, tr("Sign in and keys"),
		tr("Choose how teachers authenticate and test access to this computer."), QStringLiteral("key-round"));
	auto accessItem = addPage(connections, ui->accessControlPage, tr("Access control"),
		tr("Define which users may access this computer and when access is permitted."), QStringLiteral("shield"));
	addPage(connections, section(ui->generalConfigurationPage, QStringLiteral("groupBox_2")),
		tr("User groups"), tr("Configure the user and group backend used by access rules."), QStringLiteral("users"), true);
	auto masterPage = section(ui->masterConfigurationPage, QStringLiteral("groupBox_4"));
	for (const auto& name : {"groupBox", "advancedSettingsGroupBox", "groupBox_2", "groupBox_6", "groupBox_3"})
	{
		if (auto group = ui->masterConfigurationPage->findChild<QGroupBox*>(QString::fromLatin1(name)))
			static_cast<QVBoxLayout*>(masterPage->layout())->insertWidget(masterPage->layout()->count() - 1, group);
	}
	addPage(features, masterPage, tr("Monitoring and demonstration"),
		tr("Configure monitoring, remote access, startup behavior and classroom features."), QStringLiteral("monitor"));
	auto serviceItem = addPage(system, ui->serviceConfigurationPage, tr("Veyon service"),
		tr("Manage the local service, server sessions and connection behavior."), QStringLiteral("server-cog"));
	auto networkPage = section(ui->serviceConfigurationPage, QStringLiteral("networkPortNumbersGroupBox"));
	for (const auto& name : {"miscSettingsGroupBox", "vncServerGroupBox"})
	{
		if (auto group = ui->serviceConfigurationPage->findChild<QGroupBox*>(QString::fromLatin1(name)))
			static_cast<QVBoxLayout*>(networkPage->layout())->insertWidget(networkPage->layout()->count() - 1, group);
	}
	if (auto remoteAccess = ui->masterConfigurationPage->findChild<QGroupBox*>(QStringLiteral("groupBox_5")))
		static_cast<QVBoxLayout*>(networkPage->layout())->insertWidget(networkPage->layout()->count() - 1, remoteAccess);
	addPage(connections, networkPage, tr("Connections"), tr("Configure network ports, remote access and server connections."), QStringLiteral("network"), true);
	addPage(system, section(ui->generalConfigurationPage, QStringLiteral("groupBox_7")),
		tr("Logs"), tr("Set logging detail, open the log folder and clear old log files."), QStringLiteral("scroll-text"));
	addPage(system, section(ui->generalConfigurationPage, QStringLiteral("groupBox_11")),
		tr("User interface"), tr("Choose the language, style and color scheme for Veyon."), QStringLiteral("palette"));
	QTreeWidgetItem* ldapItem = nullptr;
	for (int i = 4; i < ui->pageSelector->count(); ++i)
	{
		auto original = ui->pageSelector->item(i);
		auto page = original->data(Qt::UserRole).value<ConfigurationPage*>();
		const auto title = original->text();
		const auto name = QString::fromLatin1(page->metaObject()->className());
		const bool computers = name.contains(QStringLiteral("Directory"), Qt::CaseInsensitive);
		const bool keys = name.contains(QStringLiteral("Auth"), Qt::CaseInsensitive);
		const bool ldap = name.contains(QStringLiteral("Ldap"), Qt::CaseInsensitive);
		if (name == QStringLiteral("DemoConfigurationPage"))
		{
			static_cast<QVBoxLayout*>(masterPage->layout())->insertWidget(masterPage->layout()->count() - 1, page);
			continue;
		}
		if (computers || keys)
		{
			auto target = computers ? computersPage : authenticationPage;
			static_cast<QVBoxLayout*>(target->layout())->insertWidget(target->layout()->count() - 1, page);
			continue;
		}
		const auto icon = ldap ? QStringLiteral("network") : name == QStringLiteral("DesktopServicesConfigurationPage") ?
			QStringLiteral("app-window") : name == QStringLiteral("FileTransferConfigurationPage") ? QStringLiteral("folder-up") : QStringLiteral("settings");
		auto item = addPage(ldap ? system : features, page, ldap ? tr("LDAP / Active Directory") : title,
			tr("Configure %1.").arg(title), icon,
			Configuration::UiMapping::flags(page).testFlag(Configuration::Property::Flag::Advanced));
		if (ldap) ldapItem = item;
	}
	const auto overviewRow = [this, overviewLayout](const QString& title, QLabel* value, QTreeWidgetItem* target, const QString& icon, const QString& action) {
		auto row = new QWidget;
		row->setObjectName(QStringLiteral("ConfiguratorOverviewRow"));
		auto layout = new QHBoxLayout(row);
		layout->setContentsMargins(0, 10, 0, 10);
		layout->setSpacing(10);
		auto symbol = new QLabel(row);
		symbol->setFixedSize(28, 20);
		symbol->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		symbol->setPixmap(DesktopIcons::icon(icon).pixmap(18, 18));
		value->setProperty("overviewIndicator", QVariant::fromValue(symbol));
		layout->addWidget(symbol);
		auto text = new QVBoxLayout;
		text->setSpacing(1);
		auto label = new QLabel(title, row);
		auto font = label->font(); font.setBold(true); label->setFont(font);
		text->addWidget(label);
		label->setWordWrap(true);
		value->setWordWrap(true);
		value->setObjectName(QStringLiteral("ConfiguratorOverviewValue"));
		text->addWidget(value);
		layout->addLayout(text, 1);
		auto button = new QPushButton(action, row);
		button->setFixedHeight(28);
		connect(button, &QPushButton::clicked, this, [this, target]() {
			m_navigation->setCurrentItem(target);
		});
		layout->addWidget(button);
		overviewLayout->addWidget(row);
		return button;
	};
	m_serviceStatus = new QLabel;
	m_authenticationStatus = new QLabel;
	m_directoryStatus = new QLabel;
	m_ldapStatus = new QLabel;
	m_accessStatus = new QLabel;
	m_serviceAction = overviewRow(tr("Veyon service"), m_serviceStatus, serviceItem, QStringLiteral("circle-dashed"), tr("Start service"));
	disconnect(m_serviceAction, nullptr, this, nullptr);
	connect(m_serviceAction, &QPushButton::clicked, this, [this]() {
		VeyonServiceControl control(this);
		if (control.isServiceRunning()) control.restartService();
		else control.startService();
		updateStatus();
	});
	overviewRow(tr("Authentication"), m_authenticationStatus, authItem, QStringLiteral("key-round"), tr("Open"));
	overviewRow(tr("Computers and rooms"), m_directoryStatus, computersItem, QStringLiteral("monitor"), tr("Open"));
	overviewRow(tr("Access control"), m_accessStatus, accessItem, QStringLiteral("shield"), tr("Configure"));
	overviewRow(tr("LDAP / Active Directory"), m_ldapStatus, ldapItem ? ldapItem : computersItem, QStringLiteral("circle-dashed"), tr("Connect"));
	auto configurationRow = new QWidget;
	auto configurationLayout = new QHBoxLayout(configurationRow);
	configurationLayout->setContentsMargins(0, 14, 0, 14);
	configurationLayout->setSpacing(8);
	auto configurationLabel = new QLabel(tr("Configuration"), configurationRow);
	configurationLabel->setFixedWidth(200);
	auto configurationFont = configurationLabel->font();
	configurationFont.setBold(true);
	configurationLabel->setFont(configurationFont);
	configurationLayout->addWidget(configurationLabel);
	configurationLayout->addSpacing(8);
	for (auto action : {ui->actionSaveSettings, ui->actionLoadSettings})
	{
		auto button = new QPushButton(action == ui->actionSaveSettings ? tr("Export...") : tr("Import..."), configurationRow);
		button->setObjectName(QStringLiteral("ConfiguratorImportExport"));
		button->setFixedHeight(28);
		button->setProperty("desktopIcon", action == ui->actionSaveSettings ? QStringLiteral("upload") : QStringLiteral("download"));
		button->setIcon(action->icon());
		connect(button, &QPushButton::clicked, action, &QAction::trigger);
		configurationLayout->addWidget(button);
	}
	configurationLayout->addStretch();
	auto factoryReset = new QPushButton(tr("Factory reset..."), configurationRow);
	factoryReset->setObjectName(QStringLiteral("ConfiguratorFactoryReset"));
	factoryReset->setFixedHeight(28);
	connect(factoryReset, &QPushButton::clicked, ui->actionResetConfiguration, &QAction::trigger);
	configurationLayout->addWidget(factoryReset);
	overviewLayout->addWidget(configurationRow);
	overviewLayout->addStretch();
	for (int groupIndex = 0; groupIndex < m_navigation->topLevelItemCount(); ++groupIndex)
	{
		auto group = m_navigation->topLevelItem(groupIndex);
		for (int i = 0; i < group->childCount(); ++i)
		{
			auto item = group->child(i);
			auto page = item->data(0, Qt::UserRole).value<QWidget*>();
			QStringList keywords;
			for (auto label : page->findChildren<QLabel*>()) keywords.append(label->text());
			for (auto box : page->findChildren<QGroupBox*>()) keywords.append(box->title());
			for (auto button : page->findChildren<QAbstractButton*>()) keywords.append(button->text());
			item->setData(0, Qt::UserRole + 3, keywords.join(QLatin1Char(' ')));
		}
	}
	connect(m_navigation, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* item) { selectNavigationPage(item); });
	connect(m_pageSearch, &QLineEdit::textChanged, this, &MainWindow::filterPages);
	auto searchAction = new QAction(this);
	searchAction->setShortcut(QKeySequence::Find);
	addAction(searchAction);
	connect(searchAction, &QAction::triggered, this, [this]() { m_pageSearch->setFocus(); m_pageSearch->selectAll(); });
	auto timer = new QTimer(this);
	timer->setInterval(5000);
	connect(timer, &QTimer::timeout, this, &MainWindow::updateStatus);
	timer->start();
	for (auto button : findChildren<QAbstractButton*>())
	{
		if (button->icon().isNull() || button->inherits("QLineEditIconButton") || qobject_cast<QLineEdit*>(button->parentWidget())) continue;
		if (!button->property("desktopIcon").toString().isEmpty())
		{
			button->setIcon(DesktopIcons::icon(button->property("desktopIcon").toString()));
			continue;
		}
		const auto name = button->objectName().toLower();
		if (name.startsWith(QStringLiteral("qt_"))) continue;
		QString icon = QStringLiteral("settings");
		if (name.contains(QStringLiteral("open")) || name.contains(QStringLiteral("directory"))) icon = QStringLiteral("folder-open");
		else if (name.contains(QStringLiteral("test")) || name.contains(QStringLiteral("check"))) icon = QStringLiteral("check");
		else if (name.contains(QStringLiteral("remove")) || name.contains(QStringLiteral("delete")) || name.contains(QStringLiteral("clear"))) icon = QStringLiteral("trash-2");
		else if (name.contains(QStringLiteral("add"))) icon = QStringLiteral("plus");
		else if (name.contains(QStringLiteral("edit"))) icon = QStringLiteral("pencil");
		else if (name.contains(QStringLiteral("up"))) icon = QStringLiteral("arrow-up");
		else if (name.contains(QStringLiteral("down"))) icon = QStringLiteral("arrow-down");
		else if (name.contains(QStringLiteral("start"))) icon = QStringLiteral("play");
		else if (name.contains(QStringLiteral("stop"))) icon = QStringLiteral("square");
		else if (name.contains(QStringLiteral("next")) || name.contains(QStringLiteral("enable"))) icon = QStringLiteral("chevron-right");
		else if (name.contains(QStringLiteral("previous")) || name.contains(QStringLiteral("disable"))) icon = QStringLiteral("chevron-left");
		else if (button == ui->buttonBox->button(QDialogButtonBox::Apply)) icon = QStringLiteral("check");
		else if (button == ui->buttonBox->button(QDialogButtonBox::Reset))
		{
			button->setIcon({});
			continue;
		}
		else if (button == more) icon = QStringLiteral("ellipsis");
		else if (button->actions().contains(ui->actionLoadSettings)) icon = QStringLiteral("download");
		else if (button->actions().contains(ui->actionSaveSettings)) icon = QStringLiteral("upload");
		button->setIcon(DesktopIcons::icon(icon));
	}
	m_navigation->setCurrentItem(overviewItem);
}
