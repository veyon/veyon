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

#include <QtCrypto>

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



ItalcConfiguration *ItalcCore::config = NULL;
AuthenticationCredentials *ItalcCore::authenticationCredentials = NULL;

ItalcCore::UserRoles ItalcCore::role = ItalcCore::RoleOther;

static QString appName = "iTALC";

static QCA::Initializer* qcaInit = nullptr;


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



bool ItalcCore::init()
{
	if( config )
	{
		return false;
	}

	killWisPtis();

	lzo_init();

	setupApplicationParameters();

	initResources();

	config = new ItalcConfiguration( ItalcConfiguration::defaultConfiguration() );
	*config += ItalcConfiguration( Configuration::Store::LocalBackend );

	QLocale configuredLocale( QLocale::C );

	QRegExp localeRegEx( "[^(]*\\(([^)]*)\\)");
	if( localeRegEx.indexIn( config->uiLanguage() ) == 0 )
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

	if( config->applicationName().isEmpty() == false )
	{
		appName = config->applicationName();
	}

	// init QCA
	qcaInit = new QCA::Initializer;

	initAuthentication( AuthenticationCredentials::None );

	return true;
}




bool ItalcCore::initAuthentication( int credentialTypes )
{
	if( authenticationCredentials )
	{
		delete authenticationCredentials;
		authenticationCredentials = nullptr;
	}

	authenticationCredentials = new AuthenticationCredentials;

	bool success = true;

	if( credentialTypes & AuthenticationCredentials::UserLogon &&
			config->isLogonAuthenticationEnabled() )
	{
		if( qobject_cast<QApplication *>( QCoreApplication::instance() ) )
		{
			PasswordDialog dlg( QApplication::activeWindow() );
			if( dlg.exec() &&
				dlg.credentials().hasCredentials( AuthenticationCredentials::UserLogon ) )
			{
				authenticationCredentials->setLogonUsername( dlg.username() );
				authenticationCredentials->setLogonPassword( dlg.password() );

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
			config->isKeyAuthenticationEnabled() )
	{
		const QString privKeyFile = LocalSystem::Path::privateKeyPath( ItalcCore::role );
		qDebug() << "Loading private key" << privKeyFile << "for role" << ItalcCore::role;
		if( authenticationCredentials->loadPrivateKey( privKeyFile ) )
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




void ItalcCore::destroy()
{
	delete authenticationCredentials;
	authenticationCredentials = NULL;

	delete qcaInit;
	qcaInit = nullptr;

	delete config;
	config = NULL;
}



QString ItalcCore::applicationName()
{
	return appName;
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



static const char *userRoleNames[] =
{
	"none",
	"teacher",
	"admin",
	"supporter",
	"other"
} ;



QString ItalcCore::userRoleName( UserRole role )
{
	return userRoleNames[role];
}




namespace ItalcCore
{


bool Msg::send()
{
	char messageType = rfbItalcCoreRequest;
	m_ioDevice->write( &messageType, sizeof(messageType) );

	VariantStream stream( m_ioDevice );

	stream.write( m_cmd );
	stream.write( m_args );

	return true;
}



Msg &Msg::receive()
{
	VariantStream stream( m_ioDevice );

	m_cmd = stream.read().toString();
	m_args = stream.read().toMap();

	return *this;
}



const Command LogonUserCmd = "LogonUser";
const Command LogoutUser = "LogoutUser";
const Command ExecCmds = "ExecCmds";
const Command DisableLocalInputs = "DisableLocalInputs";
const Command SetRole = "SetRole";

const Command ReportSlaveStateFlags = "ReportSlaveStateFlags";

} ;
