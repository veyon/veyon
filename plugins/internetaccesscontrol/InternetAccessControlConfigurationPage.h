/*
 * InternetAccessControlConfigurationPage.h - header for the InternetAccessControlConfigurationPage class
 *
 * Copyright (c) 2016-2018 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
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

#ifndef INTERNET_ACCESS_CONTROL_CONFIGURATION_PAGE_H
#define INTERNET_ACCESS_CONTROL_CONFIGURATION_PAGE_H

#include "ConfigurationPage.h"
#include "Plugin.h"

class InternetAccessControlBackendInterface;
class InternetAccessControlConfiguration;

namespace Ui {
class InternetAccessControlConfigurationPage;
}

class InternetAccessControlConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	InternetAccessControlConfigurationPage( InternetAccessControlConfiguration& configuration, QWidget* parent = nullptr );
	~InternetAccessControlConfigurationPage() override;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

private slots:
	void updateBackendConfigurationWidget();

private:
	void populateBackends();

	Ui::InternetAccessControlConfigurationPage *ui;

	InternetAccessControlConfiguration& m_configuration;

	QMap<Plugin::Uid, InternetAccessControlBackendInterface*> m_backendInterfaces;
	QWidget* m_backendConfigurationWidget;

};

#endif // INTERNET_ACCESS_CONTROL_CONFIGURATION_PAGE_H
