/*
 * VncServerPluginInterface.h - abstract interface class for VNC server plugins
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

#include "CryptoCore.h"
#include "PluginInterface.h"

// clazy:excludeall=copyable-polymorphic

class VncServerPluginInterface
{
public:
	using Password = CryptoCore::SecureArray;

	virtual ~VncServerPluginInterface() = default;

	virtual QStringList supportedSessionTypes() const = 0;

	/*!
	 * \brief Create configuration widget for VNC server plugin - used in Configurator
	 */
	virtual QWidget* configurationWidget() = 0;

	virtual void prepareServer() = 0;

	/*!
	 * \brief Run the VNC server and make it listen at given port and use given password - function has to block
	 * \param serverPort the port the VNC server should listen at
	 * \param password the password to be used for VNC authentication
	 */
	virtual bool runServer( int serverPort, const Password& password ) = 0;

	virtual int configuredServerPort() = 0;

	virtual Password configuredPassword() = 0;

} ;

using VncServerPluginInterfaceList = QList<VncServerPluginInterface *>;

#define VncServerPluginInterface_iid "io.veyon.Veyon.Plugins.VncServerPluginInterface"

Q_DECLARE_INTERFACE(VncServerPluginInterface, VncServerPluginInterface_iid)
