/*
 * MainWindow.cpp - implementation of MainWindow class
 *
 * Copyright (c) 2010-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#include <italcconfig.h>

#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtGui/QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressBar>
#include <QProgressDialog>

#include "Configuration/XmlStore.h"
#include "Configuration/UiMapping.h"

#include "AboutDialog.h"
#include "KeyFileAssistant.h"
#include "FileSystemBrowser.h"
#include "ConfiguratorCore.h"
#include "ItalcConfiguration.h"
#include "LocalSystem.h"
#include "LogonAclSettings.h"
#include "LogonAuthentication.h"
#include "MainWindow.h"
#include "PasswordDialog.h"

#include "ui_MainWindow.h"

#include <QDebug>
#include <QImage>


MainWindow::MainWindow() :
	QMainWindow(),
	ui( new Ui::MainWindow ),
	m_configChanged( false )
{
	ui->setupUi( this );

	setWindowTitle( tr( "%1 Configurator %2" ).arg( ItalcCore::applicationName() ).arg( ITALC_VERSION ) );

	// reset all widget's values to current configuration
	reset();

	// if local configuration is incomplete, re-enable the apply button
	if( ItalcConfiguration(
			Configuration::Store::LocalBackend ).data().size() <
										ItalcCore::config->data().size() )
	{
		configurationChanged();
	}

	// connect widget signals to configuration property write methods
	FOREACH_ITALC_VNC_SERVER_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_ITALC_DEMO_SERVER_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_ITALC_NETWORK_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
	FOREACH_ITALC_AUTHENTICATION_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);

	for( auto page : findChildren<ConfigurationPage *>() )
	{
		page->connectWidgetsToProperties();
	}

#define CONNECT_BUTTON_SLOT(name) \
			connect( ui->name, SIGNAL( clicked() ), this, SLOT( name() ) );

	CONNECT_BUTTON_SLOT( openPublicKeyBaseDir );
	CONNECT_BUTTON_SLOT( openPrivateKeyBaseDir );

	CONNECT_BUTTON_SLOT( launchKeyFileAssistant );
	CONNECT_BUTTON_SLOT( testLogonAuthentication );

	CONNECT_BUTTON_SLOT( generateBugReportArchive );

	connect( ui->buttonBox, SIGNAL( clicked( QAbstractButton * ) ),
				this, SLOT( resetOrApply( QAbstractButton * ) ) );

	connect( ui->actionLoadSettings, SIGNAL( triggered() ),
				this, SLOT( loadSettingsFromFile() ) );
	connect( ui->actionSaveSettings, SIGNAL( triggered() ),
				this, SLOT( saveSettingsToFile() ) );

	connect( ui->actionAboutQt, SIGNAL( triggered() ),
				QApplication::instance(), SLOT( aboutQt() ) );

	connect( ItalcCore::config, SIGNAL( configurationChanged() ),
				this, SLOT( configurationChanged() ) );

	ItalcCore::enforceBranding( this );
}




MainWindow::~MainWindow()
{
}



void MainWindow::reset( bool onlyUI )
{
	if( onlyUI == false )
	{
		ItalcCore::config->clear();
		*ItalcCore::config += ItalcConfiguration::defaultConfiguration();
		*ItalcCore::config += ItalcConfiguration( Configuration::Store::LocalBackend );
	}

#ifdef ITALC_BUILD_WIN32
	// always make sure we do not have a LogonACL string in our config
	ItalcCore::config->removeValue( "LogonACL", "Authentication" );

	// revert LogonACL to what has been saved in the encoded logon ACL
	LogonAclSettings().setACL(
		ItalcCore::config->value( "EncodedLogonACL", "Authentication" ) );
#endif

	FOREACH_ITALC_VNC_SERVER_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_ITALC_DEMO_SERVER_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_ITALC_NETWORK_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_ITALC_AUTHENTICATION_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);

	for( auto page : findChildren<ConfigurationPage *>() )
	{
		page->resetWidgets();
	}

	ui->buttonBox->setEnabled( false );
	m_configChanged = false;
}




void MainWindow::apply()
{
#ifdef ITALC_BUILD_WIN32
	ItalcCore::config->setValue( "EncodedLogonACL", LogonAclSettings().acl(),
															"Authentication" );
#endif
	if( ConfiguratorCore::applyConfiguration( *ItalcCore::config ) )
	{
#ifdef ITALC_BUILD_WIN32
		if( ui->generalConfigurationPage->isServiceRunning() &&
			QMessageBox::question( this, tr( "Restart %1 Service" ).arg( ItalcCore::applicationName() ),
				tr( "All settings were saved successfully. In order to take "
					"effect the %1 service needs to be restarted. "
					"Restart it now?" ).arg( ItalcCore::applicationName() ),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes ) == QMessageBox::Yes )
		{
			ui->generalConfigurationPage->stopService();
			ui->generalConfigurationPage->startService();
		}
#endif
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



void MainWindow::openPublicKeyBaseDir()
{
	FileSystemBrowser( FileSystemBrowser::ExistingDirectory ).
												exec( ui->publicKeyBaseDir );
}




void MainWindow::openPrivateKeyBaseDir()
{
	FileSystemBrowser( FileSystemBrowser::ExistingDirectory ).
												exec( ui->privateKeyBaseDir );
}




void MainWindow::loadSettingsFromFile()
{
	QString fileName = QFileDialog::getOpenFileName( this, tr( "Load settings from file" ),
											QDir::homePath(), tr( "XML files (*.xml)" ) );
	if( !fileName.isEmpty() )
	{
		// write current configuration to output file
		Configuration::XmlStore( Configuration::XmlStore::System,
										fileName ).load( ItalcCore::config );
		reset( true );
		configurationChanged();	// give user a chance to apply possible changes
	}
}




void MainWindow::saveSettingsToFile()
{
	QString fileName = QFileDialog::getSaveFileName( this, tr( "Save settings to file" ),
											QDir::homePath(), tr( "XML files (*.xml)" ) );
	if( !fileName.isEmpty() )
	{
		if( !fileName.endsWith( ".xml", Qt::CaseInsensitive ) )
		{
			fileName += ".xml";
		}

		bool configChangedPrevious = m_configChanged;

#ifdef ITALC_BUILD_WIN32
		ItalcCore::config->removeValue( "LogonACL", "Authentication" );
		ItalcCore::config->setValue( "EncodedLogonACL",
								LogonAclSettings().acl(), "Authentication" );
#endif

		// write current configuration to output file
		Configuration::XmlStore( Configuration::XmlStore::System,
										fileName ).flush( ItalcCore::config );

		m_configChanged = configChangedPrevious;
		ui->buttonBox->setEnabled( m_configChanged );
	}
}




void MainWindow::launchKeyFileAssistant()
{
	KeyFileAssistant().exec();
}



void MainWindow::testLogonAuthentication()
{
	PasswordDialog dlg( this );
	if( dlg.exec() )
	{
		bool result = LogonAuthentication::authenticateUser( dlg.credentials() );
		if( result )
		{
			QMessageBox::information( this, tr( "Logon authentication test" ),
							tr( "Authentication with provided credentials "
								"was successful." ) );
		}
		else
		{
			QMessageBox::critical( this, tr( "Logon authentication test" ),
							tr( "Authentication with provided credentials "
								"failed!" ) );
		}
	}
}



void MainWindow::generateBugReportArchive()
{
	FileSystemBrowser fsb( FileSystemBrowser::SaveFile );
	fsb.setShrinkPath( false );
	fsb.setExpandPath( false );
	QString outfile = fsb.exec( QDir::homePath(),
								tr( "Save bug report archive" ),
								tr( "%1 bug report archive (*.ibra.xml)" ).arg( ItalcCore::applicationName() ) );
	if( outfile.isEmpty() )
	{
		return;
	}

	if( !outfile.endsWith( ".ibra.xml" ) )
	{
		outfile += ".ibra.xml";
	}

	Configuration::XmlStore bugReportXML(
							Configuration::Store::BugReportArchive, outfile );
	Configuration::Object obj( &bugReportXML );


	// retrieve some basic system information

#ifdef ITALC_BUILD_WIN32

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

#elif defined( ITALC_BUILD_LINUX )

	QFile f( "/etc/lsb-release" );
	f.open( QFile::ReadOnly );

	const QString os = "Linux\n" + f.readAll().trimmed();

	QProcess p;
	p.start( "uname", QStringList() << "-a" );
	p.waitForFinished();
	const QString machineInfo = p.readAll().trimmed();

#endif

#ifdef ITALC_HOST_X86
	const QString buildType = "x86";
#elif defined( ITALC_HOST_X86_64 )
	const QString buildType = "x86_64";
#else
	const QString buildType = "unknown";
#endif
	obj.setValue( "OS", os, "General" );
	obj.setValue( "MachineInfo", machineInfo, "General" );
	obj.setValue( "BuildType", buildType, "General" );
	obj.setValue( "Version", ITALC_VERSION, "General" );


	// add current iTALC configuration
	obj.addSubObject( ItalcCore::config, "Configuration" );


	// compress all log files and encode them as base64
	QStringList paths;
	paths << LocalSystem::Path::expand( ItalcCore::config->logFileDirectory() );
#ifdef ITALC_BUILD_WIN32
	paths << "C:\\Windows\\Temp";
#else
	paths << "/tmp";
#endif
	foreach( const QString &p, paths )
	{
		QDir d( p );
		foreach( const QString &f, d.entryList( QStringList() << "Italc*.log" ) )
		{
			QFile logfile( d.absoluteFilePath( f ) );
			logfile.open( QFile::ReadOnly );
			QByteArray data = qCompress( logfile.readAll() ).toBase64();
			obj.setValue( QFileInfo( logfile ).baseName(), data, "LogFiles" );
		}
	}

	// write the file
	obj.flushStore();

	QMessageBox::information( this, tr( "%1 bug report archive saved" ).arg( ItalcCore::applicationName() ),
			tr( "An %1 bug report archive has been saved to %2. "
				"It includes %3 log files and information about your "
				"operating system. You can attach it to a bug report." ).arg( ItalcCore::applicationName() ).
				arg( QDTNS( outfile ) ).arg( ItalcCore::applicationName() ) );
}



void MainWindow::aboutItalc()
{
	AboutDialog( this ).exec();
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

	// make sure to revert the LogonACL
	reset();

	closeEvent->accept();
	QMainWindow::closeEvent( closeEvent );
}
