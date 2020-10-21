/*
 * BuiltinWayVncServer.cpp - implementation of BuiltinWayVncServer class
 *
 * Copyright (c) 2020 Tobias Junghans <tobydox@veyon.io>
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

#include <QCoreApplication>

#include "BuiltinWayVncServer.h"
#include "VeyonConfiguration.h"
#include "WayVncConfigurationWidget.h"

extern "C" int wayvnc_main( int argc, char** argv );

extern "C"
{
#include "cfg.h"

	static cfg wayVncConfig{};

	int cfg_load( struct cfg* self, const char* path )
	{
		Q_UNUSED(path)

		*self = wayVncConfig;

		return 0;
	}
	void cfg_destroy(struct cfg* self)
	{
#define DESTROY_bool(...)
#define DESTROY_uint(...)
#define DESTROY_string(p) delete[](p)

#define X(type, name) DESTROY_ ## type(self->name);
		X_CFG_LIST
#undef X

#undef DESTROY_string
#undef DESTROY_uint
#undef DESTROY_bool
	}
}


BuiltinWayVncServer::BuiltinWayVncServer( QObject* parent ) :
	QObject( parent ),
	m_configuration( &VeyonCore::config() )
{
}



QWidget* BuiltinWayVncServer::configurationWidget()
{
	return new WayVncConfigurationWidget( m_configuration );
}



void BuiltinWayVncServer::prepareServer()
{
}



bool BuiltinWayVncServer::runServer( int serverPort, const Password& password )
{
	QStringList cmdline{} ;

	const auto extraArguments = m_configuration.extraArguments();

	if( extraArguments.isEmpty() == false )
	{
		cmdline.append( extraArguments.split( QLatin1Char(' ') ) );
	}

	wayVncConfig.address = qstrdup("127.0.0.1");
	wayVncConfig.port = serverPort;

	// TODO: simple auth support
	Q_UNUSED(password)
	// wayVncConfig.enable_auth = true;
	// wayVncConfig.password = qstrdup( password.toByteArray().constData() );

	// build new C-style command line array based on cmdline-QStringList
	const auto appArguments = QCoreApplication::arguments();
	auto argv = new char *[cmdline.size()+1]; // Flawfinder: ignore
	argv[0] = qstrdup( appArguments.first().toUtf8().constData() );
	int argc = 1;

	for( auto it = cmdline.begin(), end = cmdline.end(); it != end; ++it, ++argc )
	{
		const auto len = static_cast<size_t>( it->length() );
		argv[argc] = new char[len + 1];
		strncpy( argv[argc], it->toUtf8().constData(), len ); // Flawfinder: ignore
		argv[argc][len] = 0;
	}

	// run wayvnc-server
	const auto retval = wayvnc_main( argc, argv );

	for( int i = 0; i < argc; ++i )
	{
		delete[] argv[i];
	}

	delete[] argv;

	return retval == 1;
}


IMPLEMENT_CONFIG_PROXY(WayVncConfiguration)
