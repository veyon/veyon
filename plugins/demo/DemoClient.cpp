// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DemoClient.h"
#include "LockWidget.h"
#include "QuickUi.h"
#include "PlatformCoreFunctions.h"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCoreApplication>
#include <QTimer>
#include <utility>
DemoClient::DemoClient(const QString& host, int port, bool fullscreen, QRect viewport, QObject* parent)
	: QObject(parent ? parent : QCoreApplication::instance()),
	m_computerControlInterface(ComputerControlInterface::Pointer::create(Computer({}, host, host), port, this))
{
	connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &DemoClient::cleanup);
	if (fullscreen)
	{
		m_lock = new LockWidget(LockWidget::NoBackground, {}, this);
		if (m_lock->hasQmlContent())
		{
			m_window = m_lock->window();
			m_window->setProperty("demoViewport", viewport);
			m_window->setProperty("demoComputer", QVariant::fromValue(static_cast<QObject*>(m_computerControlInterface.data())));
		}
	}
	else
	{
		m_engine = QuickUi::createEngine(this);
		m_engine->rootContext()->setContextProperty(QStringLiteral("demoConnection"), m_computerControlInterface.data());
		m_engine->rootContext()->setContextProperty(QStringLiteral("demoViewport"), viewport);
		m_engine->load(QUrl(QStringLiteral("qrc:/quick/DemoWindow.qml")));
		if (!m_engine->rootObjects().isEmpty()) m_window = qobject_cast<QWindow*>(m_engine->rootObjects().first());
	}
	if (!m_window)
	{
		vCritical() << "Could not create the demonstration window";
		cleanup();
		QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit);
		return;
	}
	VeyonCore::platform().coreFunctions().raiseWindow(m_window.data(), fullscreen);
	VeyonCore::platform().coreFunctions().disableScreenSaver();
	m_screenSaverDisabled = true;
}
DemoClient::~DemoClient()
{
	cleanup();
}

void DemoClient::cleanup()
{
	delete std::exchange(m_engine, nullptr);
	delete std::exchange(m_lock, nullptr);
	m_window = nullptr;
	m_computerControlInterface.clear();
	if (std::exchange(m_screenSaverDisabled, false))
	VeyonCore::platform().coreFunctions().restoreScreenSaverSettings();
}
