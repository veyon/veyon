/*
 * MainWindow.cpp - implementation of MainWindow class
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifdef ITALC_BUILD_WIN32
#include <windows.h>
#include <QtCore/QSettings>
#endif

#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtGui/QCloseEvent>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

#include "Configuration/XmlStore.h"
#include "Configuration/UiMapping.h"

#include "AccessKeyAssistant.h"
#include "FileSystemBrowser.h"
#include "ImcCore.h"
#include "ItalcConfiguration.h"
#include "Logger.h"
#include "MainWindow.h"

#include "ui_MainWindow.h"


#ifdef ITALC_BUILD_WIN32
class LogonAclSettings
{
public:
	LogonAclSettings() :
		m_s( "HKEY_LOCAL_MACHINE\\Software\\iTALC Solutions\\"
							"iTALC\\Authentication", QSettings::NativeFormat )
	{
	}

	QSettings *operator->()
	{
		return &m_s;
	}

private:
	QSettings m_s;
} ;


#endif

MainWindow::MainWindow() :
	QMainWindow(),
	ui( new Ui::MainWindow ),
	m_configChanged( false )
{
	ui->setupUi( this );

	setWindowTitle( tr( "iTALC Management Console %1" ).arg( ITALC_VERSION ) );

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
	FOREACH_ITALC_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY)

#define CONNECT_BUTTON_SLOT(name) \
			connect( ui->name, SIGNAL( clicked() ), this, SLOT( name() ) );

	CONNECT_BUTTON_SLOT( startService );
	CONNECT_BUTTON_SLOT( stopService );

	CONNECT_BUTTON_SLOT( openLogFileDirectory );

	CONNECT_BUTTON_SLOT( openGlobalConfig );
	CONNECT_BUTTON_SLOT( openPersonalConfig );
	CONNECT_BUTTON_SLOT( openSnapshotDirectory );

	CONNECT_BUTTON_SLOT( openPublicKeyBaseDir );
	CONNECT_BUTTON_SLOT( openPrivateKeyBaseDir );

	CONNECT_BUTTON_SLOT( launchAccessKeyAssistant );
	CONNECT_BUTTON_SLOT( manageACLs );
	CONNECT_BUTTON_SLOT( addLogonGroup );
	CONNECT_BUTTON_SLOT( removeLogonGroup );

	connect( ui->buttonBox, SIGNAL( clicked( QAbstractButton * ) ),
				this, SLOT( resetOrApply( QAbstractButton * ) ) );

	connect( ui->actionLoadSettings, SIGNAL( triggered() ),
				this, SLOT( loadSettingsFromFile() ) );
	connect( ui->actionSaveSettings, SIGNAL( triggered() ),
				this, SLOT( saveSettingsToFile() ) );

	updateServiceControl();

	QTimer *serviceUpdateTimer = new QTimer( this );
	serviceUpdateTimer->start( 2000 );

	connect( serviceUpdateTimer, SIGNAL( timeout() ),
				this, SLOT( updateServiceControl() ) );

	connect( ItalcCore::config, SIGNAL( configurationChanged() ),
				this, SLOT( configurationChanged() ) );

	// show/hide platform-specific controls
#ifdef ITALC_BUILD_WIN32
	ui->logonGroupManagement->hide();
#else
	ui->manageACLs->hide();

	updateLogonGroupsUI();
#endif
}




MainWindow::~MainWindow()
{
}



void MainWindow::reset( bool onlyUI )
{
	if( onlyUI == false )
	{
#ifdef ITALC_BUILD_WIN32
		LogonAclSettings s;
		s->remove( "NewLogonACL" );
		// previously saved ACL existing?
		if( s->contains( "OldLogonACL" ) )
		{
			// then roll back
			s->setValue( "LogonACL", s->value( "OldLogonACL" ) );
		}
		else
		{
			// otherwise save current ACL as old state for future reset()s
			s->setValue( "OldLogonACL", s->value( "LogonACL" ) );
		}
#endif
		ItalcCore::config->clear();
		*ItalcCore::config += ItalcConfiguration::defaultConfiguration();
		*ItalcCore::config += ItalcConfiguration( Configuration::Store::LocalBackend );
	}

	FOREACH_ITALC_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY)

	ui->buttonBox->setEnabled( false );
	m_configChanged = false;
}




void MainWindow::apply()
{
	reloadLogonACL();

	ImcCore::applyConfiguration( *ItalcCore::config );

	ui->buttonBox->setEnabled( false );
	m_configChanged = false;
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




void MainWindow::startService()
{
	QProcess::startDetached( ImcCore::icaFilePath(),
								QStringList() << "-startservice" );

	updateServiceControl();
}




void MainWindow::stopService()
{
	QProcess::startDetached( ImcCore::icaFilePath(),
								QStringList() << "-stopservice" );

	updateServiceControl();
}




void MainWindow::updateServiceControl()
{
	bool running = isServiceRunning();
#ifdef ITALC_BUILD_WIN32
	ui->startService->setEnabled( !running );
	ui->stopService->setEnabled( running );
#else
	ui->startService->setEnabled( false );
	ui->stopService->setEnabled( false );
#endif
	ui->serviceState->setText( running ? tr( "Running" ) : tr( "Stopped" ) );
}




void MainWindow::openLogFileDirectory()
{
	FileSystemBrowser( FileSystemBrowser::ExistingDirectory ).
												exec( ui->logFileDirectory );
}




void MainWindow::openGlobalConfig()
{
	FileSystemBrowser( FileSystemBrowser::ExistingFile ).
										exec( ui->globalConfigurationPath );
}




void MainWindow::openPersonalConfig()
{
	FileSystemBrowser( FileSystemBrowser::ExistingFile ).
										exec( ui->personalConfigurationPath );
}




void MainWindow::openSnapshotDirectory()
{
	FileSystemBrowser( FileSystemBrowser::ExistingDirectory ).
												exec( ui->snapshotDirectory );
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
#ifdef ITALC_BUILD_WIN32
		LogonAclSettings()->setValue( "NewLogonACL",
					ItalcCore::config->value( "LogonACL", "Authentication" ) );
#endif
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

		reloadLogonACL();

		// write current configuration to output file
		Configuration::XmlStore( Configuration::XmlStore::System,
										fileName ).flush( ItalcCore::config );
	}
}




void MainWindow::launchAccessKeyAssistant()
{
	AccessKeyAssistant().exec();
}



#ifdef ITALC_BUILD_WIN32
void Win32AclEditor( HWND hwnd );
#endif

void MainWindow::manageACLs()
{
#ifdef ITALC_BUILD_WIN32
	if( LogonAclSettings()->contains( "NewLogonACL" ) )
	{
		QMessageBox::information( this, windowTitle(),
			tr( "You have to apply the configuration first before managing "
				"ACLs." ) );
		return;
	}
	Win32AclEditor( winId() );
	configurationChanged();
#endif
}




void MainWindow::addLogonGroup()
{
	QStringList logonGroups = ItalcCore::config->logonGroups();
	foreach( QListWidgetItem *item, ui->allGroups->selectedItems() )
	{
		logonGroups.removeAll( item->text() );
		logonGroups += item->text();
	}
	logonGroups.removeAll( "" );
	ItalcCore::config->setLogonGroups( logonGroups );

	updateLogonGroupsUI();
}




void MainWindow::removeLogonGroup()
{
	QStringList logonGroups = ItalcCore::config->logonGroups();
	foreach( QListWidgetItem *item, ui->logonGroups->selectedItems() )
	{
		logonGroups.removeAll( item->text() );
	}
	logonGroups.removeAll( "" );
	ItalcCore::config->setLogonGroups( logonGroups );

	updateLogonGroupsUI();
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




void MainWindow::reloadLogonACL()
{
#ifdef ITALC_BUILD_WIN32
	LogonAclSettings s;
	if( s->contains( "NewLogonACL" ) )
	{
		ItalcCore::config->setValue( "LogonACL",
						s->value( "NewLogonACL" ).toString(), "Authentication" );
		s->remove( "NewLogonACL" );
	}
	else
	{
		ItalcCore::config->setValue( "LogonACL",
						s->value( "LogonACL" ).toString(), "Authentication" );
	}
#endif
}




void MainWindow::updateLogonGroupsUI()
{
	QProcess p;
	p.start( "getent", QStringList() << "group" );
	p.waitForFinished();

	QStringList groups = QString( p.readAll() ).split( '\n' );
	QStringList groupNames;
	foreach( const QString &group, groups )
	{
		groupNames += group.split( ':' ).first();
	}
	static const char *ignoredGroups[] = {
		"root",
		"daemon",
		"bin",
		"tty",
		"disk",
		"lp",
		"mail",
		"news",
		"uucp",
		"man",
		"proxy",
		"kmem",
		"dialout",
		"fax",
		"voice",
		"cdrom",
		"tape",
		"audio",
		"dip",
		"www-data",
		"backup",
		"list",
		"irc",
		"src",
		"gnats",
		"shadow",
		"utmp",
		"video",
		"sasl",
		"plugdev",
		"games",
		"users",
		"nogroup",
		"libuuid",
		"syslog",
		"fuse",
		"lpadmin",
		"ssl-cert",
		"messagebus",
		"crontab",
		"mlocate",
		"avahi-autoipd",
		"netdev",
		"saned",
		"sambashare",
		"haldaemon",
		"polkituser",
		"mysql",
		"avahi",
		"klog",
		"floppy",
		"oprofile",
		"netdev",
		"dirmngr",
		"vboxusers",
		"",
		NULL };
	for( int i = 0; ignoredGroups[i] != NULL; ++i )
	{
		groupNames.removeAll( ignoredGroups[i] );
	}

	ui->logonGroupManagement->setUpdatesEnabled( false );

	ui->allGroups->clear();
	ui->logonGroups->clear();

	const QStringList logonGroups = ItalcCore::config->logonGroups();
	foreach( const QString &g, groupNames )
	{
		if( logonGroups.contains( g ) )
		{
			ui->logonGroups->addItem( g );
		}
		else
		{
			ui->allGroups->addItem( g );
		}
	}

	ui->logonGroupManagement->setUpdatesEnabled( true );
}




bool MainWindow::isServiceRunning()
{
#ifdef ITALC_BUILD_WIN32
	SC_HANDLE hsrvmanager = OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT );
	if( !hsrvmanager )
	{
		ilog_failed( "OpenSCManager()" );
		return false;
	}

	SC_HANDLE hservice = OpenService( hsrvmanager, "icas", SERVICE_QUERY_STATUS );
	if( !hservice )
	{
		ilog_failed( "OpenService()" );
		CloseServiceHandle( hsrvmanager );
		return false;
	}

	SERVICE_STATUS status;
	QueryServiceStatus( hservice, &status );

	CloseServiceHandle( hservice );
	CloseServiceHandle( hsrvmanager );

	return( status.dwCurrentState == SERVICE_RUNNING );
#else
	return false;
#endif
}
