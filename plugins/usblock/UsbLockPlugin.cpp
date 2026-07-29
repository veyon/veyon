#include "UsbLockPlugin.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"
#include "FeatureWorkerManager.h"
#include <QSettings>
#include <QProcess>
#include <QDebug>

UsbLockPlugin::UsbLockPlugin(QObject* parent) :
	QObject(parent),
	m_lockUsbFeature(QStringLiteral("LockUSB"), Feature::Flag::Action | Feature::Flag::AllComponents,
				  uid(), Feature::Uid(), tr("USB Kilitle"), {}, tr("Öğrenci bilgisayarlarında USB kullanımını yasakla."), QStringLiteral(":/core/toast-warning.png")),
	m_unlockUsbFeature(QStringLiteral("UnlockUSB"), Feature::Flag::Action | Feature::Flag::AllComponents,
				  uid(), Feature::Uid(), tr("USB İzin Ver"), {}, tr("Öğrenci bilgisayarlarında USB kullanımına izin ver."), QStringLiteral(":/core/toast-warning.png")),
	m_features({m_lockUsbFeature, m_unlockUsbFeature})
{
}

bool UsbLockPlugin::controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
								const ComputerControlInterfaceList& computerControlInterfaces)
{
	Q_UNUSED(arguments);
	if (featureUid != m_lockUsbFeature.uid() && featureUid != m_unlockUsbFeature.uid()) return false;

	if (operation == Operation::Start) {
		UsbLockCommand cmd = (featureUid == m_lockUsbFeature.uid()) ? UsbLockCommand::LockUsb : UsbLockCommand::UnlockUsb;
		FeatureMessage msg(featureUid, cmd);
		sendFeatureMessage(msg, computerControlInterfaces);
		return true;
	}
	return false;
}

bool UsbLockPlugin::handleFeatureMessage(VeyonServerInterface& server,
									  const MessageContext& messageContext,
									  const FeatureMessage& message)
{
	Q_UNUSED(messageContext);
	if (message.featureUid() != m_lockUsbFeature.uid() && message.featureUid() != m_unlockUsbFeature.uid()) return false;

	// Forward to worker
	server.featureWorkerManager().sendMessageToUnmanagedSessionWorker(message);
	return true;
}

bool UsbLockPlugin::handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message)
{
	Q_UNUSED(worker);
	if (message.featureUid() != m_lockUsbFeature.uid() && message.featureUid() != m_unlockUsbFeature.uid()) return false;
	
	auto cmd = message.command<UsbLockCommand>();
	
	if (cmd == UsbLockCommand::LockUsb) {
		setUsbLocked(true);
		return true;
	} else if (cmd == UsbLockCommand::UnlockUsb) {
		setUsbLocked(false);
		return true;
	}

	return false;
}

void UsbLockPlugin::setUsbLocked(bool locked)
{
#ifdef Q_OS_WIN
	// Change registry value
	QSettings settings("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\USBSTOR", QSettings::NativeFormat);
	if (settings.isWritable()) {
		settings.setValue("Start", locked ? 4 : 3);
	} else {
		qWarning() << "USB Lock Error: Insufficient permissions to modify registry.";
	}
#else
	// Linux fallback using modprobe
	if (locked) {
		QProcess::execute("modprobe", QStringList() << "-r" << "usb-storage");
	} else {
		QProcess::execute("modprobe", QStringList() << "usb-storage");
	}
#endif
}
