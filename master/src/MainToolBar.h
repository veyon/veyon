// SPDX-FileCopyrightText: 2007-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QToolBar>

class MainWindow;

class MainToolBar : public QToolBar
{
	Q_OBJECT
public:
	explicit MainToolBar(QWidget* parent);
	~MainToolBar() override = default;

protected:
	bool event(QEvent* event) override;

private:
	void setExpanded();
	void updateMinimumSize();
	void toggleToolTips();
	void toggleIconMode();

	void contextMenuEvent(QContextMenuEvent* event) override;

	MainWindow* m_mainWindow;
	QLayout* m_layout;
};
