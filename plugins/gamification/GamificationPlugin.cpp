#include "GamificationPlugin.h"
#include "BadgeWidget.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"
#include "FeatureWorkerManager.h"
#include <QInputDialog>
#include <QStringList>

GamificationPlugin::GamificationPlugin(QObject* parent) :
	QObject(parent),
	m_gamificationFeature(QStringLiteral("Gamification"), Feature::Flag::Action | Feature::Flag::AllComponents,
				  uid(), Feature::Uid(), tr("Rozet Gönder"), {}, tr("Öğrencilere tebrik ve ödül rozeti gönderin."), QStringLiteral(":/core/toast-success.png")),
	m_features({m_gamificationFeature})
{
}

GamificationPlugin::~GamificationPlugin()
{
	delete m_badgeWidget;
}

bool GamificationPlugin::controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
								const ComputerControlInterfaceList& computerControlInterfaces)
{
	Q_UNUSED(arguments);
	if (featureUid != m_gamificationFeature.uid()) return false;

	if (operation == Operation::Start) {
		QStringList items;
		items << tr("⭐ Yıldız") << tr("👏 Harika İş!") << tr("🚀 Mükemmel Kod") << tr("🥇 Birinci Sınıf!") << tr("💡 Harika Fikir");
		
		bool ok;
		QString item = QInputDialog::getItem(nullptr, tr("Rozet Gönder"),
											 tr("Öğrencilere göndermek istediğiniz rozeti seçin:"), items, 0, false, &ok);
		if (ok && !item.isEmpty()) {
			FeatureMessage msg(featureUid, GamificationCommand::ShowBadge);
			msg.addArgument(GamificationArgument::BadgeText, item);
			
			// Simple color mapping based on text
			QString color = "#FFD700"; // Gold by default
			if (item.contains("Harika İş")) color = "#4CAF50"; // Green
			else if (item.contains("Mükemmel Kod")) color = "#2196F3"; // Blue
			else if (item.contains("💡")) color = "#FF9800"; // Orange
			
			msg.addArgument(GamificationArgument::BadgeColor, color);
			sendFeatureMessage(msg, computerControlInterfaces);
		}
		return true;
	}
	return false;
}

bool GamificationPlugin::handleFeatureMessage(VeyonServerInterface& server,
									  const MessageContext& messageContext,
									  const FeatureMessage& message)
{
	Q_UNUSED(messageContext);
	if (message.featureUid() != m_gamificationFeature.uid()) return false;

	server.featureWorkerManager().sendMessageToUnmanagedSessionWorker(message);
	return true;
}

bool GamificationPlugin::handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message)
{
	Q_UNUSED(worker);
	if (message.featureUid() != m_gamificationFeature.uid()) return false;
	
	auto cmd = message.command<GamificationCommand>();

	if (cmd == GamificationCommand::ShowBadge) {
		QString text = message.argument(GamificationArgument::BadgeText).toString();
		QString color = message.argument(GamificationArgument::BadgeColor).toString();
		
		if (!m_badgeWidget) {
			m_badgeWidget = new BadgeWidget();
		}
		m_badgeWidget->showBadge(text, color);
		return true;
	}

	return false;
}
