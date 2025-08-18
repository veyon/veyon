/*
 * StudentFeaturesConfigurationPage.h
 *
 * This file is part of Veyon - https://veyon.io
 */

#pragma once

#include "ConfigurationPage.h"

namespace Ui {
class StudentFeaturesConfigurationPage;
}

class StudentFeaturesConfiguration;

class StudentFeaturesConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	explicit StudentFeaturesConfigurationPage(StudentFeaturesConfiguration& configuration, QWidget* parent = nullptr);
	~StudentFeaturesConfigurationPage() override;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;

private:
	Ui::StudentFeaturesConfigurationPage* ui;
	StudentFeaturesConfiguration& m_configuration;
};
