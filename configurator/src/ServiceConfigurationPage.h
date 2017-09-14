/*
 * ServiceConfigurationPage.h - header for the ServiceConfigurationPage class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#ifndef SERVICE_CONFIGURATION_PAGE_H
#define SERVICE_CONFIGURATION_PAGE_H

#include <QMap>

#include "ConfigurationPage.h"
#include "Plugin.h"

namespace Ui {
class ServiceConfigurationPage;
}

class VncServerPluginInterface;

class ServiceConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	ServiceConfigurationPage();
	~ServiceConfigurationPage() override;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

public slots:
	void startService();
	void stopService();


private slots:
	void updateServiceControl();
	void updateVncServerPluginConfigurationWidget();

private:
	void populateVncServerPluginComboBox();

	Ui::ServiceConfigurationPage *ui;

	QMap<Plugin::Uid, VncServerPluginInterface*> m_vncServerPluginInterfaces;
	QWidget* m_vncServerPluginConfigurationWidget;

};

#endif // SERVICE_CONFIGURATION_PAGE_H
