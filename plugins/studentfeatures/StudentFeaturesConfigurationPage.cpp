/*
 * StudentFeaturesConfigurationPage.cpp
 *
 * This file is part of Veyon - https://veyon.io
 */

#include "StudentFeaturesConfigurationPage.h"
#include "StudentFeaturesConfiguration.h"
#include "Configuration/UiMapping.h"

#include "ui_StudentFeaturesConfigurationPage.h"

StudentFeaturesConfigurationPage::StudentFeaturesConfigurationPage(StudentFeaturesConfiguration& configuration, QWidget* parent)
	: ConfigurationPage(parent)
	, ui(new Ui::StudentFeaturesConfigurationPage)
	, m_configuration(configuration)
{
	ui->setupUi(this);
}

StudentFeaturesConfigurationPage::~StudentFeaturesConfigurationPage()
{
	delete ui;
}

void StudentFeaturesConfigurationPage::resetWidgets()
{
	FOREACH_STUDENTFEATURES_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
}

void StudentFeaturesConfigurationPage::connectWidgetsToProperties()
{
	FOREACH_STUDENTFEATURES_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}
