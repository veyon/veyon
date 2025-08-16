/*
 * StudentFeaturesPlugin.cpp - implementation of StudentFeaturesPlugin class
 *
 * This file is part of Veyon - https://veyon.io
 */

#include "StudentFeaturesPlugin.h"
#include "StudentLoginWidget.h"
#include "StudentInfoSidebar.h"
#include "StudentFeaturesConfiguration.h"
#include "StudentFeaturesConfigurationPage.h"
#include "FeatureWorkerManager.h"
#include "VeyonServerInterface.h"
#include "PlatformSessionFunctions.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDialog>
#include <QInputDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QDialogButtonBox>

StudentFeaturesPlugin::StudentFeaturesPlugin(QObject* parent)
	: QObject(parent)
	, m_studentLoginFeature(
		  QStringLiteral("StudentLogin"),
		  Feature::Flag::Mode | Feature::Flag::AllComponents | Feature::Flag::Meta,
		  Feature::Uid("a4d8f674-e534-4067-9d7a-15ab1573c625"),
		  {},
		  tr("Student Login"),
		  tr("Student Logout"),
		  tr("Forces students to log in with their ID before using the computer."),
		  QStringLiteral(":/studentfeatures/user-login.png"))
	, m_grantPointsFeature(
		  QStringLiteral("GrantPoints"),
		  Feature::Flag::Action | Feature::Flag::SelectedComputers,
		  Feature::Uid("b5e9e048-3221-461b-9b62-92130e4a7ce7"),
		  {},
		  tr("Grant Points"),
		  tr("Grant points to selected students"),
		  tr("This will award points to the students at the selected computers."),
		  QStringLiteral(":/studentfeatures/grant-points.png")) // Will add icon later
	, m_setLessonInfoFeature(
		  QStringLiteral("SetLessonInfo"),
		  Feature::Flag::Action | Feature::Flag::AllComputers,
		  Feature::Uid("c6f8d939-2345-4b0c-8a1f-a8e4c1e4a3b8"),
		  {},
		  tr("Set Lesson Info"),
		  tr("Set lesson title and objectives"),
		  tr("This will update the lesson information on all student sidebars."),
		  QStringLiteral(":/studentfeatures/lesson-info.png"))
	, m_globalLogoutFeature(
		  QStringLiteral("GlobalLogout"),
		  Feature::Flag::Action | Feature::Flag::AllComputers,
		  Feature::Uid("d7e7f82a-1469-4f8b-99d3-b8e5d2e3b4c9"),
		  {},
		  tr("Logout All Students"),
		  tr("Logs out all currently logged-in students"),
		  tr("This will bring all student computers back to the login screen."),
		  QStringLiteral(":/studentfeatures/logout-user.png")) // Will add icon later
	, m_features({m_studentLoginFeature, m_grantPointsFeature, m_setLessonInfoFeature, m_globalLogoutFeature})
	, m_loginWidget(nullptr)
	, m_sidebar(nullptr)
	, m_configuration(new StudentFeaturesConfiguration("StudentFeatures", this))
{
	m_authManager = new StudentAuthManager(*m_configuration, this);
}

StudentFeaturesPlugin::~StudentFeaturesPlugin()
{
	delete m_loginWidget;
	delete m_sidebar;
	// m_authManager is deleted automatically due to parent-child relationship
}

Plugin::Uid StudentFeaturesPlugin::uid() const
{
	return Plugin::Uid{QStringLiteral("a9f2a24d-87a8-4c33-a558-b7a1a5801831")};
}

QVersionNumber StudentFeaturesPlugin::version() const
{
	return QVersionNumber(1, 0);
}

QString StudentFeaturesPlugin::name() const
{
	return QStringLiteral("StudentFeatures");
}

QString StudentFeaturesPlugin::description() const
{
	return tr("Provides student-centric features like login, points, and lesson tracking.");
}

QString StudentFeaturesPlugin::vendor() const
{
	return QStringLiteral("Veyon Community");
}

QString StudentFeaturesPlugin::copyright() const
{
	return QStringLiteral("Your Name");
}

const FeatureList& StudentFeaturesPlugin::featureList() const
{
	return m_features;
}

bool StudentFeaturesPlugin::controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
											 const ComputerControlInterfaceList& computerControlInterfaces)
{
	Q_UNUSED(arguments);

	if (featureUid == m_studentLoginFeature.uid())
	{
		if (operation == Operation::Start)
		{
			sendFeatureMessage(FeatureMessage{featureUid, StartLoginCommand}, computerControlInterfaces);
			return true;
		}
		if (operation == Operation::Stop)
		{
			sendFeatureMessage(FeatureMessage{featureUid, StopLoginCommand}, computerControlInterfaces);
			return true;
		}
	}
	else if (featureUid == m_grantPointsFeature.uid())
	{
		bool ok;
		int points = QInputDialog::getInt(nullptr, tr("Grant Points"), tr("Enter points to grant:"), 0, 0, 1000, 1, &ok);
		if (ok)
		{
			sendFeatureMessage(FeatureMessage{featureUid, UpdatePointsCommand, {{"points", points}}}, computerControlInterfaces);
		}
		return true;
	}
	else if (featureUid == m_setLessonInfoFeature.uid())
	{
		QDialog dialog(nullptr);
		dialog.setWindowTitle(tr("Set Lesson Information"));
		QFormLayout form(&dialog);

		auto titleEdit = new QLineEdit(&dialog);
		form.addRow(tr("Lesson Title:"), titleEdit);

		auto objectivesEdit = new QLineEdit(&dialog);
		form.addRow(tr("Lesson Objectives:"), objectivesEdit);

		QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
		form.addRow(&buttonBox);
		QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
		QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

		if (dialog.exec() == QDialog::Accepted)
		{
			sendFeatureMessage(FeatureMessage{featureUid, UpdateLessonInfoCommand,
											  {{"title", titleEdit->text()}, {"objectives", objectivesEdit->text()}}},
							   computerControlInterfaces);
		}
		return true;
	}
	else if (featureUid == m_globalLogoutFeature.uid())
	{
		sendFeatureMessage(FeatureMessage{m_studentLoginFeature.uid(), StopLoginCommand}, computerControlInterfaces);
		return true;
	}


	return false;
}

QList<ConfigurationPage*> StudentFeaturesPlugin::configurationPages() const
{
	return { new StudentFeaturesConfigurationPage(*m_configuration) };
}

bool StudentFeaturesPlugin::handleFeatureMessage(VeyonServerInterface& server, const MessageContext& messageContext,
												   const FeatureMessage& message)
{
	Q_UNUSED(messageContext);

	if (message.featureUid() == m_studentLoginFeature.uid())
	{
		if (server.featureWorkerManager().isWorkerRunning(message.featureUid()) == false &&
			message.command() == StopLoginCommand)
		{
			return true;
		}

		if (VeyonCore::platform().sessionFunctions().currentSessionHasUser() == false)
		{
			vDebug() << "not starting student login since not running in a user session";
			return true;
		}

		server.featureWorkerManager().sendMessageToManagedSystemWorker(message);
		return true;
	}

	return false;
}

bool StudentFeaturesPlugin::handleFeatureMessage(VeyonWorkerInterface& worker, const FeatureMessage& message)
{
	Q_UNUSED(worker);

	if (message.featureUid() == m_studentLoginFeature.uid())
	{
		switch (message.command())
		{
		case StartLoginCommand:
			startLoginProcess();
			return true;

		case StopLoginCommand:
			delete m_loginWidget;
			m_loginWidget = nullptr;
			delete m_sidebar;
			m_sidebar = nullptr;
			QCoreApplication::quit();
			return true;

		case UpdatePointsCommand:
			if (m_sidebar)
			{
				m_sidebar->setPoints(message.arguments().value("points").toInt());
			}
			return true;

		case UpdateLessonInfoCommand:
			if (m_sidebar)
			{
				m_sidebar->setLessonInfo(message.arguments().value("title").toString(),
										 message.arguments().value("objectives").toString());
			}
			return true;

		default:
			break;
		}
	}

	return false;
}

void StudentFeaturesPlugin::startLoginProcess()
{
	if (m_loginWidget == nullptr)
	{
		m_loginWidget = new StudentLoginWidget();
		connect(m_loginWidget, &StudentLoginWidget::loginRequested, this, &StudentFeaturesPlugin::processLoginRequest);
		m_loginWidget->show();
	}
}

void StudentFeaturesPlugin::processLoginRequest(const QString& civilId)
{
	qDebug() << "Processing login request for Civil ID:" << civilId;

	if (!m_loginWidget || !m_authManager)
	{
		return;
	}

	StudentInfo studentInfo;
	const auto result = m_authManager->authenticate(civilId, studentInfo);

	switch (result)
	{
	case StudentAuthManager::LoginSuccessful:
		m_currentStudentInfo = studentInfo;
		// TODO: Notify master about successful login for attendance tracking

		delete m_loginWidget;
		m_loginWidget = nullptr;

		qDebug() << "Login successful for" << m_currentStudentInfo.name << ". Creating sidebar.";
		m_sidebar = new StudentInfoSidebar(m_currentStudentInfo);
		connect(m_sidebar, &StudentInfoSidebar::logoutRequested, this, &StudentFeaturesPlugin::processLogoutRequest);
		m_sidebar->show();
		break;

	case StudentAuthManager::AdminLoginSuccessful:
		delete m_loginWidget;
		m_loginWidget = nullptr;

		qDebug() << "Admin login successful. Unlocking computer.";
		// For admin, we just unlock and don't show a sidebar or login screen again.
		QCoreApplication::quit();
		break;

	case StudentAuthManager::LoginFailed:
		qDebug() << "Login failed.";
		m_loginWidget->showLoginError(tr("Invalid ID. Please try again."));
		break;
	}
}

void StudentFeaturesPlugin::processLogoutRequest()
{
	qDebug() << "Logout requested.";
	if (m_sidebar)
	{
		delete m_sidebar;
		m_sidebar = nullptr;
	}
	// After logout, show the login screen again.
	startLoginProcess();
}
