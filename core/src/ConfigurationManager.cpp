// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Configuration/LocalStore.h"
#include "ConfigurationManager.h"
#include "Filesystem.h"
#include "PlatformCoreFunctions.h"
#include "PlatformNetworkFunctions.h"
#include "VeyonConfiguration.h"
#include "VeyonServiceControl.h"



ConfigurationManager::ConfigurationManager( QObject* parent ) :
	ConfigurationManager(VeyonCore::config(), parent)
{
}

ConfigurationManager::ConfigurationManager(VeyonConfiguration& configuration, QObject* parent) :
	QObject(parent), m_configuration(configuration)
{
}



bool ConfigurationManager::clearConfiguration()
{
	return Configuration::LocalStore(Configuration::LocalStore::Scope::System).clearChecked(&m_errorString);
}



bool ConfigurationManager::applyConfiguration()
{
	m_errorString.clear();
	// update Veyon Service configuration
	if (VeyonServiceControl().setAutostart(m_configuration.autostartService()) == false)
	{
		m_errorString = tr("Could not modify the autostart property for the Veyon Service.");
		return false;
	}

	auto& network = VeyonCore::platform().networkFunctions();

	if (network.configureFirewallException(VeyonCore::filesystem().serverFilePath(),
										   QStringLiteral("Veyon Server"),
										   m_configuration.isFirewallExceptionEnabled()) == false)
	{
		m_errorString = tr("Could not configure the firewall configuration for the Veyon Server.");
		return false;
	}

	if (network.configureFirewallException(VeyonCore::filesystem().workerFilePath(),
										   QStringLiteral("Veyon Worker"),
										   m_configuration.isFirewallExceptionEnabled()) == false)
	{
		m_errorString = tr("Could not configure the firewall configuration for the Veyon Worker.");
		return false;
	}

	if (VeyonCore::platform().coreFunctions().applyConfiguration() == false)
	{
		m_errorString = tr("Could not apply platform-specific configuration settings.");
		return false;
	}

	return true;
}



bool ConfigurationManager::saveConfiguration()
{
	// write global configuration
	Configuration::LocalStore localStore(Configuration::LocalStore::Scope::System);
	if (localStore.isWritable() == false)
	{
		m_errorString = tr("Configuration is not writable. Please check your permissions!");
		return false;
	}

	return localStore.flushChecked(&m_configuration, &m_errorString);
}
