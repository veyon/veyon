#ifndef PAGEACCESSCONTROL_H
#define PAGEACCESSCONTROL_H

#include <QWidget>

namespace Ui {
class PageAccessControl;
}

class PageAccessControl : public QWidget
{
	Q_OBJECT

public:
	explicit PageAccessControl(QWidget *parent = 0);
	~PageAccessControl();

private:
	Ui::PageAccessControl *ui;
};

#endif // PAGEACCESSCONTROL_H
