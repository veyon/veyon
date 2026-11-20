// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

#include "Configuration/JsonStore.h"
#include "Configuration/SecretPropertyRegistry.h"
#include "ExternalVncServerConfiguration.h"
#include "LdapClient.h"
#include "LdapConfiguration.h"
#include "PlatformSecretFunctions.h"
#include "SecretStore.h"
#include "VeyonConfiguration.h"

#include <atomic>
#include <future>
#include <map>
#include <thread>

using Veyon::Crypto::SecureBuffer;

IMPLEMENT_CONFIG_PROXY(ExternalVncServerConfiguration)


class LdapTestSecretFunctions : public PlatformSecretFunctions
{
public:
	std::optional<SecureBuffer> readSecret(const QString& key) override
	{
		const auto it = m_secrets.find(key);
		return it == m_secrets.end() ? std::optional<SecureBuffer>() : it->second.clone();
	}
	bool writeSecret(const QString& key, const SecureBuffer& value) override
	{
		m_secrets[key] = value.clone();
		return true;
	}
	bool removeSecret(const QString& key) override
	{
		m_secrets.erase(key);
		return true;
	}
	QStringList secretKeys() override
	{
		return {};
	}
	bool prepareStorage() override
	{
		return true;
	}

	bool isAvailable() override
	{
		return true;
	}

	std::map<QString, SecureBuffer> m_secrets;
};


// M1 of the WP4 review: a configured bind DN without its password must never result in an
// unauthenticated simple bind (RFC 4513 §5.1.2) or an anonymous one
class LdapClientBindTest : public QObject
{
	Q_OBJECT
private:
	// TCP server in its own thread that counts and immediately closes connections, so that an
	// attempted bind fails quickly instead of waiting for an LDAP response
	class ConnectionCounter
	{
	public:
		ConnectionCounter()
		{
			std::promise<quint16> portPromise;
			auto portFuture = portPromise.get_future();
			m_thread = std::thread([this, &portPromise]() {
				QTcpServer server;
				portPromise.set_value(server.listen(QHostAddress::LocalHost) ? server.serverPort() : 0);
				while (m_stop == false)
				{
					if (server.waitForNewConnection(20))
					{
						auto socket = server.nextPendingConnection();
						++m_connections;
						socket->abort();
						delete socket;
					}
				}
			});
			m_port = portFuture.get();
		}

		~ConnectionCounter()
		{
			m_stop = true;
			m_thread.join();
		}

		quint16 port() const
		{
			return m_port;
		}

		int connections() const
		{
			return m_connections;
		}

	private:
		std::thread m_thread;
		std::atomic<bool> m_stop{false};
		std::atomic<int> m_connections{0};
		quint16 m_port = 0;
	};

	LdapTestSecretFunctions m_backend;

	void runClient(ConnectionCounter& counter, bool expectConnection)
	{
		QTemporaryDir directory;
		Configuration::JsonStore store(Configuration::Store::Scope::System, directory.filePath(QStringLiteral("c.json")));
		VeyonConfiguration configuration(&store);
		LdapConfiguration ldapConfiguration(&configuration);
		ldapConfiguration.setServerHost(QStringLiteral("127.0.0.1"));
		ldapConfiguration.setServerPort(counter.port());
		ldapConfiguration.setUseBindCredentials(true);
		ldapConfiguration.setBindDn(QStringLiteral("cn=veyon,dc=example,dc=org"));

		LdapClient client(ldapConfiguration);
		QVERIFY(client.isBound() == false);
		// queries must not reconnect without credentials either
		QVERIFY(client.queryNamingContexts().isEmpty());

		if (expectConnection)
		{
			QTRY_VERIFY(counter.connections() > 0);
		}
		else
		{
			QTest::qWait(100);
			QCOMPARE(counter.connections(), 0);
		}
	}

private Q_SLOTS:
	// the rows LdapPlugin::secretProperties() and ExternalVncServer::secretProperties() declare
	void initTestCase()
	{
		QVERIFY(Configuration::SecretPropertyRegistry::add({QStringLiteral("LDAP"), QStringLiteral("BindPassword"),
															QStringLiteral("ldap/bindPassword"), true},
														   QStringLiteral("LDAP")));
		QVERIFY(Configuration::SecretPropertyRegistry::add({QStringLiteral("ExternalVncServer"), QStringLiteral("Password"),
															QStringLiteral("vncserver-external/password"), true},
														   QStringLiteral("ExternalVncServer")));
	}

	void cleanupTestCase()
	{
		Configuration::SecretPropertyRegistry::resetForTests();
	}

	void init()
	{
		m_backend.m_secrets.clear();
		SecretStore::setTestBackend(&m_backend, VeyonCore::InitializationFlag::SecretStore);
	}

	void cleanup()
	{
		SecretStore::setTestBackend(nullptr, VeyonCore::InitializationFlags());
	}

	void missingBindPassword()
	{
		ConnectionCounter counter;
		QVERIFY(counter.port() != 0);
		runClient(counter, false);
	}

	void emptyBindPassword()
	{
		m_backend.m_secrets.emplace(QStringLiteral("ldap/bindPassword"), SecureBuffer());
		ConnectionCounter counter;
		runClient(counter, false);
	}

	void secretStoreUnavailable()
	{
		m_backend.m_secrets.emplace(QStringLiteral("ldap/bindPassword"), SecureBuffer::fromByteArray("secret"));
		SecretStore::setTestBackend(&m_backend, VeyonCore::defaultInitialization() | VeyonCore::InitializationFlag::ServiceIpc);
		ConnectionCounter counter;
		runClient(counter, false);
	}

	// the properties of the plugins and the rows they declare agree: every SecretReference property
	// (core and plugins with secrets) has a row in the registry with a matching default value and
	// vice versa (WP4 re-review N6)
	void registeredSecretProperties()
	{
		QTemporaryDir directory;
		Configuration::JsonStore store(Configuration::Store::Scope::System, directory.filePath(QStringLiteral("c.json")));
		VeyonConfiguration configuration(&store);
		LdapConfiguration ldapConfiguration(&configuration);
		ExternalVncServerConfiguration externalVncServerConfiguration(&configuration);

		QStringList foundSecretKeys;
		const auto properties = configuration.findChildren<Configuration::Property*>();
		for (const auto* property : properties)
		{
			if (property->isSecretReference() == false)
			{
				continue;
			}
			const auto secretProperty = Configuration::SecretPropertyRegistry::find(property->parentKey(), property->key());
			QVERIFY2(secretProperty.has_value(), qPrintable(property->absoluteKey()));
			QCOMPARE(property->defaultValue(), Configuration::SecretReference::defaultValue(secretProperty->secretKey));
			foundSecretKeys.append(secretProperty->secretKey);
		}

		for (const auto& secretProperty : Configuration::SecretPropertyRegistry::properties())
		{
			QVERIFY2(foundSecretKeys.contains(secretProperty.secretKey), qPrintable(secretProperty.secretKey));
		}
	}

	// WP9 review L2: a bind password missing at start (e.g. imported later) is read again after a
	// back-off instead of disabling the client for the lifetime of the process
	void bindPasswordAvailableLater()
	{
		ConnectionCounter counter;
		QTemporaryDir directory;
		Configuration::JsonStore store(Configuration::Store::Scope::System, directory.filePath(QStringLiteral("c.json")));
		VeyonConfiguration configuration(&store);
		LdapConfiguration ldapConfiguration(&configuration);
		ldapConfiguration.setServerHost(QStringLiteral("127.0.0.1"));
		ldapConfiguration.setServerPort(counter.port());
		ldapConfiguration.setUseBindCredentials(true);
		ldapConfiguration.setBindDn(QStringLiteral("cn=veyon,dc=example,dc=org"));

		LdapClient client(ldapConfiguration);
		client.setBindCredentialsRetryInterval(50);
		QVERIFY(client.queryNamingContexts().isEmpty());
		QVERIFY(client.hasQueryFailed());

		m_backend.m_secrets.emplace(QStringLiteral("ldap/bindPassword"), SecureBuffer::fromByteArray("secret"));

		// not before the back-off has passed
		QVERIFY(client.queryNamingContexts().isEmpty());
		QTest::qWait(100);
		QCOMPARE(counter.connections(), 0);

		QTest::qWait(100);
		(void)client.queryNamingContexts();
		QTRY_VERIFY(counter.connections() > 0);
	}

	// positive control: with a password the client connects
	void bindPasswordAvailable()
	{
		m_backend.m_secrets.emplace(QStringLiteral("ldap/bindPassword"), SecureBuffer::fromByteArray("secret"));
		ConnectionCounter counter;
		runClient(counter, true);
	}
};


QTEST_GUILESS_MAIN(LdapClientBindTest)
#include "LdapClientBindTest.moc"
