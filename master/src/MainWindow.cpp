/*
 * MainWindow.cpp - implementation of MainWindow class
 *
 * Copyright (c) 2004-2018 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <QCloseEvent>
#include <QKeyEvent>
#include <QHostAddress>
#include <QMenu>
#include <QMessageBox>
#include <QSplitter>

#include "AboutDialog.h"
#include "AccessControlProvider.h"
#include "MainWindow.h"
#include "BuiltinFeatures.h"
#include "AuthenticationCredentials.h"
#include "ComputerControlListModel.h"
#include "ComputerManager.h"
#include "ComputerManagementView.h"
#include "ScreenshotManagementView.h"
#include "FeatureManager.h"
#include "MonitoringMode.h"
#include "NetworkObjectDirectory.h"
#include "NetworkObjectDirectoryManager.h"
#include "ToolButton.h"
#include "VeyonConfiguration.h"
#include "MasterCore.h"
#include "UserConfig.h"

#include "ui_MainWindow.h"


MainWindow::MainWindow( MasterCore &masterCore, QWidget* parent ) :
	QMainWindow( parent ),
	ui( new Ui::MainWindow ),
	m_masterCore( masterCore ),
	m_modeGroup( new QButtonGroup( this ) ),
	m_systemTrayIcon( this )
{
	ui->setupUi( this );

	setWindowTitle( QStringLiteral( "%1 Master" ).arg( VeyonCore::applicationName() ) );

	restoreState( QByteArray::fromBase64( m_masterCore.userConfig().windowState().toUtf8() ) );
	restoreGeometry( QByteArray::fromBase64( m_masterCore.userConfig().windowGeometry().toUtf8() ) );

	ui->computerMonitoringView->setMasterCore( m_masterCore );

	// add widgets to status bar
	ui->statusBar->addWidget( ui->computerManagementButton );
	ui->statusBar->addWidget( ui->screenshotManagementButton );
	ui->statusBar->addWidget( ui->spacerLabel1 );
	ui->statusBar->addWidget( ui->filterLineEdit, 2 );
	ui->statusBar->addWidget( ui->spacerLabel2, 1 );
	ui->statusBar->addWidget( ui->gridSizeSlider, 2 );
	ui->statusBar->addWidget( ui->autoFitButton );
	ui->statusBar->addWidget( ui->spacerLabel3 );
	ui->statusBar->addWidget( ui->aboutButton );

	// create all views
	auto splitter = new QSplitter( Qt::Horizontal, ui->centralWidget );
	splitter->setChildrenCollapsible( false );

	ui->centralLayout->addWidget( splitter );

	m_computerManagementView = new ComputerManagementView( m_masterCore.computerManager(), splitter );
	m_screenshotManagementView = new ScreenshotManagementView( splitter );

	splitter->addWidget( m_computerManagementView );
	splitter->addWidget( m_screenshotManagementView );
	splitter->addWidget( ui->computerMonitoringView );

	// hide views per default and connect related button
	m_computerManagementView->hide();
	m_screenshotManagementView->hide();

	connect( ui->computerManagementButton, &QAbstractButton::toggled,
			 m_computerManagementView, &QWidget::setVisible );
	connect( ui->screenshotManagementButton, &QAbstractButton::toggled,
			 m_screenshotManagementView, &QWidget::setVisible );

	if( VeyonCore::config().openComputerManagementAtStart() )
	{
		ui->computerManagementButton->setChecked( true );
	}

	// initialize search filter
	connect( ui->filterLineEdit, &QLineEdit::textChanged,
			 ui->computerMonitoringView, &ComputerMonitoringView::setSearchFilter );

	// initialize monitoring screen size slider
	ui->gridSizeSlider->setMinimum( ComputerMonitoringView::MinimumComputerScreenSize );
	ui->gridSizeSlider->setMaximum( ComputerMonitoringView::MaximumComputerScreenSize );

	connect( ui->gridSizeSlider, &QSlider::valueChanged,
			 ui->computerMonitoringView, &ComputerMonitoringView::setComputerScreenSize );
	connect( ui->computerMonitoringView, &ComputerMonitoringView::computerScreenSizeAdjusted,
			 ui->gridSizeSlider, &QSlider::setValue );
	connect( ui->autoFitButton, &QToolButton::clicked,
			 ui->computerMonitoringView, &ComputerMonitoringView::autoAdjustComputerScreenSize );

	int size = ComputerMonitoringView::DefaultComputerScreenSize;
	if( m_masterCore.userConfig().monitoringScreenSize() >= ComputerMonitoringView::MinimumComputerScreenSize )
	{
		size = m_masterCore.userConfig().monitoringScreenSize();
	}

	ui->gridSizeSlider->setValue( size );
	ui->computerMonitoringView->setComputerScreenSize( size );


	// create the main toolbar
	ui->toolBar->layout()->setSpacing( 4 );
	ui->toolBar->toggleViewAction()->setEnabled( false );

	addToolBar( Qt::TopToolBarArea, ui->toolBar );

	addFeaturesToToolBar();
	addFeaturesToSystemTrayMenu();

	m_modeGroup->button( qHash( m_masterCore.builtinFeatures().monitoringMode().feature().uid() ) )->setChecked( true );

	// setup system tray icon
	QIcon icon( QStringLiteral(":/resources/icon16.png") );
	icon.addFile( QStringLiteral(":/resources/icon22.png") );
	icon.addFile( QStringLiteral(":/resources/icon32.png") );
	icon.addFile( QStringLiteral(":/resources/icon64.png") );

	m_systemTrayIcon.setIcon( icon );
	m_systemTrayIcon.setToolTip( tr( "%1 Master Control" ).arg( VeyonCore::applicationName() ) );
	m_systemTrayIcon.show();
	connect( &m_systemTrayIcon, &QSystemTrayIcon::activated, this, &MainWindow::handleSystemTrayEvent );

	VeyonCore::enforceBranding( this );
}




MainWindow::~MainWindow()
{
	m_systemTrayIcon.hide();
}




bool MainWindow::initAuthentication()
{
	if( VeyonCore::instance()->initAuthentication( AuthenticationCredentials::AllTypes ) )
	{
		return true;
	}

	// if we have logon credentials, assume they are fine and continue
	if( VeyonCore::authenticationCredentials().hasCredentials( AuthenticationCredentials::UserLogon ) )
	{
		return true;
	}

	QMessageBox::information( nullptr,
			tr( "Authentication impossible" ),
			tr(	"No authentication key files were found or your current ones "
				"are outdated. Please create new key files using the %1 "
				"Configurator. Alternatively set up logon authentication "
				"using the %1 Configurator. Otherwise you won't be "
				"able to access computers using %1." ).arg( VeyonCore::applicationName() ) );

	return false;
}



bool MainWindow::initAccessControl()
{
	if( VeyonCore::config().accessControlForMasterEnabled() &&
			VeyonCore::authenticationCredentials().hasCredentials( AuthenticationCredentials::UserLogon ) )
	{
		const auto accessControlResult =
				AccessControlProvider().checkAccess( VeyonCore::authenticationCredentials().logonUsername(),
													 QHostAddress( QHostAddress::LocalHost ).toString(),
													 QStringList() );
		if( accessControlResult == AccessControlProvider::AccessDeny )
		{
			qWarning() << "MainWindow::initAccessControl(): user"
					   << VeyonCore::authenticationCredentials().logonUsername()
					   << "is not allowed to access computers";
			QMessageBox::critical( nullptr, tr( "Access denied" ),
								   tr( "According to the local configuration you're not allowed "
									   "to access computers in the network. Please log in with a different "
									   "account or let your system administrator check the local configuration." ) );
			return false;
		}
	}

	return true;
}



void MainWindow::closeEvent( QCloseEvent* event )
{
	if( m_masterCore.currentMode() != m_masterCore.builtinFeatures().monitoringMode().feature().uid() )
	{
		const Feature& activeFeature = m_masterCore.featureManager().feature( m_masterCore.currentMode() );

		QMessageBox::information( this, tr( "Feature active" ),
								  tr( "The feature \"%1\" is still active. Please stop it before closing %2." ).
								  arg( activeFeature.displayName(), VeyonCore::applicationName() ) );
		event->ignore();
		return;
	}

	m_masterCore.userConfig().setWindowState( saveState().toBase64() );
	m_masterCore.userConfig().setWindowGeometry( saveGeometry().toBase64() );

	QMainWindow::closeEvent( event );
}



void MainWindow::keyPressEvent( QKeyEvent* event )
{
	switch( event->key() )
	{
	case Qt::Key_F5:
		VeyonCore::networkObjectDirectoryManager().configuredDirectory()->update();
		m_masterCore.computerControlListModel().reload();
		event->accept();
		break;
	case Qt::Key_F11:
		QWidget::setWindowState( QWidget::windowState() ^ Qt::WindowFullScreen );
		event->accept();
		break;
	default:
		QMainWindow::keyPressEvent( event );
		break;
	}
}



void MainWindow::handleSystemTrayEvent( QSystemTrayIcon::ActivationReason reason )
{
	switch( reason )
	{
		case QSystemTrayIcon::Trigger:
			setVisible( !isVisible() );
			break;
#if 0
		case QSystemTrayIcon::Context:
		{
			QMenu m( this );
			m.addAction( m_systemTrayIcon.toolTip() )->setEnabled( false );
			for( auto action : qAsConst( m_sysTrayActions ) )
			{
				m.addAction( action );
			}

			m.addSeparator();

			QMenu rcm( this );
			QAction * rc = m.addAction( tr( "Remote control" ) );
			rc->setMenu( &rcm );
			for( const auto& computer : qAsConst( m_masterCore.computerManager().computerList() ) )
			{
				rcm.addAction( computer.name() )->setData( computer.hostAddress() );
			}
			connect( &rcm, SIGNAL( triggered( QAction * ) ),
				this,
				SLOT( remoteControlClient( QAction * ) ) );

			m.addSeparator();

			QAction * qa = m.addAction(
					QIcon( ":/resources/application-exit.png" ),
					tr( "Quit" ) );
			connect( qa, SIGNAL( triggered( bool ) ),
					this, SLOT( close() ) );
			m.exec( QCursor::pos() );
			break;
		}
#endif
		default:
			break;
	}
}



void MainWindow::showAboutDialog()
{
	AboutDialog( this ).exec();
}



void MainWindow::addFeaturesToToolBar()
{
	for( const auto& feature : m_masterCore.features() )
	{
		ToolButton* btn = new ToolButton( feature.iconUrl(),
										  feature.displayName(),
										  feature.displayNameActive(),
										  feature.description() );
		connect( btn, &QToolButton::clicked, this, [=] () {
			m_masterCore.runFeature( feature, this );
			updateModeButtonGroup();
		} );
		btn->addTo( ui->toolBar );

		if( feature.testFlag( Feature::Mode ) )
		{
			btn->setCheckable( true );
			m_modeGroup->addButton( btn, qHash( feature.uid() ) );
		}

		addSubFeaturesToToolButton( btn, feature.uid() );
	}
}



void MainWindow::addSubFeaturesToToolButton( ToolButton* button, Feature::Uid parentFeatureUid )
{
	const auto subFeatures = m_masterCore.subFeatures( parentFeatureUid );

	if( subFeatures.isEmpty() )
	{
		return;
	}

	auto menu = new QMenu( button );

	for( const auto& subFeature : subFeatures )
	{
#if QT_VERSION < 0x050600
#warning Building legacy compat code for unsupported version of Qt
		auto action = menu->addAction( QIcon( subFeature.iconUrl() ), subFeature.displayName() );
		action->setShortcut( subFeature.shortcut() );
		connect( action, &QAction::triggered, this, [=] () { m_masterCore.runFeature( subFeature, this ); } );
#else
		menu->addAction( QIcon( subFeature.iconUrl() ), subFeature.displayName(), this,
						 [=]() { m_masterCore.runFeature( subFeature, this ); }, subFeature.shortcut() );
#endif
	}

	button->setMenu( menu );
}



void MainWindow::addFeaturesToSystemTrayMenu()
{
	// TODO
}



void MainWindow::updateModeButtonGroup()
{
	const Feature::Uid& monitoringMode = m_masterCore.builtinFeatures().monitoringMode().feature().uid();

	if( m_masterCore.currentMode() == monitoringMode )
	{
		m_modeGroup->button( qHash( monitoringMode ) )->setChecked( true );
	}
}
