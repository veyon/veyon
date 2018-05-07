/*
 * InternetAccessControlBackendInterface.h - interface class for internet access
 *                                           control backend plugins
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@veyon.io>
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

#ifndef INTERNET_ACCESS_CONTROL_BACKEND_INTERFACE_H
#define INTERNET_ACCESS_CONTROL_BACKEND_INTERFACE_H

#include "PluginInterface.h"

// clazy:excludeall=copyable-polymorphic

class VEYON_CORE_EXPORT InternetAccessControlBackendInterface
{
public:
	/*!
	 * \brief Create configuration widget for internet access control backend - used in Configurator
	 */
	virtual QWidget* configurationWidget() = 0;

	/*!
	 * \brief Function called inside Veyon Service to block the internet access
	 */
	virtual void blockInternetAccess() = 0;

	/*!
	 * \brief Function called inside Veyon Service to unblock the previously blocked internet access
	 */
	virtual void unblockInternetAccess() = 0;

};

typedef QList<InternetAccessControlBackendInterface *> InternetAccessControlBackendInterfaceList;

#define InternetAccessControlBackendInterface_iid "io.veyon.Veyon.Plugins.InternetAccessControl.BackendInterface"

Q_DECLARE_INTERFACE(InternetAccessControlBackendInterface, InternetAccessControlBackendInterface_iid)

#endif // INTERNET_ACCESS_CONTROL_BACKEND_INTERFACE_H
