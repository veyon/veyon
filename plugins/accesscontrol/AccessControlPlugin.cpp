#include "AccessControlPlugin.h"

#include <QDebug>
#include <QCoreApplication>

#include "FeatureWorkerManager.h"
#include "VeyonWorkerInterface.h"
#include "VeyonServerInterface.h"
#include "ComputerControlInterface.h"
#include "AccessControlConfigurationPage.h"
#include <QProcess>
#include <QTimer>

IMPLEMENT_CONFIG_PROXY(AccessControlConfiguration)

AccessControlPlugin::AccessControlPlugin( QObject* parent ) :
	QObject( parent ),
	m_blockInternetFeature(QStringLiteral("BlockInternet"),
						   Feature::Flag::Action | Feature::Flag::AllComponents | Feature::Flag::Checked,
						   Feature::Uid("1c841103-625d-4f1e-9276-8fbd8ccba9d3"),
						   Feature::Uid(),
						   tr("Block Internet"), {},
						   tr("Click this button to block internet access on selected computers."),
						   QStringLiteral(":/core/block-internet.png")),
	m_blockAppsFeature(QStringLiteral("BlockApps"),
					   Feature::Flag::Action | Feature::Flag::AllComponents | Feature::Flag::Checked,
					   Feature::Uid("4f3f22da-714c-41c3-b456-c4d32a4a9c8b"),
					   Feature::Uid(),
					   tr("Block Apps"), {},
					   tr("Click this button to block restricted applications on selected computers."),
					   QStringLiteral(":/core/block-apps.png")),
	m_blockUSBFeature(QStringLiteral("BlockUSB"),
					   Feature::Flag::Action | Feature::Flag::AllComponents | Feature::Flag::Checked,
					   Feature::Uid("43e9a111-2b8f-4d91-b3fa-1a2b3c4d5e6f"),
					   Feature::Uid(),
					   tr("Block USB"), {},
					   tr("Click this button to block USB storage devices on selected computers."),
					   QStringLiteral(":/core/block-usb.png")),
	m_features({m_blockInternetFeature, m_blockAppsFeature, m_blockUSBFeature}),
	m_configuration(&VeyonCore::config())
{
}

AccessControlPlugin::~AccessControlPlugin()
{
}

ConfigurationPage* AccessControlPlugin::createConfigurationPage()
{
	return new AccessControlConfigurationPage( m_configuration );
}

bool AccessControlPlugin::controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
										 const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(arguments)

	if (featureUid == m_blockInternetFeature.uid())
	{
		if (operation == Operation::Start || operation == Operation::Stop)
		{
			bool enable = (operation == Operation::Start);
			FeatureMessage msg(featureUid, enable ? FeatureCommand::StartInternetBlock : FeatureCommand::StopInternetBlock);
			sendFeatureMessage(msg, computerControlInterfaces);
			return true;
		}
	}
	else if (featureUid == m_blockAppsFeature.uid())
	{
		if (operation == Operation::Start || operation == Operation::Stop)
		{
			bool enable = (operation == Operation::Start);
			FeatureMessage msg(featureUid, enable ? FeatureCommand::StartAppBlock : FeatureCommand::StopAppBlock);
			sendFeatureMessage(msg, computerControlInterfaces);
			return true;
		}
	}
	else if (featureUid == m_blockUSBFeature.uid())
	{
		if (operation == Operation::Start || operation == Operation::Stop)
		{
			bool enable = (operation == Operation::Start);
			FeatureMessage msg(featureUid, enable ? FeatureCommand::StartUSBBlock : FeatureCommand::StopUSBBlock);
			sendFeatureMessage(msg, computerControlInterfaces);
			return true;
		}
	}

	return false;
}

bool AccessControlPlugin::handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message )
{
	Q_UNUSED(messageContext)

	server.featureWorkerManager().sendMessageToUnmanagedSessionWorker( message );
	return true;
}

bool AccessControlPlugin::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	Q_UNUSED(worker)

	if (message.featureUid() == m_blockInternetFeature.uid())
	{
		if (message.command<FeatureCommand>() == FeatureCommand::StartInternetBlock)
		{
			qDebug() << "AccessControl: Blocking internet";
#ifdef Q_OS_WIN
			QProcess::execute(QStringLiteral("netsh"), {QStringLiteral("advfirewall"), QStringLiteral("firewall"), QStringLiteral("add"), QStringLiteral("rule"), QStringLiteral("name=VeyonBlockInternet"), QStringLiteral("dir=out"), QStringLiteral("action=block"), QStringLiteral("protocol=TCP"), QStringLiteral("remoteport=80,443")});
#else
			QProcess::execute(QStringLiteral("iptables"), {QStringLiteral("-I"), QStringLiteral("OUTPUT"), QStringLiteral("-p"), QStringLiteral("tcp"), QStringLiteral("-m"), QStringLiteral("multiport"), QStringLiteral("--dports"), QStringLiteral("80,443"), QStringLiteral("-j"), QStringLiteral("DROP")});
#endif
			return true;
		}
		else if (message.command<FeatureCommand>() == FeatureCommand::StopInternetBlock)
		{
			qDebug() << "AccessControl: Unblocking internet";
#ifdef Q_OS_WIN
			QProcess::execute(QStringLiteral("netsh"), {QStringLiteral("advfirewall"), QStringLiteral("firewall"), QStringLiteral("delete"), QStringLiteral("rule"), QStringLiteral("name=VeyonBlockInternet")});
#else
			QProcess::execute(QStringLiteral("iptables"), {QStringLiteral("-D"), QStringLiteral("OUTPUT"), QStringLiteral("-p"), QStringLiteral("tcp"), QStringLiteral("-m"), QStringLiteral("multiport"), QStringLiteral("--dports"), QStringLiteral("80,443"), QStringLiteral("-j"), QStringLiteral("DROP")});
#endif
			return true;
		}
	}
	else if (message.featureUid() == m_blockAppsFeature.uid())
	{
		if (message.command<FeatureCommand>() == FeatureCommand::StartAppBlock)
		{
			qDebug() << "AccessControl: Blocking apps";
			// TODO: Timer baslatilip dongusel sekilde uygulamalar kill edilecek (simdilik anlik yapiyoruz)
			QString appsString = m_configuration.blockedApplications();
			QStringList apps = appsString.split(QLatin1Char(','), Qt::SkipEmptyParts);
			for (const QString& app : apps) {
#ifdef Q_OS_WIN
				QProcess::execute(QStringLiteral("taskkill"), {QStringLiteral("/F"), QStringLiteral("/IM"), app.trimmed()});
#else
				QProcess::execute(QStringLiteral("killall"), {app.trimmed()});
#endif
			}
			return true;
		}
		else if (message.command<FeatureCommand>() == FeatureCommand::StopAppBlock)
		{
			qDebug() << "AccessControl: Unblocking apps";
			return true;
		}
	}
	else if (message.featureUid() == m_blockUSBFeature.uid())
	{
		if (message.command<FeatureCommand>() == FeatureCommand::StartUSBBlock)
		{
			qDebug() << "AccessControl: Blocking USB";
#ifdef Q_OS_WIN
			QProcess::execute(QStringLiteral("reg"), {QStringLiteral("add"), QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\USBSTOR"), QStringLiteral("/v"), QStringLiteral("Start"), QStringLiteral("/t"), QStringLiteral("REG_DWORD"), QStringLiteral("/d"), QStringLiteral("4"), QStringLiteral("/f")});
#else
			QProcess::execute(QStringLiteral("modprobe"), {QStringLiteral("-r"), QStringLiteral("usb-storage")});
#endif
			return true;
		}
		else if (message.command<FeatureCommand>() == FeatureCommand::StopUSBBlock)
		{
			qDebug() << "AccessControl: Unblocking USB";
#ifdef Q_OS_WIN
			QProcess::execute(QStringLiteral("reg"), {QStringLiteral("add"), QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\USBSTOR"), QStringLiteral("/v"), QStringLiteral("Start"), QStringLiteral("/t"), QStringLiteral("REG_DWORD"), QStringLiteral("/d"), QStringLiteral("3"), QStringLiteral("/f")});
#else
			QProcess::execute(QStringLiteral("modprobe"), {QStringLiteral("usb-storage")});
#endif
			return true;
		}
	}

	return false;
}
