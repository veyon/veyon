// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QActionGroup>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QTimer>

#include "AboutDialog.h"
#include "Configuration/JsonStore.h"
#include "Configuration/LegacySecretMigration.h"
#include "Configuration/SecretPropertyRegistry.h"
#include "Configuration/UiMapping.h"
#include "ConfigurationManager.h"
#include "ConfigurationPagePluginInterface.h"
#include "MainWindow.h"
#include "PluginManager.h"
#include "SecretStore.h"
#include "VeyonConfiguration.h"

#include "ui_MainWindow.h"


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

	resize(ui->pageSelector->width() + ui->generalConfigurationPage->minimumSizeHint().width(),
		   ui->generalConfigurationPage->minimumSizeHint().height());

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
		}

		const auto pages = findChildren<ConfigurationPage*>();
		for (auto page : pages)
		{
			page->applyConfiguration();
		}

		ui->buttonBox->setEnabled(false);
		m_configChanged = false;
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
		Configuration::JsonStore jsonStore(Configuration::JsonStore::Scope::System, fileName);
		VeyonConfiguration importedConfiguration(&jsonStore);

		// encrypted secrets of an export can only be imported with veyon-cli (passphrase), and secret
		// properties never refer to other keys than their own
		Configuration::LegacySecretMigration::scrubConfiguration(importedConfiguration);

		// never store legacy ciphertext (readable with the public default-pkey.pem) again
		if (Configuration::LegacySecretMigration::hasLegacySecrets(importedConfiguration))
		{
			if (SecretStore::isAvailable() == false)
			{
				QMessageBox::critical(this, tr("Load settings from file"),
									  tr("The file contains legacy encrypted passwords. Loading it requires "
										 "administrator privileges so that they can be moved to the secure "
										 "secret storage."));
				return;
			}
			QStringList replacedSecretKeys;
			const auto result = Configuration::LegacySecretMigration::migrate(
				importedConfiguration, VeyonCore::cryptoCore(),
				Configuration::LegacySecretMigration::ExistingSecrets::Overwrite, &replacedSecretKeys);
			if (replacedSecretKeys.isEmpty() == false)
			{
				QMessageBox::warning(this, tr("Load settings from file"),
									 tr("The following stored secrets have been replaced with the imported values: %1")
										 .arg(replacedSecretKeys.join(QStringLiteral(", "))));
			}
			if (result != Configuration::LegacySecretMigration::Result::Migrated)
			{
				QMessageBox::critical(this, tr("Load settings from file"),
									  tr("Could not migrate the legacy encrypted passwords of the file."));
				return;
			}
		}

		VeyonCore::config() += importedConfiguration;
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

		// write current configuration to output file - with references to secrets only
		Configuration::Object exportedConfiguration(VeyonCore::config());
		(void)Configuration::LegacySecretMigration::removeLegacySecrets(exportedConfiguration);
		Configuration::JsonStore(Configuration::JsonStore::Scope::System, fileName).flush(&exportedConfiguration);

		m_configChanged = configChangedPrevious;
		ui->buttonBox->setEnabled(m_configChanged);
	}
}



void MainWindow::resetConfiguration()
{
	if (QMessageBox::warning(this, tr("Reset configuration"),
							 tr("Do you really want to reset the local configuration and revert "
								"all settings to their defaults? Stored secrets such as the LDAP bind "
								"password are deleted as well."),
							 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ==
		QMessageBox::Yes)
	{
		// like veyon-cli config clear: a reset configuration must not keep old secrets effective
		if (SecretStore::isAvailable())
		{
			for (const auto& secretProperty : Configuration::SecretPropertyRegistry::properties())
			{
				if (SecretStore::remove(secretProperty.secretKey) == false)
				{
					QMessageBox::critical(this, tr("Reset configuration"),
										  tr("Could not remove secret %1.").arg(secretProperty.secretKey));
					return;
				}
			}
		}
		else
		{
			QMessageBox::warning(this, tr("Reset configuration"),
								 tr("Stored secrets have not been removed (requires administrator privileges)."));
		}

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
	ui->configPages->setMinimumSize(ui->scrollArea->width() - ui->scrollArea->verticalScrollBar()->width(),
									ui->configPages->currentWidget()->minimumSizeHint().height());
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

	// hide page selector items for advanced pages
	for (int i = 0; i < ui->pageSelector->count(); ++i)
	{
		auto item = ui->pageSelector->item(i);
		auto page = item->data(Qt::UserRole).value<ConfigurationPage*>();
		if (page)
		{
			const auto flags = Configuration::UiMapping::flags(page);

			ui->pageSelector->setRowHidden(i, flags & Configuration::Property::Flag::Advanced);
			ui->configPages->widget(i)->setVisible(i == ui->pageSelector->currentRow());
			if (flags & Configuration::Property::Flag::Advanced &&
				i == ui->pageSelector->currentRow())
			{
				ui->pageSelector->setCurrentRow(0);
			}
		}
	}
}



void MainWindow::switchToAdvancedView()
{
	const auto widgets = findChildren<QWidget*>();
	for (auto widget : widgets)
	{
		const auto flags = Configuration::UiMapping::flags(widget);
		if (flags & Configuration::Property::Flag::Advanced)
		{
			widget->show();
		}
	}

	for (int i = 0; i < ui->pageSelector->count(); ++i)
	{
		ui->pageSelector->setRowHidden(i, false);
		ui->configPages->widget(i)->setVisible(i == ui->pageSelector->currentRow());
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
