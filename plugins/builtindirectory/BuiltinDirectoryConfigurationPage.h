/*
 * BuiltinDirectoryConfigurationPage.h - header for the BuiltinDirectoryConfigurationPage class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include "ConfigurationPage.h"
#include "NetworkObject.h"

namespace Ui {
class BuiltinDirectoryConfigurationPage;
}

class BuiltinDirectoryConfiguration;

class BuiltinDirectoryConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	BuiltinDirectoryConfigurationPage( BuiltinDirectoryConfiguration& configuration, QWidget* parent = nullptr );
	~BuiltinDirectoryConfigurationPage();

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

private slots:
	void addLocation();
	void updateLocation();
	void removeLocation();
	void addComputer();
	void updateComputer();
	void removeComputer();

private:
	void populateLocations();
	void populateComputers();

	NetworkObject currentLocationObject() const;
	NetworkObject currentComputerObject() const;

	Ui::BuiltinDirectoryConfigurationPage *ui;

	BuiltinDirectoryConfiguration& m_configuration;

};
