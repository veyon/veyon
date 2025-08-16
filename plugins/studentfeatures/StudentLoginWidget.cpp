/*
 * StudentLoginWidget.cpp - implementation of StudentLoginWidget
 *
 * This file is part of Veyon - https://veyon.io
 */

#include "StudentLoginWidget.h"
#include "PlatformCoreFunctions.h"
#include "PlatformInputDeviceFunctions.h"

#include <QApplication>
#include <QPainter>
#include <QScreen>
#include <QWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QGuiApplication>

StudentLoginWidget::StudentLoginWidget(QWidget* parent)
	: QWidget(parent, Qt::X11BypassWindowManagerHint)
{
	VeyonCore::platform().coreFunctions().setSystemUiState(false);
	VeyonCore::platform().inputDeviceFunctions().disableInputDevices();

	setupUi();

	auto primaryScreen = QGuiApplication::primaryScreen();
	move(primaryScreen->geometry().topLeft());
	resize(primaryScreen->size());

	VeyonCore::platform().coreFunctions().raiseWindow(this, true);
	showFullScreen();

	setFocusPolicy(Qt::StrongFocus);
	setFocus();
	grabMouse();
	grabKeyboard();

	// We don't hide the cursor, user needs it to interact with the widget
}

StudentLoginWidget::~StudentLoginWidget()
{
	VeyonCore::platform().inputDeviceFunctions().enableInputDevices();
	VeyonCore::platform().coreFunctions().setSystemUiState(true);
}

void StudentLoginWidget::setupUi()
{
	// Set a dark background
	QPalette pal = palette();
	pal.setColor(QPalette::Window, QColor(40, 40, 40));
	setAutoFillBackground(true);
	setPalette(pal);

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(100, 100, 100, 100);
	layout->setSpacing(20);
	layout->setAlignment(Qt::AlignCenter);

	auto* titleLabel = new QLabel(tr("Student Login"), this);
	titleLabel->setAlignment(Qt::AlignCenter);
	QFont titleFont = titleLabel->font();
	titleFont.setPointSize(24);
	titleFont.setBold(true);
	titleLabel->setFont(titleFont);
	titleLabel->setStyleSheet("color: white;");

	m_civilIdLineEdit = new QLineEdit(this);
	m_civilIdLineEdit->setPlaceholderText(tr("Enter your Civil ID"));
	m_civilIdLineEdit->setMinimumHeight(40);
	QFont editFont = m_civilIdLineEdit->font();
	editFont.setPointSize(16);
	m_civilIdLineEdit->setFont(editFont);

	m_loginButton = new QPushButton(tr("Login"), this);
	m_loginButton->setMinimumHeight(40);
	QFont buttonFont = m_loginButton->font();
	buttonFont.setPointSize(16);
	m_loginButton->setFont(buttonFont);

	m_messageLabel = new QLabel(this);
	m_messageLabel->setAlignment(Qt::AlignCenter);
	m_messageLabel->setStyleSheet("color: red;");
	QFont messageFont = m_messageLabel->font();
	messageFont.setPointSize(14);
	m_messageLabel->setFont(messageFont);

	QWidget* centralWidget = new QWidget(this);
	centralWidget->setFixedSize(400, 300);
	centralWidget->setStyleSheet("background-color: rgba(60, 60, 60, 200); border-radius: 15px;");

	QVBoxLayout* centralLayout = new QVBoxLayout(centralWidget);
	centralLayout->addWidget(titleLabel);
	centralLayout->addWidget(m_civilIdLineEdit);
	centralLayout->addWidget(m_loginButton);
	centralLayout->addWidget(m_messageLabel);
	centralLayout->setAlignment(Qt::AlignCenter);

	layout->addWidget(centralWidget);

	setLayout(layout);

	connect(m_loginButton, &QPushButton::clicked, this, [this]() {
		emit loginRequested(m_civilIdLineEdit->text());
	});

	// Allow login on pressing Enter
	connect(m_civilIdLineEdit, &QLineEdit::returnPressed, this, [this]() {
		emit loginRequested(m_civilIdLineEdit->text());
	});

	m_messageLabel->setText("");
}

void StudentLoginWidget::showLoginError(const QString& message)
{
	m_messageLabel->setText(message);
	m_civilIdLineEdit->selectAll();
	m_civilIdLineEdit->setFocus();
}
