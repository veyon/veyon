/*
 * StudentAuthManager.cpp - implementation of StudentAuthManager
 *
 * This file is part of Veyon - https://veyon.io
 */

#include "StudentAuthManager.h"
#include "StudentFeaturesConfiguration.h"
#include "plugins/builtindirectory/BuiltinDirectoryConfiguration.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>

StudentAuthManager::StudentAuthManager(StudentFeaturesConfiguration& configuration, QObject* parent)
	: QObject(parent)
	, m_configuration(configuration)
{
	loadAdminPassword();
	// loadStudentDatabase() is called on demand to ensure it has fresh data
}

void StudentAuthManager::loadAdminPassword()
{
	m_configuration.reloadFromStore();
	m_adminPassword = m_configuration.adminPassword();

	if (m_adminPassword.isEmpty()) {
		m_adminPassword = "veyon_admin";
	}

	qDebug() << "Admin password loaded.";
}

void StudentAuthManager::loadStudentDatabase()
{
	qDebug() << "Loading student database from configuration...";
	m_studentDatabase.clear();

	// We need to access the configuration of the built-in directory.
	// We assume its configuration is accessible.
	BuiltinDirectoryConfiguration directoryConfig;
	const QJsonArray networkObjects = directoryConfig.networkObjects();

	for (const QJsonValue& value : networkObjects)
	{
		const QJsonObject obj = value.toObject();
		if (obj.contains("studentData"))
		{
			const QJsonObject studentData = obj.value("studentData").toObject();
			const QString civilId = studentData.value("civilId").toString();

			if (!civilId.isEmpty())
			{
				StudentInfo info;
				info.civilId = civilId;
				info.name = studentData.value("name").toString();
				info.className = studentData.value("class").toString();
				info.computerName = obj.value("name").toString();
				m_studentDatabase.insert(civilId, info);
			}
		}
	}

	qDebug() << "Loaded" << m_studentDatabase.size() << "students from configuration.";
}

StudentAuthManager::AuthResult StudentAuthManager::authenticate(const QString& id, StudentInfo& info)
{
	// Always reload the database to get the latest data
	loadStudentDatabase();

	if (id == m_adminPassword)
	{
		qDebug() << "Admin login successful.";
		return AdminLoginSuccessful;
	}

	if (m_studentDatabase.contains(id))
	{
		info = m_studentDatabase.value(id);
		qDebug() << "Student login successful for" << info.name;
		return LoginSuccessful;
	}

	qDebug() << "Login failed for ID:" << id;
	return LoginFailed;
}
