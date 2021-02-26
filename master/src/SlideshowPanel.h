/*
 * SlideshowPanel.h - declaration of SlideshowPanel
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
class SlideshowModel;
class UserConfig;

namespace Ui {
class SlideshowPanel;
}

class SlideshowPanel : public QWidget
{
	Q_OBJECT
public:
	explicit SlideshowPanel( UserConfig& config, ComputerMonitoringWidget* computerMonitoringWidget,
							 QWidget* parent = nullptr );
	~SlideshowPanel() override;

	static constexpr auto DefaultDuration = 3000;

protected:
	void resizeEvent( QResizeEvent* event ) override;

private:
	void updateDuration();

	Ui::SlideshowPanel* ui;

	UserConfig& m_config;
	SlideshowModel* m_model;

} ;
