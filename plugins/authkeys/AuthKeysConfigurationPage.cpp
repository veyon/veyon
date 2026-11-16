// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "AuthKeysConfigurationPage.h"
#include "AuthKeysManager.h"
#include "Configuration/UiMapping.h"
#include "FileSystemBrowser.h"
#include "UserGroupsBackendManager.h"
#include "VeyonConfiguration.h"

#include "ui_AuthKeysConfigurationPage.h"


AuthKeysConfigurationPage::AuthKeysConfigurationPage() :
	ConfigurationPage(),
	ui(new Ui::AuthKeysConfigurationPage),
	m_authKeyTableModel(this),
	m_keyFilesFilter(tr("Key files (*.pem)"))
{
	ui->setupUi(this);

	Configuration::UiMapping::setFlags(ui->keyFileDirectories, Configuration::Property::Flag::Advanced);

#define CONNECT_BUTTON_SLOT(name) \
	connect(ui->name, &QAbstractButton::clicked, this, &AuthKeysConfigurationPage::name);

	CONNECT_BUTTON_SLOT(openPublicKeyBaseDir);
	CONNECT_BUTTON_SLOT(openPrivateKeyBaseDir);
	CONNECT_BUTTON_SLOT(createKeyPair);
	CONNECT_BUTTON_SLOT(deleteKey);
	CONNECT_BUTTON_SLOT(importKey);
	CONNECT_BUTTON_SLOT(exportKey);
	CONNECT_BUTTON_SLOT(setAccessGroup);

	reloadKeyTable();

	ui->keyTable->setModel(&m_authKeyTableModel);
}


AuthKeysConfigurationPage::~AuthKeysConfigurationPage()
{
	delete ui;
}



void AuthKeysConfigurationPage::resetWidgets()
{
	FOREACH_VEYON_KEY_AUTHENTICATION_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);

	reloadKeyTable();
}



void AuthKeysConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_VEYON_KEY_AUTHENTICATION_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



void AuthKeysConfigurationPage::applyConfiguration()
{
}



void AuthKeysConfigurationPage::openPublicKeyBaseDir()
{
	FileSystemBrowser(FileSystemBrowser::ExistingDirectory, this).exec(ui->publicKeyBaseDir);
}



void AuthKeysConfigurationPage::openPrivateKeyBaseDir()
{
	FileSystemBrowser(FileSystemBrowser::ExistingDirectory, this).exec(ui->privateKeyBaseDir);
}



void AuthKeysConfigurationPage::createKeyPair()
{
	const auto keyName = QInputDialog::getText(this, tr("Authentication key name"),
											   tr("Please enter the name of the user group or role for which to create an authentication key pair:"));
	if (keyName.isEmpty() == false)
	{
		AuthKeysManager authKeysManager;
		const auto success = authKeysManager.createKeyPair(keyName);

		showResultMessage(success, tr("Create key pair"), authKeysManager.resultMessage());

		reloadKeyTable();
	}
}



void AuthKeysConfigurationPage::deleteKey()
{
	const auto title = ui->deleteKey->text();

	const auto nameAndType = selectedKey().split(QLatin1Char('/'));

	if (nameAndType.size() > 1)
	{
		const auto name = nameAndType[0];
		const auto type = nameAndType[1];

		if (QMessageBox::question(this, title, tr("Do you really want to delete authentication key \"%1/%2\"?").arg(name, type)) ==
			QMessageBox::Yes)
		{
			AuthKeysManager authKeysManager;
			const auto success = authKeysManager.deleteKey(name, type);

			showResultMessage(success, title, authKeysManager.resultMessage());

			reloadKeyTable();
		}
	}
	else
	{
		showResultMessage(false, title, tr("Please select a key to delete!"));
	}
}



void AuthKeysConfigurationPage::importKey()
{
	const auto title = ui->importKey->text();

	const auto inputFile = QFileDialog::getOpenFileName(this, title, {}, m_keyFilesFilter);
	if (inputFile.isEmpty())
	{
		return;
	}

	auto keyName = AuthKeysManager::keyNameFromExportedKeyFile(inputFile);
	if (keyName.isEmpty())
	{
		keyName = QInputDialog::getText(this, tr("Authentication key name"),
										tr("Please enter the name of the user group or role for which to import the authentication key.\n\nMake sure that the names of the keys belonging to each other are identical on all computers."),
										QLineEdit::Normal);
	}

	if (keyName.isEmpty())
	{
		return;
	}

	AuthKeysManager authKeysManager;
	const auto keyType = authKeysManager.detectKeyType(inputFile);
	const auto success = authKeysManager.importKey(keyName, keyType, inputFile);

	showResultMessage(success, title, authKeysManager.resultMessage());

	reloadKeyTable();
}



void AuthKeysConfigurationPage::exportKey()
{
	const auto title = ui->exportKey->text();

	const auto nameAndType = selectedKey().split(QLatin1Char('/'));

	if (nameAndType.size() > 1)
	{
		const auto name = nameAndType[0];
		const auto type = nameAndType[1];

		const auto outputFile = QFileDialog::getSaveFileName(this, title, QDir::homePath() + QDir::separator() + AuthKeysManager::exportedKeyFileName(name, type),
															 m_keyFilesFilter);
		if (outputFile.isEmpty() == false)
		{
			AuthKeysManager authKeysManager;
			const auto success = authKeysManager.exportKey(name, type, outputFile, true);

			showResultMessage(success, title, authKeysManager.resultMessage());
		}
	}
	else
	{
		showResultMessage(false, title, tr("Please select a key to export!"));
	}
}



void AuthKeysConfigurationPage::setAccessGroup()
{
	const auto title = ui->setAccessGroup->text();

	const auto key = selectedKey();

	if (key.isEmpty() == false)
	{
		const auto userGroups = VeyonCore::userGroupsBackendManager().configuredBackend()->userGroups(VeyonCore::config().useDomainUserGroups());
		const auto currentGroup = AuthKeysManager().accessGroup(key);

		bool ok = false;
		const auto selectedGroup = QInputDialog::getItem(this, title,
														 tr("Please select a user group which to grant access to key \"%1\":").arg(key),
														 userGroups, userGroups.indexOf(currentGroup), true, &ok);

		if (ok && selectedGroup.isEmpty() == false)
		{
			AuthKeysManager manager;
			const auto success = manager.setAccessGroup(key, selectedGroup);

			showResultMessage(success, title, manager.resultMessage());

			reloadKeyTable();
		}
	}
	else
	{
		showResultMessage(false, title, tr("Please select a key which to set the access group for!"));
	}
}



void AuthKeysConfigurationPage::reloadKeyTable()
{
	m_authKeyTableModel.reload();
	ui->keyTable->resizeColumnsToContents();
}



QString AuthKeysConfigurationPage::selectedKey() const
{
	const auto row = ui->keyTable->currentIndex().row();
	if (row >= 0 && row < m_authKeyTableModel.rowCount())
	{
		return m_authKeyTableModel.key(row);
	}

	return {};
}



void AuthKeysConfigurationPage::showResultMessage(bool success, const QString& title, const QString& message)
{
	if (message.isEmpty())
	{
		return;
	}

	if (success)
	{
		QMessageBox::information(this, title, message);
	}
	else
	{
		QMessageBox::critical(this, title, message);
	}
}
