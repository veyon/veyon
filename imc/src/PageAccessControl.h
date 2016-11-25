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

private slots:
	void addGroup();
	void removeGroup();
	void addAuthorizationRule();
	void removeAuthorizationRule();
	void editAuthorizationRule();
	void moveAuthorizationRuleDown();
	void moveAuthorizationRuleUp();

private:
	Ui::PageAccessControl *ui;
};

#endif // PAGEACCESSCONTROL_H
