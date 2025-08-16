/*
 * StudentInfoSidebar.h - sidebar for displaying student information
 *
 * This file is part of Veyon - https://veyon.io
 */

#pragma once

#include <QWidget>
#include "StudentAuthManager.h" // For the StudentInfo struct

class QLabel;
class QPushButton;

class StudentInfoSidebar : public QWidget
{
	Q_OBJECT
public:
	explicit StudentInfoSidebar(const StudentInfo& info, QWidget* parent = nullptr);
	~StudentInfoSidebar() override;

	void setPoints(int points);
	void setLessonInfo(const QString& title, const QString& objectives);

signals:
	void logoutRequested();

private:
	void setupUi();

	StudentInfo m_studentInfo;

	// UI Elements
	QLabel* m_nameLabel;
	QLabel* m_classLabel;
	QLabel* m_computerLabel;
	QLabel* m_pointsLabel;
	QLabel* m_lessonTitleLabel;
	QLabel* m_lessonObjectivesLabel;
	QPushButton* m_logoutButton;
};
