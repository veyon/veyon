/*
 * MainWindow.cpp - implementation of MainWindow class
 *
 * Copyright (c) 2004-2020 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
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
#include "ComputerSelectPanel.h"
#include "ScreenshotManagementPanel.h"
#include "FeatureManager.h"
#include "MonitoringMode.h"
#include "NetworkObjectDirectory.h"
#include "NetworkObjectDirectoryManager.h"
#include "SlideshowPanel.h"
#include "SpotlightPanel.h"
#include "ToolButton.h"
#include "VeyonConfiguration.h"
#include "VeyonMaster.h"
#include "UserConfig.h"

#include "ui_MainWindow.h"


MainWindow::MainWindow( VeyonMaster &masterCore, QWidget* parent ) :
	QMainWindow( parent ),
	ui( new Ui::MainWindow ),
	m_master( masterCore ),
	m_modeGroup( new QButtonGroup( this ) )
{
	ui->setupUi( this );

	setWindowTitle( QStringLiteral( "%1 Master" ).arg( VeyonCore::applicationName() ) );

	restoreState( QByteArray::fromBase64( m_master.userConfig().windowState().toUtf8() ) );
	restoreGeometry( QByteArray::fromBase64( m_master.userConfig().windowGeometry().toUtf8() ) );

	ui->computerMonitoringWidget->setVeyonMaster( m_master );

	// add widgets to status bar
	ui->statusBar->addWidget( ui->panelButtons );
	ui->statusBar->addWidget( ui->spacerLabel1 );
	ui->statusBar->addWidget( ui->filterLineEdit, 2 );
	ui->statusBar->addWidget( ui->filterPoweredOnComputersButton );
	ui->statusBar->addWidget( ui->spacerLabel2, 1 );
	ui->statusBar->addWidget( ui->gridSizeSlider, 2 );
	ui->statusBar->addWidget( ui->autoFitButton );
	ui->statusBar->addWidget( ui->spacerLabel3 );
	ui->statusBar->addWidget( ui->useCustomComputerArrangementButton );
	ui->statusBar->addWidget( ui->alignComputersButton );
	ui->statusBar->addWidget( ui->spacerLabel4 );
	ui->statusBar->addWidget( ui->aboutButton );

	// create all views
	auto splitter = new QSplitter( Qt::Horizontal, ui->centralWidget );
	splitter->setChildrenCollapsible( false );

	auto monitoringSplitter = new QSplitter( Qt::Vertical, splitter );
	monitoringSplitter->setChildrenCollapsible( false );

	auto specialMonitoringSplitter = new QSplitter( Qt::Horizontal, monitoringSplitter );
	specialMonitoringSplitter->setChildrenCollapsible( false );

	ui->centralLayout->addWidget( splitter );

	m_computerSelectPanel = new ComputerSelectPanel( m_master.computerManager(), splitter );
	m_screenshotManagementPanel = new ScreenshotManagementPanel( splitter );

	auto slideshowPanel = new SlideshowPanel( m_master.userConfig(), ui->computerMonitoringWidget, specialMonitoringSplitter );
	auto spotlightPanel = new SpotlightPanel( m_master.userConfig(), ui->computerMonitoringWidget, specialMonitoringSplitter );

	specialMonitoringSplitter->addWidget( slideshowPanel );
	specialMonitoringSplitter->addWidget( spotlightPanel );

	monitoringSplitter->addWidget( specialMonitoringSplitter );
	monitoringSplitter->addWidget( ui->computerMonitoringWidget );

	splitter->addWidget( m_computerSelectPanel );
	splitter->addWidget( m_screenshotManagementPanel );
	splitter->addWidget( monitoringSplitter );

	// hide views per default and connect related button
	m_computerSelectPanel->hide();
	slideshowPanel->hide();
	spotlightPanel->hide();
	m_screenshotManagementPanel->hide();

	connect( ui->computerSelectPanelButton, &QAbstractButton::toggled,
			 m_computerSelectPanel, &QWidget::setVisible );
	connect( ui->slideshowPanelButton, &QAbstractButton::toggled, slideshowPanel, &QWidget::setVisible );
	connect( ui->spotlightPanelButton, &QAbstractButton::toggled, spotlightPanel, &QWidget::setVisible );
	connect( ui->screenshotManagementPanelButton, &QAbstractButton::toggled,
			 m_screenshotManagementPanel, &QWidget::setVisible );

	if( VeyonCore::config().autoOpenComputerSelectPanel() )
	{
		ui->computerSelectPanelButton->setChecked( true );
	}

	// initialize search filter
	ui->filterPoweredOnComputersButton->setChecked( m_master.userConfig().filterPoweredOnComputers() );
	connect( ui->filterLineEdit, &QLineEdit::textChanged,
			 ui->computerMonitoringWidget, &ComputerMonitoringWidget::setSearchFilter );
	connect( ui->filterPoweredOnComputersButton, &QToolButton::toggled,
			 ui->computerMonitoringWidget, &ComputerMonitoringWidget::setFilterPoweredOnComputers );

	// initialize monitoring screen size slider
	ui->gridSizeSlider->setMinimum( ComputerMonitoringWidget::MinimumComputerScreenSize );
	ui->gridSizeSlider->setMaximum( ComputerMonitoringWidget::MaximumComputerScreenSize );

	connect( ui->gridSizeSlider, &QSlider::valueChanged,
			 ui->computerMonitoringWidget, &ComputerMonitoringWidget::setComputerScreenSize );
	connect( ui->computerMonitoringWidget, &ComputerMonitoringWidget::computerScreenSizeAdjusted,
			 ui->gridSizeSlider, &QSlider::setValue );
	connect( ui->autoFitButton, &QToolButton::clicked,
			 ui->computerMonitoringWidget, &ComputerMonitoringWidget::autoAdjustComputerScreenSize );

	int size = ComputerMonitoringWidget::DefaultComputerScreenSize;
	if( m_master.userConfig().monitoringScreenSize() >= ComputerMonitoringWidget::MinimumComputerScreenSize )
	{
		size = m_master.userConfig().monitoringScreenSize();
	}

	ui->gridSizeSlider->setValue( size );
	ui->computerMonitoringWidget->setComputerScreenSize( size );

	// initialize computer placement controls
	ui->useCustomComputerArrangementButton->setChecked( m_master.userConfig().useCustomComputerPositions() );
	connect( ui->useCustomComputerArrangementButton, &QToolButton::toggled,
			 ui->computerMonitoringWidget, &ComputerMonitoringWidget::setUseCustomComputerPositions );
	connect( ui->alignComputersButton, &QToolButton::clicked,
			 ui->computerMonitoringWidget, &ComputerMonitoringWidget::alignComputers );


	// create the main toolbar
	ui->toolBar->layout()->setSpacing( 2 );
	ui->toolBar->toggleViewAction()->setEnabled( false );

	addToolBar( Qt::TopToolBarArea, ui->toolBar );

	addFeaturesToToolBar();
	reloadSubFeatures();

	m_modeGroup->button( static_cast<int>( qHash( VeyonCore::builtinFeatures().monitoringMode().feature().uid() ) ) )->setChecked( true );

	VeyonCore::enforceBranding( this );
}



MainWindow::~MainWindow()
{
	delete ui;
}



bool MainWindow::initAuthentication()
{
	if( VeyonCore::instance()->initAuthentication() )
	{
		return true;
	}

	if( VeyonCore::config().authenticationMethod() == VeyonCore::AuthenticationMethod::KeyFileAuthentication )
	{
		QMessageBox::information( nullptr,
				tr( "Authentication impossible" ),
				tr(	"No authentication key files were found or your current ones "
					"are outdated. Please create new key files using the %1 "
					"Configurator. Alternatively set up logon authentication "
					"using the %1 Configurator. Otherwise you won't be "
					"able to access computers using %1." ).arg( VeyonCore::applicationName() ) );

	}

	return false;
}



bool MainWindow::initAccessControl()
{
	if( VeyonCore::config().accessControlForMasterEnabled() &&
			VeyonCore::authenticationCredentials().hasCredentials( AuthenticationCredentials::Type::UserLogon ) )
	{
		const auto accessControlResult =
				AccessControlProvider().checkAccess( VeyonCore::authenticationCredentials().logonUsername(),
													 QHostAddress( QHostAddress::LocalHost ).toString(),
													 QStringList() );
		if( accessControlResult == AccessControlProvider::Access::Deny )
		{
			vWarning() << "user" << VeyonCore::authenticationCredentials().logonUsername()
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



void MainWindow::reloadSubFeatures()
{
	for( const auto& feature : m_master.features() )
	{
		auto button = ui->toolBar->findChild<QToolButton *>( feature.name() );
		if( button )
		{
			addSubFeaturesToToolButton( button, feature );
		}
	}
}



ComputerControlInterfaceList MainWindow::selectedComputerControlInterfaces() const
{
	return ui->computerMonitoringWidget->selectedComputerControlInterfaces();
}



void MainWindow::closeEvent( QCloseEvent* event )
{
	if( m_master.currentMode() != VeyonCore::builtinFeatures().monitoringMode().feature().uid() )
	{
		const Feature& activeFeature = m_master.featureManager().feature( m_master.currentMode() );

		QMessageBox::information( this, tr( "Feature active" ),
								  tr( "The feature \"%1\" is still active. Please stop it before closing %2." ).
								  arg( activeFeature.displayName(), VeyonCore::applicationName() ) );
		event->ignore();
		return;
	}

	m_master.userConfig().setWindowState( QString::fromLatin1( saveState().toBase64() ) );
	m_master.userConfig().setWindowGeometry( QString::fromLatin1( saveGeometry().toBase64() ) );

	QMainWindow::closeEvent( event );
}



void MainWindow::keyPressEvent( QKeyEvent* event )
{
	switch( event->key() )
	{
	case Qt::Key_F5:
		VeyonCore::networkObjectDirectoryManager().configuredDirectory()->update();
		m_master.computerControlListModel().reload();
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



void MainWindow::showAboutDialog()
{
	AboutDialog( this ).exec();
}



void MainWindow::addFeaturesToToolBar()
{
	for( const auto& feature : m_master.features() )
	{
		if( feature.testFlag( Feature::Internal ) )
		{
			continue;
		}

		ToolButton* btn = new ToolButton( QIcon( feature.iconUrl() ),
										  feature.displayName(),
										  feature.displayNameActive(),
										  feature.description(),
										  feature.shortcut() );
		connect( btn, &QToolButton::clicked, this, [=] () {
			m_master.runFeature( feature );
			updateModeButtonGroup();
			if( feature.testFlag( Feature::Mode ) )
			{
				reloadSubFeatures();
			}
		} );
		btn->setObjectName( feature.name() );
		btn->addTo( ui->toolBar );

		if( feature.testFlag( Feature::Mode ) )
		{
			btn->setCheckable( true );
			m_modeGroup->addButton( btn, buttonId( feature ) );
		}
	}
}



void MainWindow::addSubFeaturesToToolButton( QToolButton* button, const Feature& parentFeature )
{
	if( button->menu() )
	{
		button->menu()->close();
		button->menu()->deleteLater();
		button->setMenu( nullptr );
	}

	const auto parentFeatureIsMode = parentFeature.testFlag( Feature::Mode );
	const auto subFeatures = m_master.subFeatures( parentFeature.uid() );

	if( subFeatures.isEmpty() ||
		( parentFeatureIsMode && button->isChecked() ) )
	{
		return;
	}

	auto menu = new QMenu( button );
	menu->setObjectName( parentFeature.name() );
	menu->setToolTipsVisible( true );

	for( const auto& subFeature : subFeatures )
	{
#if QT_VERSION < 0x050600
#warning Building legacy compat code for unsupported version of Qt
		auto action = menu->addAction( QIcon( subFeature.iconUrl() ), subFeature.displayName() );
		action->setShortcut( subFeature.shortcut() );
		connect( action, &QAction::triggered, this,
			[=]() {
				m_master.runFeature( subFeature );
				if( parentFeatureIsMode )
				{
					button->setChecked( true );
					reloadSubFeatures();
				}
			}
		);
#else
		auto action = menu->addAction( QIcon( subFeature.iconUrl() ), subFeature.displayName(), this,
			[=]() {
				m_master.runFeature( subFeature );
				if( parentFeatureIsMode )
				{
					button->setChecked( true );
					reloadSubFeatures();
				}
			},
			subFeature.shortcut() );
#endif
		action->setToolTip( subFeature.description() );
		action->setObjectName( subFeature.uid().toString() );
	}

	button->setMenu( menu );
	button->setPopupMode( ToolButton::InstantPopup );
}



void MainWindow::updateModeButtonGroup()
{
	const auto& monitoringMode = VeyonCore::builtinFeatures().monitoringMode().feature();

	if( m_master.currentMode() == monitoringMode.uid() )
	{
		m_modeGroup->button( buttonId( monitoringMode ) )->setChecked( true );
	}
}
