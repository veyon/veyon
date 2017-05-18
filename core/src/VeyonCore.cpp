/*
 * VeyonCore.cpp - implementation of Veyon Core
 *
 * Copyright (c) 2006-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#ifdef VEYON_BUILD_WIN32
#include <windows.h>
#endif

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QTranslator>
#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QGroupBox>
#include <QLabel>
#include <QWizardPage>

#include "AccessControlDataBackendManager.h"
#include "VeyonConfiguration.h"
#include "VeyonRfbExt.h"
#include "LocalSystem.h"
#include "Logger.h"
#include "PasswordDialog.h"
#include "PlatformPluginManager.h"
#include "PluginManager.h"
#include "VariantStream.h"

#include "rfb/rfbclient.h"

#include <lzo/lzo1x.h>


VeyonCore *VeyonCore::s_instance = nullptr;


static void killWisPtis()
{
#ifdef VEYON_BUILD_WIN32
	int pid = -1;
	while( ( pid = LocalSystem::Process::findProcessId( "wisptis.exe" ) ) >= 0 )
	{
		HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
										PROCESS_TERMINATE |
										PROCESS_VM_READ,
										false, pid );
		if( !TerminateProcess( hProcess, 0 ) )
		{
			CloseHandle( hProcess );
			break;
		}
		CloseHandle( hProcess );
	}

	if( pid >= 0 )
	{
		LocalSystem::User user = LocalSystem::User::loggedOnUser();
		while( ( pid = LocalSystem::Process::findProcessId( "wisptis.exe", -1, &user ) ) >= 0 )
		{
			HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
											PROCESS_TERMINATE |
											PROCESS_VM_READ,
											false, pid );
			if( !TerminateProcess( hProcess, 0 ) )
			{
				CloseHandle( hProcess );
				break;
			}
			CloseHandle( hProcess );
		}
	}
#endif
}



void VeyonCore::setupApplicationParameters()
{
	QCoreApplication::setOrganizationName( "Veyon Solutions" );
	QCoreApplication::setOrganizationDomain( "veyon.org" );
	QCoreApplication::setApplicationName( "Veyon" );

	if( VeyonConfiguration().isHighDPIScalingEnabled() )
	{

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
		QApplication::setAttribute( Qt::AA_EnableHighDpiScaling );
#endif
	}
}



VeyonCore::VeyonCore( QCoreApplication* application, const QString& appComponentName ) :
	QObject( application ),
	m_config( nullptr ),
	m_logger( nullptr ),
	m_authenticationCredentials( nullptr ),
	m_cryptoCore( nullptr ),
	m_pluginManager( nullptr ),
	m_accessControlDataBackendManager( nullptr ),
	m_applicationName( "Veyon" ),
	m_userRole( RoleTeacher )
{
	killWisPtis();

	lzo_init();

	setupApplicationParameters();

	s_instance = this;

	m_config = new VeyonConfiguration( VeyonConfiguration::defaultConfiguration() );
	*m_config += VeyonConfiguration( Configuration::Store::LocalBackend );

	m_logger = new Logger( appComponentName, m_config );

	QLocale configuredLocale( QLocale::C );

	QRegExp localeRegEx( "[^(]*\\(([^)]*)\\)");
	if( localeRegEx.indexIn( config().uiLanguage() ) == 0 )
	{
		configuredLocale = QLocale( localeRegEx.cap( 1 ) );
	}

	if( configuredLocale.language() != QLocale::English )
	{
		auto tr = new QTranslator;
		if( configuredLocale == QLocale::C ||
				tr->load( QString( ":/resources/%1.qm" ).arg( configuredLocale.name() ) ) == false )
		{
			configuredLocale = QLocale::system();
			tr->load( QString( ":/resources/%1.qm" ).arg( QLocale::system().name() ) );
		}

		QLocale::setDefault( configuredLocale );

		QCoreApplication::installTranslator( tr );

		auto qtTr = new QTranslator;
#ifdef QT_TRANSLATIONS_DIR
		qtTr->load( QString( "qt_%1.qm" ).arg( configuredLocale.name() ), QT_TRANSLATIONS_DIR );
#else
		qtTr->load( QString( ":/qttranslations/qt_%1.qm" ).arg( configuredLocale.name() ) );
#endif
		QCoreApplication::installTranslator( qtTr );
	}

	if( configuredLocale.language() == QLocale::Hebrew ||
		configuredLocale.language() == QLocale::Arabic )
	{
		QApplication::setLayoutDirection( Qt::RightToLeft );
	}

	if( config().applicationName().isEmpty() == false )
	{
		m_applicationName = config().applicationName();
	}

	m_cryptoCore = new CryptoCore;

	initAuthentication( AuthenticationCredentials::None );

	m_pluginManager = new PluginManager( this );

	m_accessControlDataBackendManager = new AccessControlDataBackendManager( *m_pluginManager );
	m_platformPluginManager = new PlatformPluginManager( *m_pluginManager );

	m_platformPlugin = m_platformPluginManager->platformPlugin();
}



VeyonCore::~VeyonCore()
{
	delete m_accessControlDataBackendManager;
	m_accessControlDataBackendManager = nullptr;

	delete m_platformPluginManager;
	m_platformPluginManager = nullptr;

	delete m_pluginManager;
	m_pluginManager = nullptr;

	delete m_authenticationCredentials;
	m_authenticationCredentials = nullptr;

	delete m_cryptoCore;
	m_cryptoCore = nullptr;

	delete m_logger;
	m_logger = nullptr;

	delete m_config;
	m_config = nullptr;

	s_instance = nullptr;
}



VeyonCore* VeyonCore::instance()
{
	Q_ASSERT(s_instance != nullptr);

	return s_instance;
}




bool VeyonCore::initAuthentication( int credentialTypes )
{
	if( m_authenticationCredentials )
	{
		delete m_authenticationCredentials;
		m_authenticationCredentials = nullptr;
	}

	m_authenticationCredentials = new AuthenticationCredentials;

	bool success = false;

	if( credentialTypes & AuthenticationCredentials::UserLogon &&
			config().isLogonAuthenticationEnabled() )
	{
		if( qobject_cast<QApplication *>( QCoreApplication::instance() ) )
		{
			PasswordDialog dlg( QApplication::activeWindow() );
			if( dlg.exec() &&
				dlg.credentials().hasCredentials( AuthenticationCredentials::UserLogon ) )
			{
				m_authenticationCredentials->setLogonUsername( dlg.username() );
				m_authenticationCredentials->setLogonPassword( dlg.password() );

				success = true;
			}
			else
			{
				success = false;
			}
		}
		else
		{
			success = false;
		}
	}

	if( credentialTypes & AuthenticationCredentials::PrivateKey &&
			config().isKeyAuthenticationEnabled() )
	{
		const QString privKeyFile = LocalSystem::Path::privateKeyPath( userRole() );
		qDebug() << "Loading private key" << privKeyFile << "for role" << userRole();
		if( m_authenticationCredentials->loadPrivateKey( privKeyFile ) )
		{
			success = true;
		}
		else
		{
			success = false;
		}
	}

	return success;
}



QString VeyonCore::applicationName()
{
	return instance()->m_applicationName;
}



void VeyonCore::enforceBranding( QWidget *topLevelWidget )
{
	auto labels = topLevelWidget->findChildren<QLabel *>();
	for( auto label : labels )
	{
		label->setText( label->text().replace( "Veyon", VeyonCore::applicationName() ) );
	}

	auto buttons = topLevelWidget->findChildren<QAbstractButton *>();
	for( auto button : buttons )
	{
		button->setText( button->text().replace( "Veyon", VeyonCore::applicationName() ) );
	}

	auto groupBoxes = topLevelWidget->findChildren<QGroupBox *>();
	for( auto groupBox : groupBoxes )
	{
		groupBox->setTitle( groupBox->title().replace( "Veyon", VeyonCore::applicationName() ) );
	}

	auto actions = topLevelWidget->findChildren<QAction *>();
	for( auto action : actions )
	{
		action->setText( action->text().replace( "Veyon", VeyonCore::applicationName() ) );
	}

	auto widgets = topLevelWidget->findChildren<QWidget *>();
	for( auto widget : widgets )
	{
		widget->setWindowTitle( widget->windowTitle().replace( "Veyon", VeyonCore::applicationName() ) );
	}

	auto wizardPages = topLevelWidget->findChildren<QWizardPage *>();
	for( auto wizardPage : wizardPages )
	{
		wizardPage->setTitle( wizardPage->title().replace( "Veyon", VeyonCore::applicationName() ) );
	}

	topLevelWidget->setWindowTitle( topLevelWidget->windowTitle().replace( "Veyon", VeyonCore::applicationName() ) );
}



QString VeyonCore::userRoleName( VeyonCore::UserRole role )
{
	static const char *userRoleNames[] =
	{
		"none",
		"teacher",
		"admin",
		"supporter",
		"other"
	} ;

	return userRoleNames[role];
}
