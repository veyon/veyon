// SPDX-FileCopyrightText: 2026 Veyon Community
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QHostAddress>
#include <QTcpSocket>

#include "ComputerControlInterface.h"
#include "InternetBlockAddressRanges.h"
#include "InternetBlockConfigurationPage.h"
#include "InternetBlockFeaturePlugin.h"
#include "VeyonServerInterface.h"


InternetBlockFeaturePlugin::InternetBlockFeaturePlugin(QObject* parent) :
	QObject(parent),
	m_configuration(&VeyonCore::config()),
	m_internetBlockFeature(QStringLiteral("InternetBlock"),
						   Feature::Flag::Mode | Feature::Flag::Master | Feature::Flag::Service,
						   Feature::Uid("5f8c5b1e-8e0c-4c58-9b62-2b6a7b7d3c11"),
						   Feature::Uid(),
						   tr("Block internet"), tr("Unblock internet"),
						   tr("Use this button to block internet access on the selected computers. "
							  "Computers in the local network, including this one, stay reachable."),
						   QStringLiteral(":/internetblock/internet-block.png")),
	m_features({m_internetBlockFeature})
{
	if (VeyonCore::component() == VeyonCore::Component::Service)
	{
		connect(VeyonCore::instance(), &VeyonCore::initialized, this, [this]() {
			if (m_configuration.unblockOnServiceStart() && InternetBlocker::isBlocked())
			{
				vInfo() << "restoring internet access on service start";
				InternetBlocker::unblock();
			}
		});
	}
}



bool InternetBlockFeaturePlugin::controlFeature(Feature::Uid featureUid, Operation operation,
												const QVariantMap& arguments,
												const ComputerControlInterfaceList& computerControlInterfaces)
{
	Q_UNUSED(arguments)

	if (hasFeature(featureUid) == false)
	{
		return false;
	}

	if (operation == Operation::Start)
	{
		// never cut off the master computer itself (it may be listed as a regular
		// computer in multi-teacher setups)
		auto targets = computerControlInterfaces;
		targets.removeLocalHostInterfaces();

		sendFeatureMessage(FeatureMessage{featureUid, FeatureCommand::Block}, targets);
		return true;
	}

	if (operation == Operation::Stop)
	{
		sendFeatureMessage(FeatureMessage{featureUid, FeatureCommand::Unblock}, computerControlInterfaces);
		return true;
	}

	return false;
}



bool InternetBlockFeaturePlugin::handleFeatureMessage(VeyonServerInterface& server,
													  const MessageContext& messageContext,
													  const FeatureMessage& message)
{
	Q_UNUSED(server)

	if (message.featureUid() != m_internetBlockFeature.uid())
	{
		return false;
	}

	switch (message.command<FeatureCommand>())
	{
	case FeatureCommand::Block:
	{
		QString masterAddress;
		if (m_configuration.allowMasterComputer())
		{
			if (const auto socket = qobject_cast<QTcpSocket*>(messageContext.ioDevice()); socket)
			{
				masterAddress = socket->peerAddress().toString();
			}
		}

		const auto settings = settingsFromConfiguration(m_configuration, masterAddress);
		if (InternetBlocker::block(settings))
		{
			vInfo() << "internet access blocked";
			m_blockedState = 1;
		}
		else
		{
			vCritical() << "failed to block internet access";
			m_blockedState = -1;
		}
		return true;
	}

	case FeatureCommand::Unblock:
		if (InternetBlocker::unblock())
		{
			vInfo() << "internet access restored";
			m_blockedState = 0;
		}
		else
		{
			vCritical() << "failed to restore internet access";
			m_blockedState = -1;
		}
		return true;

	default:
		break;
	}

	return false;
}



bool InternetBlockFeaturePlugin::isFeatureActive(VeyonServerInterface& server, Feature::Uid featureUid) const
{
	Q_UNUSED(server)

	if (featureUid != m_internetBlockFeature.uid())
	{
		return false;
	}

	// query the firewall once and cache the result afterwards since this
	// function is called frequently
	if (m_blockedState < 0)
	{
		m_blockedState = InternetBlocker::isBlocked() ? 1 : 0;
	}

	return m_blockedState > 0;
}



ConfigurationPage* InternetBlockFeaturePlugin::createConfigurationPage()
{
	return new InternetBlockConfigurationPage(m_configuration);
}



InternetBlocker::Settings InternetBlockFeaturePlugin::settingsFromConfiguration(const InternetBlockConfiguration& configuration,
																				const QString& additionalAllowedAddress)
{
	InternetBlocker::Settings settings;

	settings.blockIPv6 = configuration.blockIPv6();

	if (configuration.allowPrivateNetworks())
	{
		settings.allowedIPv4Networks = InternetBlockAddressRanges::defaultAllowedIPv4Networks();
		settings.allowedIPv6Networks = InternetBlockAddressRanges::defaultAllowedIPv6Networks();
	}
	else
	{
		// loopback is always required for local IPC (Veyon itself!)
		settings.allowedIPv4Networks = {QStringLiteral("127.0.0.0/8")};
		settings.allowedIPv6Networks = {QStringLiteral("::1/128")};
	}

	QStringList invalid;
	QStringList ipv6;
	const auto userNetworks = InternetBlockAddressRanges::splitNetworkList(configuration.allowedNetworks());
	InternetBlockAddressRanges::parseIPv4Networks(userNetworks, &invalid, &ipv6);
	if (invalid.isEmpty() == false)
	{
		vWarning() << "ignoring invalid allowed networks" << invalid;
	}

	for (const auto& network : userNetworks)
	{
		if (invalid.contains(network) == false && ipv6.contains(network) == false)
		{
			settings.allowedIPv4Networks.append(network);
		}
	}
	settings.allowedIPv6Networks.append(ipv6);

	if (additionalAllowedAddress.isEmpty() == false)
	{
		QHostAddress address(additionalAllowedAddress);
		bool isIPv4 = false;
		const auto ipv4 = address.toIPv4Address(&isIPv4); // also handles IPv4-mapped IPv6 addresses
		if (isIPv4)
		{
			settings.allowedIPv4Networks.append(QHostAddress(ipv4).toString());
		}
		else if (address.protocol() == QAbstractSocket::IPv6Protocol)
		{
			settings.allowedIPv6Networks.append(address.toString());
		}
	}

	return settings;
}


IMPLEMENT_CONFIG_PROXY(InternetBlockConfiguration)
