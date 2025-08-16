/*
 * StudentLoginWidget.h - widget for forcing student login
 *
 * This file is part of Veyon - https://veyon.io
 */

#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QLabel;

class StudentLoginWidget : public QWidget
{
	Q_OBJECT
public:
	explicit StudentLoginWidget(QWidget* parent = nullptr);
	~StudentLoginWidget() override;

	void showLoginError(const QString& message);

signals:
	void loginRequested(const QString& civilId);

private:
	void setupUi();

	QLineEdit* m_civilIdLineEdit;
	QPushButton* m_loginButton;
	QLabel* m_messageLabel;
};
