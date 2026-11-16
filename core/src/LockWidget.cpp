// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include <QPainter>
#include <QScreen>
#include <QWindow>

#include "LockWidget.h"
#include "PlatformCoreFunctions.h"
#include "PlatformInputDeviceFunctions.h"


LockWidget::LockWidget(Mode mode, const QPixmap& background, QWidget* parent) :
	QWidget(parent, Qt::X11BypassWindowManagerHint),
	m_background(background),
	m_mode(mode)
{
	auto leftMostScreen = QGuiApplication::primaryScreen();
	int minimumX = 0;
	const auto screens = QGuiApplication::screens();
	for (auto* screen : screens)
	{
		if (screen->geometry().x() < minimumX)
		{
			minimumX = screen->geometry().x();
			leftMostScreen = screen;
		}
	}

	if (mode == DesktopVisible)
	{
		m_background = leftMostScreen->grabWindow(0);
	}

	VeyonCore::platform().coreFunctions().setSystemUiState(false);
	VeyonCore::platform().inputDeviceFunctions().disableInputDevices();

	setWindowTitle({});

#ifdef Q_OS_LINUX
	show();
#endif
	move(leftMostScreen->geometry().topLeft());
#ifndef Q_OS_LINUX
	showFullScreen();
#endif
	windowHandle()->setScreen(leftMostScreen);
	setFixedSize(leftMostScreen->virtualSize());

	VeyonCore::platform().coreFunctions().raiseWindow(this, true);
#ifdef Q_OS_LINUX
	showFullScreen();
#endif

	setFocusPolicy(Qt::StrongFocus);
	setFocus();
	grabMouse();
	grabKeyboard();
	setCursor(Qt::BlankCursor);
	QGuiApplication::setOverrideCursor(Qt::BlankCursor);

	QCursor::setPos(mapToGlobal(QPoint(0, 0)));
}



LockWidget::~LockWidget()
{
	VeyonCore::platform().inputDeviceFunctions().enableInputDevices();
	VeyonCore::platform().coreFunctions().setSystemUiState(true);

	QGuiApplication::restoreOverrideCursor();
}



void LockWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter p(this);
	switch (m_mode)
	{
	case DesktopVisible:
		p.drawPixmap(0, 0, m_background);
		break;

	case BackgroundPixmap:
		p.fillRect(rect(), QColor(64, 64, 64));
		p.drawPixmap((width() - m_background.width()) / 2,
					 (height() - m_background.height()) / 2,
					 m_background);
		break;

	default:
		break;
	}
}
