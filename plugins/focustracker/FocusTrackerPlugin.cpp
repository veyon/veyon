#include "FocusTrackerPlugin.h"
#include "FocusTrackerDialog.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"
#include "FeatureWorkerManager.h"

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <QProcess>
#endif

FocusTrackerPlugin::FocusTrackerPlugin(QObject* parent) :
	QObject(parent),
	m_focusTrackerFeature(QStringLiteral("FocusTracker"), Feature::Flag::Action | Feature::Flag::AllComponents,
				  uid(), Feature::Uid(), tr("Dikkat Takibi"), {}, tr("Öğrencilerin anlık olarak hangi pencerede çalıştığını takip edin."), QStringLiteral(":/core/edit-find.png")),
	m_features({m_focusTrackerFeature})
{
}

FocusTrackerPlugin::~FocusTrackerPlugin()
{
	if (m_dialog) {
		m_dialog->close();
		m_dialog->deleteLater();
	}
}

bool FocusTrackerPlugin::controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
								const ComputerControlInterfaceList& computerControlInterfaces)
{
	Q_UNUSED(arguments);
	if (featureUid != m_focusTrackerFeature.uid()) return false;

	if (operation == Operation::Start) {
		if (!m_dialog) {
			m_dialog = new FocusTrackerDialog();
			// Pass the control interfaces to the dialog so it can broadcast
			m_dialog->setControlInterfaces(computerControlInterfaces);
			m_dialog->setFeatureUid(featureUid);
			m_dialog->setAttribute(Qt::WA_DeleteOnClose);
		}
		m_dialog->show();
		m_dialog->activateWindow();
		return true;
	}
	return false;
}

bool FocusTrackerPlugin::handleFeatureMessage(VeyonServerInterface& server,
									  const MessageContext& messageContext,
									  const FeatureMessage& message)
{
	if (message.featureUid() != m_focusTrackerFeature.uid()) return false;

	auto cmd = message.command<FocusTrackerCommand>();

	if (cmd == FocusTrackerCommand::PollFocus) {
		// Server simply forwards poll to unmanaged workers
		server.featureWorkerManager().sendMessageToUnmanagedSessionWorker(message);
		return true;
	} else if (cmd == FocusTrackerCommand::FocusInfo) {
		// Worker responded with focus info, forward to the Master
		if (m_dialog) {
			QString host = messageContext.peerAddress().toString();
			QString title = message.argument(FocusTrackerArgument::ActiveWindowTitle).toString();
			m_dialog->updateComputerInfo(host, title);
		}
		return true;
	}

	return false;
}

bool FocusTrackerPlugin::handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message)
{
	if (message.featureUid() != m_focusTrackerFeature.uid()) return false;
	
	auto cmd = message.command<FocusTrackerCommand>();
	
	if (cmd == FocusTrackerCommand::PollFocus) {
		// Worker received poll request, reply with active window title
		QString activeTitle = getActiveWindowTitle();
		
		FeatureMessage reply(m_focusTrackerFeature.uid(), FocusTrackerCommand::FocusInfo);
		reply.addArgument(FocusTrackerArgument::ActiveWindowTitle, activeTitle);
		
		worker.sendMessageToMaster(reply);
		return true;
	}

	return false;
}

QString FocusTrackerPlugin::getActiveWindowTitle()
{
#ifdef Q_OS_WIN
	HWND hwnd = GetForegroundWindow();
	if (hwnd) {
		wchar_t title[512];
		if (GetWindowTextW(hwnd, title, 512) > 0) {
			return QString::fromWCharArray(title);
		}
	}
	return tr("Bilinmiyor / Masaüstü");
#else
	// Linux fallback using xdotool
	QProcess process;
	process.start("xdotool", QStringList() << "getwindowfocus" << "getwindowname");
	if (process.waitForFinished(1000)) {
		QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
		if (!output.isEmpty()) {
			return output;
		}
	}
	return tr("Bilinmeyen Pencere (xdotool kurulu olmayabilir)");
#endif
}
