// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

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
	explicit SlideshowPanel(UserConfig& config, ComputerMonitoringWidget* computerMonitoringWidget,
							QWidget* parent = nullptr);
	~SlideshowPanel() override;

	static constexpr auto DefaultDuration = 3000;

protected:
	void resizeEvent(QResizeEvent* event) override;

private:
	void updateDuration();
	void switchViewMode();

	Ui::SlideshowPanel* ui;

	UserConfig& m_config;
	SlideshowModel* m_model;
	QWidget* m_originalParent = nullptr;
};
