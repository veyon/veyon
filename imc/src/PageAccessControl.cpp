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
