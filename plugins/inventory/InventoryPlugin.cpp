#include "InventoryPlugin.h"
#include "InventoryMasterDialog.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"
#include "ComputerControlInterface.h"
#include "FeatureWorkerManager.h"
#include <QProcess>
#include <QDebug>
#include <QHeaderView>

InventoryPlugin::InventoryPlugin(QObject* parent) :
	QObject(parent),
	m_inventoryFeature(QStringLiteral("Inventory"),
					   Feature::Flag::Action | Feature::Flag::AllComponents | Feature::Flag::Checked,
					   Feature::Uid(INVENTORY_FEATURE_UID),
					   Feature::Uid(),
					   tr("System Inventory"), {},
					   tr("Click this button to fetch hardware/software inventory of selected computers."), QStringLiteral(":/core/list-add.png")),
	m_features({m_inventoryFeature}),
	m_dialog(nullptr)
{
}

InventoryPlugin::~InventoryPlugin()
{
	if (m_dialog)
	{
		delete m_dialog;
	}
}

bool InventoryPlugin::controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						 const ComputerControlInterfaceList& computerControlInterfaces)
{
	Q_UNUSED(arguments)

	if (featureUid == m_inventoryFeature.uid() && operation == Operation::Start)
	{
		if (!m_dialog)
		{
			m_dialog = new InventoryMasterDialog();
		}
		m_dialog->setComputers(computerControlInterfaces);
		m_dialog->show();
		m_dialog->raise();
		m_dialog->activateWindow();
		return true;
	}

	return false;
}

bool InventoryPlugin::handleFeatureMessage(ComputerControlInterface::Pointer computerControlInterface,
							  const FeatureMessage& message)
{
	if (message.featureUid() == m_inventoryFeature.uid() && message.command<InventoryCommand>() == InventoryCommand::InventoryData)
	{
		if (m_dialog)
		{
			QVariantMap data = message.argument(InventoryArgument::Data).toMap();
			m_dialog->addInventoryData(computerControlInterface->computerName(), data);
		}
		return true;
	}
	return false;
}

bool InventoryPlugin::handleFeatureMessage(VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message)
{
	Q_UNUSED(messageContext)

	if (message.featureUid() == m_inventoryFeature.uid())
	{
		server.featureWorkerManager().sendMessageToUnmanagedSessionWorker(message);
		return true;
	}
	return false;
}

bool InventoryPlugin::handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message)
{
	if (message.featureUid() == m_inventoryFeature.uid() && message.command<InventoryCommand>() == InventoryCommand::GetInventory)
	{
		qDebug() << "Inventory: Gathering data...";
		QVariantMap data;

#ifdef Q_OS_WIN
		QProcess wmic;
		wmic.start("wmic", {"os", "get", "Caption", "/value"});
		wmic.waitForFinished();
		QString os = QString::fromLocal8Bit(wmic.readAllStandardOutput()).trimmed();
		data.insert(QStringLiteral("OS"), os.remove("Caption=").trimmed());

		wmic.start("wmic", {"cpu", "get", "Name", "/value"});
		wmic.waitForFinished();
		QString cpu = QString::fromLocal8Bit(wmic.readAllStandardOutput()).trimmed();
		data.insert(QStringLiteral("CPU"), cpu.remove("Name=").trimmed());
		
		wmic.start("wmic", {"memorychip", "get", "Capacity", "/value"});
		wmic.waitForFinished();
		QString ram = QString::fromLocal8Bit(wmic.readAllStandardOutput()).trimmed();
		data.insert(QStringLiteral("RAM (Raw)"), ram.remove("Capacity=").trimmed());
#else
		QProcess proc;
		proc.start(QStringLiteral("sh"), {QStringLiteral("-c"), QStringLiteral("grep PRETTY_NAME /etc/os-release | cut -d'=' -f2 | tr -d '\"'")});
		proc.waitForFinished();
		data.insert(QStringLiteral("OS"), QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed());

		proc.start(QStringLiteral("sh"), {QStringLiteral("-c"), QStringLiteral("grep 'model name' /proc/cpuinfo | head -n 1 | cut -d':' -f2")});
		proc.waitForFinished();
		data.insert(QStringLiteral("CPU"), QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed());

		proc.start(QStringLiteral("sh"), {QStringLiteral("-c"), QStringLiteral("free -h | awk '/^Mem:/ {print $2}'")});
		proc.waitForFinished();
		data.insert(QStringLiteral("RAM"), QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed());
#endif

		FeatureMessage reply(Feature::Uid(INVENTORY_FEATURE_UID), InventoryCommand::InventoryData);
		reply.addArgument(InventoryArgument::Data, data);
		worker.sendFeatureMessageReply(reply);

		return true;
	}

	return false;
}
