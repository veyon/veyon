#include "AccessControlConfigurationPage.h"
#include "ui_AccessControlConfigurationPage.h"

AccessControlConfigurationPage::AccessControlConfigurationPage( AccessControlConfiguration& configuration, QWidget* parent ) :
	ConfigurationPage( parent ),
	ui( new Ui::AccessControlConfigurationPage ),
	m_configuration( configuration )
{
	ui->setupUi( this );
}

AccessControlConfigurationPage::~AccessControlConfigurationPage()
{
	delete ui;
}

void AccessControlConfigurationPage::resetWidgets()
{
	ui->BlockedApplications->setText( m_configuration.blockedApplications() );
	ui->BlockInternetEntirely->setChecked( m_configuration.blockInternetEntirely() );
}

void AccessControlConfigurationPage::connectWidgetsToProperties()
{
	connect( ui->BlockedApplications, &QLineEdit::textChanged, this, &ConfigurationPage::widgetsChanged );
	connect( ui->BlockInternetEntirely, &QCheckBox::toggled, this, &ConfigurationPage::widgetsChanged );
}

void AccessControlConfigurationPage::applyConfiguration()
{
	m_configuration.setBlockedApplications( ui->BlockedApplications->text() );
	m_configuration.setBlockInternetEntirely( ui->BlockInternetEntirely->isChecked() );
}
