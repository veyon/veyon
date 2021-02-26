/*
 * WebApiConfigurationPage.cpp - implementation of WebApiConfigurationPage
 *
 * Copyright (c) 2020-2021 Tobias Junghans <tobydox@veyon.io>
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
#include "WebApiConfiguration.h"
#include "WebApiConfigurationPage.h"
#include "Configuration/UiMapping.h"

#include "ui_WebApiConfigurationPage.h"

WebApiConfigurationPage::WebApiConfigurationPage( WebApiConfiguration& configuration, QWidget* parent ) :
	ConfigurationPage( parent ),
	ui( new Ui::WebApiConfigurationPage ),
	m_configuration( configuration )
{
	ui->setupUi(this);

	connect( ui->browseTlsCertificateFile, &QAbstractButton::clicked, this, [this]() {
		FileSystemBrowser( FileSystemBrowser::ExistingFile ).exec( ui->tlsCertificateFile );
	} );

	connect( ui->browseTlsPrivateKeyFile, &QAbstractButton::clicked, this, [this]() {
		FileSystemBrowser( FileSystemBrowser::ExistingFile ).exec( ui->tlsPrivateKeyFile );
	} );

	Configuration::UiMapping::setFlags( this, Configuration::Property::Flag::Advanced );
}



WebApiConfigurationPage::~WebApiConfigurationPage()
{
	delete ui;
}



void WebApiConfigurationPage::resetWidgets()
{
	FOREACH_HTTP_API_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
}



void WebApiConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_HTTP_API_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY)
}



void WebApiConfigurationPage::applyConfiguration()
{
}
