#include "PageAccessControl.h"
#include "ui_PageAccessControl.h"

PageAccessControl::PageAccessControl(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::PageAccessControl)
{
	ui->setupUi(this);
}



PageAccessControl::~PageAccessControl()
{
	delete ui;
}



void PageAccessControl::addGroup()
{

}



void PageAccessControl::removeGroup()
{

}



void PageAccessControl::addAuthorizationRule()
{

}



void PageAccessControl::removeAuthorizationRule()
{

}



void PageAccessControl::editAuthorizationRule()
{

}



void PageAccessControl::moveAuthorizationRuleDown()
{

}



void PageAccessControl::moveAuthorizationRuleUp()
{

}
