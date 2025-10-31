#include "Configuration/UiMapping.h"
#include "MacVncConfiguration.h"
#include "MacVncConfigurationWidget.h"

#include "ui_MacVncConfigurationWidget.h"

MacVncConfigurationWidget::MacVncConfigurationWidget( MacVncConfiguration& configuration, QWidget* parent ) :
	QWidget( parent ),
	ui( new Ui::MacVncConfigurationWidget ),
	m_configuration( configuration )
{
	ui->setupUi( this );

	FOREACH_MAC_VNC_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_MAC_VNC_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



MacVncConfigurationWidget::~MacVncConfigurationWidget()
{
	delete ui;
}
