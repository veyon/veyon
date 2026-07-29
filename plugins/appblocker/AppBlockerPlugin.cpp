#include "AppBlockerPlugin.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"
#include "FeatureWorkerManager.h"
#include "AppBlockerDialog.h"
#include <QProcess>
#include <QStringList>
#include <QSettings>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#else
#include <QDir>
#include <QFile>
#endif

AppBlockerPlugin::AppBlockerPlugin(QObject* parent) :
	QObject(parent),
	m_appBlockerFeature(QStringLiteral("AppBlocker"), Feature::Flag::Action | Feature::Flag::AllComponents,
				  uid(), Feature::Uid(), tr("Uygulama Engelle (Kara Liste)"), {}, tr("Öğrencilerin belirli programları çalıştırmasını engeller."), QStringLiteral(":/core/toast-error.png")),
	m_features({m_appBlockerFeature})
{
	connect(&m_killTimer, &QTimer::timeout, this, &AppBlockerPlugin::killBlockedApps);
}

AppBlockerPlugin::~AppBlockerPlugin()
{
	m_killTimer.stop();
}

bool AppBlockerPlugin::controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
								const ComputerControlInterfaceList& computerControlInterfaces)
{
	Q_UNUSED(arguments);
	if (featureUid != m_appBlockerFeature.uid()) return false;

	if (operation == Operation::Start) {
		AppBlockerDialog dialog(nullptr);
		if (dialog.exec() == QDialog::Accepted) {
			QStringList apps = dialog.getBlockedApps();
			bool whitelistMode = dialog.getWhitelistMode();
			FeatureMessage msg(featureUid, AppBlockerCommand::UpdateBlacklist);
			msg.addArgument(AppBlockerArgument::AppList, apps.join(","));
			msg.addArgument(AppBlockerArgument::WhitelistMode, whitelistMode);
			sendFeatureMessage(msg, computerControlInterfaces);
		}
		return true;
	} else if (operation == Operation::Stop) {
		FeatureMessage msg(featureUid, AppBlockerCommand::StopBlocking);
		sendFeatureMessage(msg, computerControlInterfaces);
		return true;
	}
	return false;
}

bool AppBlockerPlugin::handleFeatureMessage(VeyonServerInterface& server,
									  const MessageContext& messageContext,
									  const FeatureMessage& message)
{
	Q_UNUSED(messageContext);
	if (message.featureUid() != m_appBlockerFeature.uid()) return false;

	server.featureWorkerManager().sendMessageToUnmanagedSessionWorker(message);
	return true;
}

bool AppBlockerPlugin::handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message)
{
	Q_UNUSED(worker);
	if (message.featureUid() != m_appBlockerFeature.uid()) return false;
	
	auto cmd = message.command<AppBlockerCommand>();
	
	if (cmd == AppBlockerCommand::UpdateBlacklist) {
		QString appString = message.argument(AppBlockerArgument::AppList).toString();
		bool whitelistMode = message.argument(AppBlockerArgument::WhitelistMode).toBool();
		
		// Save persistently
		QSettings settings("VeyonCommunity", "AppBlocker_Worker");
		settings.setValue("Blacklist", appString.split(",", Qt::SkipEmptyParts));
		settings.setValue("WhitelistMode", whitelistMode);
		
		loadWorkerBlacklist();
		return true;
	} else if (cmd == AppBlockerCommand::StopBlocking) {
		m_killTimer.stop();
		m_blockedApps.clear();
		m_whitelistModeEnabled = false;
		return true;
	}

	return false;
}

void AppBlockerPlugin::startWorker(VeyonWorkerInterface& worker)
{
	Q_UNUSED(worker);
	loadWorkerBlacklist();
}

void AppBlockerPlugin::loadWorkerBlacklist()
{
	QSettings settings("VeyonCommunity", "AppBlocker_Worker");
	QStringList apps = settings.value("Blacklist").toStringList();
	m_whitelistModeEnabled = settings.value("WhitelistMode", false).toBool();
	
	m_blockedApps.clear();
	for(const QString& app : apps) {
		QString trimmed = app.trimmed();
		if (!trimmed.isEmpty()) {
			m_blockedApps.append(trimmed);
		}
	}
	
	if (!m_blockedApps.isEmpty() || m_whitelistModeEnabled) {
		m_killTimer.start(5000); // Check every 5 seconds
	} else {
		m_killTimer.stop();
	}
}

void AppBlockerPlugin::killBlockedApps()
{
	// 1. Blacklist Logic
	for (const QString& app : m_blockedApps) {
#ifdef Q_OS_WIN
		QString target = app;
		if (!target.toLower().endsWith(".exe")) {
			target += ".exe";
		}
		QProcess::execute("taskkill", QStringList() << "/F" << "/IM" << target);
#else
		QProcess::execute("killall", QStringList() << "-9" << app);
#endif
	}

	// 2. Whitelist Logic
	if (m_whitelistModeEnabled) {
#ifdef Q_OS_WIN
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot != INVALID_HANDLE_VALUE) {
			PROCESSENTRY32W pe;
			pe.dwSize = sizeof(PROCESSENTRY32W);

			if (Process32FirstW(hSnapshot, &pe)) {
				do {
					// Skip System Idle Process (0) and System (4)
					if (pe.th32ProcessID == 0 || pe.th32ProcessID == 4) continue;
					
					HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
					if (hProc) {
						WCHAR path[MAX_PATH];
						DWORD size = MAX_PATH;
						if (QueryFullProcessImageNameW(hProc, 0, path, &size)) {
							QString procPath = QString::fromWCharArray(path).toLower();
							procPath = procPath.replace("/", "\\");
							
							// Allowed directories:
							// C:\Windows, C:\Program Files, C:\Program Files (x86), C:\ProgramData
							// Veyon
							// Zirve Finansman: C:\zirvenet, C:\zirve
							if (!procPath.startsWith("c:\\windows\\") &&
								!procPath.startsWith("c:\\program files\\") &&
								!procPath.startsWith("c:\\program files (x86)\\") &&
								!procPath.startsWith("c:\\programdata\\") &&
								!procPath.startsWith("c:\\zirvenet\\") &&
								!procPath.startsWith("c:\\zirve\\") &&
								!procPath.contains("veyon")) {
								
								// Blocked!
								TerminateProcess(hProc, 0);
							}
						}
						CloseHandle(hProc);
					}
				} while (Process32NextW(hSnapshot, &pe));
			}
			CloseHandle(hSnapshot);
		}
#else
		QDir procDir("/proc");
		for (const QString& pidStr : procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
			bool ok;
			int pid = pidStr.toInt(&ok);
			if (ok) {
				QFile exeFile(QString("/proc/%1/exe").arg(pid));
				QString procPath = exeFile.symLinkTarget().toLower();
				if (!procPath.isEmpty()) {
					if (!procPath.startsWith("/usr/") &&
						!procPath.startsWith("/bin/") &&
						!procPath.startsWith("/sbin/") &&
						!procPath.startsWith("/opt/") &&
						!procPath.startsWith("/lib") &&
						!procPath.contains("veyon")) {
						
						QProcess::execute("kill", QStringList() << "-9" << QString::number(pid));
					}
				}
			}
		}
#endif
	}
}
