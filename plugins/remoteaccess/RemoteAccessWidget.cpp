// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QActionGroup>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include "RemoteAccessWidget.h"
#include "DesktopIcons.h"
#include "VncViewWidget.h"
#include "VeyonConfiguration.h"
#include "VeyonConnection.h"
#include "VeyonMasterInterface.h"
#include "PlatformCoreFunctions.h"
#include "Screenshot.h"
#include "ToolButton.h"
#include "rfb/keysym.h"

namespace
{
QToolButton* remoteButton(QWidget* parent, const QString& icon, const QString& text)
{
	auto button = new QToolButton(parent);
	button->setIcon(DesktopIcons::icon(icon));
	button->setText(text);
	button->setToolTip(text);
	button->setIconSize(QSize(16, 16));
	button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	button->setMinimumHeight(30);
	button->setFocusPolicy(Qt::NoFocus);
	return button;
}
}

RemoteAccessWidgetToolBar::RemoteAccessWidgetToolBar(RemoteAccessWidget* parent,
	bool startViewOnly, bool showViewOnlyToggleButton) :
	QWidget(parent), m_parent(parent),
	m_viewButton(remoteButton(this, QStringLiteral("eye"), tr("View"))),
	m_controlButton(remoteButton(this, QStringLiteral("mouse-pointer-2"), tr("Control"))),
	m_selectScreenButton(remoteButton(this, QStringLiteral("monitor"), tr("Select screen"))),
	m_sendShortcutButton(remoteButton(this, QStringLiteral("keyboard"), tr("Ctrl+Alt+Del"))),
	m_screenshotButton(remoteButton(this, QStringLiteral("camera"), tr("Screenshot"))),
	m_fullScreenButton(remoteButton(this, QStringLiteral("maximize"), tr("Fullscreen"))),
	m_viewOnly(startViewOnly), m_screenSelectActions(new QActionGroup(this))
{
	setObjectName(QStringLiteral("remoteToolbar"));
	setAttribute(Qt::WA_StyledBackground);
	setStyleSheet(QStringLiteral("QToolButton#remoteFitScreen:checked, QToolButton#remoteFullScreen:checked { background: transparent; color: palette(button-text); border-color: transparent; }"));
	setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
	auto layout = new QHBoxLayout(this);
	layout->setContentsMargins(10, 7, 10, 7);
	layout->setSpacing(4);
	auto modes = new QWidget(this);
	modes->setObjectName(QStringLiteral("remoteModeSwitch"));
	auto modeLayout = new QHBoxLayout(modes);
	modeLayout->setContentsMargins(0, 0, 0, 0);
	modeLayout->setSpacing(1);
	auto modeGroup = new QButtonGroup(this);
	for (auto button : {m_viewButton, m_controlButton})
	{
		button->setCheckable(true);
		modeGroup->addButton(button);
		modeLayout->addWidget(button);
	}
	m_viewButton->setObjectName(QStringLiteral("remoteViewMode"));
	m_controlButton->setObjectName(QStringLiteral("remoteControlMode"));
	m_viewButton->setEnabled(showViewOnlyToggleButton || startViewOnly);
	m_controlButton->setEnabled(showViewOnlyToggleButton || !startViewOnly);
	connect(m_viewButton, &QToolButton::clicked, this, [this]() { m_parent->setViewOnly(true); });
	connect(m_controlButton, &QToolButton::clicked, this, [this]() { m_parent->setViewOnly(false); });
	layout->addWidget(modes);
	layout->addSpacing(10);
	layout->addWidget(m_sendShortcutButton);
	layout->addWidget(m_screenshotButton);
	layout->addWidget(m_fullScreenButton);
	auto fitButton = remoteButton(this, QStringLiteral("scan"), tr("Fit screen"));
	fitButton->setObjectName(QStringLiteral("remoteFitScreen"));
	fitButton->setIcon(QIcon(DesktopIcons::icon(QStringLiteral("scan")).pixmap(16, 16)));
	fitButton->setCheckable(true);
	fitButton->setChecked(true);
	connect(fitButton, &QToolButton::toggled, parent, &RemoteAccessWidget::setFitToWindow);
	layout->addWidget(fitButton);
	layout->addWidget(m_selectScreenButton);
	layout->addStretch();
	auto endButton = remoteButton(this, QStringLiteral("log-out"), tr("End session"));
	endButton->setObjectName(QStringLiteral("remoteEndSession"));
	connect(endButton, &QToolButton::clicked, parent, &QWidget::close);
	layout->addWidget(endButton);
	m_fullScreenButton->setCheckable(true);
	m_fullScreenButton->setObjectName(QStringLiteral("remoteFullScreen"));
	m_fullScreenButton->setIcon(QIcon(DesktopIcons::icon(QStringLiteral("maximize")).pixmap(16, 16)));
	connect(m_fullScreenButton, &QToolButton::toggled, parent, &RemoteAccessWidget::toggleFullScreen);
	connect(m_screenshotButton, &QToolButton::clicked, parent, &RemoteAccessWidget::takeScreenshot);
	auto view = parent->vncView();
	connect(view->connection(), &VncConnection::stateChanged, this, &RemoteAccessWidgetToolBar::updateConnectionState);
	m_selectScreenButton->setMenu(new QMenu(this));
	m_selectScreenButton->setPopupMode(QToolButton::InstantPopup);
	auto shortcutMenu = new QMenu(this);
	const QList<QPair<QString, VncView::Shortcut>> shortcuts{
		{tr("Ctrl+Alt+Del"), VncView::ShortcutCtrlAltDel},
		{tr("Ctrl+Esc"), VncView::ShortcutCtrlEscape},
		{tr("Alt+Tab"), VncView::ShortcutAltTab},
		{tr("Alt+F4"), VncView::ShortcutAltF4},
		{tr("Win+Tab"), VncView::ShortcutWinTab},
		{tr("Win"), VncView::ShortcutWin},
		{tr("Menu"), VncView::ShortcutMenu},
		{tr("Alt+Ctrl+F1"), VncView::ShortcutAltCtrlF1}
	};
	for (const auto& shortcut : shortcuts)
	{
		shortcutMenu->addAction(shortcut.first, this, [this, view, shortcut]() {
			if (!m_viewOnly && view->connection()->isConnected())
			{
				view->setViewOnly(false);
				view->sendShortcut(shortcut.second);
				m_parent->setViewOnly(false);
			}
		});
	}
	m_sendShortcutButton->setMenu(shortcutMenu);
	m_sendShortcutButton->setToolTip(tr("Send shortcut"));
	m_sendShortcutButton->setPopupMode(QToolButton::InstantPopup);
	connect(view->computerControlInterface().data(), &ComputerControlInterface::screensChanged,
		this, &RemoteAccessWidgetToolBar::updateScreens);
	connect(parent, &RemoteAccessWidget::screenChangedInRemoteAccessWidget,
		this, &RemoteAccessWidgetToolBar::updateScreenSelectActions);
	updateScreens();
	updateControls(startViewOnly);
}

void RemoteAccessWidgetToolBar::updateControls(bool viewOnly)
{
	m_viewOnly = viewOnly;
	m_viewButton->setChecked(viewOnly);
	m_controlButton->setChecked(!viewOnly);
	updateConnectionState();
}

void RemoteAccessWidgetToolBar::updateFullScreen(bool fullScreen)
{
	const QSignalBlocker blocker(m_fullScreenButton);
	m_fullScreenButton->setChecked(fullScreen);
}

void RemoteAccessWidgetToolBar::updateConnectionState()
{
	const auto connected = m_parent->vncView()->connection()->isConnected();
	m_sendShortcutButton->setEnabled(connected && !m_viewOnly);
	m_screenshotButton->setEnabled(connected);
}

void RemoteAccessWidgetToolBar::updateScreenSelectActions(int newScreen)
{
	const auto actions = m_screenSelectActions->actions();
	if (newScreen >= -1 && newScreen + 1 < actions.size())
	{
		actions[newScreen + 1]->setChecked(true);
	}
}

void RemoteAccessWidgetToolBar::updateScreens()
{
	const auto checkedScreenName = m_screenSelectActions->checkedAction() ?
		m_screenSelectActions->checkedAction()->text() :
		QString{};

	const auto screens = m_parent->vncView()->computerControlInterface()->screens();
	m_selectScreenButton->setVisible(screens.size() > 1);

	const auto menu = m_selectScreenButton->menu();
	menu->clear();

	if (screens.size() > 1)
	{
		const auto showAllScreens = menu->addAction(tr("All screens"), this, [=, this]() {
			m_parent->vncView()->setViewport({});
		});

		m_screenSelectActions->addAction(showAllScreens);
		showAllScreens->setCheckable(true);
		showAllScreens->setChecked(checkedScreenName.isEmpty() ||
								   checkedScreenName == showAllScreens->text());

		menu->addSeparator();

		QPoint minimumScreenPosition{};
		for (const auto& screen : screens)
		{
			minimumScreenPosition.setX(qMin(minimumScreenPosition.x(), screen.geometry.x()));
			minimumScreenPosition.setY(qMin(minimumScreenPosition.y(), screen.geometry.y()));
		}

		for (const auto& screen : screens)
		{
			const auto action = menu->addAction(screen.name, this, [=, this]() {
				m_parent->vncView()->setViewport(screen.geometry.translated(-minimumScreenPosition));
			});
			action->setCheckable(true);
			if (action->text() == checkedScreenName)
			{
				action->setChecked(true);
			}
			m_screenSelectActions->addAction(action);
		}

		if (m_screenSelectActions->checkedAction())
		{
			m_screenSelectActions->checkedAction()->trigger();
		}
		else
		{
			showAllScreens->setChecked(true);
			showAllScreens->trigger();
		}
	}
	else
	{
		m_parent->vncView()->setViewport({});
	}
}



RemoteAccessWidget::RemoteAccessWidget(const ComputerControlInterface::Pointer& computerControlInterface,
									   bool startViewOnly, bool showViewOnlyToggleButton) :
	QWidget(nullptr), m_computerControlInterface(computerControlInterface),
	m_vncView(new VncViewWidget(computerControlInterface, {}, this)),
	m_toolBar(new RemoteAccessWidgetToolBar(this, startViewOnly, showViewOnlyToggleButton)),
	m_stage(new QScrollArea(this)), m_controlBanner(new QWidget(this)),
	m_sessionLabel(new QLabel(this)), m_connectionLabel(new QLabel(this)), m_modeLabel(new QLabel(this))
{
	setPalette(QPalette());
	setObjectName(QStringLiteral("remoteAccessWindow"));
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowIcon(DesktopIcons::icon(QStringLiteral("monitor")));
	auto layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	auto sessionHeader = new QWidget(this);
	sessionHeader->setObjectName(QStringLiteral("remoteSessionHeader"));
	sessionHeader->setAttribute(Qt::WA_StyledBackground);
	auto sessionLayout = new QHBoxLayout(sessionHeader);
	sessionLayout->setContentsMargins(10, 2, 12, 2);
	auto back = remoteButton(sessionHeader, QStringLiteral("arrow-left"), tr("Back to class"));
	connect(back, &QToolButton::clicked, this, [this]() {
		if (const auto master = VeyonCore::instance()->findChild<VeyonMasterInterface*>())
		{
			master->mainWindow()->show();
			VeyonCore::platform().coreFunctions().raiseWindow(master->mainWindow(), false);
	}
		close();
	});
	sessionLayout->addWidget(back);
	sessionLayout->addSpacing(12);
	sessionLayout->addWidget(m_sessionLabel);
	sessionLayout->addStretch();
	layout->addWidget(sessionHeader);
	layout->addWidget(m_toolBar);
	m_controlBanner->setObjectName(QStringLiteral("remoteControlBanner"));
	m_controlBanner->setStyleSheet(QStringLiteral("QWidget#remoteControlBanner { background: #ae1800; color: white; } QWidget#remoteControlBanner QLabel, QWidget#remoteControlBanner QToolButton { color: white; }"));
	auto bannerLayout = new QHBoxLayout(m_controlBanner);
	bannerLayout->setContentsMargins(12, 5, 10, 5);
	auto bannerLabel = new QLabel(tr("You are controlling %1. Mouse and keyboard input is sent to this computer.")
		.arg(computerControlInterface->computerName()), m_controlBanner);
	bannerLabel->setWordWrap(true);
	bannerLayout->addWidget(bannerLabel, 1);
	auto backToView = remoteButton(m_controlBanner, QStringLiteral("eye"), tr("Back to view") + QStringLiteral(" · Esc"));
	connect(backToView, &QToolButton::clicked, this, [this]() { setViewOnly(true); });
	bannerLayout->addWidget(backToView);
	layout->addWidget(m_controlBanner);
	m_stage->setObjectName(QStringLiteral("remoteStage"));
	m_stage->setAlignment(Qt::AlignCenter);
	m_stage->setWidgetResizable(false);
	m_stage->setFrameShape(QFrame::NoFrame);
	m_stage->setStyleSheet(QStringLiteral("QScrollArea#remoteStage { background: #121110; border: 2px solid #121110; } QScrollArea#remoteStage[remoteControl=true] { border-color: #ae1800; }"));
	m_stage->viewport()->setAutoFillBackground(true);
	auto stagePalette = m_stage->viewport()->palette();
	stagePalette.setColor(QPalette::Window, QColor(QStringLiteral("#121110")));
	m_stage->viewport()->setPalette(stagePalette);
	m_stage->setWidget(m_vncView);
	m_stage->viewport()->installEventFilter(this);
	layout->addWidget(m_stage, 1);
	auto statusBar = new QWidget(this);
	statusBar->setObjectName(QStringLiteral("remoteStatusBar"));
	statusBar->setFixedHeight(28);
	statusBar->setAttribute(Qt::WA_StyledBackground);
	auto statusLayout = new QHBoxLayout(statusBar);
	statusLayout->setContentsMargins(12, 5, 12, 5);
	statusLayout->setSpacing(18);
	statusLayout->addWidget(m_connectionLabel);
	statusLayout->addWidget(m_modeLabel);
	auto hostLabel = new QLabel(m_vncView->connection()->host(), statusBar);
	hostLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	statusLayout->addWidget(hostLabel);
	statusLayout->addStretch();
	layout->addWidget(statusBar);
	m_vncView->installEventFilter(this);
	connect(m_vncView, &VncViewWidget::sizeHintChanged, this, &RemoteAccessWidget::updateSize);
	connect(m_vncView->connection(), &VncConnection::stateChanged, this, &RemoteAccessWidget::updateConnectionState);
	connect(m_computerControlInterface.data(), &ComputerControlInterface::userChanged, this, &RemoteAccessWidget::updateRemoteAccessTitle);
	updateRemoteAccessTitle();
	updateConnectionState();
	const auto master = VeyonCore::instance()->findChild<VeyonMasterInterface*>();
	if (master && VeyonCore::config().showFeatureWindowsOnSameScreen())
	{
		move(master->mainWindow()->pos());
	}
	resize(1100, 760);
	show();
	VeyonCore::platform().coreFunctions().raiseWindow(this, false);
	updateSize();
	setViewOnly(startViewOnly);
}

RemoteAccessWidget::~RemoteAccessWidget()
{
	delete m_vncView;
}

VncView* RemoteAccessWidget::vncView() const
{
	return m_vncView;
}

bool RemoteAccessWidget::eventFilter(QObject* object, QEvent* event)
{
	if (object == m_stage->viewport() && event->type() == QEvent::Resize)
	{
		updateStageGeometry();
	}
	if (object == m_vncView && (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease))
	{
		const auto key = static_cast<QKeyEvent*>(event)->key();
		if (key == Qt::Key_Escape && !m_viewOnly)
		{
			if (event->type() == QEvent::KeyPress)
			{
				setViewOnly(true);
			}
		return true;
	}
		if (key == Qt::Key_Escape && !m_vncView->connection()->isConnected())
	{
			if (event->type() == QEvent::KeyRelease)
			{
				close();
	}
			return true;
		}
		if (event->type() == QEvent::KeyPress && m_viewOnly)
		{
			const auto screens = m_computerControlInterface->screens();
			if (screens.size() > 1 && (key == Qt::Key_Tab || key == Qt::Key_Backtab))
			{
				m_currentScreen = qBound(-1, m_currentScreen, int(screens.size()) - 1);
				if (key == Qt::Key_Tab)
				{
					m_currentScreen = m_currentScreen < screens.size() - 1 ? m_currentScreen + 1 : -1;
			}
			else
			{
					m_currentScreen = m_currentScreen == -1 ? screens.size() - 1 : m_currentScreen - 1;
			}
				QPoint minimumPosition;
				for (const auto& screen : screens)
				{
					minimumPosition.setX(qMin(minimumPosition.x(), screen.geometry.x()));
					minimumPosition.setY(qMin(minimumPosition.y(), screen.geometry.y()));
				}
				m_vncView->setViewport(m_currentScreen < 0 ? QRect{} : screens[m_currentScreen].geometry.translated(-minimumPosition));
			Q_EMIT screenChangedInRemoteAccessWidget(m_currentScreen);
			return true;
		}
	}
	}
	return QWidget::eventFilter(object, event);
}

void RemoteAccessWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	const auto buttons = m_toolBar->findChildren<QToolButton*>();
	int labeledWidth = 50;
	for (auto button : buttons)
		{
		button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		labeledWidth += button->sizeHint().width();
		}
	if (width() < labeledWidth)
	{
		for (auto button : buttons)
		{
			if (button->objectName() != QStringLiteral("remoteViewMode") &&
				button->objectName() != QStringLiteral("remoteControlMode"))
			{
				button->setToolButtonStyle(Qt::ToolButtonIconOnly);
			}
		}
	}
	updateStageGeometry();
}

void RemoteAccessWidget::changeEvent(QEvent* event)
{
	QWidget::changeEvent(event);
	if (event->type() == QEvent::WindowStateChange)
	{
		m_toolBar->updateFullScreen(isFullScreen());
	}
}

void RemoteAccessWidget::updateStageGeometry()
{
	auto size = m_vncView->effectiveFramebufferSize();
	const auto available = m_stage->viewport()->size() - QSize(32, 32);
	if (size.isEmpty())
	{
		size = available;
	}
	else if (m_fitToWindow && available.width() > 0 && available.height() > 0 &&
		(size.width() > available.width() || size.height() > available.height()))
	{
		size.scale(available, Qt::KeepAspectRatio);
	}
	m_vncView->resize(size.expandedTo(QSize(1, 1)));
}

void RemoteAccessWidget::updateSize()
{
	updateStageGeometry();
}

void RemoteAccessWidget::toggleFullScreen(bool fullScreen)
{
	if (fullScreen)
	{
		showFullScreen();
	}
	else
	{
		showNormal();
	}
}

void RemoteAccessWidget::setViewOnly(bool viewOnly)
{
	m_viewOnly = viewOnly;
	m_vncView->setViewOnly(viewOnly);
	m_toolBar->updateControls(viewOnly);
	m_controlBanner->setVisible(!viewOnly);
	m_modeLabel->setText(viewOnly ? tr("View only") : tr("Mouse and keyboard control"));
	m_stage->setProperty("remoteControl", !viewOnly);
	m_stage->style()->unpolish(m_stage);
	m_stage->style()->polish(m_stage);
	m_stage->update();
	m_vncView->setFocus();
	updateStageGeometry();
}

void RemoteAccessWidget::setFitToWindow(bool fit)
{
	m_fitToWindow = fit;
	updateStageGeometry();
}

void RemoteAccessWidget::takeScreenshot()
{
	Screenshot().take(m_computerControlInterface);
}

void RemoteAccessWidget::updateConnectionState()
{
	QString state;
	switch (m_vncView->connection()->state())
	{
	case VncConnection::State::Connected: state = tr("Connected"); break;
	case VncConnection::State::Connecting: state = tr("Connecting…"); break;
	case VncConnection::State::HostOffline: state = tr("Computer offline"); break;
	case VncConnection::State::HostNameResolutionFailed: state = tr("Host name resolution failed"); break;
	case VncConnection::State::ServerNotRunning: state = tr("Veyon Server is not running"); break;
	case VncConnection::State::AuthenticationFailed: state = tr("Authentication failed"); break;
	case VncConnection::State::AccessControlFailed: state = tr("Access denied"); break;
	case VncConnection::State::ConnectionFailed: state = tr("Connection failed"); break;
	default: state = tr("Disconnected"); break;
	}
	m_connectionLabel->setText(state);
	updateStageGeometry();
}

void RemoteAccessWidget::updateRemoteAccessTitle()
{
	m_sessionLabel->setText(m_computerControlInterface->computerName());
	const auto username = m_computerControlInterface->userFullName().isEmpty() ?
		VeyonCore::stripDomain(m_computerControlInterface->userLoginName()) :
		m_computerControlInterface->userFullName();

	if (username.isEmpty())
	{
		setWindowTitle(tr("%1 - Veyon Remote Access").arg(m_computerControlInterface->computerName()));
	}
	else
	{
		setWindowTitle(tr("%1 - %2 - Veyon Remote Access").arg(username, m_computerControlInterface->computerName()));
	}
}
