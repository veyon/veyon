// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

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
	explicit SpotlightPanel(UserConfig& config, ComputerMonitoringWidget* computerMonitoringWidget,
							QWidget* parent = nullptr);
	~SpotlightPanel() override;

protected:
	void resizeEvent(QResizeEvent* event) override;

private:
	void add();
	void remove();
	void setRealtimeView(bool enabled);
	void updateIconSize();

	void addPressedItem(const QModelIndex& index);
	void removePressedItem(const QModelIndex& index);

	Ui::SpotlightPanel* ui;

	UserConfig& m_config;

	ComputerMonitoringWidget* m_globalComputerMonitoringWidget;
	SpotlightModel* m_model;
};
