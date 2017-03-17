/*
 * ItalcCore.cpp - implementation of iTALC Core
 *
 * Copyright (c) 2006-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "ItalcCore.h"
#ifdef ITALC_BUILD_WIN32
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

#include "ItalcConfiguration.h"
#include "ItalcRfbExt.h"
#include "LocalSystem.h"
#include "Logger.h"
#include "PasswordDialog.h"
#include "VariantStream.h"

#include "rfb/rfbclient.h"

#include <lzo/lzo1x.h>


ItalcCore *ItalcCore::s_instance = nullptr;


void initResources()
{
	Q_INIT_RESOURCE(core);
#ifndef QT_TRANSLATIONS_DIR
#if QT_VERSION < 0x050000
	Q_INIT_RESOURCE(qt_qm);
#endif
#endif
}



static void killWisPtis()
{
#ifdef ITALC_BUILD_WIN32
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



void ItalcCore::setupApplicationParameters()
{
	QCoreApplication::setOrganizationName( "iTALC Solutions" );
	QCoreApplication::setOrganizationDomain( "italcsolutions.org" );
	QCoreApplication::setApplicationName( "iTALC" );

	if( ItalcConfiguration().isHighDPIScalingEnabled() )
	{

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
		QApplication::setAttribute( Qt::AA_EnableHighDpiScaling );
#endif
	}
}



ItalcCore::ItalcCore( QCoreApplication* application, const QString& appComponentName ) :
	QObject( application ),
	m_config( nullptr ),
	m_logger( nullptr ),
	m_authenticationCredentials( nullptr ),
	m_cryptoCore( nullptr ),
	m_applicationName( "iTALC" ),
	m_userRole( RoleTeacher )
{
	killWisPtis();

	lzo_init();

	setupApplicationParameters();

	initResources();

	s_instance = this;

	m_config = new ItalcConfiguration( ItalcConfiguration::defaultConfiguration() );
	*m_config += ItalcConfiguration( Configuration::Store::LocalBackend );

	m_logger = new Logger( appComponentName, m_config );

	QLocale configuredLocale( QLocale::C );

	QRegExp localeRegEx( "[^(]*\\(([^)]*)\\)");
	if( localeRegEx.indexIn( config().uiLanguage() ) == 0 )
	{
		configuredLocale = QLocale( localeRegEx.cap( 1 ) );
	}

	if( configuredLocale.language() != QLocale::English )
	{
		QTranslator *tr = new QTranslator;
		if( configuredLocale == QLocale::C ||
				tr->load( QString( ":/resources/%1.qm" ).arg( configuredLocale.name() ) ) == false )
		{
			configuredLocale = QLocale::system();
			tr->load( QString( ":/resources/%1.qm" ).arg( QLocale::system().name() ) );
		}

		QLocale::setDefault( configuredLocale );

		QCoreApplication::installTranslator( tr );
	}

	QTranslator *qtTr = new QTranslator;
#ifdef QT_TRANSLATIONS_DIR
	qtTr->load( QString( "qt_%1.qm" ).arg( configuredLocale.name() ), QT_TRANSLATIONS_DIR );
#else
	qtTr->load( QString( ":/qt_%1.qm" ).arg( configuredLocale.name() ) );
#endif
	QCoreApplication::installTranslator( qtTr );

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
}



ItalcCore::~ItalcCore()
{
	delete m_authenticationCredentials;
	m_authenticationCredentials = nullptr;

	delete m_cryptoCore;
	m_cryptoCore = nullptr;

	delete m_config;
	m_config = nullptr;

	s_instance = nullptr;
}



ItalcCore* ItalcCore::instance()
{
	Q_ASSERT(s_instance != nullptr);

	return s_instance;
}




bool ItalcCore::initAuthentication( int credentialTypes )
{
	if( m_authenticationCredentials )
	{
		delete m_authenticationCredentials;
		m_authenticationCredentials = nullptr;
	}

	m_authenticationCredentials = new AuthenticationCredentials;

	bool success = true;

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

				success &= true;
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
			success &= true;
		}
		else
		{
			success = false;
		}
	}

	return success;
}



QString ItalcCore::applicationName()
{
	return instance()->m_applicationName;
}



void ItalcCore::enforceBranding( QWidget *topLevelWidget )
{
	for( auto label : topLevelWidget->findChildren<QLabel *>() )
	{
		label->setText( label->text().replace( "iTALC", ItalcCore::applicationName() ) );
	}

	for( auto button : topLevelWidget->findChildren<QAbstractButton *>() )
	{
		button->setText( button->text().replace( "iTALC", ItalcCore::applicationName() ) );
	}

	for( auto groupBox : topLevelWidget->findChildren<QGroupBox *>() )
	{
		groupBox->setTitle( groupBox->title().replace( "iTALC", ItalcCore::applicationName() ) );
	}

	for( auto action : topLevelWidget->findChildren<QAction *>() )
	{
		action->setText( action->text().replace( "iTALC", ItalcCore::applicationName() ) );
	}

	for( auto widget : topLevelWidget->findChildren<QWidget *>() )
	{
		widget->setWindowTitle( widget->windowTitle().replace( "iTALC", ItalcCore::applicationName() ) );
	}

	topLevelWidget->setWindowTitle( topLevelWidget->windowTitle().replace( "iTALC", ItalcCore::applicationName() ) );
}



QString ItalcCore::userRoleName( ItalcCore::UserRole role )
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
