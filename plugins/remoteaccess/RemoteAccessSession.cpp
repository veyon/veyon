// SPDX-License-Identifier: GPL-2.0-or-later
#include "RemoteAccessSession.h"
#include "QuickUi.h"
#include "Screenshot.h"
#include "VeyonMasterInterface.h"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QGuiApplication>
#include <QWindow>
#include <algorithm>
namespace { QPointer<RemoteAccessWorkspace> defaultWorkspace; }
RemoteAccessWorkspace::RemoteAccessWorkspace(QObject* parent) : QObject(parent) {}
RemoteAccessWorkspace::~RemoteAccessWorkspace() { delete m_engine; }
QVariantList RemoteAccessWorkspace::sessions() const
{
	QVariantList result;
	for(auto session:m_sessions)result.append(QVariant::fromValue<QObject*>(session));
	return result;
}
QObject* RemoteAccessWorkspace::currentSession() const{return m_sessions.isEmpty()?nullptr:m_sessions.at(m_current);}
void RemoteAccessWorkspace::add(RemoteAccessSession* session)
{
	m_sessions.append(session);session->m_workspace=this;m_current=m_sessions.size()-1;
	if(!m_engine) {
		m_engine=QuickUi::createEngine(this);
		connect(m_engine, &QQmlEngine::warnings, this, [this](const QList<QQmlError>& warnings) {
			for (const auto& warning : warnings) m_qmlWarnings.append(warning.toString());
			Q_EMIT qmlWarningsChanged();
		});
		m_engine->rootContext()->setContextProperty(QStringLiteral("remoteWorkspace"),this);
		m_engine->load(QUrl(QStringLiteral("qrc:/remoteaccess/RemoteAccess.qml")));
		if(!m_engine->rootObjects().isEmpty())m_window=qobject_cast<QWindow*>(m_engine->rootObjects().first());
		if(!m_window){closeAll();return;}
	}
	Q_EMIT changed();
	m_window->show();m_window->raise();m_window->requestActivate();
}
void RemoteAccessWorkspace::remove(RemoteAccessSession* session)
{
	const auto index=m_sessions.indexOf(session);if(index<0)return;
	m_sessions.removeAt(index);
	if(m_current>index)--m_current;
	m_current=std::max(0,std::min(m_current,int(m_sessions.size())-1));
	if(m_sessions.isEmpty()){if(!m_closing)closeAll();}
	else Q_EMIT changed();
}
void RemoteAccessWorkspace::activate(int index){if(index>=0&&index<m_sessions.size()){m_current=index;Q_EMIT changed();}}
void RemoteAccessWorkspace::detachCurrent()
{
	if(m_sessions.isEmpty())return;
	auto session=m_sessions.at(m_current);
	auto workspace=new RemoteAccessWorkspace(qApp);
	workspace->add(session);
	remove(session);
}
void RemoteAccessWorkspace::closeAll(bool closeWindow)
{
	if(m_closing)return;
	m_closing=true;
	if(defaultWorkspace==this)defaultWorkspace=nullptr;
	const auto copy=m_sessions;
	for(auto session:copy)session->close();
	if(closeWindow&&m_window)m_window->close();
	deleteLater();
}
RemoteAccessSession::RemoteAccessSession(const ComputerControlInterface::Pointer& computer,bool viewOnly,bool canToggle,QObject* parent) :
	QObject(parent),m_computer(computer),m_viewOnly(viewOnly),m_canToggle(canToggle)
{
	connect(computer.data(),&ComputerControlInterface::stateChanged,this,&RemoteAccessSession::changed);
	connect(computer.data(),&ComputerControlInterface::userChanged,this,&RemoteAccessSession::changed);
	connect(computer.data(),&ComputerControlInterface::screensChanged,this,&RemoteAccessSession::changed);
	if(!defaultWorkspace)defaultWorkspace=new RemoteAccessWorkspace(qApp);
	defaultWorkspace->add(this);
}
RemoteAccessSession::~RemoteAccessSession(){if(m_workspace)m_workspace->remove(this);}
QString RemoteAccessSession::title() const{return m_computer->computerName()+(m_computer->userLoginName().isEmpty()?QString{}:QStringLiteral(" · ")+m_computer->userLoginName());}
bool RemoteAccessSession::connected() const{return m_computer->state()==ComputerControlInterface::State::Connected;}
QString RemoteAccessSession::status() const
{
	if(connected())return tr("Connected");
	if(m_computer->state()==ComputerControlInterface::State::AccessControlFailed)return m_computer->accessControlDetails();
	if(m_computer->state()==ComputerControlInterface::State::AuthenticationFailed)return tr("Authentication failed");
	if(m_computer->state()==ComputerControlInterface::State::HostNameResolutionFailed)return tr("Hostname could not be resolved");
	return tr("Connecting to computer…");
}
void RemoteAccessSession::setViewOnly(bool value){if((m_canToggle||value==m_viewOnly)&&value!=m_viewOnly){m_viewOnly=value;Q_EMIT changed();}}
QVariantList RemoteAccessSession::screens() const
{
	QVariantList values{QVariantMap{{QStringLiteral("name"),tr("All screens")},{QStringLiteral("index"),-1}}};
	for(const auto& screen:m_computer->screens())values.append(QVariantMap{{QStringLiteral("name"),screen.name},{QStringLiteral("index"),screen.index}});
	return values;
}
QRect RemoteAccessSession::viewport() const
{
	// QScreen positions can be negative, while the VNC framebuffer starts at (0,0).
	QPoint minimumScreenPosition;
	for (const auto& screen : m_computer->screens()) {
		minimumScreenPosition.setX(qMin(minimumScreenPosition.x(), screen.geometry.x()));
		minimumScreenPosition.setY(qMin(minimumScreenPosition.y(), screen.geometry.y()));
	}
	for (const auto& screen : m_computer->screens())
		if (screen.index == m_screen) return screen.geometry.translated(-minimumScreenPosition);
	return {};
}
void RemoteAccessSession::selectScreen(int index){m_screen=index;Q_EMIT changed();}
void RemoteAccessSession::screenshot(){Screenshot{}.take(m_computer);}
void RemoteAccessSession::close(){if(m_workspace){auto workspace=m_workspace;m_workspace=nullptr;workspace->remove(this);}deleteLater();}
void RemoteAccessSession::showMaster()
{
	if(auto master=VeyonCore::instance()->findChild<VeyonMasterInterface*>())if(auto window=master->mainQmlWindow()){window->show();window->raise();window->requestActivate();}
}
