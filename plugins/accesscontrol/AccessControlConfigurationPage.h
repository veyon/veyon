#pragma once

#include "ConfigurationPage.h"
#include "AccessControlConfiguration.h"

namespace Ui {
class AccessControlConfigurationPage;
}

class AccessControlConfigurationPage : public ConfigurationPage
{
	Q_OBJECT

public:
	explicit AccessControlConfigurationPage( AccessControlConfiguration& configuration,
											 QWidget* parent = nullptr );
	~AccessControlConfigurationPage() override;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

private:
	Ui::AccessControlConfigurationPage* ui;
	AccessControlConfiguration& m_configuration;
};
