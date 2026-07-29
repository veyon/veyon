#include "SurveyPlugin.h"
#include "SurveyMasterDialog.h"
#include "SurveyStudentDialog.h"
#include "VeyonServerInterface.h"
#include "FeatureWorkerManager.h"
#include "PlatformCoreFunctions.h"
#include "VeyonCore.h"

SurveyPlugin::SurveyPlugin( QObject* parent ) :
	QObject( parent ),
	m_surveyFeature( QStringLiteral("Survey"),
					 Feature::Flag::Action | Feature::Flag::AllComponents,
					 Feature::Uid(SURVEY_FEATURE_UID),
					 Feature::Uid(),
					 tr("Survey"), tr("Survey"),
					 tr("Send a multiple choice question to students"), QStringLiteral(":/core/survey.png")),
	m_features( {m_surveyFeature} ),
	m_masterDialog( nullptr )
{
}

SurveyPlugin::~SurveyPlugin()
{
	delete m_masterDialog;
}

bool SurveyPlugin::controlFeature( Feature::Uid featureUid, Operation operation,
								   const QVariantMap& arguments,
								   const ComputerControlInterfaceList& computerControlInterfaces )
{
    Q_UNUSED(arguments);
	if (featureUid != m_surveyFeature.uid())
		return false;

	if (operation == Operation::Start)
	{
		if (!m_masterDialog)
		{
			m_masterDialog = new SurveyMasterDialog(nullptr);
		}
		// Sadece secili bilgisayarlara anket yolla
		auto targetComputers = computerControlInterfaces;
		targetComputers.removeLocalHostInterfaces();

		m_masterDialog->setComputers(targetComputers);
		m_masterDialog->show();
		m_masterDialog->raise();
		m_masterDialog->activateWindow();
		return true;
	}
	return false;
}

// Master (Öğretmen) tarafından cevabın yakalanması
bool SurveyPlugin::handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface,
										 const FeatureMessage& message )
{
	if (message.featureUid() == m_surveyFeature.uid() &&
		message.command<SurveyCommand>() == SurveyCommand::SubmitAnswer)
	{
		if (m_masterDialog)
		{
			QString answer = message.argument(SurveyArgument::Answer).toString();
			m_masterDialog->addAnswer(computerControlInterface->computerName(), answer);
		}
		return true;
	}
	return false;
}

// Client service routing to worker
bool SurveyPlugin::handleFeatureMessage( VeyonServerInterface& server,
										 const MessageContext& messageContext,
										 const FeatureMessage& message )
{
    Q_UNUSED(messageContext);
	if (message.featureUid() == m_surveyFeature.uid())
	{
		server.featureWorkerManager().sendMessageToManagedSystemWorker(message);
		return true;
	}
	return false;
}

// Worker displaying the student dialog
bool SurveyPlugin::handleFeatureMessage( VeyonWorkerInterface& worker,
										 const FeatureMessage& message )
{
	if (message.featureUid() == m_surveyFeature.uid())
	{
		if (message.command<SurveyCommand>() == SurveyCommand::StartSurvey)
		{
			SurveyStudentDialog::showDialog(message, &worker);
			return true;
		}
		else if (message.command<SurveyCommand>() == SurveyCommand::StopSurvey)
		{
			SurveyStudentDialog::closeDialog();
			return true;
		}
	}
	return false;
}
