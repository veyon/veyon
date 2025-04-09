/*
 *  RemoteAccessWidget.cpp - widget containing a VNC-view and controls for it
 *
 *  Copyright (c) 2006-2025 Tobias Junghans <tobydox@veyon.io>
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

#include <QActionGroup>
#include <QBitmap>
#include <QLayout>
#include <QMenu>
#include <QPainter>
#include <QPaintEvent>

#include "rfb/keysym.h"

#include "RemoteAccessWidget.h"
#include "RemoteAccessFeaturePlugin.h"
#include "VncViewWidget.h"
#include "VeyonConfiguration.h"
#include "VeyonConnection.h"
#include "VeyonMasterInterface.h"
#include "Computer.h"
#include "ComputerControlInterface.h"
#include "PlatformCoreFunctions.h"
#include "ToolButton.h"
#include "Screenshot.h"


// toolbar for remote-control-widget
RemoteAccessWidgetToolBar::RemoteAccessWidgetToolBar( RemoteAccessWidget* parent,
													  bool startViewOnly, bool showViewOnlyToggleButton ) :
	QWidget( parent ),
	m_parent( parent ),
	m_showHideTimeLine( ShowHideAnimationDuration, this ),
	m_viewOnlyButton( showViewOnlyToggleButton ? new ToolButton( QPixmap( QStringLiteral(":/remoteaccess/kmag.png") ), tr( "View only" ), tr( "Remote control" ) ) : nullptr ),
	m_selectScreenButton( new ToolButton( QPixmap( QStringLiteral(":/remoteaccess/preferences-system-windows-effect-desktopgrid.png") ), tr( "Select screen" ) ) ),
	m_sendShortcutButton( new ToolButton( QPixmap( QStringLiteral(":/remoteaccess/preferences-desktop-keyboard.png") ), tr( "Send shortcut" ) ) ),
	m_screenshotButton( new ToolButton( QPixmap( QStringLiteral(":/remoteaccess/camera-photo.png") ), tr( "Screenshot" ) ) ),
	m_fullScreenButton( new ToolButton( QPixmap( QStringLiteral(":/core/view-fullscreen.png") ), tr( "Fullscreen" ), tr( "Window" ) ) ),
	m_exitButton( new ToolButton( QPixmap( QStringLiteral(":/remoteaccess/application-exit.png") ), tr( "Exit" ) ) ),
	m_screenSelectActions( new QActionGroup(this) )
{
	setAttribute( Qt::WA_NoSystemBackground, true );
	move( 0, 0 );
	show();

	if( m_viewOnlyButton )
	{
		m_viewOnlyButton->setCheckable( true );
		m_viewOnlyButton->setChecked( startViewOnly );
		connect( m_viewOnlyButton, &ToolButton::toggled, this, &RemoteAccessWidgetToolBar::updateControls );
		connect( m_viewOnlyButton, &QAbstractButton::toggled, parent, &RemoteAccessWidget::setViewOnly );
	}

	m_fullScreenButton->setCheckable( true );
	m_fullScreenButton->setChecked( false );

	connect( m_fullScreenButton, &QAbstractButton::toggled, parent, &RemoteAccessWidget::toggleFullScreen );
	connect( m_screenshotButton, &QAbstractButton::clicked, parent, &RemoteAccessWidget::takeScreenshot );
	connect( m_exitButton, &QAbstractButton::clicked, parent, &QWidget::close );

	auto vncView = parent->vncView();
	connect( vncView->connection(), &VncConnection::stateChanged, this, &RemoteAccessWidgetToolBar::updateConnectionState );

	m_selectScreenButton->setMenu( new QMenu );
	m_selectScreenButton->setPopupMode( QToolButton::InstantPopup );
	m_selectScreenButton->setObjectName( QStringLiteral("screens") );

	updateScreens();

	auto shortcutMenu = new QMenu();
	shortcutMenu->addAction( tr( "Ctrl+Alt+Del" ), this, [=]() { vncView->sendShortcut( VncView::ShortcutCtrlAltDel ); }  );
	shortcutMenu->addAction( tr( "Ctrl+Esc" ), this, [=]() { vncView->sendShortcut( VncView::ShortcutCtrlEscape ); }  );
	shortcutMenu->addAction( tr( "Alt+Tab" ), this, [=]() { vncView->sendShortcut( VncView::ShortcutAltTab ); }  );
	shortcutMenu->addAction( tr( "Alt+F4" ), this, [=]() { vncView->sendShortcut( VncView::ShortcutAltF4 ); }  );
	shortcutMenu->addAction( tr( "Win+Tab" ), this, [=]() { vncView->sendShortcut( VncView::ShortcutWinTab ); }  );
	shortcutMenu->addAction( tr( "Win" ), this, [=]() { vncView->sendShortcut( VncView::ShortcutWin ); }  );
	shortcutMenu->addAction( tr( "Menu" ), this, [=]() { vncView->sendShortcut( VncView::ShortcutMenu ); }  );
	shortcutMenu->addAction( tr( "Alt+Ctrl+F1" ), this, [=]() { vncView->sendShortcut( VncView::ShortcutAltCtrlF1 ); }  );

	m_sendShortcutButton->setMenu( shortcutMenu );
	m_sendShortcutButton->setPopupMode( QToolButton::InstantPopup );
	m_sendShortcutButton->setObjectName( QStringLiteral("shortcuts") );

	auto layout = new QHBoxLayout( this );
	layout->setContentsMargins( 1, 1, 1, 1 );
	layout->setSpacing( 1 );
	layout->addStretch( 0 );
	layout->addWidget( m_selectScreenButton );
	layout->addWidget( m_sendShortcutButton );
	if( m_viewOnlyButton )
	{
		layout->addWidget( m_viewOnlyButton );
	}
	layout->addWidget( m_screenshotButton );
	layout->addWidget( m_fullScreenButton );
	layout->addWidget( m_exitButton );
	layout->addSpacing( 5 );

	setFixedHeight(m_exitButton->minimumSizeHint().height());

	connect( &m_showHideTimeLine, &QTimeLine::valueChanged, this, &RemoteAccessWidgetToolBar::updatePosition );

	connect( vncView->computerControlInterface().data(), &ComputerControlInterface::screensChanged,
			 this, &RemoteAccessWidgetToolBar::updateScreens );

	connect( m_parent, &RemoteAccessWidget::screenChangedInRemoteAccessWidget,
			 this, &RemoteAccessWidgetToolBar::updateScreenSelectActions );
}



void RemoteAccessWidgetToolBar::appear()
{
	m_showHideTimeLine.setDirection( QTimeLine::Backward );
	if( m_showHideTimeLine.state() != QTimeLine::Running )
	{
		m_showHideTimeLine.resume();
	}
}



void RemoteAccessWidgetToolBar::disappear()
{
	if( rect().contains( mapFromGlobal( QCursor::pos() ) ) == false )
	{
		QTimer::singleShot( DisappearDelay, this, [this]() {
			if( m_showHideTimeLine.state() != QTimeLine::Running )
			{
				m_showHideTimeLine.setDirection( QTimeLine::Forward );
				m_showHideTimeLine.resume();
			}
		} );
	}
}



void RemoteAccessWidgetToolBar::updateControls( bool viewOnly )
{
	m_sendShortcutButton->setVisible( viewOnly == false );
}



void RemoteAccessWidgetToolBar::updateScreenSelectActions( int newScreen )
{
	const auto screens = m_parent->vncView()->computerControlInterface()->screens();
	const auto m_screenSelectActions = m_selectScreenButton->menu()->actions();
	for (const auto& screenSelectAction : m_screenSelectActions)
	{
		if ( newScreen == -1 )
		{
			screenSelectAction->setChecked(true);
			break;
		}
		if ( screenSelectAction->text() == screens[newScreen].name )
		{
			screenSelectAction->setChecked(true);
		}
	}
}



void RemoteAccessWidgetToolBar::leaveEvent( QEvent *event )
{
	disappear();
	QWidget::leaveEvent( event );
}



void RemoteAccessWidgetToolBar::paintEvent( QPaintEvent *paintEv )
{
	QPainter p( this );

	p.setOpacity( 0.8 );
	p.fillRect( paintEv->rect(), palette().brush( QPalette::Window ) );
	p.setOpacity( 1 );

	auto f = p.font();
	f.setPointSize( 10 );
	f.setBold( true );
	p.setFont( f );

	p.setPen(palette().color(QPalette::Text));
	p.drawText( height() / 2, height() / 2 + fontMetrics().height() / 2,
				m_parent->vncView() && m_parent->vncView()->connection() &&
						m_parent->vncView()->connection()->state() == VncConnection::State::Connected ?
					tr( "Connected." ) : tr( "Connecting..." ) );
}



void RemoteAccessWidgetToolBar::updatePosition()
{
	const auto newY = static_cast<int>( m_showHideTimeLine.currentValue() * height() );

	if( newY != -y() )
	{
		move( x(), qMax( -height(), -newY ) );
	}
}



void RemoteAccessWidgetToolBar::updateConnectionState()
{
	if( m_parent->vncView()->connection()->state() == VncConnection::State::Connected )
	{
		disappear();
	}
	else
	{
		appear();
	}
}



void RemoteAccessWidgetToolBar::updateScreens()
{
	const auto checkedScreenName = m_screenSelectActions->checkedAction() ?
										m_screenSelectActions->checkedAction()->text() : QString{};

	const auto screens = m_parent->vncView()->computerControlInterface()->screens();
	m_selectScreenButton->setVisible(screens.size() > 1);

	const auto menu = m_selectScreenButton->menu();
	menu->clear();

	if(screens.size() > 1)
	{
		const auto showAllScreens = menu->addAction( tr( "All screens" ), this, [=]() {
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
			const auto action = menu->addAction(screen.name, this, [=]() {
				m_parent->vncView()->setViewport(screen.geometry.translated(-minimumScreenPosition));
			});
			action->setCheckable(true);
			if(action->text() == checkedScreenName)
			{
				action->setChecked(true);
			}
			m_screenSelectActions->addAction(action);
		}

		if(m_screenSelectActions->checkedAction())
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



RemoteAccessWidget::RemoteAccessWidget( const ComputerControlInterface::Pointer& computerControlInterface,
										bool startViewOnly, bool showViewOnlyToggleButton ) :
	QWidget( nullptr ),
	m_computerControlInterface( computerControlInterface ),
	m_vncView( new VncViewWidget( computerControlInterface, {}, this ) ),
	m_toolBar( new RemoteAccessWidgetToolBar( this, startViewOnly, showViewOnlyToggleButton ) )
{
	const auto openOnMasterScreen = VeyonCore::config().showFeatureWindowsOnSameScreen();
	const auto master = VeyonCore::instance()->findChild<VeyonMasterInterface *>();
	if( master && openOnMasterScreen )
	{
		const auto masterWindow = master->mainWindow();
		move( masterWindow->x(), masterWindow->y() );
	} else {
		move( 0, 0 );
	}

	updateRemoteAccessTitle();
	connect( m_computerControlInterface.data(), &ComputerControlInterface::userChanged, this, &RemoteAccessWidget::updateRemoteAccessTitle );

	setWindowIcon( QPixmap( QStringLiteral(":/remoteaccess/kmag.png") ) );
	setAttribute( Qt::WA_DeleteOnClose, true );

	m_vncView->move( 0, 0 );
	m_vncView->installEventFilter( this );
	connect( m_vncView, &VncViewWidget::mouseAtBorder, m_toolBar, &RemoteAccessWidgetToolBar::appear );
	connect( m_vncView, &VncViewWidget::sizeHintChanged, this, &RemoteAccessWidget::updateSize );

	showMaximized();
	VeyonCore::platform().coreFunctions().raiseWindow( this, false );

	showNormal();

	updateSize();

	setViewOnly( startViewOnly );
}



RemoteAccessWidget::~RemoteAccessWidget()
{
	delete m_vncView;
}



VncView* RemoteAccessWidget::vncView() const
{
	return m_vncView;
}



bool RemoteAccessWidget::eventFilter( QObject* object, QEvent* event )
{
	if( event->type() == QEvent::KeyRelease &&
		dynamic_cast<QKeyEvent *>( event )->key() == Qt::Key_Escape &&
		m_vncView->connection()->isConnected() == false )
	{
		close();
		return true;
	}

	if( object == m_vncView && event->type() == QEvent::FocusOut )
	{
		m_toolBar->disappear();
	}

	if( event->type() == QEvent::KeyPress && m_vncView->viewOnly() )
	{
		const auto screens = m_vncView->computerControlInterface()->screens();
		const auto key = static_cast<QKeyEvent *>( event )->key();
		if ( screens.size() > 1 && ( key == Qt::Key_Tab || key == Qt::Key_Backtab ) )
		{
			if( key == Qt::Key_Tab )
			{
				if ( m_currentScreen < screens.size() - 1 )
				{
					m_currentScreen++;
				} else
				{
					m_currentScreen = -1;
				}
			}

			if( key == Qt::Key_Backtab )
			{
				if ( m_currentScreen == -1 )
				{
					m_currentScreen = screens.size()-1;
				} else if ( m_currentScreen > 0 )
				{
					m_currentScreen--;
				} else
				{
					m_currentScreen = -1;
				}
			}

			if ( m_currentScreen == -1)
			{
				m_vncView->setViewport( {} );
			}
			else
			{
				m_vncView->setViewport(screens[m_currentScreen].geometry);
			}
			Q_EMIT screenChangedInRemoteAccessWidget(m_currentScreen);
			return true;
		}

		return false;
	}

	return QWidget::eventFilter( object, event );
}



#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void RemoteAccessWidget::enterEvent( QEnterEvent* event )
#else
void RemoteAccessWidget::enterEvent( QEvent* event )
#endif
{
	m_toolBar->disappear();

	QWidget::enterEvent( event );
}



void RemoteAccessWidget::leaveEvent( QEvent* event )
{
	QTimer::singleShot( AppearDelay, this, [this]() {
		if( underMouse() == false && window()->isActiveWindow() )
		{
			m_toolBar->appear();
		}
	} );

	QWidget::leaveEvent( event );
}



void RemoteAccessWidget::resizeEvent( QResizeEvent* event )
{
	m_vncView->setFixedSize(size());
	m_toolBar->setFixedSize( width(), m_toolBar->height() );

	QWidget::resizeEvent( event );
}



void RemoteAccessWidget::updateSize()
{
	if (!(windowState() & Qt::WindowFullScreen))
	{
		resize( m_vncView->sizeHint() );
	}
}



void RemoteAccessWidget::toggleFullScreen( bool _on )
{
	if( _on )
	{
		setWindowState( windowState() | Qt::WindowFullScreen );
	}
	else
	{
		setWindowState( windowState() & ~Qt::WindowFullScreen );
	}
}



void RemoteAccessWidget::setViewOnly( bool viewOnly )
{
	m_vncView->setViewOnly( viewOnly );
	m_toolBar->updateControls( viewOnly );
	m_toolBar->update();
}



void RemoteAccessWidget::takeScreenshot()
{
	Screenshot().take( m_computerControlInterface );
}



void RemoteAccessWidget::updateRemoteAccessTitle()
{
	const auto username = m_computerControlInterface->userFullName().isEmpty() ?
							  m_computerControlInterface->userLoginName() : m_computerControlInterface->userFullName();

	if (username.isEmpty() )
	{
		setWindowTitle(tr("%1 - %2 Remote Access").arg(m_computerControlInterface->computerName(),
													   VeyonCore::applicationName()));
	}
	else
	{
		setWindowTitle(tr("%1 - %2 - %3 Remote Access").arg(username,
															m_computerControlInterface->computerName(),
															VeyonCore::applicationName()));
	}
}
