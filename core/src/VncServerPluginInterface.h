// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

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
	virtual bool runServer(int serverPort, const Password& password) = 0;

	virtual int configuredServerPort() = 0;

	virtual Password configuredPassword() = 0;
};

using VncServerPluginInterfaceList = QList<VncServerPluginInterface*>;

#define VncServerPluginInterface_iid "io.veyon.Veyon.Plugins.VncServerPluginInterface"

Q_DECLARE_INTERFACE(VncServerPluginInterface, VncServerPluginInterface_iid)
