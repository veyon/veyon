#include "KioskModePlugin.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"
#include "FeatureWorkerManager.h"
#include <QInputDialog>
#include <QProcess>
#include <QDir>
#include <QDebug>

KioskModePlugin::KioskModePlugin(QObject* parent) :
	QObject(parent),
	m_kioskModeFeature(QStringLiteral("KioskMode"), Feature::Flag::Action | Feature::Flag::AllComponents,
				  uid(), Feature::Uid(), tr("Sınav Modu (Kiosk)"), {}, tr("Öğrencileri sadece belirlediğiniz URL'ye girmeye zorlayın."), QStringLiteral(":/core/view-fullscreen.png")),
	m_features({m_kioskModeFeature})
{
}

KioskModePlugin::~KioskModePlugin()
{
}

bool KioskModePlugin::controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
								const ComputerControlInterfaceList& computerControlInterfaces)
{
	Q_UNUSED(arguments);
	if (featureUid != m_kioskModeFeature.uid()) return false;

	if (operation == Operation::Start) {
		bool ok;
		QString url = QInputDialog::getText(nullptr, tr("Sınav Modunu Başlat"),
											tr("Sınav adresini veya web sitesi URL'sini girin:"),
											QLineEdit::Normal, "https://", &ok);
		if (ok && !url.isEmpty()) {
			FeatureMessage msg(featureUid, KioskModeCommand::StartKiosk);
			msg.addArgument(KioskModeArgument::Url, url);
			sendFeatureMessage(msg, computerControlInterfaces);
		}
		return true;
	} else if (operation == Operation::Stop) {
		FeatureMessage msg(featureUid, KioskModeCommand::StopKiosk);
		sendFeatureMessage(msg, computerControlInterfaces);
		return true;
	}
	return false;
}

bool KioskModePlugin::handleFeatureMessage(VeyonServerInterface& server,
									  const MessageContext& messageContext,
									  const FeatureMessage& message)
{
	Q_UNUSED(messageContext);
	if (message.featureUid() != m_kioskModeFeature.uid()) return false;

	server.featureWorkerManager().sendMessageToUnmanagedSessionWorker(message);
	return true;
}

bool KioskModePlugin::handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message)
{
	Q_UNUSED(worker);
	if (message.featureUid() != m_kioskModeFeature.uid()) return false;
	
	auto cmd = message.command<KioskModeCommand>();
	
	if (cmd == KioskModeCommand::StartKiosk) {
		QString url = message.argument(KioskModeArgument::Url).toString();
		launchKioskBrowser(url);
		return true;
	} else if (cmd == KioskModeCommand::StopKiosk) {
		killKioskBrowser();
		return true;
	}

	return false;
}

void KioskModePlugin::launchKioskBrowser(const QString& url)
{
#ifdef Q_OS_WIN
	// Close existing edges
	QProcess::execute("taskkill", QStringList() << "/F" << "/IM" << "msedge.exe");
	QProcess::execute("taskkill", QStringList() << "/F" << "/IM" << "chrome.exe");
	
	// Start edge in kiosk
	QString cmd = QString("cmd.exe /c start msedge --kiosk \"%1\"").arg(url);
	QProcess::startDetached(cmd);
#else
	// Linux fallback - try chrome or firefox
	QProcess::execute("killall", QStringList() << "google-chrome");
	QProcess::execute("killall", QStringList() << "firefox");
	
	QProcess::startDetached("google-chrome", QStringList() << "--kiosk" << url);
#endif
}

void KioskModePlugin::killKioskBrowser()
{
#ifdef Q_OS_WIN
	QProcess::execute("taskkill", QStringList() << "/F" << "/IM" << "msedge.exe");
	QProcess::execute("taskkill", QStringList() << "/F" << "/IM" << "chrome.exe");
#else
	QProcess::execute("killall", QStringList() << "google-chrome");
#endif
}
