/*
 * StudentFeaturesPlugin.h
 *
 * This file is part of Veyon - https://veyon.io
 */

#pragma once

#include "FeatureProviderInterface.h"
#include "PluginInterface.h"
#include "ConfigurationPageProviderInterface.h"
#include "StudentAuthManager.h"

class StudentLoginWidget;
class StudentInfoSidebar;
class StudentFeaturesConfiguration;
class StudentFeaturesConfigurationPage;

class StudentFeaturesPlugin : public QObject, public FeatureProviderInterface, public ConfigurationPageProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.StudentFeatures")
	Q_INTERFACES(PluginInterface FeatureProviderInterface ConfigurationPageProviderInterface)

public:
	explicit StudentFeaturesPlugin(QObject* parent = nullptr);
	~StudentFeaturesPlugin() override;

	// PluginInterface
	Plugin::Uid uid() const override;
	QVersionNumber version() const override;
	QString name() const override;
	QString description() const override;
	QString vendor() const override;
	QString copyright() const override;

	// FeatureProviderInterface
	const FeatureList& featureList() const override;
	bool controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces) override;
	bool handleFeatureMessage(VeyonServerInterface& server, const MessageContext& messageContext,
							  const FeatureMessage& message) override;
	bool handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message) override;

	// ConfigurationPageProviderInterface
	QList<ConfigurationPage*> configurationPages() const override;

private slots:
    void processLoginRequest(const QString& civilId);
	void processLogoutRequest();
    void startLoginProcess();

private:
	enum Commands
	{
		StartLoginCommand,
		StopLoginCommand,
		UpdatePointsCommand,
		UpdateLessonInfoCommand,
		SetStudentNameCommand, // Sent from worker to service
		CommandCount
	};

	const Feature m_studentLoginFeature;
	const Feature m_grantPointsFeature;
	const Feature m_setLessonInfoFeature;
	const Feature m_globalLogoutFeature;
	FeatureList m_features;

	// Worker-side widgets
	StudentLoginWidget* m_loginWidget;
	StudentInfoSidebar* m_sidebar;

	StudentAuthManager* m_authManager;
	StudentInfo m_currentStudentInfo;
	StudentFeaturesConfiguration* m_configuration;
};
