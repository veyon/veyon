// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>

#include "ComputerControlInterface.h"

class VncViewWidget;

class DemoClient : public QObject
{
	Q_OBJECT
public:
	DemoClient(const QString& host, int port, bool fullscreen, QRect viewport, QObject* parent = nullptr);
	~DemoClient() override;

protected:
	bool eventFilter(QObject* watched, QEvent* event) override;

private:
	void viewDestroyed(QObject* obj);
	void resizeToplevelWidget();

	QWidget* m_toplevel{nullptr};

	ComputerControlInterface::Pointer m_computerControlInterface;
	VncViewWidget* m_vncView{nullptr};
};
