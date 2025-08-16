/*
 * StudentInfoSidebar.cpp - implementation of StudentInfoSidebar
 *
 * This file is part of Veyon - https://veyon.io
 */

#include "StudentInfoSidebar.h"

#include <QApplication>
#include <QScreen>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>

StudentInfoSidebar::StudentInfoSidebar(const StudentInfo& info, QWidget* parent)
	: QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
	, m_studentInfo(info)
{
	setupUi();

	// Set initial values
	m_nameLabel->setText(m_studentInfo.name);
	m_classLabel->setText(tr("Class: %1").arg(m_studentInfo.className));
	m_computerLabel->setText(tr("Computer: %1").arg(m_studentInfo.computerName));
	setPoints(0); // Default points
	setLessonInfo(tr("No lesson active"), tr("N/A"));

	// Position the widget
	auto* screen = QGuiApplication::primaryScreen();
	const QRect screenGeometry = screen->geometry();
	const int width = 300;
	const int height = screenGeometry.height();
	setGeometry(screenGeometry.width() - width, 0, width, height);
}

StudentInfoSidebar::~StudentInfoSidebar()
{
}

void StudentInfoSidebar::setupUi()
{
	// Set a semi-transparent dark background
	QPalette pal = palette();
	pal.setColor(QPalette::Window, QColor(20, 20, 20, 220));
	setAutoFillBackground(true);
	setPalette(pal);

	setStyleSheet(
        "QLabel { color: white; font-size: 14px; }"
        "QPushButton { background-color: #c0392b; color: white; border: none; padding: 10px; font-size: 14px; }"
        "QPushButton:hover { background-color: #e74c3c; }"
        "QGroupBox { color: #f1c40f; font-size: 16px; font-weight: bold; border: 1px solid #f1c40f; border-radius: 5px; margin-top: 1ex; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top center; padding: 0 3px; }"
    );

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(15, 15, 15, 15);
	layout->setSpacing(10);

	m_nameLabel = new QLabel(this);
	m_nameLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #3498db;");
	m_nameLabel->setAlignment(Qt::AlignCenter);

	m_classLabel = new QLabel(this);
	m_computerLabel = new QLabel(this);

	m_pointsLabel = new QLabel(this);
	m_pointsLabel->setStyleSheet("font-size: 18px; color: #2ecc71;");

	auto* lessonBox = new QGroupBox(tr("Current Lesson"), this);
	auto* lessonLayout = new QVBoxLayout();
	m_lessonTitleLabel = new QLabel(this);
	m_lessonObjectivesLabel = new QLabel(this);
	m_lessonObjectivesLabel->setWordWrap(true);
	lessonLayout->addWidget(m_lessonTitleLabel);
	lessonLayout->addWidget(m_lessonObjectivesLabel);
	lessonBox->setLayout(lessonLayout);

	// TODO: Add Honor Roll / Leaderboard UI

	m_logoutButton = new QPushButton(tr("Logout"), this);

	layout->addWidget(m_nameLabel);
	layout->addSpacing(10);
	layout->addWidget(m_classLabel);
	layout->addWidget(m_computerLabel);
	layout->addSpacing(10);
	layout->addWidget(m_pointsLabel);
	layout->addSpacing(10);
	layout->addWidget(lessonBox);
	layout->addStretch();
	layout->addWidget(m_logoutButton);

	setLayout(layout);

	connect(m_logoutButton, &QPushButton::clicked, this, &StudentInfoSidebar::logoutRequested);
}

void StudentInfoSidebar::setPoints(int points)
{
	m_pointsLabel->setText(tr("Points: %1").arg(points));
}

void StudentInfoSidebar::setLessonInfo(const QString& title, const QString& objectives)
{
	m_lessonTitleLabel->setText(tr("Title: %1").arg(title));
	m_lessonObjectivesLabel->setText(tr("Objectives: %1").arg(objectives));
}
