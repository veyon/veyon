/*
 * SpotlightPanel.h - declaration of SpotlightPanel
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

#pragma once

#include <QWidget>

class ComputerMonitoringWidget;
class SpotlightModel;
class UserConfig;

namespace Ui {
class SpotlightPanel;
}

class SpotlightPanel : public QWidget
{
	Q_OBJECT
public:
	explicit SpotlightPanel( UserConfig& config, ComputerMonitoringWidget* computerMonitoringWidget,
							 QWidget* parent = nullptr );
	~SpotlightPanel() override;

protected:
	void resizeEvent( QResizeEvent* event ) override;

private:
	void add();
	void remove();
	void setRealtimeView( bool enabled );
	void updateIconSize();

	void addPressedItem( const QModelIndex& index );
	void removePressedItem( const QModelIndex& index );

	Ui::SpotlightPanel* ui;

	UserConfig& m_config;

	ComputerMonitoringWidget* m_globalComputerMonitoringWidget;
	SpotlightModel* m_model;

} ;
