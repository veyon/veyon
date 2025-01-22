/*
 * NetworkObjectDirectoryConfigurationPageTab.h - header for the NetworkObjectDirectoryConfigurationPageTab class
 *
 * Copyright (c) 2021-2025 Tobias Junghans <tobydox@veyon.io>
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

#include <QWidget>

namespace Ui {
class NetworkObjectDirectoryConfigurationPageTab;
}

class QBoxLayout;
class QCheckBox;
class QPushButton;
class ConfigurationPage;
class NetworkObjectDirectoryPluginInterface;

class NetworkObjectDirectoryConfigurationPageTab : public QWidget
{
	Q_OBJECT
public:
	NetworkObjectDirectoryConfigurationPageTab( NetworkObjectDirectoryPluginInterface* plugin, QWidget* parent = nullptr );
	~NetworkObjectDirectoryConfigurationPageTab() override;

	QCheckBox* enabledCheckBox() const;

	NetworkObjectDirectoryPluginInterface* plugin() const
	{
		return m_plugin;
	}

	ConfigurationPage* content() const
	{
		return m_content;
	}

private:
	Ui::NetworkObjectDirectoryConfigurationPageTab *ui;
	NetworkObjectDirectoryPluginInterface* m_plugin;
	ConfigurationPage* m_content;
};
