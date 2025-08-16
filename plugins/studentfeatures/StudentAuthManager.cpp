/*
 * StudentAuthManager.cpp - implementation of StudentAuthManager
 *
 * This file is part of Veyon - https://veyon.io
 */

#include "StudentAuthManager.h"
#include "ConfigurationManager.h"
#include "Configuration.h"

#include <QDebug>

StudentAuthManager::StudentAuthManager(QObject* parent)
	: QObject(parent)
{
	loadAdminPassword();
	loadStudentDatabase();
}

void StudentAuthManager::loadAdminPassword()
{
	// In a real implementation, this should be a secure setting.
	// For now, we read it from a general configuration key.
	// Default to "veyon_admin" if not set.
	m_adminPassword = VeyonCore::configuration->get(
		"Plugins/StudentFeatures/AdminPassword", "veyon_admin").toString();

	if (m_adminPassword.isEmpty()) {
		m_adminPassword = "veyon_admin";
	}

	qDebug() << "Admin password loaded.";
}

void StudentAuthManager::loadStudentDatabase()
{
	// This is a placeholder until Step 5 (CSV import) is implemented.
	// The real implementation will load this from the JSON configuration
	// populated by the modified builtindirectory plugin.
	m_studentDatabase.clear();
	m_studentDatabase.insert("2222", {"2222", "John Doe", "Class A", "PC-01"});
	m_studentDatabase.insert("3333", {"3333", "Jane Smith", "Class B", "PC-02"});
	m_studentDatabase.insert("4444", {"4444", "Peter Jones", "Class A", "PC-03"});

	qDebug() << "Placeholder student database loaded with" << m_studentDatabase.size() << "students.";
}

StudentAuthManager::AuthResult StudentAuthManager::authenticate(const QString& id, StudentInfo& info)
{
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
