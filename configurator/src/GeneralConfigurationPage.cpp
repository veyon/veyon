// SPDX-FileCopyrightText: 2016-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDir>
#include <QMessageBox>

#include "AuthenticationCredentials.h"
#include "Configuration/UiMapping.h"
#include "FileSystemBrowser.h"
#include "Filesystem.h"
#include "GeneralConfigurationPage.h"
#include "NetworkObjectDirectoryManager.h"
#include "PlatformFilesystemFunctions.h"
#include "PlatformUserFunctions.h"
#include "PluginManager.h"
#include "UserGroupsBackendManager.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"
#include "VeyonServiceControl.h"

#include "ui_GeneralConfigurationPage.h"


GeneralConfigurationPage::GeneralConfigurationPage() :
	ConfigurationPage(),
	ui(new Ui::GeneralConfigurationPage)
{
	ui->setupUi(this);

	// retrieve list of builtin translations and populate language combobox
	QStringList languages;
	const auto qmFiles = QDir(VeyonCore::translationsDirectory()).entryList({QStringLiteral("veyon*.qm")});

	languages.reserve(qmFiles.count());

	for (const auto& qmFile : qmFiles)
	{
		QLocale loc(qmFile.split(QLatin1Char('_')).mid(1).join(QLatin1Char('_')));
		if (loc.language() == QLocale::C)
		{
			loc = QLocale(QLocale::English);
		}
		languages += QStringLiteral("%1 - %2 (%3)").arg(QLocale::languageToString(loc.language()), loc.nativeLanguageName(), loc.name());
	}

	std::sort(languages.begin(), languages.end());

	ui->uiLanguage->addItems(languages);

	const auto backends = VeyonCore::userGroupsBackendManager().availableBackends();
	if (backends.count() <= 0)
	{
		QMessageBox::critical(this,
							  tr("Missing user groups backend"),
							  tr("No user groups plugin was found. Please check your installation!"));
		qFatal("GeneralConfigurationPage: missing user groups backend");
	}

	for (auto it = backends.constBegin(), end = backends.constEnd(); it != end; ++it)
	{
		ui->userGroupsBackend->addItem(it.value(), it.key());
	}

	connect(ui->testAuthenticationButton, &QPushButton::clicked, this, &GeneralConfigurationPage::testAuthentication);
	connect(ui->openLogFileDirectory, &QPushButton::clicked, this, &GeneralConfigurationPage::openLogFileDirectory);
	connect(ui->clearLogFiles, &QPushButton::clicked, this, &GeneralConfigurationPage::clearLogFiles);
	connect(ui->logFileDirectory, &QLineEdit::textChanged, this, &GeneralConfigurationPage::updateLogFileLocations);
	connect(ui->userGroupsBackend, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
		const auto plugin = VeyonCore::pluginManager().pluginInterface(ui->userGroupsBackend->currentData().toUuid());
		ui->useDomainUserGroups->setEnabled(plugin && plugin->flags().testFlag(Plugin::ProvidesDefaultImplementation));
	});

	populateNetworkObjectDirectories();
}



GeneralConfigurationPage::~GeneralConfigurationPage()
{
	delete ui;
}



void GeneralConfigurationPage::resetWidgets()
{
	FOREACH_VEYON_UI_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_VEYON_LOGGING_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_VEYON_NETWORK_OBJECT_DIRECTORY_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_VEYON_USER_GROUPS_BACKEND_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_VEYON_AUTHENTICATION_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);

	updateLogFileLocations();
}



void GeneralConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_VEYON_UI_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_VEYON_LOGGING_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_VEYON_NETWORK_OBJECT_DIRECTORY_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_VEYON_USER_GROUPS_BACKEND_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_VEYON_AUTHENTICATION_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



void GeneralConfigurationPage::applyConfiguration()
{
	VeyonCore::userGroupsBackendManager().reloadConfiguration();
}



void GeneralConfigurationPage::testAuthentication()
{
	switch (VeyonCore::config().authenticationMethod())
	{
	case VeyonCore::AuthenticationMethod::LogonAuthentication:
		if (testLogonAuthentication() == false)
		{
			return;
		}
		break;
	case VeyonCore::AuthenticationMethod::KeyFileAuthentication:
		if (testKeyFileAuthentication() == false)
		{
			return;
		}
		break;
	}

	QMessageBox::information(this, authenticationTestTitle(),
							 tr("Authentication is set up properly on this computer."));
}



bool GeneralConfigurationPage::testLogonAuthentication()
{
	return VeyonCore::instance()->initAuthentication() &&
		VeyonCore::platform().userFunctions().authenticate(VeyonCore::authenticationCredentials().logonUsername(),
														   VeyonCore::authenticationCredentials().logonPassword());
}



bool GeneralConfigurationPage::testKeyFileAuthentication()
{
	if (VeyonCore::instance()->initAuthentication() == false)
	{
		QMessageBox::critical(this, authenticationTestTitle(),
							  tr("Authentication keys are not set up properly on this computer."));
		return false;
	}

	return true;
}



void GeneralConfigurationPage::openLogFileDirectory()
{
	FileSystemBrowser(FileSystemBrowser::ExistingDirectory, this).exec(ui->logFileDirectory);
}



void GeneralConfigurationPage::updateLogFileLocations()
{
	const auto& filesystemFunctions = VeyonCore::platform().filesystemFunctions();
	const auto baseDirectory = VeyonCore::filesystem().expandPath(ui->logFileDirectory->text());

	ui->logFileLocations->setText(
		tr("Log files of the Veyon Service and other privileged processes are written to %1, log files of "
		   "processes of logged on users to %2.")
			.arg(QDir::toNativeSeparators(filesystemFunctions.systemLogFileDirectory()),
				 QDir::toNativeSeparators(filesystemFunctions.userLogFileDirectory(baseDirectory, AllUsersPattern))));
}



void GeneralConfigurationPage::clearLogFiles()
{
	bool serviceStopped = false;

	VeyonServiceControl serviceControl(this);

	if (serviceControl.isServiceRunning())
	{
		if (QMessageBox::question(this, tr("Veyon service"),
								  tr("The Veyon service needs to be stopped temporarily "
									 "in order to remove the log files. Continue?"),
								  QMessageBox::Yes | QMessageBox::No,
								  QMessageBox::Yes) == QMessageBox::Yes)
		{
			serviceControl.stopService();
			serviceStopped = true;
		}
		else
		{
			return;
		}
	}

	bool success = true;
	const QStringList logFilesFilter({QStringLiteral("Veyon*.log"), QStringLiteral("Veyon*.log.*")});

	for (const auto& directory : logFileDirectories())
	{
		QDir logFileDirectory(directory);
		const auto logFileNames = logFileDirectory.entryList(logFilesFilter, QDir::Files | QDir::System);

		for (const auto& logFileName : logFileNames)
		{
			// the log file of this process is still open
			if (logFileName.startsWith(QLatin1String("VeyonConfigurator")))
			{
				logFileDirectory.remove(logFileName);
			}
			else
			{
				success &= logFileDirectory.remove(logFileName);
			}
		}
	}

	if (serviceStopped)
	{
		serviceControl.startService();
	}

	if (success)
	{
		QMessageBox::information(this, tr("Log files cleared"),
								 tr("All log files were cleared successfully."));
	}
	else
	{
		QMessageBox::critical(this, tr("Error"),
							  tr("Could not remove all log files."));
	}
}



QStringList GeneralConfigurationPage::logFileDirectories()
{
	const auto& filesystemFunctions = VeyonCore::platform().filesystemFunctions();
	const auto baseDirectory = VeyonCore::filesystem().expandPath(VeyonCore::config().logFileDirectory());

	QStringList directories{filesystemFunctions.systemLogFileDirectory()};

	// a pattern if there's one directory per user (Linux), otherwise the directory itself
	const QFileInfo userDirectories(filesystemFunctions.userLogFileDirectory(baseDirectory, AllUsersPattern));
	if (userDirectories.fileName().contains(AllUsersPattern))
	{
		const auto parentDirectory = userDirectories.dir();
		const auto names = parentDirectory.entryList({userDirectories.fileName()},
													 QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
		for (const auto& name : names)
		{
			directories.append(parentDirectory.filePath(name));
		}
	}
	else
	{
		directories.append(userDirectories.filePath());
	}

	// shared locations used by earlier versions
	directories.append(baseDirectory);
	directories.append(filesystemFunctions.globalTempPath());

	directories.removeDuplicates();

	return directories;
}



void GeneralConfigurationPage::populateNetworkObjectDirectories()
{
	const auto directories = VeyonCore::networkObjectDirectoryManager().availableDirectories();

	for (auto it = directories.constBegin(), end = directories.constEnd(); it != end; ++it)
	{
		ui->networkObjectDirectoryPlugin->addItem(it.value(), it.key());
	}
}
