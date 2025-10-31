#pragma once

#include <QWidget>

namespace Ui {
class MacVncConfigurationWidget;
}

class MacVncConfiguration;

class MacVncConfigurationWidget : public QWidget
{
	Q_OBJECT

public:
	explicit MacVncConfigurationWidget( MacVncConfiguration& configuration, QWidget* parent = nullptr );
	~MacVncConfigurationWidget() override;

private:
	Ui::MacVncConfigurationWidget* ui;
	MacVncConfiguration& m_configuration;
};
