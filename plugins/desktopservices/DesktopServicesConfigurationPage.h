/*
 * DesktopServicesConfigurationPage.h - header for the DesktopServicesConfigurationPage class
 *
 * Copyright (c) 2018-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef DESKTOP_SERVICES_CONFIGURATION_PAGE_H
#define DESKTOP_SERVICES_CONFIGURATION_PAGE_H

#include "ConfigurationPage.h"
#include "DesktopServiceObject.h"

class QTableWidget;
class DesktopServicesConfiguration;

namespace Ui {
class DesktopServicesConfigurationPage;
}

class DesktopServicesConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	DesktopServicesConfigurationPage( DesktopServicesConfiguration& configuration, QWidget* parent = nullptr );
	~DesktopServicesConfigurationPage() override;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

private slots:
	void addProgram();
	void updateProgram();
	void removeProgram();
	void addWebsite();
	void updateWebsite();
	void removeWebsite();

private:
	static void addServiceObject( QTableWidget* tableWidget, DesktopServiceObject::Type type, const QString& name, QJsonArray& objects );
	static void updateServiceObject( QTableWidget* tableWidget, DesktopServiceObject::Type type, QJsonArray& objects );
	static void removeServiceObject( QTableWidget* tableWidget, DesktopServiceObject::Type type, QJsonArray& objects );

	static DesktopServiceObject currentServiceObject( QTableWidget* tableWidget, DesktopServiceObject::Type type );

	static void loadObjects( const QJsonArray& objects, QTableWidget* tableWidget );

	Ui::DesktopServicesConfigurationPage *ui;

	DesktopServicesConfiguration& m_configuration;

};

#endif // DESKTOP_SERVICES_CONFIGURATION_PAGE_H
