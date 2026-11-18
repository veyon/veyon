// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <QObject>
#include <QPointer>
#include <QWindow>
#include "ComputerControlInterface.h"
class LockWidget;
class QQmlApplicationEngine;
class DemoClient : public QObject
{
	Q_OBJECT
public:
	DemoClient(const QString& host, int port, bool fullscreen, QRect viewport, QObject* parent = nullptr);
	~DemoClient() override;
private:
	void cleanup();
	ComputerControlInterface::Pointer m_computerControlInterface;
	LockWidget* m_lock = nullptr;
	QQmlApplicationEngine* m_engine = nullptr;
	QPointer<QWindow> m_window;
	bool m_screenSaverDisabled{false};
};
