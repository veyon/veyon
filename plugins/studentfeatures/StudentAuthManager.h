/*
 * StudentAuthManager.h - handles student authentication logic
 *
 * This file is part of Veyon - https://veyon.io
 */

#pragma once

#include <QString>
#include <QVariantMap>

// A simple struct to hold student information
struct StudentInfo
{
	QString civilId;
	QString name;
	QString className;
	QString computerName;
};

class StudentFeaturesConfiguration;

class StudentAuthManager : public QObject
{
	Q_OBJECT
public:
	explicit StudentAuthManager(StudentFeaturesConfiguration& configuration, QObject* parent = nullptr);

	enum AuthResult
	{
		LoginSuccessful,
		AdminLoginSuccessful,
		LoginFailed
	};

	AuthResult authenticate(const QString& id, StudentInfo& info);

private:
	void loadStudentDatabase(); // Placeholder for Step 5
	void loadAdminPassword();

	QMap<QString, StudentInfo> m_studentDatabase;
	QString m_adminPassword;
	StudentFeaturesConfiguration& m_configuration;
};
