/*
 * VncServerPluginInterface.h - abstract interface class for VNC server plugins
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef VNC_SERVER_PLUGIN_INTERFACE_H
#define VNC_SERVER_PLUGIN_INTERFACE_H

#include "PluginInterface.h"

// clazy:excludeall=copyable-polymorphic

class VncServerPluginInterface
{
public:
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
	virtual void runServer( int serverPort, const QString& password ) = 0;

	virtual int configuredServerPort() = 0;

	virtual QString configuredPassword() = 0;

} ;

typedef QList<VncServerPluginInterface *> VncServerPluginInterfaceList;

#define VncServerPluginInterface_iid "io.veyon.Veyon.Plugins.VncServerPluginInterface"

Q_DECLARE_INTERFACE(VncServerPluginInterface, VncServerPluginInterface_iid)

#endif
