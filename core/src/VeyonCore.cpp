/*
 * VeyonCore.cpp - implementation of Veyon Core
 *
 * Copyright (c) 2006-2018 Tobias Junghans <tobydox@users.sf.net>
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

#include <veyonconfig.h>

#include <QLocale>
#include <QTranslator>
#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QGroupBox>
#include <QHostAddress>
#include <QLabel>
#include <QProcessEnvironment>
#include <QWizardPage>

#include "ComputerControlInterface.h"
#include "Filesystem.h"
#include "Logger.h"
#include "NetworkObjectDirectoryManager.h"
#include "PasswordDialog.h"
#include "PlatformPluginManager.h"
#include "PlatformCoreFunctions.h"
#include "PluginManager.h"
#include "UserGroupsBackendManager.h"
#include "VeyonConfiguration.h"


VeyonCore* VeyonCore::s_instance = nullptr;

void VeyonCore::setupApplicationParameters()
{
	QCoreApplication::setOrganizationName( QStringLiteral( "Veyon Solutions" ) );
	QCoreApplication::setOrganizationDomain( QStringLiteral( "veyon.io" ) );
	QCoreApplication::setApplicationName( QStringLiteral( "Veyon" ) );

	if( VeyonConfiguration().isHighDPIScalingEnabled() )
	{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
		QApplication::setAttribute( Qt::AA_EnableHighDpiScaling );
#endif
	}
}



VeyonCore::VeyonCore( QCoreApplication* application, const QString& appComponentName ) :
	QObject( application ),
	m_filesystem( new Filesystem ),
	m_config( nullptr ),
	m_logger( nullptr ),
	m_authenticationCredentials( nullptr ),
	m_cryptoCore( nullptr ),
	m_pluginManager( nullptr ),
	m_platformPluginManager( nullptr ),
	m_platformPlugin( nullptr ),
	m_userGroupsBackendManager( nullptr ),
	m_networkObjectDirectoryManager( nullptr ),
	m_localComputerControlInterface( nullptr ),
	m_applicationName( QStringLiteral( "Veyon" ) ),
	m_authenticationKeyName()
{
	Q_ASSERT( application != nullptr );

	setupApplicationParameters();

	s_instance = this;

	// initialize plugin manager and load platform plugins first
	m_pluginManager = new PluginManager( this );
	m_pluginManager->loadPlatformPlugins();

	// initialize platform plugin manager and initialize used platform plugin
	m_platformPluginManager = new PlatformPluginManager( *m_pluginManager );
	m_platformPlugin = m_platformPluginManager->platformPlugin();

	m_config = new VeyonConfiguration();

	m_logger = new Logger( appComponentName );

	QLocale configuredLocale( QLocale::C );

	QRegExp localeRegEx( QStringLiteral( "[^(]*\\(([^)]*)\\)") );
	if( localeRegEx.indexIn( config().uiLanguage() ) == 0 )
	{
		configuredLocale = QLocale( localeRegEx.cap( 1 ) );
	}

	if( configuredLocale.language() != QLocale::English )
	{
		auto tr = new QTranslator;
		if( configuredLocale == QLocale::C ||
				tr->load( QStringLiteral( ":/resources/%1.qm" ).arg( configuredLocale.name() ) ) == false )
		{
			configuredLocale = QLocale::system();
			tr->load( QStringLiteral( ":/resources/%1.qm" ).arg( QLocale::system().name() ) );
		}

		QLocale::setDefault( configuredLocale );

		QCoreApplication::installTranslator( tr );

		auto qtTr = new QTranslator;
#ifdef QT_TRANSLATIONS_DIR
		qtTr->load( QStringLiteral( "qt_%1.qm" ).arg( configuredLocale.name() ), QStringLiteral( QT_TRANSLATIONS_DIR ) );
#else
		qtTr->load( QStringLiteral( ":/qttranslations/qt_%1.qm" ).arg( configuredLocale.name() ) );
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

	// initialize plugin manager and load platform plugins first
	m_pluginManager = new PluginManager( this );
	m_pluginManager->loadPlatformPlugins();

	// initialize platform plugin manager and initialize used platform plugin
	m_platformPluginManager = new PlatformPluginManager( *m_pluginManager );
	m_platformPlugin = m_platformPluginManager->platformPlugin();

	m_cryptoCore = new CryptoCore;

	initAuthentication( AuthenticationCredentials::None );

	// load all other plugins
	m_pluginManager->loadPlugins();
	m_pluginManager->upgradePlugins();

	m_userGroupsBackendManager = new UserGroupsBackendManager( this );
	m_networkObjectDirectoryManager = new NetworkObjectDirectoryManager( this );

	const Computer localComputer( NetworkObject::Uid::createUuid(),
								  QStringLiteral("localhost"),
								  QHostAddress( QHostAddress::LocalHost ).toString() );
	m_localComputerControlInterface = new ComputerControlInterface( localComputer, this );
}



VeyonCore::~VeyonCore()
{
	delete m_userGroupsBackendManager;
	m_userGroupsBackendManager = nullptr;

	delete m_authenticationCredentials;
	m_authenticationCredentials = nullptr;

	delete m_cryptoCore;
	m_cryptoCore = nullptr;

	delete m_logger;
	m_logger = nullptr;

	delete m_platformPluginManager;
	m_platformPluginManager = nullptr;

	delete m_pluginManager;
	m_pluginManager = nullptr;

	delete m_config;
	m_config = nullptr;

	delete m_filesystem;
	m_filesystem = nullptr;

	s_instance = nullptr;
}



VeyonCore* VeyonCore::instance()
{
	Q_ASSERT(s_instance != nullptr);

	return s_instance;
}



QString VeyonCore::version()
{
	return QStringLiteral( VEYON_VERSION );
}



QString VeyonCore::pluginDir()
{
	return QStringLiteral( VEYON_PLUGIN_DIR );
}



QString VeyonCore::executableSuffix()
{
	return QStringLiteral( VEYON_EXECUTABLE_SUFFIX );
}



QString VeyonCore::sharedLibrarySuffix()
{
	return QStringLiteral( VEYON_SHARED_LIBRARY_SUFFIX );
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
			config().authenticationMethod() == LogonAuthentication )
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
			config().authenticationMethod() == KeyFileAuthentication )
	{
		success = initKeyFileAuthentication();
	}

	return success;
}



QString VeyonCore::applicationName()
{
	return instance()->m_applicationName;
}



void VeyonCore::enforceBranding( QWidget *topLevelWidget )
{
	const auto appName = QStringLiteral( "Veyon" );

	auto labels = topLevelWidget->findChildren<QLabel *>();
	for( auto label : labels )
	{
		label->setText( label->text().replace( appName, VeyonCore::applicationName() ) );
	}

	auto buttons = topLevelWidget->findChildren<QAbstractButton *>();
	for( auto button : buttons )
	{
		button->setText( button->text().replace( appName, VeyonCore::applicationName() ) );
	}

	auto groupBoxes = topLevelWidget->findChildren<QGroupBox *>();
	for( auto groupBox : groupBoxes )
	{
		groupBox->setTitle( groupBox->title().replace( appName, VeyonCore::applicationName() ) );
	}

	auto actions = topLevelWidget->findChildren<QAction *>();
	for( auto action : actions )
	{
		action->setText( action->text().replace( appName, VeyonCore::applicationName() ) );
	}

	auto widgets = topLevelWidget->findChildren<QWidget *>();
	for( auto widget : widgets )
	{
		widget->setWindowTitle( widget->windowTitle().replace( appName, VeyonCore::applicationName() ) );
	}

	auto wizardPages = topLevelWidget->findChildren<QWizardPage *>();
	for( auto wizardPage : wizardPages )
	{
		wizardPage->setTitle( wizardPage->title().replace( appName, VeyonCore::applicationName() ) );
	}

	topLevelWidget->setWindowTitle( topLevelWidget->windowTitle().replace( appName, VeyonCore::applicationName() ) );
}



QString VeyonCore::stripDomain( const QString& username )
{
	// remove the domain part of username (e.g. "EXAMPLE.COM\Teacher" -> "Teacher")
	int domainSeparator = username.indexOf( '\\' );
	if( domainSeparator >= 0 )
	{
		return username.mid( domainSeparator + 1 );
	}

	return username;
}



bool VeyonCore::isAuthenticationKeyNameValid( const QString& authKeyName )
{
	return QRegExp( "\\w+" ).exactMatch( authKeyName );
}



bool VeyonCore::initKeyFileAuthentication()
{
	auto authKeyName = QProcessEnvironment::systemEnvironment().value( QStringLiteral("VEYON_AUTH_KEY_NAME") );

	if( authKeyName.isEmpty() == false )
	{
		if( isAuthenticationKeyNameValid( authKeyName ) &&
				m_authenticationCredentials->loadPrivateKey( VeyonCore::filesystem().privateKeyPath( authKeyName ) ) )
		{
			m_authenticationKeyName = authKeyName;
		}
	}
	else
	{
		// try to auto-detect private key file by searching for readable file
		const auto privateKeyBaseDir = VeyonCore::filesystem().expandPath( VeyonCore::config().privateKeyBaseDir() );
		const auto privateKeyDirs = QDir( privateKeyBaseDir ).entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name );

		for( const auto& privateKeyDir : privateKeyDirs )
		{
			if( m_authenticationCredentials->loadPrivateKey( VeyonCore::filesystem().privateKeyPath( privateKeyDir ) ) )
			{
				m_authenticationKeyName = privateKeyDir;
				return true;
			}
		}
	}

	return false;
}
