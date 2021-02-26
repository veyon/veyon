/*
 * FileTransferConfigurationPage.cpp - implementation of FileTransferConfigurationPage
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "FileSystemBrowser.h"
#include "FileTransferConfiguration.h"
#include "FileTransferConfigurationPage.h"
#include "Configuration/UiMapping.h"

#include "ui_FileTransferConfigurationPage.h"

FileTransferConfigurationPage::FileTransferConfigurationPage( FileTransferConfiguration& configuration, QWidget* parent ) :
	ConfigurationPage( parent ),
	ui( new Ui::FileTransferConfigurationPage ),
	m_configuration( configuration )
{
	ui->setupUi(this);

	connect( ui->browseDefaultSourceDirectory, &QAbstractButton::clicked,
			 this, &FileTransferConfigurationPage::browseDefaultSourceDirectory );

	connect( ui->browseDestinationDirectory, &QAbstractButton::clicked,
			 this, &FileTransferConfigurationPage::browseDestinationDirectory );

	Configuration::UiMapping::setFlags( this, Configuration::Property::Flag::Advanced );
}



FileTransferConfigurationPage::~FileTransferConfigurationPage()
{
	delete ui;
}



void FileTransferConfigurationPage::resetWidgets()
{
	FOREACH_FILE_TRANSFER_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
}



void FileTransferConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_FILE_TRANSFER_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY)
}



void FileTransferConfigurationPage::applyConfiguration()
{
}


void FileTransferConfigurationPage::browseDefaultSourceDirectory()
{
	FileSystemBrowser( FileSystemBrowser::ExistingDirectory ).exec( ui->fileTransferDefaultSourceDirectory );
}



void FileTransferConfigurationPage::browseDestinationDirectory()
{
	FileSystemBrowser( FileSystemBrowser::ExistingDirectory ).exec( ui->fileTransferDestinationDirectory );
}
