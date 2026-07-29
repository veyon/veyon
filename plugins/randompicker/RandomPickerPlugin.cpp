#include "RandomPickerPlugin.h"
#include "LuckyWidget.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"
#include "FeatureWorkerManager.h"
#include <QRandomGenerator>
#include <QMessageBox>

RandomPickerPlugin::RandomPickerPlugin(QObject* parent) :
	QObject(parent),
	m_pickerFeature(QStringLiteral("RandomPicker"), Feature::Flag::Action | Feature::Flag::AllComponents,
				  uid(), Feature::Uid(), tr("Öğrenci Seç"), {}, tr("Laboratuvardaki aktif öğrenciler arasından rastgele birini seçerek animasyon gösterir."), QStringLiteral(":/core/media-playback-start.png")),
	m_features({m_pickerFeature})
{
}

RandomPickerPlugin::~RandomPickerPlugin()
{
	delete m_luckyWidget;
}

bool RandomPickerPlugin::controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
								const ComputerControlInterfaceList& computerControlInterfaces)
{
	Q_UNUSED(arguments);
	if (featureUid != m_pickerFeature.uid()) return false;

	if (operation == Operation::Start) {
		if (computerControlInterfaces.isEmpty()) {
			QMessageBox::warning(nullptr, tr("Uyarı"), tr("Seçim yapılabilecek hiçbir aktif bilgisayar bulunamadı!"));
			return true;
		}

		// Pick a random index
		int randomIndex = QRandomGenerator::global()->bounded(computerControlInterfaces.size());
		auto luckyComputer = computerControlInterfaces.at(randomIndex);

		// Send message ONLY to the lucky computer
		FeatureMessage msg(featureUid, RandomPickerCommand::YouAreSelected);
		ComputerControlInterfaceList luckyList;
		luckyList.append(luckyComputer);
		
		sendFeatureMessage(msg, luckyList);
		
		QMessageBox::information(nullptr, tr("Sonuç"), tr("Şanslı Bilgisayar Seçildi! Ekranda animasyon oynatılıyor..."));
		return true;
	}
	return false;
}

bool RandomPickerPlugin::handleFeatureMessage(VeyonServerInterface& server,
									  const MessageContext& messageContext,
									  const FeatureMessage& message)
{
	Q_UNUSED(messageContext);
	if (message.featureUid() != m_pickerFeature.uid()) return false;

	server.featureWorkerManager().sendMessageToUnmanagedSessionWorker(message);
	return true;
}

bool RandomPickerPlugin::handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message)
{
	Q_UNUSED(worker);
	if (message.featureUid() != m_pickerFeature.uid()) return false;
	
	auto cmd = message.command<RandomPickerCommand>();
	
	if (cmd == RandomPickerCommand::YouAreSelected) {
		if (!m_luckyWidget) {
			m_luckyWidget = new LuckyWidget();
		}
		m_luckyWidget->playAnimation();
		return true;
	}

	return false;
}
