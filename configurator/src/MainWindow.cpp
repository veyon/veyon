/*
 * MainWindow.cpp - implementation of MainWindow class
 *
 * Copyright (c) 2010-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "VeyonCore.h"

#include <QDir>
#include <QProcess>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>

#include "Configuration/JsonStore.h"
#include "Configuration/LocalStore.h"

#include "AboutDialog.h"
#include "FileSystemBrowser.h"
#include "ConfigurationPagePluginInterface.h"
#include "ConfiguratorCore.h"
#include "VeyonConfiguration.h"
#include "LocalSystem.h"
#include "MainWindow.h"
#include "PluginManager.h"

#include "ui_MainWindow.h"


MainWindow::MainWindow() :
	QMainWindow(),
	ui( new Ui::MainWindow ),
	m_configChanged( false )
{
	ui->setupUi( this );

	setWindowTitle( tr( "%1 Configurator %2" ).arg( VeyonCore::applicationName(), VEYON_VERSION ) );

	loadConfigurationPagePlugins();

	// reset all widget's values to current configuration
	reset( true );

	// if local configuration is incomplete, re-enable the apply button
	if( VeyonConfiguration(
			Configuration::Store::LocalBackend ).data().size() <
										VeyonCore::config().data().size() )
	{
		configurationChanged();
	}

	const auto pages = findChildren<ConfigurationPage *>();
	for( auto page : pages )
	{
		page->connectWidgetsToProperties();
	}

	connect( ui->generateBugReportArchive, &QPushButton::clicked, this, &MainWindow::generateBugReportArchive );

	connect( ui->buttonBox, &QDialogButtonBox::clicked, this, &MainWindow::resetOrApply );

	connect( ui->actionLoadSettings, &QAction::triggered, this, &MainWindow::loadSettingsFromFile );
	connect( ui->actionSaveSettings, &QAction::triggered, this, &MainWindow::saveSettingsToFile );

	connect( ui->actionAboutQt, &QAction::triggered, QApplication::instance(), &QApplication::aboutQt );

	connect( &VeyonCore::config(), &VeyonConfiguration::configurationChanged, this, &MainWindow::configurationChanged );

	VeyonCore::enforceBranding( this );
}




MainWindow::~MainWindow()
{
}



void MainWindow::reset( bool onlyUI )
{
	if( onlyUI == false )
	{
		VeyonCore::config().clear();
		VeyonCore::config() += VeyonConfiguration::defaultConfiguration();
		VeyonCore::config() += VeyonConfiguration( Configuration::Store::LocalBackend );
	}

	const auto pages = findChildren<ConfigurationPage *>();
	for( auto page : pages )
	{
		page->resetWidgets();
	}

	ui->buttonBox->setEnabled( false );
	m_configChanged = false;
}




void MainWindow::apply()
{
	if( ConfiguratorCore::applyConfiguration( VeyonCore::config() ) )
	{
		const auto pages = findChildren<ConfigurationPage *>();
		for( auto page : pages )
		{
			page->applyConfiguration();
		}

		ui->buttonBox->setEnabled( false );
		m_configChanged = false;
	}
}




void MainWindow::configurationChanged()
{
	ui->buttonBox->setEnabled( true );
	m_configChanged = true;
}




void MainWindow::resetOrApply( QAbstractButton *btn )
{
	if( ui->buttonBox->standardButton( btn ) & QDialogButtonBox::Apply )
	{
		apply();
	}
	else if( ui->buttonBox->standardButton( btn ) & QDialogButtonBox::Reset )
	{
		reset();
	}
}



void MainWindow::loadSettingsFromFile()
{
	QString fileName = QFileDialog::getOpenFileName( this, tr( "Load settings from file" ),
											QDir::homePath(), tr( "JSON files (*.json)" ) );
	if( !fileName.isEmpty() )
	{
		// write current configuration to output file
		Configuration::JsonStore( Configuration::JsonStore::System, fileName ).load( &VeyonCore::config() );
		reset( true );
		configurationChanged();	// give user a chance to apply possible changes
	}
}




void MainWindow::saveSettingsToFile()
{
	QString fileName = QFileDialog::getSaveFileName( this, tr( "Save settings to file" ),
											QDir::homePath(), tr( "JSON files (*.json)" ) );
	if( !fileName.isEmpty() )
	{
		if( !fileName.endsWith( ".json", Qt::CaseInsensitive ) )
		{
			fileName += ".json";
		}

		bool configChangedPrevious = m_configChanged;

		// write current configuration to output file
		Configuration::JsonStore( Configuration::JsonStore::System, fileName ).flush( &VeyonCore::config() );

		m_configChanged = configChangedPrevious;
		ui->buttonBox->setEnabled( m_configChanged );
	}
}



void MainWindow::resetConfiguration()
{
	if( QMessageBox::warning( this, tr( "Reset configuration" ),
							  tr( "Do you really want to reset the local configuration and revert "
								  "all settings to their defaults?" ), QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) ==
			QMessageBox::Yes )
	{
		Configuration::LocalStore( Configuration::LocalStore::System ).clear();
		reset( false );
	}
}




void MainWindow::generateBugReportArchive()
{
	FileSystemBrowser fsb( FileSystemBrowser::SaveFile );
	fsb.setShrinkPath( false );
	fsb.setExpandPath( false );
	QString outfile = fsb.exec( QDir::homePath(),
								tr( "Save bug report archive" ),
								tr( "%1 bug report (*.json)" ).arg( VeyonCore::applicationName() ) );
	if( outfile.isEmpty() )
	{
		return;
	}

	if( !outfile.endsWith( ".json" ) )
	{
		outfile += ".json";
	}

	Configuration::JsonStore bugReportJson( Configuration::Store::BugReportArchive, outfile );
	Configuration::Object obj( &bugReportJson );


	// retrieve some basic system information

#ifdef VEYON_BUILD_WIN32

	OSVERSIONINFOEX ovi;
	ovi.dwOSVersionInfoSize = sizeof( ovi );
	GetVersionEx( (LPOSVERSIONINFO) &ovi );

	QString os = "Windows %1 SP%2 (%3.%4.%5)";
	switch( QSysInfo::windowsVersion() )
	{
	case QSysInfo::WV_NT: os = os.arg( "NT 4.0" ); break;
	case QSysInfo::WV_2000: os = os.arg( "2000" ); break;
	case QSysInfo::WV_XP: os = os.arg( "XP" ); break;
	case QSysInfo::WV_VISTA: os = os.arg( "Vista" ); break;
	case QSysInfo::WV_WINDOWS7: os = os.arg( "7" ); break;
	case QSysInfo::WV_WINDOWS8: os = os.arg( "8" ); break;
	case QSysInfo::WV_WINDOWS8_1: os = os.arg( "8.1" ); break;
	case QSysInfo::WV_WINDOWS10: os = os.arg( "10" ); break;
	default: os = os.arg( "<unknown>" );
	}

	os = os.arg( ovi.wServicePackMajor ).
			arg( ovi.dwMajorVersion ).
			arg( ovi.dwMinorVersion ).
			arg( ovi.dwBuildNumber );

	const QString machineInfo =
		QProcessEnvironment::systemEnvironment().value( "PROCESSOR_IDENTIFIER" );

#elif defined( VEYON_BUILD_LINUX )

	QFile f( "/etc/lsb-release" );
	f.open( QFile::ReadOnly );

	const QString os = "Linux\n" + f.readAll().trimmed();

	QProcess p;
	p.start( "uname", QStringList() << "-a" );
	p.waitForFinished();
	const QString machineInfo = p.readAll().trimmed();

#endif

#ifdef VEYON_HOST_X86
	const QString buildType = "x86";
#elif defined( VEYON_HOST_X86_64 )
	const QString buildType = "x86_64";
#else
	const QString buildType = "unknown";
#endif
	obj.setValue( "OS", os, "General" );
	obj.setValue( "MachineInfo", machineInfo, "General" );
	obj.setValue( "BuildType", buildType, "General" );
	obj.setValue( "Version", VEYON_VERSION, "General" );


	// add current Veyon configuration
	obj.addSubObject( &VeyonCore::config(), "Configuration" );


	// compress all log files and encode them as base64
	const QStringList paths( { LocalSystem::Path::expand( VeyonCore::config().logFileDirectory() ),
#ifdef VEYON_BUILD_WIN32
	"C:\\Windows\\Temp"
#else
	"/tmp"
#endif
		} );

	for( const QString &p : paths )
	{
		QDir d( p );
		const auto entries = d.entryList( QStringList() << "Veyon*.log" );
		for( const auto& f : entries )
		{
			QFile logfile( d.absoluteFilePath( f ) );
			logfile.open( QFile::ReadOnly );
			QByteArray data = qCompress( logfile.readAll() ).toBase64();
			obj.setValue( QFileInfo( logfile ).baseName(), data, "LogFiles" );
		}
	}

	// write the file
	obj.flushStore();

	QMessageBox::information( this, tr( "%1 bug report archive saved" ).arg( VeyonCore::applicationName() ),
			tr( "An %1 bug report archive has been saved to %2. "
				"It includes %3 log files and information about your "
				"operating system. You can attach it to a bug report." ).arg(
				VeyonCore::applicationName(), QDTNS( outfile ), VeyonCore::applicationName() ) );
}



void MainWindow::aboutVeyon()
{
	AboutDialog( this ).exec();
}



void MainWindow::loadConfigurationPagePlugins()
{
	for( auto pluginObject : qAsConst( VeyonCore::pluginManager().pluginObjects() ) )
	{
		auto pluginInterface = qobject_cast<PluginInterface *>( pluginObject );
		auto configurationPagePluginInterface = qobject_cast<ConfigurationPagePluginInterface *>( pluginObject );

		if( pluginInterface && configurationPagePluginInterface )
		{
			auto page = configurationPagePluginInterface->createConfigurationPage();
			ui->configPages->addWidget( page );

			auto item = new QListWidgetItem( page->windowIcon(), page->windowTitle() );
			item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
			ui->pageSelector->addItem( item );
		}
	}
}



void MainWindow::closeEvent( QCloseEvent *closeEvent )
{
	if( m_configChanged &&
			QMessageBox::question( this, tr( "Unsaved settings" ),
									tr( "There are unsaved settings. "
										"Quit anyway?" ),
									QMessageBox::Yes | QMessageBox::No ) !=
															QMessageBox::Yes )
	{
		closeEvent->ignore();
		return;
	}

	closeEvent->accept();
	QMainWindow::closeEvent( closeEvent );
}
