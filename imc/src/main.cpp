/*
 * main.cpp - main file for iTALC Management Console
 *
 * Copyright (c) 2010-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QtCore/QProcessEnvironment>
#include <QApplication>

#include "Configuration/XmlStore.h"
#include "ImcCore.h"
#include "ItalcConfiguration.h"
#include "ItalcCore.h"
#include "MainWindow.h"
#include "Ldap/LdapDirectory.h"
#include "LocalSystem.h"
#include "Logger.h"


bool checkPrivileges( int argc, char **argv )
{
	// make sure to run as admin
	if( LocalSystem::Process::isRunningAsAdmin() == false )
	{
		QCoreApplication app( argc, argv );
		QStringList args = app.arguments();
		args.removeFirst();
		LocalSystem::Process::runAsAdmin(
				QCoreApplication::applicationFilePath(),
				args.join( " " ) );
		return false;
	}

	return true;
}



bool checkWritableConfiguration()
{
	if( !ItalcConfiguration().isStoreWritable() &&
			ItalcCore::config->logLevel() < Logger::LogLevelDebug )
	{
		ImcCore::criticalMessage( MainWindow::tr( "Configuration not writable" ),
			MainWindow::tr( "The local configuration backend reported that the "
							"configuration is not writable! Please run the iTALC "
							"Management Console with higher privileges." ) );
		return false;
	}

	return true;
}


int applySettings( QStringListIterator& argIt )
{
	if( argIt.hasNext() == false )
	{
		qCritical( "Please specify settings file!" );
		return -1;
	}

	const QString file = argIt.next();
	Configuration::XmlStore xs( Configuration::XmlStore::System, file );

	if( ImcCore::applyConfiguration( ItalcConfiguration( &xs ) ) )
	{
		ImcCore::informationMessage(
			MainWindow::tr( "iTALC Management Console" ),
			MainWindow::tr( "All settings were applied successfully." ) );
	}
	else
	{
		ImcCore::criticalMessage(
			MainWindow::tr( "iTALC Management Console" ),
			MainWindow::tr( "An error occured while applying settings!" ) );
	}

	return 0;
}

int setConfigurationValue( QStringListIterator& argIt )
{
	if( !argIt.hasNext() )
	{
		qCritical( "No configuration property specified!" );
		return -1;
	}

	QString prop = argIt.next();
	QString value;
	if( !argIt.hasNext() )
	{
		if( !prop.contains( '=' ) )
		{
			qCritical() << "No value for property" << prop << "specified!";
			return -1;
		}
		else
		{
			value = prop.section( '=', -1, -1 );
			prop = prop.section( '=', 0, -2 );
		}
	}
	else
	{
		value = argIt.next();
	}

	const QString key = prop.section( '/', -1, -1 );
	const QString parentKey = prop.section( '/', 0, -2 );

	ItalcCore::config->setValue( key, value, parentKey );

	ImcCore::applyConfiguration( *ItalcCore::config );

	return 0;
}



int createKeyPair( QStringListIterator& argIt )
{
	const QString destDir = argIt.hasNext() ? argIt.next() : QString();
	ImcCore::createKeyPair( ItalcCore::role, destDir );
	return 0;
}



int importPublicKey( QStringListIterator& argIt )
{
	QString pubKeyFile;
	if( !argIt.hasNext() )
	{
		QStringList l =
			QDir::current().entryList( QStringList() << "*.key.txt",
										QDir::Files | QDir::Readable );
		if( l.size() != 1 )
		{
			qCritical( "Please specify location of the public key "
						"to import" );
			return -1;
		}
		pubKeyFile = QDir::currentPath() + QDir::separator() +
											l.first();
		qWarning() << "No public key file specified. Trying to import "
						"the public key file found at" << pubKeyFile;
	}
	else
	{
		pubKeyFile = argIt.next();
	}

	if( !ImcCore::importPublicKey( ItalcCore::role, pubKeyFile, QString() ) )
	{
		LogStream( Logger::LogLevelInfo ) << "Public key import "
											"failed";
		return -1;
	}
	LogStream( Logger::LogLevelInfo ) << "Public key successfully "
											"imported";
	return 0;
}



bool parseRole( QStringListIterator& argIt )
{
	if( argIt.hasNext() )
	{
		const QString role = argIt.next();
		if( role == "teacher" )
		{
			ItalcCore::role = ItalcCore::RoleTeacher;
		}
		else if( role == "admin" )
		{
			ItalcCore::role = ItalcCore::RoleAdmin;
		}
		else if( role == "supporter" )
		{
			ItalcCore::role = ItalcCore::RoleSupporter;
		}
	}
	else
	{
		qCritical( "-role needs an argument:\n"
			"	teacher\n"
			"	admin\n"
			"	supporter\n\n" );
		return false;
	}

	return true;
}



int autoConfigureLdapBaseDn( QStringListIterator& argIt )
{
	QUrl ldapUrl;

	if( argIt.hasNext() )
	{
		ldapUrl.setUrl( argIt.next(), QUrl::StrictMode );
		if( ldapUrl.isValid() == false || ldapUrl.host().isEmpty() )
		{
			qCritical() << "Please specify a valid LDAP url following the schema \"ldap[s]://[user[:password]@]hostname[:port]\"";
			return -1;
		}

		if( argIt.hasNext() )
		{
			ItalcCore::config->setLdapNamingContextAttribute( argIt.next() );
		}
		else
		{
			qWarning() << "No naming context attribute name given - falling back to configured value.";
		}
	}

	LdapDirectory ldapDirectory( ldapUrl );
	QString baseDn = ldapDirectory.queryNamingContext();

	if( baseDn.isEmpty() )
	{
		qCritical() << "Could not query base DN. Please check your LDAP configuration.";
		return -1;
	}

	qInfo() << "Configuring" << baseDn << "as base DN and disabling naming context queries.";

	ItalcCore::config->setLdapBaseDn( baseDn );
	ItalcCore::config->setLdapQueryNamingContext( false );
	ImcCore::applyConfiguration( *ItalcCore::config );

	return 0;
}



int main( int argc, char **argv )
{
	if( checkPrivileges( argc, argv ) == false )
	{
		return 0;
	}

	QCoreApplication* app = Q_NULLPTR;

#ifdef ITALC_BUILD_LINUX
	if( QProcessEnvironment::systemEnvironment().contains( "DISPLAY" ) == false )
	{
		app = new QCoreApplication( argc, argv );
	}
	else
	{
		app = new QApplication( argc, argv );
	}
#else
	app = new QApplication( argc, argv );
#endif

	ItalcCore::init();

	ImcCore::silent = app->arguments().contains( "-quiet" ) ||
						app->arguments().contains( "-silent" ) ||
						app->arguments().contains( "-q" );


	// default to teacher role for various command line operations
	ItalcCore::role = ItalcCore::RoleTeacher;

	Logger l( "ItalcManagementConsole" );

	if( checkWritableConfiguration() == false )
	{
		return -1;
	}

	// parse arguments
	QStringListIterator argIt( app->arguments() );
	argIt.next();

	while( argc > 1 && argIt.hasNext() )
	{
		const QString a = argIt.next().toLower();
		if( a == "-applysettings" || a == "-a"  )
		{
			return applySettings( argIt );
		}
		else if( a == "-listconfig" || a == "-l" )
		{
			ImcCore::listConfiguration( *ItalcCore::config );

			return 0;
		}
		else if( a == "-setconfigvalue" || a == "-s" )
		{
			return setConfigurationValue( argIt );
		}
		else if( a == "-role" )
		{
			if( parseRole( argIt ) == false )
			{
				return -1;
			}
		}
		else if( a == "-createkeypair" )
		{
			return createKeyPair( argIt );
		}
		else if( a == "-importpublickey" || a == "-i" )
		{
			return importPublicKey( argIt );
		}
		else if( a == "-autoconfigureldapbasedn" )
		{
			return autoConfigureLdapBaseDn( argIt );
		}
	}

	// now create the main window
	MainWindow *mainWindow = new MainWindow;

	mainWindow->show();

	ilog( Info, "App.Exec" );

	int ret = app->exec();

	ItalcCore::destroy();

	delete app;

	return ret;
}

