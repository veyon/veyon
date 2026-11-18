/*
 *  LockWidget.cpp - widget for locking a client
 *
 *  Copyright (c) 2006-2026 Tobias Junghans <tobydox@veyon.io>
 *
 *  This file is part of Veyon - https://veyon.io
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include "LockWidget.h"
#include "QuickUi.h"
#include "PlatformCoreFunctions.h"
#include "PlatformInputDeviceFunctions.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickImageProvider>
#include <QQuickWindow>
#include <QScreen>
#include <utility>
namespace
{
class LockImageProvider final : public QQuickImageProvider
{
public:
	explicit LockImageProvider(QImage image) : QQuickImageProvider(Image), m_image(std::move(image)) {}
	QImage requestImage(const QString&, QSize* size, const QSize&) override
	{
		if (size) *size = m_image.size();
		return m_image;
		}
private:
	QImage m_image;
};
}
LockWidget::LockWidget(Mode mode, const QPixmap& background, QObject* parent) : QObject(parent), m_mode(mode)
{
	connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &LockWidget::cleanup);
	auto screen = QGuiApplication::primaryScreen();
	if (!screen)
	{
		vCritical() << "Could not lock the desktop: no screen is available";
		return;
	}
	for (auto candidate : QGuiApplication::screens())
		if (candidate->geometry().x() < screen->geometry().x()) screen = candidate;
	m_engine = QuickUi::createEngine(this);
	m_engine->addImageProvider(QStringLiteral("lockBackground"), new LockImageProvider(
		mode == DesktopVisible ? screen->grabWindow(0).toImage() : background.toImage()));
	m_engine->rootContext()->setContextProperty(QStringLiteral("lockState"), this);
	m_engine->load(QUrl(QStringLiteral("qrc:/quick/LockWindow.qml")));
	if (!m_engine->rootObjects().isEmpty()) m_window = qobject_cast<QWindow*>(m_engine->rootObjects().first());
	m_hasQmlContent = !m_window.isNull();
	if (!m_window)
	{
		auto fallback = new QQuickWindow;
		fallback->setColor(Qt::black);
		fallback->QObject::setParent(m_engine);
		m_window = fallback;
	}
	auto flags = Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint;
#ifdef Q_OS_LINUX
	flags |= Qt::X11BypassWindowManagerHint;
#endif
	m_window->setFlags(flags);
	m_window->setScreen(screen);
	const auto desktopGeometry = screen->virtualGeometry();
	m_window->setGeometry(desktopGeometry);
	VeyonCore::platform().coreFunctions().setSystemUiState(false);
	VeyonCore::platform().inputDeviceFunctions().disableInputDevices();
	m_locked = true;
	m_window->show();
	VeyonCore::platform().coreFunctions().raiseWindow(m_window.data(), true);
	m_window->showFullScreen();
	m_window->setMinimumSize(desktopGeometry.size());
	m_window->setMaximumSize(desktopGeometry.size());
	m_window->setGeometry(desktopGeometry);
	m_window->setMouseGrabEnabled(true);
	m_window->setKeyboardGrabEnabled(true);
	m_window->setCursor(QCursor(Qt::BlankCursor));
	QGuiApplication::setOverrideCursor(Qt::BlankCursor);
}
LockWidget::~LockWidget()
{
	cleanup();
}

void LockWidget::cleanup()
{
	if (m_window)
	{
		m_window->setMouseGrabEnabled(false);
		m_window->setKeyboardGrabEnabled(false);
	}
	delete std::exchange(m_engine, nullptr);
	m_window = nullptr;
	m_hasQmlContent = false;
	if (std::exchange(m_locked, false))
	{
		VeyonCore::platform().inputDeviceFunctions().enableInputDevices();
		VeyonCore::platform().coreFunctions().setSystemUiState(true);
		QGuiApplication::restoreOverrideCursor();
	}
}
