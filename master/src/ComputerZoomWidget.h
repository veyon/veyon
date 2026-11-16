// SPDX-FileCopyrightText: 2021-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QWidget>

#include "ComputerControlInterface.h"

class VncViewWidget;

// clazy:excludeall=ctor-missing-parent-argument
class ComputerZoomWidget : public QWidget
{
	Q_OBJECT
public:
	ComputerZoomWidget(const ComputerControlInterface::Pointer& computerControlInterface);
	~ComputerZoomWidget() override;

protected:
	bool eventFilter(QObject* object, QEvent* event) override;

	void resizeEvent(QResizeEvent* event) override;
	void closeEvent(QCloseEvent* event) override;

private:
	void updateSize();
	void updateComputerZoomWidgetTitle();

	int m_currentScreen{-1};

	VncViewWidget* m_vncView;

Q_SIGNALS:
	void keypressInComputerZoomWidget();
};
