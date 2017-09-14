/*
 * UserConfig.cpp - a Configuration object storing personal settings
 *                      for the Veyon Master Application
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#include <QMessageBox>

#include "VeyonCore.h"
#include "UserConfig.h"

FOREACH_PERSONAL_CONFIG_PROPERTY(IMPLEMENT_CONFIG_SET_PROPERTY)


UserConfig::UserConfig(Configuration::Store::Backend backend) :
	Configuration::Object( backend, Configuration::Store::User )
{
	if( isStoreWritable() == false )
	{
		QMessageBox::information( nullptr,
								  tr( "No write access" ),
								  tr( "Could not save your personal settings! "
									  "Please check the user configuration "
									  "file path using the %1 Configurator." ).arg( VeyonCore::applicationName() ) );
	}
}
