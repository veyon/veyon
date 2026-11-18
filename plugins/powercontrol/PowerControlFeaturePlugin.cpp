// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QEvent>
#include "QuickUi.h"
#include <QNetworkInterface>
#include <QProgressBar>

#include <QUdpSocket>
#include <QtConcurrent>

#include "Computer.h"
#include "ComputerControlInterface.h"
#include "FeatureWorkerManager.h"
#include "NetworkObjectDirectoryManager.h"
#include "PlatformCoreFunctions.h"
#include "PlatformUserFunctions.h"
#include "PowerControlFeaturePlugin.h"
#include "PowerDownTimeInputDialog.h"
#include "VeyonConfiguration.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"


PowerControlFeaturePlugin::PowerControlFeaturePlugin(QObject* parent) :
	QObject(parent),
	m_commands({
		{QStringLiteral("on"), tr("Power on a computer via Wake-on-LAN (WOL)")},
	}),
	m_powerOnFeature(QStringLiteral("PowerOn"),
					 Feature::Flag::Action | Feature::Flag::AllComponents,
					 Feature::Uid("f483c659-b5e7-4dbc-bd91-2c9403e70ebd"),
					 Feature::Uid(),
					 tr("Power on"), {},
					 tr("Click this button to power on all computers. "
						"This way you do not have to power on each computer by hand."),
					 QStringLiteral(":/powercontrol/preferences-system-power-management.png")),
	m_rebootFeature(QStringLiteral("Reboot"),
					Feature::Flag::Action | Feature::Flag::AllComponents,
					Feature::Uid("4f7d98f0-395a-4fff-b968-e49b8d0f748c"),
					Feature::Uid(),
					tr("Reboot"), {},
					tr("Click this button to reboot all computers."),
					QStringLiteral(":/powercontrol/system-reboot.png")),
	m_powerDownFeature(QStringLiteral("PowerDown"),
					   Feature::Flag::Action | Feature::Flag::AllComponents,
					   Feature::Uid("6f5a27a0-0e2f-496e-afcc-7aae62eede10"),
					   Feature::Uid(),
					   tr("Power down"), {},
					   tr("Click this button to power down all computers. "
						  "This way you do not have to power down each computer by hand."),
					   QStringLiteral(":/powercontrol/system-shutdown.png")),
	m_powerDownNowFeature(QStringLiteral("PowerDownNow"),
						  Feature::Flag::Action | Feature::Flag::AllComponents,
						  Feature::Uid("a88039f2-6716-40d8-b4e1-9f5cd48e91ed"),
						  m_powerDownFeature.uid(),
						  tr("Power down now"), {}, {}),
	m_installUpdatesAndPowerDownFeature(QStringLiteral("InstallUpdatesAndPowerDown"),
										Feature::Flag::Action | Feature::Flag::AllComponents,
										Feature::Uid("09bcb3a1-fc11-4d03-8cf1-efd26be8655b"),
										m_powerDownFeature.uid(),
										tr("Install updates and power down"), {}, {}),
	m_powerDownConfirmedFeature(QStringLiteral("PowerDownConfirmed"),
								Feature::Flag::Action | Feature::Flag::AllComponents,
								Feature::Uid("ea2406be-d5c7-42b8-9f04-53469d3cc34c"),
								m_powerDownFeature.uid(),
								tr("Power down after user confirmation"), {}, {}),
	m_powerDownDelayedFeature(QStringLiteral("PowerDownDelayed"),
							  Feature::Flag::Action | Feature::Flag::AllComponents,
							  Feature::Uid("352de795-7fc4-4850-bc57-525bcb7033f5"),
							  m_powerDownFeature.uid(),
							  tr("Power down after timeout"), {}, {}),
	m_features({m_powerOnFeature, m_rebootFeature, m_powerDownFeature, m_powerDownNowFeature,
				m_installUpdatesAndPowerDownFeature, m_powerDownConfirmedFeature, m_powerDownDelayedFeature})
{
}



QStringList PowerControlFeaturePlugin::commands() const
{
	return m_commands.keys();
}



QString PowerControlFeaturePlugin::commandHelp(const QString& command) const
{
	return m_commands.value(command);
}



const FeatureList& PowerControlFeaturePlugin::featureList() const
{
	return m_features;
}



bool PowerControlFeaturePlugin::controlFeature(Feature::Uid featureUid,
											   Operation operation,
											   const QVariantMap& arguments,
											   const ComputerControlInterfaceList& computerControlInterfaces)
{
	if (hasFeature(featureUid) == false || operation != Operation::Start)
	{
		return false;
	}

	if (featureUid == m_powerOnFeature.uid())
	{
		QList<Computer> hosts;
		hosts.reserve(computerControlInterfaces.size());
		for (const auto& controlInterface : computerControlInterfaces)
		{
			hosts.append(controlInterface->computer());
		}

		(void)QtConcurrent::run([hosts]() {
			const auto directory = VeyonCore::networkObjectDirectoryManager().configuredDirectory();

			for (const auto& host : hosts)
			{
				auto macAddress = host.macAddress();
				if (macAddress.isEmpty())
				{
					macAddress = directory->queryObjectAttribute(host.networkObjectUid(),
																 NetworkObject::Attribute::MacAddress)
									 .toString();
				}

				if (macAddress.isEmpty() == false)
				{
					broadcastWOLPacket(macAddress);
				}
				else
				{
					vWarning() << "no MAC address available for host" << host.hostName() << "with ID" << host.networkObjectUid();
				}
			}
		});
	}
	else if (featureUid == m_powerDownDelayedFeature.uid())
	{
		const auto shutdownTimeout = arguments.value(argToString(Argument::ShutdownTimeout), 60).toInt();

		sendFeatureMessage(FeatureMessage{featureUid, FeatureMessage::Command::Default}
							   .addArgument(Argument::ShutdownTimeout, shutdownTimeout),
						   computerControlInterfaces);
	}
	else
	{
		sendFeatureMessage(FeatureMessage{featureUid, FeatureMessage::Command::Default}, computerControlInterfaces);
	}

	return true;
}



bool PowerControlFeaturePlugin::startFeature(VeyonMasterInterface& master, const Feature& feature,
											 const ComputerControlInterfaceList& computerControlInterfaces)
{
	if (feature == m_powerOnFeature)
	{
		return controlFeature(feature.uid(), Operation::Start, {}, computerControlInterfaces);
	}

	const auto executeOnAllComputers =
		computerControlInterfaces.size() >= master.filteredComputerControlInterfaces().size();

	if (confirmFeatureExecution(feature, executeOnAllComputers, master.mainWindow()) == false)
	{
		return false;
	}

	if (feature == m_powerDownDelayedFeature)
	{
		PowerDownTimeInputDialog dialog(master.mainWindow());

		if (dialog.exec())
		{
			return controlFeature(feature.uid(), Operation::Start,
								  {{argToString(Argument::ShutdownTimeout), dialog.seconds()}},
								  computerControlInterfaces);
		}

		return true;
	}


	return controlFeature(feature.uid(), Operation::Start, {}, computerControlInterfaces);
}



bool PowerControlFeaturePlugin::handleFeatureMessage(VeyonServerInterface& server,
													 const MessageContext& messageContext,
													 const FeatureMessage& message)
{
	Q_UNUSED(messageContext)

	auto& featureWorkerManager = server.featureWorkerManager();

	if (message.featureUid() == m_powerDownFeature.uid() ||
		message.featureUid() == m_powerDownNowFeature.uid() ||
		message.featureUid() == m_installUpdatesAndPowerDownFeature.uid())
	{
		const auto installUpdates = message.featureUid() == m_installUpdatesAndPowerDownFeature.uid();
		VeyonCore::platform().coreFunctions().powerDown(installUpdates);
	}
	else if (message.featureUid() == m_powerDownConfirmedFeature.uid())
	{
		if (VeyonCore::platform().userFunctions().isAnyUserLoggedOn() == false)
		{
			VeyonCore::platform().coreFunctions().powerDown(false);
		}
		else
		{
			featureWorkerManager.sendMessageToManagedSystemWorker(message);
		}
	}
	else if (message.featureUid() == m_powerDownDelayedFeature.uid())
	{
		featureWorkerManager.sendMessageToManagedSystemWorker(message);
	}
	else if (message.featureUid() == m_rebootFeature.uid())
	{
		VeyonCore::platform().coreFunctions().reboot();
	}
	else
	{
		return false;
	}

	return true;
}



bool PowerControlFeaturePlugin::handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message)
{
	Q_UNUSED(worker)

	if (message.featureUid() == m_powerDownConfirmedFeature.uid())
	{
		confirmShutdown();
		qApp->quit();
		return true;
	}
	else if (message.featureUid() == m_powerDownDelayedFeature.uid())
	{
		displayShutdownTimeout( message.argument( Argument::ShutdownTimeout ).toInt() );
		qApp->quit();
		return true;
	}

	return false;
}



CommandLinePluginInterface::RunResult PowerControlFeaturePlugin::handle_help(const QStringList& arguments)
{
	const auto command = arguments.value(0);

	const QMap<QString, QStringList> commands = {
		{QStringLiteral("on"),
		 QStringList({QStringLiteral("<%1>").arg(tr("MAC ADDRESS")),
					  tr("This command broadcasts a Wake-on-LAN (WOL) packet to the network in order to power on the computer with the given MAC address.")})},
	};

	if (commands.contains(command))
	{
		const auto& helpString = commands[command];
		print(QStringLiteral("\n%1 %2 %3\n\n%4\n\n").arg(commandLineModuleName(), command, helpString[0], helpString[1]));

		return NoResult;
	}

	return InvalidCommand;
}



CommandLinePluginInterface::RunResult PowerControlFeaturePlugin::handle_on(const QStringList& arguments)
{
	if (arguments.size() < 1)
	{
		return NotEnoughArguments;
	}

	return broadcastWOLPacket(arguments.first()) ? Successful : Failed;
}



bool PowerControlFeaturePlugin::eventFilter(QObject* watched, QEvent* event)
{

	return QObject::eventFilter(watched, event);
}



bool PowerControlFeaturePlugin::confirmFeatureExecution(const Feature& feature, bool all, QWidget* parent)
{
	if (VeyonCore::config().confirmUnsafeActions() == false)
	{
		return true;
	}

	Q_UNUSED(parent)
	QString title;
	QString text;
	if (feature == m_rebootFeature)
	{
		title = tr("Confirm reboot");
		text = all ? tr("Do you really want to reboot ALL computers?") : tr("Do you really want to reboot the selected computers?");
	}
	else if (feature == m_powerDownFeature || feature == m_powerDownNowFeature ||
		feature == m_installUpdatesAndPowerDownFeature || feature == m_powerDownConfirmedFeature || feature == m_powerDownDelayedFeature)
	{
		title = tr("Confirm power down");
		text = all ? tr("Do you really want to power down ALL computers?") : tr("Do you really want to power down the selected computers?");
	}
	if (!title.isEmpty()) return QuickUi::execDialog(QStringLiteral("confirm"),
		{{QStringLiteral("title"), title}, {QStringLiteral("text"), text}}).value(QStringLiteral("accepted")).toBool();

	return true;
}



bool PowerControlFeaturePlugin::broadcastWOLPacket(QString macAddress)
{
	if (macAddress.isEmpty())
	{
		return false;
	}

	const auto originalMacAddress = macAddress;

	// remove all possible delimiters
	macAddress.replace(QLatin1Char(':'), QString());
	macAddress.replace(QLatin1Char('-'), QString());
	macAddress.replace(QLatin1Char('.'), QString());

	const auto macAddressBytes = QByteArray::fromHex(macAddress.toUtf8());
	static constexpr auto MacAddressSize = 6;

	if (macAddressBytes.size() != MacAddressSize)
	{
		CommandLineIO::error(tr("Invalid MAC address specified!"));
		vWarning() << "invalid MAC address" << originalMacAddress;
		return false;
	}

	static constexpr auto MagicPacketFieldCount = 17;

	QByteArray datagram(MacAddressSize * MagicPacketFieldCount, char(0xff));

	for (int i = 1; i < MagicPacketFieldCount; ++i)
	{
		datagram.replace(i * MacAddressSize, MacAddressSize, macAddressBytes);
	}

	QUdpSocket udpSocket;

	vDebug() << "broadcasting WOL packet for" << originalMacAddress;
	bool success = (udpSocket.writeDatagram(datagram, QHostAddress::Broadcast, 9) == datagram.size());

	const auto networkInterfaces = QNetworkInterface::allInterfaces();
	for (const auto& networkInterface : networkInterfaces)
	{
		const auto addressEntries = networkInterface.addressEntries();
		for (const auto& addressEntry : addressEntries)
		{
			if (addressEntry.broadcast().isNull() == false)
			{
				vDebug() << "broadcasting WOL packet for" << originalMacAddress << "via" << addressEntry.broadcast().toString();
				success &= (udpSocket.writeDatagram(datagram, addressEntry.broadcast(), 9) == datagram.size());
			}
		}
	}

	return success;
}



void PowerControlFeaturePlugin::confirmShutdown()
{
	if (QuickUi::execDialog(QStringLiteral("confirm"), {
		{QStringLiteral("title"), tr("Confirm power down")}, {QStringLiteral("stayOnTop"), true},
		{QStringLiteral("text"), tr("The computer was remotely requested to power down. Do you want to power down the computer now?")}})
		.value(QStringLiteral("accepted")).toBool()) VeyonCore::platform().coreFunctions().powerDown(false);
}

void PowerControlFeaturePlugin::displayShutdownTimeout(int shutdownTimeout)
{
	if (QuickUi::execDialog(QStringLiteral("countdown"), {
		{QStringLiteral("title"), tr("Confirm power down")}, {QStringLiteral("seconds"), shutdownTimeout}})
		.value(QStringLiteral("accepted")).toBool()) VeyonCore::platform().coreFunctions().powerDown(false);
}
