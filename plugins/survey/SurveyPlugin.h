#pragma once

#include <QObject>
#include "FeatureProviderInterface.h"
#include "PluginInterface.h"

enum class SurveyCommand : int {
	StartSurvey = 1,
	StopSurvey = 2,
	SubmitAnswer = 3
};

enum class QuestionType : int {
	SingleChoice = 1,
	MultipleChoice = 2,
	ShortText = 3,
	LongText = 4,
	TrueFalse = 5
};

enum class SurveyArgument : int {
	Question = 1,
	QuestionType,
	Options,
	Answer
};

const char* const SURVEY_FEATURE_UID = "5d7f2a1b-3c4d-5e6f-aaaa-111122223333";

class SurveyMasterDialog;

class SurveyPlugin : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA( IID "io.veyon.Veyon.Plugins.Survey" )
	Q_INTERFACES( PluginInterface FeatureProviderInterface )

public:
	explicit SurveyPlugin( QObject* parent = nullptr );
	~SurveyPlugin() override;

	Plugin::Uid uid() const override { return Plugin::Uid{ QStringLiteral("77d7f2a1-3c4d-5e6f-aaaa-111122223333") }; }
	QVersionNumber version() const override { return QVersionNumber( 1, 0 ); }
	QString name() const override { return QStringLiteral("Survey"); }
	QString description() const override { return tr("Multiple Choice Survey"); }
	QString vendor() const override { return QStringLiteral("Veyon Community"); }
	QString copyright() const override { return QStringLiteral("Community"); }

	const FeatureList& featureList() const override { return m_features; }

	bool controlFeature( Feature::Uid featureUid, Operation operation,
						 const QVariantMap& arguments,
						 const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface,
							   const FeatureMessage& message ) override;

	bool handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message ) override;

	bool handleFeatureMessage( VeyonWorkerInterface& worker,
							   const FeatureMessage& message ) override;

private:
	Feature m_surveyFeature;
	FeatureList m_features;
	SurveyMasterDialog* m_masterDialog;
};
