// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QJsonArray>
#include <QtTest>

#include "AccessControlProvider.h"
#include "DesktopAccessDialog.h"
#include "FeatureWorkerManager.h"
#include "ServerAccessControlManager.h"
#include "VeyonServerInterface.h"

#include <cstdlib>
#include <memory>


class NullServer : public VeyonServerInterface
{
public:
	NullServer() :
		VeyonServerInterface(nullptr)
	{
	}

	FeatureWorkerManager& featureWorkerManager() override
	{
		std::abort();
	}

	bool sendFeatureMessageReply(const MessageContext& context, const FeatureMessage& reply) override
	{
		Q_UNUSED(context)
		Q_UNUSED(reply)
		return false;
	}

	int vncServerBasePort() const override
	{
		return 0;
	}

	QString vncServerLocalSocketPath() const override
	{
		return {};
	}

	void setMinimumFramebufferUpdateInterval(const MessageContext& context, int interval) override
	{
		Q_UNUSED(context)
		Q_UNUSED(interval)
	}

	const VncServerClient* vncServerClient(const MessageContext& context) const override
	{
		Q_UNUSED(context)
		return nullptr;
	}
};



// access decisions per user: allowed, denied, directory unavailable, or allowed only while another
// user is connected (like AccessControlRule::Condition::AccessFromAlreadyConnectedUser)
class ScriptedAccessControlProvider : public AccessControlProvider
{
public:
	enum class Decision {
		Allow,
		Deny,
		DirectoryUnavailable,
		AllowIfConnected,
	};

	ScriptedAccessControlProvider() :
		AccessControlProvider(nullptr, nullptr, QJsonArray{})
	{
	}

	CheckResult checkAccess(const QString& accessingUser, const QString& accessingComputer,
							const QStringList& connectedUsers) override
	{
		Q_UNUSED(accessingComputer)
		++checks[accessingUser];

		switch (decisions.value(accessingUser, Decision::Deny))
		{
		case Decision::Allow:
			return {Access::Allow};
		case Decision::DirectoryUnavailable:
			return {Access::Deny, Reason::DirectoryUnavailable};
		case Decision::AllowIfConnected:
			if (connectedUsers.contains(requiredConnectedUser))
			{
				return {Access::Allow};
			}
			break;
		case Decision::Deny:
			break;
		}

		return {Access::Deny, Reason::NoAccessControlRuleMatched};
	}

	QMap<QString, Decision> decisions;
	QString requiredConnectedUser;
	QMap<QString, int> checks;
};



// grace period and re-check wiring of the session server (04-directory-broker.md §2, WP9 review P1,
// re-review N2)
class ServerAccessControlManagerTest : public QObject
{
	Q_OBJECT
private:
	using Decision = ScriptedAccessControlProvider::Decision;

	qint64 m_now = 100000;
	NullServer m_server;

	std::unique_ptr<VncServerClient> client(const QString& username)
	{
		auto vncServerClient = std::make_unique<VncServerClient>();
		vncServerClient->setUsername(username);
		vncServerClient->setHostAddress(QStringLiteral("10.0.0.1"));
		vncServerClient->setAuthType(RfbVeyonAuth::KeyFile);
		vncServerClient->setProtocolState(VncServerProtocol::State::AccessControl);
		return vncServerClient;
	}

	static bool isClosed(const std::unique_ptr<VncServerClient>& client)
	{
		return client->protocolState() == VncServerProtocol::State::Close;
	}

private Q_SLOTS:
	void init()
	{
		m_now = 100000;
	}

	// only the departure of an authorised client triggers re-checks
	void recheckOnAuthorisedDeparture()
	{
		FeatureWorkerManager featureWorkerManager(m_server);
		DesktopAccessDialog desktopAccessDialog;
		ScriptedAccessControlProvider provider;
		ServerAccessControlManager manager(featureWorkerManager, desktopAccessDialog, provider, nullptr,
										   [this]() { return m_now; });

		provider.decisions = {{QStringLiteral("teacher"), Decision::Allow}, {QStringLiteral("other"), Decision::Allow}};
		auto teacher = client(QStringLiteral("teacher"));
		auto other = client(QStringLiteral("other"));
		auto stranger = client(QStringLiteral("stranger"));
		manager.addClient(teacher.get());
		manager.addClient(other.get());
		manager.addClient(stranger.get());
		QCOMPARE(stranger->accessControlState(), VncServerClient::AccessControlState::Failed);
		QCOMPARE(provider.checks.value(QStringLiteral("teacher")), 1);

		// a never authorised connection leaving does not re-check the others
		manager.removeClient(stranger.get());
		QCOMPARE(provider.checks.value(QStringLiteral("teacher")), 1);

		manager.removeClient(other.get());
		QCOMPARE(provider.checks.value(QStringLiteral("teacher")), 2);
		QVERIFY(isClosed(teacher) == false);
	}

	// already authorised connections keep access during a directory outage, are re-checked
	// periodically and closed after the grace period; new connections are denied
	void gracePeriodWiring()
	{
		FeatureWorkerManager featureWorkerManager(m_server);
		DesktopAccessDialog desktopAccessDialog;
		ScriptedAccessControlProvider provider;
		ServerAccessControlManager manager(featureWorkerManager, desktopAccessDialog, provider, nullptr,
										   [this]() { return m_now; });
		manager.setGraceRecheckInterval(20);

		provider.decisions = {{QStringLiteral("teacher"), Decision::Allow}, {QStringLiteral("leaving"), Decision::Allow}};
		auto teacher = client(QStringLiteral("teacher"));
		auto leaving = client(QStringLiteral("leaving"));
		manager.addClient(teacher.get());
		manager.addClient(leaving.get());

		provider.decisions[QStringLiteral("teacher")] = Decision::DirectoryUnavailable;
		manager.removeClient(leaving.get());
		QCOMPARE(teacher->accessControlState(), VncServerClient::AccessControlState::Successful);
		QVERIFY(isClosed(teacher) == false);
		QVERIFY(manager.isGraceRecheckActive());

		// a new connection is denied
		auto newcomer = client(QStringLiteral("teacher"));
		manager.addClient(newcomer.get());
		QCOMPARE(newcomer->accessControlState(), VncServerClient::AccessControlState::Failed);
		manager.removeClient(newcomer.get());

		// periodic re-checks while in grace
		const auto checks = provider.checks.value(QStringLiteral("teacher"));
		QTRY_VERIFY(provider.checks.value(QStringLiteral("teacher")) > checks + 1);
		QVERIFY(isClosed(teacher) == false);

		// the directory is back: the grace state ends
		provider.decisions[QStringLiteral("teacher")] = Decision::Allow;
		QTRY_VERIFY(manager.isGraceRecheckActive() == false);
		QVERIFY(isClosed(teacher) == false);

		// outage again, now longer than the grace period
		provider.decisions[QStringLiteral("teacher")] = Decision::DirectoryUnavailable;
		auto second = client(QStringLiteral("second"));
		provider.decisions[QStringLiteral("second")] = Decision::Allow;
		manager.addClient(second.get());
		manager.removeClient(second.get());
		QVERIFY(isClosed(teacher) == false);
		m_now += AccessControlGracePeriod::GracePeriod;
		QTRY_VERIFY(isClosed(teacher));
		QCOMPARE(teacher->accessControlState(), VncServerClient::AccessControlState::Failed);
	}

	// re-review N2: a client closed by the grace period expiry was the reason for the access of
	// another client, which has to be re-checked then
	void closedClientTriggersRecheck()
	{
		FeatureWorkerManager featureWorkerManager(m_server);
		DesktopAccessDialog desktopAccessDialog;
		ScriptedAccessControlProvider provider;
		ServerAccessControlManager manager(featureWorkerManager, desktopAccessDialog, provider, nullptr,
										   [this]() { return m_now; });
		manager.setGraceRecheckInterval(20);

		provider.requiredConnectedUser = QStringLiteral("teacher");
		provider.decisions = {{QStringLiteral("teacher"), Decision::Allow},
							  {QStringLiteral("assistant"), Decision::AllowIfConnected},
							  {QStringLiteral("leaving"), Decision::Allow}};
		auto teacher = client(QStringLiteral("teacher"));
		auto assistant = client(QStringLiteral("assistant"));
		auto leaving = client(QStringLiteral("leaving"));
		manager.addClient(teacher.get());
		manager.addClient(assistant.get());
		manager.addClient(leaving.get());
		QCOMPARE(assistant->accessControlState(), VncServerClient::AccessControlState::Successful);

		provider.decisions[QStringLiteral("teacher")] = Decision::DirectoryUnavailable;
		manager.removeClient(leaving.get());
		QVERIFY(isClosed(teacher) == false);
		QVERIFY(isClosed(assistant) == false);

		m_now += AccessControlGracePeriod::GracePeriod;
		QTRY_VERIFY(isClosed(teacher));
		QVERIFY(isClosed(assistant));
		QCOMPARE(assistant->accessControlState(), VncServerClient::AccessControlState::Failed);
	}
};


QTEST_GUILESS_MAIN(ServerAccessControlManagerTest)
#include "ServerAccessControlManagerTest.moc"
