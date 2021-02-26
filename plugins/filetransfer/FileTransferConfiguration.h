/*
 * FileTransferConfiguration.h - configuration values for FileTransfer plugin
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "VeyonConfiguration.h"
#include "Configuration/Proxy.h"

#define FOREACH_FILE_TRANSFER_CONFIG_PROPERTY(OP) \
	OP( FileTransferConfiguration, m_configuration, bool, rememberLastFileTransferSourceDirectory, setRememberLastFileTransferSourceDirectory, "RememberLastSourceDirectory", "FileTransfer", true, Configuration::Property::Flag::Advanced )	\
	OP( FileTransferConfiguration, m_configuration, bool, fileTransferCreateDestinationDirectory, setFileTransferCreateDestinationDirectory, "CreateDestinationDirectory", "FileTransfer", true, Configuration::Property::Flag::Advanced )	\
	OP( FileTransferConfiguration, m_configuration, QString, fileTransferDefaultSourceDirectory, setFileTransferDefaultSourceDirectory, "DefaultSourceDirectory", "FileTransfer", QStringLiteral("%HOME%"), Configuration::Property::Flag::Advanced )	\
	OP( FileTransferConfiguration, m_configuration, QString, fileTransferDestinationDirectory, setFileTransferDestinationDirectory, "DestinationDirectory", "FileTransfer", QStringLiteral("%HOME%"), Configuration::Property::Flag::Advanced )	\

// clazy:excludeall=missing-qobject-macro

DECLARE_CONFIG_PROXY(FileTransferConfiguration, FOREACH_FILE_TRANSFER_CONFIG_PROPERTY)
