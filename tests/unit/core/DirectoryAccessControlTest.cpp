// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QJsonArray>
#include <QTemporaryDir>
#include <QtTest>

#include "AccessControlGracePeriod.h"
#include "AccessControlProvider.h"
#include "Configuration/JsonStore.h"
#include "ConfigurationManager.h"
#include "UserGroupsBackendInterface.h"
#include "VeyonConfiguration.h"


// user groups backend whose availability can be switched, counting the queries
class SwitchableUserGroupsBackend : public UserGroupsBackendInterface
{
public:
	QString userGroupsBackendName() const override
	{
		return QStringLiteral("test");
	}

	void reloadConfiguration() override
	{
	}

	QStringList userGroups(bool queryDomainGroups) override
	{
		Q_UNUSED(queryDomainGroups)
		return {};
	}

	QStringList groupsOfUser(const QString& username, bool queryDomainGroups) override
	{
		return tryQueryGroupsOfUser(username, queryDomainGroups).value_or(QStringList{});
	}

	std::optional<QStringList> tryQueryGroupsOfUser(const QString& username, bool queryDomainGroups) override
	{
		Q_UNUSED(queryDomainGroups)
		++queries;
		if (available == false)
		{
			return {};
		}
		return groups.value(username);
	}

	QString userGroupSecurityIdentifier(const QString& groupName) override
	{
		Q_UNUSED(groupName)
		return {};
	}

	bool available = true;
	int queries = 0;
	QMap<QString, QStringList> groups;
};



// Fail-closed access control decisions on directory failures (04-directory-broker.md §2, WP9
// review P1, H3, P3)
class DirectoryAccessControlTest : public QObject
{
	Q_OBJECT
private:
	qint64 m_now = 100000;

	static QJsonObject memberRule(const QString& name, AccessControlRule::Action action, AccessControlRule::Subject subject,
								  const QString& group, bool inverted = false)
	{
		AccessControlRule rule;
		rule.setName(name);
		rule.setAction(action);
		rule.setConditionEnabled(AccessControlRule::Condition::MemberOfUserGroup, true);
		rule.setSubject(AccessControlRule::Condition::MemberOfUserGroup, subject);
		rule.setArgument(AccessControlRule::Condition::MemberOfUserGroup, group);
		rule.setConditionsInverted(inverted);
		return rule.toJson();
	}

	AccessControlDirectoryQueries::Clock clock()
	{
		return [this]() { return m_now; };
	}

private Q_SLOTS:
	void init()
	{
		m_now = 100000;
	}

	// an inverted condition ("not a member of") must not allow access on missing data, and rule
	// processing stops at the first failed query
	void rulesStopAtFirstFailure()
	{
		SwitchableUserGroupsBackend backend;
		backend.available = false;
		const QJsonArray rules{
			memberRule(QStringLiteral("no students"), AccessControlRule::Action::Allow, AccessControlRule::Subject::AccessingUser,
					   QStringLiteral("students"), true),
			memberRule(QStringLiteral("staff"), AccessControlRule::Action::Allow, AccessControlRule::Subject::LocalUser,
					   QStringLiteral("staff")),
		};
		AccessControlProvider provider(&backend, nullptr, rules, clock());

		const auto rule = provider.processAccessControlRules(QStringLiteral("student"), QStringLiteral("pc1"),
															 QStringLiteral("teacher"), QStringLiteral("pc2"), {});
		QVERIFY(rule.isNull());
		QVERIFY(provider.hasDirectoryQueryFailed());
		QCOMPARE(backend.queries, 1);

		// positive control
		backend.available = true;
		m_now += AccessControlDirectoryQueries::FailureBackoff;
		backend.groups[QStringLiteral("student")] = QStringList{QStringLiteral("students")};
		backend.groups[QStringLiteral("teacher")] = QStringList{QStringLiteral("staff")};
		const auto allowingRule = provider.processAccessControlRules(QStringLiteral("student"), QStringLiteral("pc1"),
																	 QStringLiteral("teacher"), QStringLiteral("pc2"), {});
		QVERIFY(allowingRule.isNull() == false);
		QCOMPARE(allowingRule->name(), QStringLiteral("staff"));
		QVERIFY(provider.hasDirectoryQueryFailed() == false);
	}

	// repeated failures do not each wait for the directory (circuit breaker), but still deny
	void circuitBreaker()
	{
		SwitchableUserGroupsBackend backend;
		backend.available = false;
		const QJsonArray rules{memberRule(QStringLiteral("teachers"), AccessControlRule::Action::Allow,
										  AccessControlRule::Subject::AccessingUser, QStringLiteral("teachers"))};
		AccessControlProvider provider(&backend, nullptr, rules, clock());

		for (int index = 0; index < 5; ++index)
		{
			QVERIFY(provider.processAccessControlRules(QStringLiteral("teacher%1").arg(index), {}, {}, {}, {}).isNull());
			QVERIFY(provider.hasDirectoryQueryFailed());
			m_now += 100;
		}
		QCOMPARE(backend.queries, 1);

		// tried again after the back-off
		m_now += AccessControlDirectoryQueries::FailureBackoff;
		backend.available = true;
		backend.groups[QStringLiteral("teacher")] = QStringList{QStringLiteral("teachers")};
		QVERIFY(provider.processAccessControlRules(QStringLiteral("teacher"), {}, {}, {}, {}).isNull() == false);
		QCOMPARE(backend.queries, 2);
	}

	// within one check pass every user's groups are queried once
	void memoizationPerPass()
	{
		SwitchableUserGroupsBackend backend;
		backend.groups[QStringLiteral("teacher")] = QStringList{QStringLiteral("teachers")};
		const QJsonArray rules{
			memberRule(QStringLiteral("admins"), AccessControlRule::Action::Allow, AccessControlRule::Subject::AccessingUser,
					   QStringLiteral("admins")),
			memberRule(QStringLiteral("teachers"), AccessControlRule::Action::Allow, AccessControlRule::Subject::AccessingUser,
					   QStringLiteral("teachers")),
		};
		AccessControlProvider provider(&backend, nullptr, rules, clock());

		provider.beginCheckPass();
		for (int index = 0; index < 3; ++index)
		{
			QVERIFY(provider.processAccessControlRules(QStringLiteral("teacher"), {}, {}, {}, {}).isNull() == false);
		}
		provider.endCheckPass();
		QCOMPARE(backend.queries, 1);

		// no memoization across passes (group memberships are never cached)
		QVERIFY(provider.processAccessControlRules(QStringLiteral("teacher"), {}, {}, {}, {}).isNull() == false);
		QCOMPARE(backend.queries, 3);
	}

	// H3: the listen address optimisation must not lock out all masters after a transient outage
	void localStateOnFailure()
	{
		SwitchableUserGroupsBackend backend;
		const QJsonArray rules{memberRule(QStringLiteral("deny if not teacher"), AccessControlRule::Action::Deny,
										  AccessControlRule::Subject::LocalUser, QStringLiteral("teachers"), true)};
		AccessControlProvider provider(&backend, nullptr, rules, clock());

		QVERIFY(provider.isAccessDeniedForLocalState(QStringLiteral("student"), QStringLiteral("pc1")));

		backend.available = false;
		m_now += AccessControlDirectoryQueries::FailureBackoff;
		QVERIFY(provider.isAccessDeniedForLocalState(QStringLiteral("student"), QStringLiteral("pc1")) == false);
	}

	// P1: already authorised connections keep their access for 120 s while the directory is down
	void gracePeriod()
	{
		AccessControlGracePeriod gracePeriod([this]() { return m_now; });
		int client = 0;
		int otherClient = 0;

		QVERIFY(gracePeriod.contains(&client) == false);
		QVERIFY(gracePeriod.keepAccess(&client, QStringLiteral("teacher@pc1")));
		QVERIFY(gracePeriod.contains(&client));

		m_now += AccessControlGracePeriod::GracePeriod - 1;
		QVERIFY(gracePeriod.keepAccess(&client, QStringLiteral("teacher@pc1")));
		QVERIFY(gracePeriod.keepAccess(&otherClient, QStringLiteral("teacher@pc2")));

		m_now += 1;
		QVERIFY(gracePeriod.keepAccess(&client, QStringLiteral("teacher@pc1")) == false);
		QVERIFY(gracePeriod.contains(&client) == false);
		QVERIFY(gracePeriod.contains(&otherClient));

		// a successful check ends the grace state, a later failure starts a new one
		gracePeriod.remove(&otherClient, QStringLiteral("teacher@pc2"));
		QVERIFY(gracePeriod.isEmpty());
		m_now += AccessControlGracePeriod::GracePeriod;
		QVERIFY(gracePeriod.keepAccess(&otherClient, QStringLiteral("teacher@pc2")));
	}

	// P3: a central directory broker host is refused when saving the configuration
	void centralBrokerHostRefused()
	{
		QTemporaryDir directory;
		Configuration::JsonStore store(Configuration::Store::Scope::System, directory.filePath(QStringLiteral("c.json")));
		VeyonConfiguration configuration(&store);

		QVERIFY(ConfigurationManager::validateConfiguration(configuration).isEmpty());

		for (const auto& host : {QStringLiteral("127.0.0.1"), QStringLiteral("::1"), QStringLiteral("LOCALHOST")})
		{
			configuration.setDirectoryBrokerHost(host);
			QVERIFY(ConfigurationManager::validateConfiguration(configuration).isEmpty());
		}

		for (const auto& host : {QStringLiteral("broker.example.org"), QStringLiteral("10.1.2.3"), QStringLiteral("localhost.example.org")})
		{
			configuration.setDirectoryBrokerHost(host);
			QVERIFY(ConfigurationManager::validateConfiguration(configuration).contains(host));
		}
	}
};


QTEST_GUILESS_MAIN(DirectoryAccessControlTest)
#include "DirectoryAccessControlTest.moc"
