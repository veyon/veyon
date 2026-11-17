// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

#include "Configuration/JsonStore.h"
#include "LdapClient.h"
#include "LdapConfiguration.h"
#include "LdapNetworkObjectDirectory.h"
#include "VeyonConfiguration.h"

#include <atomic>
#include <future>
#include <mutex>
#include <thread>


// Minimal LDAP server (BER over TCP) in its own thread: accepts anonymous binds and answers
// searches according to the configured behaviour
class FakeLdapServer
{
public:
	enum class Behaviour {
		Success,
		ResultCode,
		EntriesThenResultCode,
		NoResponse,
		CloseConnection,
		Reference,
		Paged,
	};

	FakeLdapServer()
	{
		std::promise<quint16> portPromise;
		auto portFuture = portPromise.get_future();
		m_thread = std::thread([this, &portPromise]() { run(portPromise); });
		m_port = portFuture.get();
	}

	~FakeLdapServer()
	{
		m_stop = true;
		m_thread.join();
	}

	quint16 port() const
	{
		return m_port;
	}

	void setBehaviour(Behaviour behaviour, int resultCode = 0)
	{
		m_behaviour = behaviour;
		m_resultCode = resultCode;
	}

	int searches() const
	{
		return m_searches;
	}

	int abandons() const
	{
		return m_abandons;
	}

	int connections() const
	{
		return m_connections;
	}

	bool pageCookieSent() const
	{
		return m_pageCookieSent;
	}

	QByteArray referenceUrl() const
	{
		return "ldap://127.0.0.1:" + QByteArray::number(m_port) + "/DC=DomainDnsZones,dc=example,dc=org";
	}

private:
	static QByteArray length(int size)
	{
		if (size < 128)
		{
			return QByteArray(1, char(size));
		}
		if (size < 256)
		{
			return QByteArray("\x81") + char(size);
		}
		return QByteArray("\x82") + char(size >> 8) + char(size & 0xff);
	}

	static QByteArray tlv(char tag, const QByteArray& content)
	{
		return QByteArray(1, tag) + length(int(content.size())) + content;
	}

	static QByteArray integer(char tag, int value)
	{
		QByteArray bytes;
		do
		{
			bytes.prepend(char(value & 0xff));
			value >>= 8;
		} while (value > 0);
		if (quint8(bytes.at(0)) & 0x80)
		{
			bytes.prepend('\0');
		}
		return tlv(tag, bytes);
	}

	static QByteArray message(int messageId, const QByteArray& protocolOp)
	{
		return tlv(0x30, integer(0x02, messageId) + protocolOp);
	}

	static QByteArray result(char tag, int resultCode)
	{
		return tlv(tag, integer(0x0a, resultCode) + tlv(0x04, {}) + tlv(0x04, {}));
	}

	// SearchResultDone with a simple paged results control (RFC 2696)
	static QByteArray pagedResult(int messageId, const QByteArray& cookie)
	{
		const auto controlValue = tlv(0x30, integer(0x02, 0) + tlv(0x04, cookie));
		const auto control = tlv(0x30, tlv(0x04, QByteArrayLiteral("1.2.840.113556.1.4.319")) + tlv(0x04, controlValue));
		return tlv(0x30, integer(0x02, messageId) + result(0x65, 0) + tlv(char(0xa0), control));
	}

	static QByteArray entry(const QString& dn)
	{
		const auto attribute = tlv(0x30, tlv(0x04, QByteArrayLiteral("cn")) + tlv(0x31, tlv(0x04, QByteArrayLiteral("pc1"))));
		return tlv(0x64, tlv(0x04, dn.toUtf8()) + tlv(0x30, attribute));
	}

	// parses one complete LDAPMessage from the front of the buffer
	static bool takeMessage(QByteArray& buffer, int& messageId, quint8& operation, QByteArray& content)
	{
		if (buffer.size() < 2 || buffer.at(0) != 0x30)
		{
			return false;
		}
		int offset = 1;
		int size = quint8(buffer.at(offset++));
		if (size & 0x80)
		{
			const auto lengthBytes = size & 0x7f;
			if (buffer.size() < offset + lengthBytes)
			{
				return false;
			}
			size = 0;
			for (int index = 0; index < lengthBytes; ++index)
			{
				size = (size << 8) | quint8(buffer.at(offset++));
			}
		}
		if (buffer.size() < offset + size)
		{
			return false;
		}
		content = buffer.mid(offset, size);
		buffer.remove(0, offset + size);

		// INTEGER messageID, then the protocolOp tag
		const auto idSize = quint8(content.at(1));
		messageId = 0;
		for (int index = 0; index < idSize; ++index)
		{
			messageId = (messageId << 8) | quint8(content.at(2 + index));
		}
		operation = quint8(content.at(2 + idSize));
		return true;
	}

	void run(std::promise<quint16>& portPromise)
	{
		QTcpServer server;
		portPromise.set_value(server.listen(QHostAddress::LocalHost) ? server.serverPort() : 0);

		QList<QTcpSocket*> sockets;
		QHash<QTcpSocket*, QByteArray> buffers;

		while (m_stop == false)
		{
			if (server.waitForNewConnection(5))
			{
				sockets.append(server.nextPendingConnection());
				++m_connections;
			}

			for (auto socket : std::as_const(sockets))
			{
				if (socket->state() != QAbstractSocket::ConnectedState || socket->waitForReadyRead(5) == false)
				{
					continue;
				}
				auto& buffer = buffers[socket];
				buffer.append(socket->readAll());

				int messageId = 0;
				quint8 operation = 0;
				QByteArray content;
				while (takeMessage(buffer, messageId, operation, content))
				{
					if (operation == 0x63 && content.contains(QByteArrayLiteral("pagecookie")))
					{
						m_pageCookieSent = true;
					}
					handle(socket, messageId, operation);
				}
			}
		}

		qDeleteAll(sockets);
	}

	void handle(QTcpSocket* socket, int messageId, quint8 operation)
	{
		switch (operation)
		{
		case 0x60: // bind request
			socket->write(message(messageId, result(0x61, 0)));
			break;
		case 0x63: // search request
			++m_searches;
			switch (m_behaviour.load())
			{
			case Behaviour::Success:
				socket->write(message(messageId, entry(QStringLiteral("cn=pc1,ou=computers,dc=example,dc=org"))));
				socket->write(message(messageId, result(0x65, 0)));
				break;
			case Behaviour::ResultCode:
				socket->write(message(messageId, result(0x65, m_resultCode)));
				break;
			case Behaviour::EntriesThenResultCode:
				socket->write(message(messageId, entry(QStringLiteral("cn=pc1,ou=computers,dc=example,dc=org"))));
				socket->write(message(messageId, result(0x65, m_resultCode)));
				break;
			case Behaviour::NoResponse:
				break;
			case Behaviour::CloseConnection:
				socket->abort();
				return;
			case Behaviour::Reference:
				// like AD below the domain root: a chased reference would search again (answered
				// with operationsError)
				if (m_searches > 1)
				{
					socket->write(message(messageId, result(0x65, 1)));
					break;
				}
				socket->write(message(messageId, entry(QStringLiteral("cn=pc1,ou=computers,dc=example,dc=org"))));
				socket->write(message(messageId, tlv(0x73, tlv(0x04, referenceUrl()))));
				socket->write(message(messageId, result(0x65, 0)));
				break;
			case Behaviour::Paged:
				if (m_searches == 1)
				{
					socket->write(message(messageId, entry(QStringLiteral("cn=pc1,ou=computers,dc=example,dc=org"))));
					socket->write(pagedResult(messageId, QByteArrayLiteral("pagecookie")));
				}
				else
				{
					socket->write(message(messageId, entry(QStringLiteral("cn=pc2,ou=computers,dc=example,dc=org"))));
					socket->write(pagedResult(messageId, {}));
				}
				break;
			}
			break;
		case 0x50: // abandon request
			++m_abandons;
			break;
		default:
			break;
		}
		socket->flush();
	}

	std::thread m_thread;
	std::atomic<bool> m_stop{false};
	std::atomic<Behaviour> m_behaviour{Behaviour::Success};
	std::atomic<int> m_resultCode{0};
	std::atomic<int> m_searches{0};
	std::atomic<int> m_abandons{0};
	std::atomic<int> m_connections{0};
	std::atomic<bool> m_pageCookieSent{false};
	quint16 m_port = 0;
};



// WP9 review H1: an LDAP query only counts as successful if every search completed with
// LDAP_SUCCESS within the query timeout; the fail-closed queries return std::nullopt otherwise
class LdapClientQueryTest : public QObject
{
	Q_OBJECT
private:
	static constexpr int QueryTimeout = 500;

	struct Setup
	{
		QTemporaryDir directory;
		Configuration::JsonStore store{Configuration::Store::Scope::System, directory.filePath(QStringLiteral("c.json"))};
		VeyonConfiguration configuration{&store};
		LdapConfiguration ldapConfiguration{&configuration};

		explicit Setup(quint16 port)
		{
			ldapConfiguration.setServerHost(QStringLiteral("127.0.0.1"));
			ldapConfiguration.setServerPort(port);
			ldapConfiguration.setUseBindCredentials(false);
			ldapConfiguration.setQueryTimeout(QueryTimeout);
			ldapConfiguration.setBaseDn(QStringLiteral("dc=example,dc=org"));
			ldapConfiguration.setQueryNamingContext(false);
			ldapConfiguration.setComputerTree(QStringLiteral("ou=computers"));
			ldapConfiguration.setComputerHostNameAttribute(QStringLiteral("cn"));
			ldapConfiguration.setComputerHostNameAsFQDN(false);
		}
	};

	static QStringList query(LdapClient& client)
	{
		client.resetQueryStatus();
		return client.queryDistinguishedNames(QStringLiteral("ou=computers,dc=example,dc=org"), QStringLiteral("(cn=pc1)"),
											  LdapClient::Scope::Sub);
	}

private Q_SLOTS:
	void success()
	{
		FakeLdapServer server;
		Setup setup(server.port());
		LdapClient client(setup.ldapConfiguration);
		QVERIFY(client.isBound());

		QCOMPARE(query(client), QStringList{QStringLiteral("cn=pc1,ou=computers,dc=example,dc=org")});
		QVERIFY(client.hasQueryFailed() == false);
	}

	void resultCodes_data()
	{
		QTest::addColumn<int>("resultCode");
		QTest::addColumn<bool>("withEntries");

		QTest::newRow("sizeLimitExceeded") << 4 << true;
		QTest::newRow("timeLimitExceeded") << 3 << true;
		QTest::newRow("noSuchObject") << 32 << false;
		QTest::newRow("insufficientAccessRights") << 50 << false;
		QTest::newRow("busy") << 51 << false;
		QTest::newRow("unavailable") << 52 << false;
		QTest::newRow("unwillingToPerform") << 53 << false;
	}

	void resultCodes()
	{
		QFETCH(int, resultCode);
		QFETCH(bool, withEntries);

		FakeLdapServer server;
		server.setBehaviour(withEntries ? FakeLdapServer::Behaviour::EntriesThenResultCode : FakeLdapServer::Behaviour::ResultCode,
							resultCode);
		Setup setup(server.port());
		LdapClient client(setup.ldapConfiguration);

		const auto result = query(client);
		QVERIFY(client.hasQueryFailed());
		// partial results are still returned for listings
		QCOMPARE(result.isEmpty(), withEntries == false);
		// a completed search is not repeated
		QCOMPARE(server.searches(), 1);
	}

	void timeout()
	{
		FakeLdapServer server;
		server.setBehaviour(FakeLdapServer::Behaviour::NoResponse);
		Setup setup(server.port());
		LdapClient client(setup.ldapConfiguration);

		QElapsedTimer timer;
		timer.start();
		QVERIFY(query(client).isEmpty());
		QVERIFY(client.hasQueryFailed());
		QVERIFY(timer.elapsed() >= QueryTimeout);
		QVERIFY(timer.elapsed() < QueryTimeout * 4);
		// the timed out search is abandoned
		QTRY_COMPARE(server.abandons(), 1);

		// the status is reset per operation
		server.setBehaviour(FakeLdapServer::Behaviour::Success);
		QCOMPARE(query(client).size(), 1);
		QVERIFY(client.hasQueryFailed() == false);
	}

	// re-review N1: search references are not chased (no connection to server-named hosts, no
	// failure through AD's operationsError for the chased search)
	void searchReferenceNotChased()
	{
		FakeLdapServer server;
		server.setBehaviour(FakeLdapServer::Behaviour::Reference);
		Setup setup(server.port());
		LdapClient client(setup.ldapConfiguration);
		QVERIFY(client.isBound());

		QCOMPARE(query(client), QStringList{QStringLiteral("cn=pc1,ou=computers,dc=example,dc=org")});
		QVERIFY(client.hasQueryFailed() == false);
		QTest::qWait(100);
		QCOMPARE(server.searches(), 1);
		QCOMPARE(server.connections(), 1);
	}

	// re-review N5: simple paged results (AD returns at most MaxPageSize entries per search)
	void pagedResults()
	{
		FakeLdapServer server;
		server.setBehaviour(FakeLdapServer::Behaviour::Paged);
		Setup setup(server.port());
		LdapClient client(setup.ldapConfiguration);

		QCOMPARE(query(client), (QStringList{QStringLiteral("cn=pc1,ou=computers,dc=example,dc=org"), QStringLiteral("cn=pc2,ou=computers,dc=example,dc=org")}));
		QVERIFY(client.hasQueryFailed() == false);
		QCOMPARE(server.searches(), 2);
		QVERIFY(server.pageCookieSent());
	}

	// re-review N3: after a timeout the next query uses a new connection instead of waiting on a
	// possibly dead one again
	void reconnectAfterTimeout()
	{
		FakeLdapServer server;
		server.setBehaviour(FakeLdapServer::Behaviour::NoResponse);
		Setup setup(server.port());
		LdapClient client(setup.ldapConfiguration);
		QCOMPARE(server.connections(), 1);

		QVERIFY(query(client).isEmpty());
		QVERIFY(client.hasQueryFailed());

		server.setBehaviour(FakeLdapServer::Behaviour::Success);
		QCOMPARE(query(client).size(), 1);
		QTRY_COMPARE(server.connections(), 2);
	}

	void connectionLost()
	{
		FakeLdapServer server;
		server.setBehaviour(FakeLdapServer::Behaviour::CloseConnection);
		Setup setup(server.port());
		LdapClient client(setup.ldapConfiguration);

		QVERIFY(query(client).isEmpty());
		QVERIFY(client.hasQueryFailed());
		// one retry with a new connection, which fails again
		QTRY_COMPARE(server.searches(), 2);
	}

	void unreachableServer()
	{
		quint16 port = 0;
		{
			QTcpServer closedServer;
			QVERIFY(closedServer.listen(QHostAddress::LocalHost));
			port = closedServer.serverPort();
		}
		Setup setup(port);
		LdapClient client(setup.ldapConfiguration);
		QVERIFY(client.isBound() == false);
		QVERIFY(query(client).isEmpty());
		QVERIFY(client.hasQueryFailed());
	}

	// the fail-closed directory queries used for access control
	void directoryTryQueries()
	{
		FakeLdapServer server;
		Setup setup(server.port());
		LdapNetworkObjectDirectory directory(setup.ldapConfiguration, nullptr);
		directory.setHostNameResolutionEnabled(false);

		const auto hosts = directory.tryQueryObjects(NetworkObject::Type::Host, NetworkObject::Attribute::HostAddress,
													 QStringLiteral("pc1.example.org"));
		QVERIFY(hosts.has_value());
		QCOMPARE(hosts->size(), 1);
		QCOMPARE(hosts->first().hostAddress(), QStringLiteral("pc1"));
		// one search for all attributes (no search per computer)
		QCOMPARE(server.searches(), 1);

		// IP addresses are never resolved in the service
		const auto byAddress = directory.tryQueryObjects(NetworkObject::Type::Host, NetworkObject::Attribute::HostAddress,
														 QStringLiteral("10.1.2.3"));
		QVERIFY(byAddress.has_value() && byAddress->isEmpty());
		QCOMPARE(server.searches(), 1);

		for (const auto behaviour : {FakeLdapServer::Behaviour::ResultCode, FakeLdapServer::Behaviour::EntriesThenResultCode,
									 FakeLdapServer::Behaviour::NoResponse})
		{
			server.setBehaviour(behaviour, 51);
			QVERIFY(directory.tryQueryObjects(NetworkObject::Type::Host, NetworkObject::Attribute::HostAddress,
											  QStringLiteral("pc1.example.org"))
						.has_value() == false);
		}
	}
};


QTEST_GUILESS_MAIN(LdapClientQueryTest)
#include "LdapClientQueryTest.moc"
