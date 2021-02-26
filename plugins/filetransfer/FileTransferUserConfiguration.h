/*
 * FileTransferUserConfiguration.h - user config values for file transfer
 *
 * Copyright (c) 2018-2021 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include <QDir>

#include "Configuration/Proxy.h"
#include "Configuration/Property.h"

#define FOREACH_FILE_TRANSFER_USER_CONFIG_PROPERTIES(OP) \
	OP( FileTransferUserConfiguration, config, QString, lastFileTransferSourceDirectory, setLastFileTransferSourceDirectory, "LastSourceDirectory", "FileTransfer", QDir::homePath(), Configuration::Property::Flag::Standard )

class FileTransferUserConfiguration : public Configuration::Proxy
{
public:
	explicit FileTransferUserConfiguration( Configuration::Object* object ) :
		Configuration::Proxy( object )
	{
	}

	FOREACH_FILE_TRANSFER_USER_CONFIG_PROPERTIES(DECLARE_CONFIG_PROPERTY)

};
