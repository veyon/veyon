// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include <QIcon>

#include "DemoClient.h"
#include "LockWidget.h"
#include "PlatformCoreFunctions.h"
#include "VncViewWidget.h"


DemoClient::DemoClient(const QString& host, int port, bool fullscreen, QRect viewport, QObject* parent) :
	QObject(parent),
	m_computerControlInterface(ComputerControlInterface::Pointer::create(Computer({}, host, host), port, this))
{
	if (fullscreen)
	{
		m_toplevel = new LockWidget(LockWidget::NoBackground);
	}
	else
	{
		m_toplevel = new QWidget();
		m_toplevel->setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint);
		m_toplevel->move(0, 0);
	}

	m_toplevel->setWindowTitle(tr("Veyon Demo"));
	m_toplevel->setWindowIcon(QPixmap(QStringLiteral(":/core/icon64.png")));
	m_toplevel->setAttribute(Qt::WA_DeleteOnClose, false);
	m_toplevel->installEventFilter(this);

	m_vncView = new VncViewWidget(m_computerControlInterface, viewport, m_toplevel);

	connect(m_toplevel, &QObject::destroyed, this, &DemoClient::viewDestroyed);
	connect(m_vncView, &VncViewWidget::sizeHintChanged, this, &DemoClient::resizeToplevelWidget);

	if (fullscreen == false)
	{
		m_toplevel->show();
	}

	resizeToplevelWidget();

	VeyonCore::platform().coreFunctions().raiseWindow(m_toplevel, fullscreen);

	VeyonCore::platform().coreFunctions().disableScreenSaver();
}



DemoClient::~DemoClient()
{
	VeyonCore::platform().coreFunctions().restoreScreenSaverSettings();

	delete m_toplevel;
}



bool DemoClient::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == m_toplevel && event->type() == QEvent::Resize)
	{
		m_vncView->setFixedSize(m_toplevel->size());
		return true;
	}

	return QObject::eventFilter(watched, event);
}



void DemoClient::viewDestroyed(QObject* obj)
{
	// prevent double deletion of toplevel widget
	if (m_toplevel == obj)
	{
		m_toplevel = nullptr;
	}

	deleteLater();
}



void DemoClient::resizeToplevelWidget()
{
	if (m_toplevel->windowState() & Qt::WindowFullScreen)
	{
		m_vncView->setFixedSize(m_toplevel->size());
	}
	else
	{
		m_toplevel->resize(m_vncView->sizeHint());
	}
}
