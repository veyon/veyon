// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QBuffer>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QJsonDocument>
#include <QSet>
#include <QtTest>

#include "DirectoryBrokerClient.h"
#include "DirectoryBrokerFeaturePlugin.h"
#include "DirectoryBrokerLimits.h"
#include "DirectoryBrokerMessage.h"
#include "DirectoryBrokerPendingRequests.h"
#include "DirectoryBrokerService.h"
#include "DummyDirectoryPlugin.h"
#include "FeatureMessage.h"
#include "NetworkObjectDirectoryManager.h"
#include "RemoteNetworkObjectDirectory.h"
#include "RemoteUserGroupsBackend.h"
#include "VeyonServerInterface.h"
#include "VncServerClient.h"

#include <cstdlib>
#include <functional>
#include <memory>

using namespace DirectoryBrokerProtocol;
using Request = DirectoryBrokerMessage::Request;
using Reply = DirectoryBrokerMessage::Reply;


// broker client for the remote classes: either answered by a DirectoryBrokerService in this process
// (same code path as over IPC) or permanently unavailable
class TestBrokerClient : public QObject, public DirectoryBrokerClient
{
public:
	explicit TestBrokerClient(DirectoryBrokerService* service) :
		m_service(service)
	{
	}

	Reply request(const QString& command, const QVariantMap& arguments) override
	{
		++requestCount;

		std::optional<Reply> reply;
		requestAsync(command, arguments, [&reply](const Reply& result) { reply = result; });
		(void)QTest::qWaitFor([&reply]() { return reply.has_value(); }, DirectoryBrokerLimits::RequestTimeout);

		return reply.value_or(DirectoryBrokerMessage::error(0, Error::Timeout));
	}

	void requestAsync(const QString& command, const QVariantMap& arguments, const ReplyHandler& handler) override
	{
		const auto requestId = m_nextRequestId++;

		if (m_service == nullptr || available == false)
		{
			QMetaObject::invokeMethod(this, [handler, requestId]() {
				handler(DirectoryBrokerMessage::error(requestId, Error::Unavailable, QStringLiteral("broker down")));
			}, Qt::QueuedConnection);
			return;
		}

		// encode and decode as over IPC
		const auto ipcRequest = IpcMessage::decodeRequest(
			IpcMessage::encodeRequest(DirectoryBrokerMessage::toIpcRequest(int(requestId), command, arguments)));
		m_service->handleRequest(QStringLiteral("test"), DirectoryBrokerMessage::fromIpcRequest(*ipcRequest),
								 [handler, requestId, command](const Reply& reply) {
			const auto response = IpcMessage::decodeResponse(IpcMessage::encodeResponse(DirectoryBrokerMessage::toIpcResponse(int(requestId), reply)));
			handler(DirectoryBrokerMessage::fromIpcResponse(requestId, command, *response));
		});
	}

	bool available = true;
	int requestCount = 0;

private:
	DirectoryBrokerService* m_service;
	quint32 m_nextRequestId = 1;
};



class TestServer : public VeyonServerInterface
{
public:
	TestServer() :
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
		// as received by the master
		FeatureMessage receivedReply;
		if (receivedReply.fromByteArray(reply.toByteArray()) == false)
		{
			return false;
		}
		const auto decodedReply = DirectoryBrokerMessage::replyFromFeatureMessage(receivedReply);
		if (decodedReply.has_value())
		{
			replies.append(*decodedReply);
		}
		return true;
	}

	const VncServerClient* vncServerClient(const MessageContext& context) const override
	{
		Q_UNUSED(context)
		return &client;
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

	VncServerClient client;
	QList<Reply> replies;
};



// Directory and user-groups broker (docs/specs/04-directory-broker.md §7)
class DirectoryBrokerTest : public QObject
{
	Q_OBJECT
private:
	qint64 m_now = 1000;

	std::unique_ptr<DirectoryBrokerService> createService(DummyDirectoryPlugin& plugin, DummyNetworkObjectDirectory** directory = nullptr)
	{
		return std::make_unique<DirectoryBrokerService>(
			[&plugin, directory]() {
			auto createdDirectory = plugin.createNetworkObjectDirectory(nullptr);
			if (directory)
			{
				*directory = static_cast<DummyNetworkObjectDirectory*>(createdDirectory);
			}
			return createdDirectory;
		},
			[&plugin]() { return &plugin; },
			[this]() { return m_now; });
	}

	static Reply call(DirectoryBrokerService& service, const QString& command, const QVariantMap& arguments = {},
					  const QString& clientId = QStringLiteral("session1"))
	{
		static quint32 requestId = 0;
		std::optional<Reply> reply;
		int calls = 0;
		service.handleRequest(clientId, {++requestId, command, arguments}, [&reply, &calls](const Reply& result) {
			reply = result;
			++calls;
		});
		(void)QTest::qWaitFor([&reply]() { return reply.has_value(); }, 5000);
		// exactly once
		QTest::qWait(1);
		if (calls != 1)
		{
			return DirectoryBrokerMessage::error(0, QLatin1String("handler-called-more-than-once"));
		}
		return reply.value_or(DirectoryBrokerMessage::error(0, QLatin1String("no-reply")));
	}

	static int jsonSize(const Reply& reply)
	{
		return int(IpcMessage::encodeResponse(DirectoryBrokerMessage::toIpcResponse(1, reply)).size());
	}

	static Request request(const QString& command, const QVariantMap& arguments)
	{
		return {1, command, arguments};
	}

	static QString longString(int bytes)
	{
		return QString(bytes, QLatin1Char('x'));
	}

private Q_SLOTS:
	void init()
	{
		m_now = 1000;
	}

	void featureMessageEncoding()
	{
		const Request request{42, QString(Command::DirectoryQueryObjects), {{Key::Type, QStringLiteral("Host")}, {Key::Attribute, QStringLiteral("HostAddress")}, {Key::Value, QStringLiteral("pc1.example.org")}}};

		FeatureMessage message{DirectoryBrokerMessage::featureUid()};
		DirectoryBrokerMessage::toFeatureMessage(request, message);

		FeatureMessage received;
		QVERIFY(received.fromByteArray(message.toByteArray()));
		const auto decoded = DirectoryBrokerMessage::requestFromFeatureMessage(received);
		QVERIFY(decoded.has_value());
		QCOMPARE(decoded->requestId, 42u);
		QCOMPARE(decoded->command, request.command);
		QCOMPARE(decoded->arguments, request.arguments);
		QVERIFY(DirectoryBrokerMessage::validateRequest(*decoded).has_value() == false);

		// other features and malformed envelopes are not broker messages
		FeatureMessage otherFeature{Feature::Uid::createUuid()};
		DirectoryBrokerMessage::toFeatureMessage(request, otherFeature);
		QVERIFY(DirectoryBrokerMessage::requestFromFeatureMessage(otherFeature).has_value() == false);

		FeatureMessage numericRequestId{DirectoryBrokerMessage::featureUid()};
		numericRequestId.addArgument(DirectoryBrokerMessage::Argument::RequestId, QStringLiteral("42"))
			.addArgument(DirectoryBrokerMessage::Argument::Command, request.command)
			.addArgument(DirectoryBrokerMessage::Argument::Arguments, request.arguments);
		QVERIFY(DirectoryBrokerMessage::requestFromFeatureMessage(numericRequestId).has_value() == false);

		// oversized and too deeply nested arguments are not even parsed
		const auto rawRequest = [&request](const QByteArray& arguments) {
			FeatureMessage rawMessage{DirectoryBrokerMessage::featureUid()};
			rawMessage.addArgument(DirectoryBrokerMessage::Argument::RequestId, qint64(1))
				.addArgument(DirectoryBrokerMessage::Argument::Command, request.command)
				.addArgument(DirectoryBrokerMessage::Argument::Arguments, arguments);
			return DirectoryBrokerMessage::requestFromFeatureMessage(rawMessage);
		};
		QVERIFY(rawRequest(QByteArrayLiteral("{}")).has_value());
		QVERIFY(rawRequest(QByteArray("{\"Value\":\"") + QByteArray(DirectoryBrokerLimits::MaximumRequestSize, 'x') + QByteArray("\"}")).has_value() == false);
		QVERIFY(rawRequest(QByteArray("{\"a\":") + QByteArray(20, '[') + QByteArray(20, ']') + QByteArray("}")).has_value() == false);
		QVERIFY(rawRequest(QByteArrayLiteral("[]")).has_value() == false);
		QVERIFY(rawRequest(QByteArrayLiteral("{")).has_value() == false);

		// replies echo the RequestId
		FeatureMessage replyMessage{DirectoryBrokerMessage::featureUid()};
		DirectoryBrokerMessage::toFeatureMessage(DirectoryBrokerMessage::error(42, Error::Denied, QStringLiteral("no")), replyMessage);
		const auto reply = DirectoryBrokerMessage::replyFromFeatureMessage(replyMessage);
		QVERIFY(reply.has_value());
		QCOMPARE(reply->requestId, 42u);
		QVERIFY(reply->isError());
		QCOMPARE(reply->errorCode(), QString(Error::Denied));
	}

	void ipcEncoding()
	{
		const auto object = DummyNetworkObjectDirectory::host(1, 2);
		const Reply reply{7, QString(Command::DirectoryQueryObjects), {{Key::Objects, QVariantList{DirectoryBrokerMessage::objectToVariant(object)}}, {Key::NextCursor, QString()}}};

		const auto response = IpcMessage::decodeResponse(IpcMessage::encodeResponse(DirectoryBrokerMessage::toIpcResponse(7, reply)));
		QVERIFY(response.has_value());
		const auto decoded = DirectoryBrokerMessage::fromIpcResponse(7, reply.command, *response);
		QVERIFY(decoded.isError() == false);

		const auto objects = decoded.arguments.value(Key::Objects).toList();
		QCOMPARE(objects.size(), 1);
		const auto decodedObject = DirectoryBrokerMessage::objectFromVariant(objects.first());
		QVERIFY(decodedObject.has_value());
		QVERIFY(decodedObject->exactMatch(object));
		QCOMPARE(decodedObject->macAddress(), object.macAddress());

		// broker errors are passed on, other and missing error codes become "unavailable"
		const auto denied = DirectoryBrokerMessage::fromIpcResponse(1, reply.command, {1, false, QStringLiteral("denied"), {}});
		QCOMPARE(denied.errorCode(), QString(Error::Denied));
		const auto unknown = DirectoryBrokerMessage::fromIpcResponse(1, reply.command, {1, false, QStringLiteral("unknown-command"), {}});
		QCOMPARE(unknown.errorCode(), QString(Error::Unavailable));
		const auto failed = DirectoryBrokerMessage::fromIpcResponse(1, reply.command, {});
		QCOMPARE(failed.errorCode(), QString(Error::Unavailable));
	}

	void replyObjectValidation()
	{
		auto map = DirectoryBrokerMessage::objectToVariant(DummyNetworkObjectDirectory::host(0, 0)).toMap();
		QVERIFY(DirectoryBrokerMessage::objectFromVariant(map).has_value());

		auto rootType = map;
		rootType[QStringLiteral("Type")] = int(NetworkObject::Type::Root);
		QVERIFY(DirectoryBrokerMessage::objectFromVariant(rootType).has_value() == false);

		auto badType = map;
		badType[QStringLiteral("Type")] = 3.5;
		QVERIFY(DirectoryBrokerMessage::objectFromVariant(badType).has_value() == false);
		badType[QStringLiteral("Type")] = QStringLiteral("3");
		QVERIFY(DirectoryBrokerMessage::objectFromVariant(badType).has_value() == false);
		badType[QStringLiteral("Type")] = 1e300;
		QVERIFY(DirectoryBrokerMessage::objectFromVariant(badType).has_value() == false);

		auto noUid = map;
		noUid.remove(QStringLiteral("Uid"));
		QVERIFY(DirectoryBrokerMessage::objectFromVariant(noUid).has_value() == false);

		auto hugeName = map;
		hugeName[QStringLiteral("Name")] = longString(DirectoryBrokerLimits::MaximumReplyStringSize + 1);
		QVERIFY(DirectoryBrokerMessage::objectFromVariant(hugeName).has_value() == false);

		auto nested = map;
		nested[QStringLiteral("Name")] = QVariantMap{{QStringLiteral("a"), 1}};
		QVERIFY(DirectoryBrokerMessage::objectFromVariant(nested).has_value() == false);
	}

	void requestValidation_data()
	{
		QTest::addColumn<QString>("command");
		QTest::addColumn<QVariantMap>("arguments");
		QTest::addColumn<QString>("error");

		const auto uid = DummyNetworkObjectDirectory::location(0).uid().toString();

		QTest::newRow("valid update") << QString(Command::DirectoryUpdate) << QVariantMap{} << QString();
		QTest::newRow("valid query") << QString(Command::DirectoryQueryObjects)
									 << QVariantMap{{Key::Type, QStringLiteral("Host")}, {Key::Attribute, QStringLiteral("None")}} << QString();
		QTest::newRow("valid groups") << QString(Command::UserGroupsGroupsOfUser)
									  << QVariantMap{{Key::Username, QStringLiteral("teacher")}, {Key::QueryDomainGroups, true}} << QString();
		QTest::newRow("unknown command") << QStringLiteral("Directory.Delete") << QVariantMap{} << QString(Error::BadRequest);
		QTest::newRow("error as command") << QString(Command::Error) << QVariantMap{} << QString(Error::BadRequest);
		QTest::newRow("unknown type name") << QString(Command::DirectoryQueryObjects)
										   << QVariantMap{{Key::Type, QStringLiteral("Computer")}, {Key::Attribute, QStringLiteral("None")}}
										   << QString(Error::BadRequest);
		QTest::newRow("numeric type") << QString(Command::DirectoryQueryObjects)
									  << QVariantMap{{Key::Type, 3}, {Key::Attribute, QStringLiteral("None")}} << QString(Error::BadRequest);
		QTest::newRow("numeric type string") << QString(Command::DirectoryQueryObjects)
											 << QVariantMap{{Key::Type, QStringLiteral("3")}, {Key::Attribute, QStringLiteral("None")}}
											 << QString(Error::BadRequest);
		QTest::newRow("lower case attribute") << QString(Command::DirectoryQueryObjectAttribute)
											  << QVariantMap{{Key::ObjectUid, uid}, {Key::Attribute, QStringLiteral("macaddress")}}
											  << QString(Error::BadRequest);
		QTest::newRow("missing argument") << QString(Command::DirectoryQueryParents) << QVariantMap{} << QString(Error::BadRequest);
		QTest::newRow("unknown argument") << QString(Command::DirectoryUpdate) << QVariantMap{{QStringLiteral("Filter"), QStringLiteral("(cn=*)")}}
										  << QString(Error::BadRequest);
		QTest::newRow("invalid uid") << QString(Command::DirectoryQueryParents) << QVariantMap{{Key::ObjectUid, QStringLiteral("cn=admin")}}
									 << QString(Error::BadRequest);
		QTest::newRow("invalid cursor") << QString(Command::DirectoryUpdate) << QVariantMap{{Key::Cursor, QStringLiteral("../../etc")}}
										<< QString(Error::BadRequest);
		QTest::newRow("upper case cursor") << QString(Command::DirectoryUpdate) << QVariantMap{{Key::Cursor, QString(32, QLatin1Char('A'))}}
										   << QString(Error::BadRequest);
		QTest::newRow("bool as string") << QString(Command::UserGroupsList) << QVariantMap{{Key::QueryDomainGroups, QStringLiteral("true")}}
										<< QString(Error::BadRequest);
		QTest::newRow("empty user name") << QString(Command::UserGroupsGroupsOfUser) << QVariantMap{{Key::Username, QString()}}
										 << QString(Error::BadRequest);
		QTest::newRow("user name 256 bytes") << QString(Command::UserGroupsGroupsOfUser) << QVariantMap{{Key::Username, longString(256)}} << QString();
		QTest::newRow("user name 257 bytes") << QString(Command::UserGroupsGroupsOfUser) << QVariantMap{{Key::Username, longString(257)}}
											 << QString(Error::BadRequest);
		QTest::newRow("multi-byte user name over 256 bytes") << QString(Command::UserGroupsGroupsOfUser)
															 << QVariantMap{{Key::Username, QString(129, QChar(0x00e4))}} << QString(Error::BadRequest);
		QTest::newRow("control character") << QString(Command::UserGroupsGroupsOfUser)
										   << QVariantMap{{Key::Username, QStringLiteral("a\nb")}} << QString(Error::BadRequest);
		QTest::newRow("NUL character") << QString(Command::DirectoryQueryObjects)
									   << QVariantMap{{Key::Type, QStringLiteral("Host")}, {Key::Attribute, QStringLiteral("Name")}, {Key::Value, QString(QString(QChar(0)) + QStringLiteral("*"))}}
									   << QString(Error::BadRequest);
		QTest::newRow("lone surrogate") << QString(Command::UserGroupsGroupSecurityIdentifier)
										<< QVariantMap{{Key::GroupName, QString(QChar(0xd800))}} << QString(Error::BadRequest);
		QTest::newRow("too many arguments") << QString(Command::DirectoryUpdate)
											<< QVariantMap{{QStringLiteral("a"), 1}, {QStringLiteral("b"), 1}, {QStringLiteral("c"), 1}, {QStringLiteral("d"), 1}, {QStringLiteral("e"), 1}, {QStringLiteral("f"), 1}, {QStringLiteral("g"), 1}, {QStringLiteral("h"), 1}, {QStringLiteral("i"), 1}}
											<< QString(Error::TooLarge);
	}

	void requestValidation()
	{
		QFETCH(QString, command);
		QFETCH(QVariantMap, arguments);
		QFETCH(QString, error);

		const auto result = DirectoryBrokerMessage::validateRequest(request(command, arguments));
		if (error.isEmpty())
		{
			QVERIFY(result.has_value() == false);
		}
		else
		{
			QVERIFY(result.has_value());
			QCOMPARE(result->errorCode(), error);
			QCOMPARE(result->requestId, 1u);
		}
	}

	void pendingRequests()
	{
		DirectoryBrokerPendingRequests pendingRequests;

		const auto first = pendingRequests.add(QString(Command::UserGroupsList));
		const auto second = pendingRequests.add(QString(Command::UserGroupsList));
		QVERIFY(second > first);

		DirectoryBrokerPendingRequests::ReplyHandler handler;

		// unknown RequestId
		QVERIFY(pendingRequests.complete({second + 100, QString(Command::UserGroupsList), {}}, handler) == false);
		// reply to a different command
		QVERIFY(pendingRequests.complete({first, QString(Command::UserGroupsGroupsOfUser), {}}, handler) == false);

		QVERIFY(pendingRequests.complete({first, QString(Command::UserGroupsList), {{Key::Groups, QStringList{QStringLiteral("a")}}}}, handler));
		QVERIFY(handler == nullptr);
		// duplicate reply
		QVERIFY(pendingRequests.complete({first, QString(Command::UserGroupsList), {}}, handler) == false);

		const auto firstReply = pendingRequests.wait(first, QDeadlineTimer(100));
		QVERIFY(firstReply.has_value());
		QCOMPARE(firstReply->arguments.value(Key::Groups).toStringList(), QStringList{QStringLiteral("a")});

		// timeout, then late reply
		QVERIFY(pendingRequests.wait(second, QDeadlineTimer(50)).has_value() == false);
		QVERIFY(pendingRequests.complete({second, QString(Command::UserGroupsList), {}}, handler) == false);
		QCOMPARE(pendingRequests.count(), 0);

		// asynchronous request: the handler is handed out exactly once, errors match any command
		int handlerCalls = 0;
		const auto third = pendingRequests.add(QString(Command::DirectoryUpdate), [&handlerCalls](const Reply&) { ++handlerCalls; });
		QVERIFY(pendingRequests.complete(DirectoryBrokerMessage::error(third, Error::Timeout), handler));
		QVERIFY(handler != nullptr);
		handler({});
		handler = nullptr;
		QVERIFY(pendingRequests.complete({third, QString(Command::DirectoryUpdate), {}}, handler) == false);
		QCOMPARE(handlerCalls, 1);

		// a removed (timed out) asynchronous request discards its late reply
		const auto fourth = pendingRequests.add(QString(Command::DirectoryUpdate), [](const Reply&) {});
		QVERIFY(pendingRequests.remove(fourth).has_value());
		QVERIFY(pendingRequests.complete({fourth, QString(Command::DirectoryUpdate), {}}, handler) == false);
	}

	void serviceDirectory()
	{
		DummyDirectoryPlugin plugin(3, 4);
		DummyNetworkObjectDirectory* directory = nullptr;
		auto service = createService(plugin, &directory);

		const auto update = call(*service, Command::DirectoryUpdate);
		QVERIFY(update.isError() == false);
		QCOMPARE(update.arguments.value(Key::Objects).toList().size(), 3);
		QCOMPARE(update.arguments.value(Key::Generation).toInt(), 1);
		QCOMPARE(update.arguments.value(Key::NextCursor).toString(), QString());
		QCOMPARE(plugin.directoryInstantiations.load(), 1);

		// the backend is used in the directory worker thread only
		QVERIFY(directory != nullptr);
		QVERIFY(directory->usedThreads.isEmpty() == false);
		for (const auto thread : std::as_const(directory->usedThreads))
		{
			QVERIFY(thread != QThread::currentThread());
		}

		const auto location = DummyNetworkObjectDirectory::location(1);
		const auto fetch = call(*service, Command::DirectoryFetchObjects, {{Key::ObjectUid, location.uid().toString()}});
		QVERIFY(fetch.isError() == false);
		QCOMPARE(fetch.arguments.value(Key::Objects).toList().size(), 4);

		const auto host = DummyNetworkObjectDirectory::host(1, 2);
		const auto query = call(*service, Command::DirectoryQueryObjects,
								{{Key::Type, QStringLiteral("Host")}, {Key::Attribute, QStringLiteral("HostAddress")}, {Key::Value, host.hostAddress()}});
		QVERIFY(query.isError() == false);
		QCOMPARE(query.arguments.value(Key::Objects).toList().size(), 1);

		const auto parents = call(*service, Command::DirectoryQueryParents, {{Key::ObjectUid, host.uid().toString()}});
		QVERIFY(parents.isError() == false);
		const auto parentObjects = parents.arguments.value(Key::Objects).toList();
		QCOMPARE(parentObjects.size(), 1);
		QCOMPARE(DirectoryBrokerMessage::objectFromVariant(parentObjects.first())->uid(), location.uid());

		const auto mac = call(*service, Command::DirectoryQueryObjectAttribute,
							  {{Key::ObjectUid, host.uid().toString()}, {Key::Attribute, QStringLiteral("MacAddress")}});
		QVERIFY(mac.isError() == false);
		QCOMPARE(mac.arguments.value(Key::Value).toString(), host.macAddress());

		const auto typeAttribute = call(*service, Command::DirectoryQueryObjectAttribute,
										{{Key::ObjectUid, host.uid().toString()}, {Key::Attribute, QStringLiteral("Type")}});
		QCOMPARE(typeAttribute.errorCode(), QString(Error::BadRequest));

		// objects never returned before cannot be addressed
		const auto unknown = call(*service, Command::DirectoryQueryParents, {{Key::ObjectUid, QUuid::createUuid().toString()}});
		QCOMPARE(unknown.errorCode(), QString(Error::BadRequest));

		// the backend fails: no empty result
		directory->available = false;
		const auto failedQuery = call(*service, Command::DirectoryQueryObjects,
									  {{Key::Type, QStringLiteral("Host")}, {Key::Attribute, QStringLiteral("HostAddress")}, {Key::Value, host.hostAddress()}});
		QCOMPARE(failedQuery.errorCode(), QString(Error::Unavailable));
		const auto failedParents = call(*service, Command::DirectoryQueryParents, {{Key::ObjectUid, host.uid().toString()}});
		QCOMPARE(failedParents.errorCode(), QString(Error::Unavailable));

		QCOMPARE(plugin.directoryInstantiations.load(), 1);
	}

	void serviceUserGroups()
	{
		DummyDirectoryPlugin plugin(1, 1, 3);
		auto service = createService(plugin);

		const auto groups = call(*service, Command::UserGroupsGroupsOfUser, {{Key::Username, QStringLiteral("teacher")}});
		QVERIFY(groups.isError() == false);
		QCOMPARE(groups.arguments.value(Key::Groups).toStringList(), (QStringList{QStringLiteral("group0"), QStringLiteral("group1")}));
		QCOMPARE(plugin.lastUsername, QStringLiteral("teacher"));

		const auto sid = call(*service, Command::UserGroupsGroupSecurityIdentifier, {{Key::GroupName, QStringLiteral("abc")}});
		QCOMPARE(sid.arguments.value(Key::Sid).toString(), QStringLiteral("S-1-5-21-3"));

		plugin.available = false;
		const auto failed = call(*service, Command::UserGroupsGroupsOfUser, {{Key::Username, QStringLiteral("teacher")}});
		QCOMPARE(failed.errorCode(), QString(Error::Unavailable));

		// no directory configured (e.g. central mode): every request fails
		DirectoryBrokerService unavailableService([]() { return nullptr; }, []() { return nullptr; });
		QCOMPARE(call(unavailableService, Command::DirectoryUpdate).errorCode(), QString(Error::Unavailable));
		QCOMPARE(call(unavailableService, Command::UserGroupsList).errorCode(), QString(Error::Unavailable));
	}

	void servicePaging()
	{
		// 20 × 500 = 10 000 hosts, listed by a master
		DummyDirectoryPlugin plugin(20, 500);
		auto service = createService(plugin);

		const QVariantMap arguments{{Key::Type, QStringLiteral("Host")}, {Key::Attribute, QStringLiteral("None")}, {Key::Forwarded, true}};

		auto reply = call(*service, Command::DirectoryQueryObjects, arguments);
		QSet<QString> uids;
		int pages = 0;
		QString firstCursor;

		for (;;)
		{
			QVERIFY(reply.isError() == false);
			const auto objects = reply.arguments.value(Key::Objects).toList();
			QVERIFY(objects.size() > 0);
			QVERIFY(objects.size() <= DirectoryBrokerLimits::MaximumObjectsPerPage);
			QVERIFY(jsonSize(reply) <= IpcProtocol::MaximumRecordPlaintextSize);
			for (const auto& object : objects)
			{
				uids.insert(object.toMap().value(QStringLiteral("Uid")).toString());
				// no DNs for masters
				QVERIFY(object.toMap().contains(QStringLiteral("DirectoryAddress")) == false);
			}
			++pages;

			const auto cursor = reply.arguments.value(Key::NextCursor).toString();
			if (cursor.isEmpty())
			{
				break;
			}
			if (firstCursor.isEmpty())
			{
				firstCursor = cursor;
			}
			auto nextArguments = arguments;
			nextArguments[Key::Cursor] = cursor;
			// stay below the request rate
			m_now += 100;
			reply = call(*service, Command::DirectoryQueryObjects, nextArguments);
		}

		QCOMPARE(uids.size(), 10000);
		QVERIFY(pages >= 10000 / DirectoryBrokerLimits::MaximumObjectsPerPage);

		// a finished cursor is gone
		auto finishedArguments = arguments;
		finishedArguments[Key::Cursor] = firstCursor;
		QCOMPARE(call(*service, Command::DirectoryQueryObjects, finishedArguments).errorCode(), QString(Error::BadRequest));

		// cursors expire after 60 s and are bound to their command and pool
		m_now += 1000;
		const auto fresh = call(*service, Command::DirectoryQueryObjects, arguments);
		const auto cursor = fresh.arguments.value(Key::NextCursor).toString();
		QVERIFY(cursor.isEmpty() == false);

		QCOMPARE(call(*service, Command::UserGroupsList, {{Key::Cursor, cursor}, {Key::Forwarded, true}}).errorCode(), QString(Error::BadRequest));
		auto ownArguments = arguments;
		ownArguments.remove(Key::Forwarded);
		ownArguments[Key::Cursor] = cursor;
		QCOMPARE(call(*service, Command::DirectoryQueryObjects, ownArguments).errorCode(), QString(Error::BadRequest));

		m_now += DirectoryBrokerLimits::CursorLifetime + 1;
		auto expiredArguments = arguments;
		expiredArguments[Key::Cursor] = cursor;
		QCOMPARE(call(*service, Command::DirectoryQueryObjects, expiredArguments).errorCode(), QString(Error::BadRequest));

		// cursors of other clients are not visible
		const auto otherFresh = call(*service, Command::DirectoryQueryObjects, arguments);
		auto otherArguments = arguments;
		otherArguments[Key::Cursor] = otherFresh.arguments.value(Key::NextCursor).toString();
		QCOMPARE(call(*service, Command::DirectoryQueryObjects, otherArguments, QStringLiteral("session2")).errorCode(), QString(Error::BadRequest));
	}

	void servicePagingLimits()
	{
		// more than MaximumResultItems objects
		DummyDirectoryPlugin plugin(41, 500);
		auto service = createService(plugin);

		const QVariantMap forwardedArguments{{Key::Type, QStringLiteral("Host")}, {Key::Attribute, QStringLiteral("None")}, {Key::Forwarded, true}};
		QCOMPARE(call(*service, Command::DirectoryQueryObjects, forwardedArguments).errorCode(), QString(Error::TooLarge));

		// the session server's own cursor pool (1 MiB) does not hold 10 000 objects
		DummyDirectoryPlugin mediumPlugin(20, 500);
		auto mediumService = createService(mediumPlugin);
		QCOMPARE(call(*mediumService, Command::DirectoryQueryObjects, {{Key::Type, QStringLiteral("Host")}, {Key::Attribute, QStringLiteral("None")}}).errorCode(),
				 QString(Error::TooLarge));
	}

	// a full cursor pool evicts its oldest cursor instead of failing (cursors abandoned by masters
	// must not block further listings), the session server's own cursors are not affected
	void serviceCursorEviction()
	{
		DummyDirectoryPlugin plugin(1, 400);
		auto service = createService(plugin);

		const QVariantMap ownArguments{{Key::Type, QStringLiteral("Host")}, {Key::Attribute, QStringLiteral("None")}};
		auto forwardedArguments = ownArguments;
		forwardedArguments[Key::Forwarded] = true;

		const auto own = call(*service, Command::DirectoryQueryObjects, ownArguments);
		const auto ownCursor = own.arguments.value(Key::NextCursor).toString();
		QVERIFY(ownCursor.isEmpty() == false);

		QStringList forwardedCursors;
		for (int index = 0; index < DirectoryBrokerLimits::MaximumForwardedCursors + 5; ++index)
		{
			// stay below the request rate
			m_now += 100;
			const auto reply = call(*service, Command::DirectoryQueryObjects, forwardedArguments);
			QVERIFY(reply.isError() == false);
			forwardedCursors.append(reply.arguments.value(Key::NextCursor).toString());
		}

		// the oldest forwarded cursors were evicted, the newest are valid
		auto oldestArguments = forwardedArguments;
		oldestArguments[Key::Cursor] = forwardedCursors.first();
		m_now += 100;
		QCOMPARE(call(*service, Command::DirectoryQueryObjects, oldestArguments).errorCode(), QString(Error::BadRequest));

		auto newestArguments = forwardedArguments;
		newestArguments[Key::Cursor] = forwardedCursors.last();
		m_now += 100;
		QVERIFY(call(*service, Command::DirectoryQueryObjects, newestArguments).isError() == false);

		auto ownContinuation = ownArguments;
		ownContinuation[Key::Cursor] = ownCursor;
		m_now += 100;
		QVERIFY(call(*service, Command::DirectoryQueryObjects, ownContinuation).isError() == false);
	}

	// review probe scenario 4: objects known to one session survive large listings of another
	void serviceKnownObjectsPerClient()
	{
		DummyDirectoryPlugin plugin(40, 500);
		auto service = createService(plugin);

		const auto host = DummyNetworkObjectDirectory::host(0, 0);
		const auto query = call(*service, Command::DirectoryQueryObjects,
								{{Key::Type, QStringLiteral("Host")}, {Key::Attribute, QStringLiteral("HostAddress")}, {Key::Value, host.hostAddress()}});
		QCOMPARE(query.arguments.value(Key::Objects).toList().size(), 1);

		for (int index = 0; index < 2; ++index)
		{
			m_now += 1000;
			(void)call(*service, Command::DirectoryQueryObjects,
					   {{Key::Type, QStringLiteral("Host")}, {Key::Attribute, QStringLiteral("None")}, {Key::Forwarded, true}},
					   QStringLiteral("session2"));
			(void)call(*service, Command::DirectoryQueryObjects,
					   {{Key::Type, QStringLiteral("Host")}, {Key::Attribute, QStringLiteral("None")}, {Key::Forwarded, true}});
		}

		m_now += 1000;
		const auto parents = call(*service, Command::DirectoryQueryParents, {{Key::ObjectUid, host.uid().toString()}});
		QVERIFY2(parents.isError() == false, qPrintable(parents.errorMessage()));
		QCOMPARE(parents.arguments.value(Key::Objects).toList().size(), 1);

		// forwarded requests cannot address the session server's own objects and get no DNs
		const auto forwardedParents = call(*service, Command::DirectoryQueryParents,
										   {{Key::ObjectUid, host.uid().toString()}, {Key::Forwarded, true}}, QStringLiteral("session3"));
		QCOMPARE(forwardedParents.errorCode(), QString(Error::BadRequest));
		(void)call(*service, Command::DirectoryQueryObjects,
				   {{Key::Type, QStringLiteral("Host")}, {Key::Attribute, QStringLiteral("HostAddress")}, {Key::Value, host.hostAddress()}, {Key::Forwarded, true}},
				   QStringLiteral("session3"));
		QCOMPARE(call(*service, Command::DirectoryQueryObjectAttribute,
					  {{Key::ObjectUid, host.uid().toString()}, {Key::Attribute, QStringLiteral("DirectoryAddress")}, {Key::Forwarded, true}},
					  QStringLiteral("session3"))
					 .errorCode(),
				 QString(Error::BadRequest));
		QCOMPARE(call(*service, Command::DirectoryQueryObjectAttribute,
					  {{Key::ObjectUid, host.uid().toString()}, {Key::Attribute, QStringLiteral("MacAddress")}, {Key::Forwarded, true}},
					  QStringLiteral("session3"))
					 .arguments.value(Key::Value)
					 .toString(),
				 host.macAddress());
	}

	// review probe scenario 1: the session server's own (access control) queries are not delayed
	// by the forwarded queries of masters of the same session, other sessions are not starved
	void serviceOwnRequestsFirst()
	{
		DummyDirectoryPlugin plugin(2, 3);
		DummyNetworkObjectDirectory* directory = nullptr;
		auto service = createService(plugin, &directory);
		service->setRequestTimeout(3000);
		QVERIFY(call(*service, Command::DirectoryUpdate).isError() == false);

		directory->delay = 500;
		QStringList completionOrder;
		for (int index = 0; index < DirectoryBrokerLimits::MaximumForwardedOutstandingRequests; ++index)
		{
			service->handleRequest(QStringLiteral("session1"),
								   {quint32(10 + index), QString(Command::DirectoryQueryObjects), {{Key::Type, QStringLiteral("Host")}, {Key::Attribute, QStringLiteral("None")}, {Key::Forwarded, true}}},
								   [&completionOrder](const Reply& reply) {
				completionOrder.append(reply.isError() ? reply.errorCode() : QStringLiteral("forwarded"));
			});
		}
		QTest::qWait(50);

		QElapsedTimer timer;
		timer.start();
		qint64 ownDuration = -1;
		qint64 otherDuration = -1;
		service->handleRequest(QStringLiteral("session1"), {20, QString(Command::UserGroupsGroupsOfUser), {{Key::Username, QStringLiteral("teacher")}}},
							   [&](const Reply& reply) {
			completionOrder.append(reply.isError() ? reply.errorCode() : QStringLiteral("own"));
			ownDuration = timer.elapsed();
		});
		service->handleRequest(QStringLiteral("session2"), {30, QString(Command::UserGroupsGroupsOfUser), {{Key::Username, QStringLiteral("teacher")}}},
							   [&](const Reply& reply) {
			completionOrder.append(reply.isError() ? reply.errorCode() : QStringLiteral("other"));
			otherDuration = timer.elapsed();
		});

		QVERIFY(QTest::qWaitFor([&completionOrder]() { return completionOrder.size() == 5; }, 5000));
		// only the forwarded query already running is ahead of them
		QCOMPARE(completionOrder.mid(0, 3), (QStringList{QStringLiteral("forwarded"), QStringLiteral("own"), QStringLiteral("other")}));
		QVERIFY(ownDuration < 1000);
		QVERIFY(otherDuration < 1000);
		QVERIFY(completionOrder.contains(QString(Error::Timeout)) == false);
	}

	// timed out requests are aborted cooperatively and do not occupy the worker
	void serviceCooperativeAbort()
	{
		DummyDirectoryPlugin plugin(2, 3);
		DummyNetworkObjectDirectory* directory = nullptr;
		auto service = createService(plugin, &directory);
		QVERIFY(call(*service, Command::DirectoryUpdate).isError() == false);
		QVERIFY(DirectoryBrokerService::isCurrentRequestCancelled() == false);

		service->setRequestTimeout(100);
		directory->delay = 5000;
		const auto slow = call(*service, Command::DirectoryQueryObjects, {{Key::Type, QStringLiteral("Host")}, {Key::Attribute, QStringLiteral("None")}});
		QCOMPARE(slow.errorCode(), QString(Error::Timeout));

		service->setRequestTimeout(DirectoryBrokerLimits::RequestTimeout);
		QElapsedTimer timer;
		timer.start();
		QVERIFY(call(*service, Command::UserGroupsList).isError() == false);
		QVERIFY(timer.elapsed() < 1000);
		QCOMPARE(directory->abortedQueries.load(), 1);
	}

	// concurrent directory updates are answered by one update
	void serviceCoalescedUpdates()
	{
		DummyDirectoryPlugin plugin(2, 3);
		DummyNetworkObjectDirectory* directory = nullptr;
		auto service = createService(plugin, &directory);
		QVERIFY(call(*service, Command::DirectoryUpdate).isError() == false);
		QCOMPARE(directory->updateCount.load(), 1);

		directory->updateDelay = 300;
		QList<Reply> replies;
		for (int index = 0; index < 3; ++index)
		{
			service->handleRequest(QStringLiteral("session%1").arg(index), {quint32(index), QString(Command::DirectoryUpdate), {}},
								   [&replies](const Reply& reply) { replies.append(reply); });
		}
		QVERIFY(QTest::qWaitFor([&replies]() { return replies.size() == 3; }, 5000));
		for (const auto& reply : std::as_const(replies))
		{
			QVERIFY(reply.isError() == false);
			QCOMPARE(reply.arguments.value(Key::Objects).toList().size(), 2);
		}
		QCOMPARE(directory->updateCount.load(), 2);

		// later requests trigger a new update
		directory->updateDelay = 0;
		const auto later = call(*service, Command::DirectoryUpdate);
		QVERIFY(later.isError() == false);
		QVERIFY(later.arguments.value(Key::Generation).toInt() > replies.first().arguments.value(Key::Generation).toInt());
	}

	// re-review N4: a failed or aborted update is neither returned nor reused for coalesced requests
	void serviceFailedUpdates()
	{
		DummyDirectoryPlugin plugin(4, 3);
		DummyNetworkObjectDirectory* directory = nullptr;
		auto service = createService(plugin, &directory);
		const auto first = call(*service, Command::DirectoryUpdate);
		QCOMPARE(first.arguments.value(Key::Objects).toList().size(), 4);
		const auto generation = first.arguments.value(Key::Generation).toInt();

		directory->updateFails = true;
		directory->updateDelay = 200;
		QList<Reply> replies;
		for (int index = 0; index < 3; ++index)
		{
			service->handleRequest(QStringLiteral("session%1").arg(index), {quint32(index), QString(Command::DirectoryUpdate), {}},
								   [&replies](const Reply& reply) { replies.append(reply); });
		}
		QVERIFY(QTest::qWaitFor([&replies]() { return replies.size() == 3; }, 5000));
		for (const auto& reply : std::as_const(replies))
		{
			QCOMPARE(reply.errorCode(), QString(Error::Unavailable));
		}
		// not coalesced with the failed update
		QCOMPARE(directory->updateCount.load(), 4);

		// aborted (timed out) update
		directory->updateFails = false;
		directory->updateDelay = 2000;
		service->setRequestTimeout(100);
		QCOMPARE(call(*service, Command::DirectoryUpdate).errorCode(), QString(Error::Timeout));

		service->setRequestTimeout(DirectoryBrokerLimits::RequestTimeout);
		directory->updateDelay = 0;
		m_now += 1000;
		const auto later = call(*service, Command::DirectoryUpdate);
		QCOMPARE(later.arguments.value(Key::Objects).toList().size(), 4);
		QCOMPARE(later.arguments.value(Key::Generation).toInt(), generation + 1);
	}

	// re-review N6: forwarded requests have a shorter execution budget than the request timeout
	void serviceForwardedExecutionBudget()
	{
		DummyDirectoryPlugin plugin(2, 3);
		DummyNetworkObjectDirectory* directory = nullptr;
		auto service = createService(plugin, &directory);
		QVERIFY(call(*service, Command::DirectoryUpdate).isError() == false);
		service->setForwardedExecutionBudget(200);

		directory->delay = 2000;
		QElapsedTimer timer;
		timer.start();
		const auto forwarded = call(*service, Command::DirectoryQueryObjects,
									{{Key::Type, QStringLiteral("Host")}, {Key::Attribute, QStringLiteral("None")}, {Key::Forwarded, true}});
		QCOMPARE(forwarded.errorCode(), QString(Error::Unavailable));
		QVERIFY(timer.elapsed() < 1500);
		QCOMPARE(directory->abortedQueries.load(), 1);

		// own requests are not limited by it
		directory->delay = 400;
		const auto own = call(*service, Command::DirectoryQueryObjects,
							  {{Key::Type, QStringLiteral("Host")}, {Key::Attribute, QStringLiteral("None")}});
		QVERIFY(own.isError() == false);
		QCOMPARE(directory->abortedQueries.load(), 1);
	}

	// a backend that could not be created is tried again later (no latched failure)
	void serviceBackendRetry()
	{
		DummyDirectoryPlugin plugin(2, 3);
		int factoryCalls = 0;
		bool available = false;
		DirectoryBrokerService service(
			[&]() -> NetworkObjectDirectory* {
			++factoryCalls;
			return available ? plugin.createNetworkObjectDirectory(nullptr) : nullptr;
		},
			[&]() -> UserGroupsBackendInterface* { return available ? &plugin : nullptr; },
			[this]() { return m_now; });
		service.setBackendRetryInterval(200);

		QCOMPARE(call(service, Command::DirectoryUpdate).errorCode(), QString(Error::Unavailable));
		QCOMPARE(call(service, Command::DirectoryUpdate).errorCode(), QString(Error::Unavailable));
		QCOMPARE(factoryCalls, 1);
		QCOMPARE(call(service, Command::UserGroupsList).errorCode(), QString(Error::Unavailable));

		available = true;
		QTest::qWait(250);
		QVERIFY(call(service, Command::DirectoryUpdate).isError() == false);
		QVERIFY(call(service, Command::UserGroupsList).isError() == false);
		QCOMPARE(factoryCalls, 2);
	}

	void serviceGroupsPaging()
	{
		DummyDirectoryPlugin plugin(1, 1, 5000);
		auto service = createService(plugin);

		auto reply = call(*service, Command::UserGroupsList, {{Key::QueryDomainGroups, false}});
		QStringList groups;
		for (;;)
		{
			QVERIFY(reply.isError() == false);
			const auto page = reply.arguments.value(Key::Groups).toStringList();
			QVERIFY(page.size() <= DirectoryBrokerLimits::MaximumGroupsPerPage);
			QVERIFY(jsonSize(reply) <= IpcProtocol::MaximumRecordPlaintextSize);
			groups.append(page);
			const auto cursor = reply.arguments.value(Key::NextCursor).toString();
			if (cursor.isEmpty())
			{
				break;
			}
			reply = call(*service, Command::UserGroupsList, {{Key::QueryDomainGroups, false}, {Key::Cursor, cursor}});
		}

		QCOMPARE(groups.size(), 5000);
		QCOMPARE(groups.last(), QStringLiteral("group4999"));
	}

	void serviceRateLimit()
	{
		DummyDirectoryPlugin plugin;
		auto service = createService(plugin);

		for (int index = 0; index < DirectoryBrokerLimits::MaximumRequestsPerSecond; ++index)
		{
			QVERIFY(call(*service, Command::UserGroupsList).isError() == false);
		}
		QCOMPARE(call(*service, Command::UserGroupsList).errorCode(), QString(Error::RateLimited));
		// other sessions are not affected
		QVERIFY(call(*service, Command::UserGroupsList, {}, QStringLiteral("session2")).isError() == false);

		m_now += 1000;
		QVERIFY(call(*service, Command::UserGroupsList).isError() == false);
	}

	void serviceOutstandingRequests()
	{
		DummyDirectoryPlugin plugin;
		plugin.delay = 200;
		auto service = createService(plugin);

		QList<Reply> replies;
		for (int index = 0; index < DirectoryBrokerLimits::MaximumOutstandingRequests + 1; ++index)
		{
			service->handleRequest(QStringLiteral("session1"), {quint32(index), QString(Command::UserGroupsGroupsOfUser), {{Key::Username, QStringLiteral("u")}}},
								   [&replies](const Reply& reply) { replies.append(reply); });
		}

		// the fifth is refused immediately
		QCOMPARE(replies.size(), 1);
		QCOMPARE(replies.first().errorCode(), QString(Error::RateLimited));
		QCOMPARE(replies.first().requestId, quint32(DirectoryBrokerLimits::MaximumOutstandingRequests));

		QVERIFY(QTest::qWaitFor([&replies]() { return replies.size() == DirectoryBrokerLimits::MaximumOutstandingRequests + 1; }, 5000));
		for (int index = 1; index < replies.size(); ++index)
		{
			QVERIFY(replies.at(index).isError() == false);
		}
	}

	void serviceTimeout()
	{
		DummyDirectoryPlugin plugin;
		plugin.delay = 500;
		auto service = createService(plugin);
		service->setRequestTimeout(100);

		int calls = 0;
		Reply reply;
		service->handleRequest(QStringLiteral("session1"), {5, QString(Command::UserGroupsGroupsOfUser), {{Key::Username, QStringLiteral("u")}}},
							   [&calls, &reply](const Reply& result) {
			++calls;
			reply = result;
		});

		QVERIFY(QTest::qWaitFor([&calls]() { return calls > 0; }, 2000));
		QCOMPARE(reply.errorCode(), QString(Error::Timeout));
		QCOMPARE(reply.requestId, 5u);

		// the late result is discarded
		QTest::qWait(800);
		QCOMPARE(calls, 1);

		// the outstanding slot was released
		plugin.delay = 0;
		QVERIFY(call(*service, Command::UserGroupsList).isError() == false);
	}

	void serviceRoundRobin()
	{
		DummyDirectoryPlugin plugin;
		plugin.delay = 100;
		auto service = createService(plugin);

		QStringList completionOrder;
		for (int index = 0; index < DirectoryBrokerLimits::MaximumOutstandingRequests; ++index)
		{
			service->handleRequest(QStringLiteral("busy"), {quint32(index), QString(Command::UserGroupsGroupsOfUser), {{Key::Username, QStringLiteral("u")}}},
								   [&completionOrder](const Reply&) { completionOrder.append(QStringLiteral("busy")); });
		}
		service->handleRequest(QStringLiteral("other"), {99, QString(Command::UserGroupsGroupsOfUser), {{Key::Username, QStringLiteral("u")}}},
							   [&completionOrder](const Reply&) { completionOrder.append(QStringLiteral("other")); });

		QVERIFY(QTest::qWaitFor([&completionOrder]() { return completionOrder.size() == 5; }, 5000));
		// besides the running job, at most one queued job of the busy session is taken before the
		// other session's job (first-come-first-served would make it the last one)
		QVERIFY(completionOrder.indexOf(QStringLiteral("other")) <= 2);
	}

	void serviceRemoveClient()
	{
		DummyDirectoryPlugin plugin;
		plugin.delay = 100;
		auto service = createService(plugin);

		int calls = 0;
		service->handleRequest(QStringLiteral("session1"), {1, QString(Command::UserGroupsGroupsOfUser), {{Key::Username, QStringLiteral("u")}}},
							   [&calls](const Reply&) { ++calls; });
		service->removeClient(QStringLiteral("session1"));
		QTest::qWait(300);
		QCOMPARE(calls, 0);
	}

	void placement()
	{
		using Component = VeyonCore::Component;
		using Mode = VeyonCore::DirectoryBrokerMode;
		using Placement = DirectoryBrokerClient::Placement;

		// backends without secrets stay in-process everywhere
		for (const auto component : {Component::Server, Component::Worker, Component::Master, Component::Service})
		{
			QCOMPARE(DirectoryBrokerClient::placement(component, Mode::Required, false, false, true), Placement::InProcess);
		}

		// veyon-server and veyon-worker never, regardless of mode and privilege
		for (const auto mode : {Mode::Auto, Mode::Required, Mode::Disabled})
		{
			QCOMPARE(DirectoryBrokerClient::placement(Component::Server, mode, true, true, true), Placement::Broker);
			QCOMPARE(DirectoryBrokerClient::placement(Component::Worker, mode, true, true, true), Placement::Broker);
		}

		// the service runs the backend, except in central mode
		QCOMPARE(DirectoryBrokerClient::placement(Component::Service, Mode::Auto, true, true, true), Placement::InProcess);
		QCOMPARE(DirectoryBrokerClient::placement(Component::Service, Mode::Auto, true, true, false), Placement::Broker);

		// unprivileged master: broker in Auto and Required (the secret is never readable there)
		QCOMPARE(DirectoryBrokerClient::placement(Component::Master, Mode::Auto, true, false, true), Placement::Broker);
		QCOMPARE(DirectoryBrokerClient::placement(Component::Master, Mode::Required, true, false, true), Placement::Broker);
		QCOMPARE(DirectoryBrokerClient::placement(Component::Master, Mode::Disabled, true, false, true), Placement::InProcess);

		// elevated configurator/CLI with access to the SecretStore
		QCOMPARE(DirectoryBrokerClient::placement(Component::Configurator, Mode::Auto, true, true, true), Placement::InProcess);
		QCOMPARE(DirectoryBrokerClient::placement(Component::CLI, Mode::Required, true, true, true), Placement::InProcess);
		QCOMPARE(DirectoryBrokerClient::placement(Component::CLI, Mode::Required, true, false, true), Placement::Broker);
	}

	void localHost()
	{
		QVERIFY(DirectoryBrokerClient::isLocalHost(QStringLiteral("localhost")));
		QVERIFY(DirectoryBrokerClient::isLocalHost(QStringLiteral("LocalHost")));
		QVERIFY(DirectoryBrokerClient::isLocalHost(QStringLiteral("127.0.0.1")));
		QVERIFY(DirectoryBrokerClient::isLocalHost(QStringLiteral("::1")));
		QVERIFY(DirectoryBrokerClient::isLocalHost(QStringLiteral("broker.example.org")) == false);
		QVERIFY(DirectoryBrokerClient::isLocalHost(QStringLiteral("10.0.0.1")) == false);
		QVERIFY(DirectoryBrokerClient::isLocalHost(QStringLiteral("localhost.example.org")) == false);
		QVERIFY(DirectoryBrokerClient::isLocalHost(QString()) == false);
	}

	void requiredModeBrokerDown()
	{
		DummyDirectoryPlugin plugin;
		const auto client = std::make_shared<TestBrokerClient>(nullptr);

		std::unique_ptr<NetworkObjectDirectory> directory(
			NetworkObjectDirectoryManager::createDirectory(&plugin, DirectoryBrokerClient::Placement::Broker,
														   [client]() { return client; }, nullptr));
		QVERIFY(qobject_cast<RemoteNetworkObjectDirectory*>(directory.get()) != nullptr);

		directory->update();
		QVERIFY(QTest::qWaitFor([&directory]() { return directory->childCount(directory->rootId()) > 0; }, 2000));

		// visible error state, no backend instantiation
		const auto& objects = directory->objects(directory->rootObject());
		QCOMPARE(objects.size(), 1);
		QCOMPARE(objects.first().type(), NetworkObject::Type::Label);
		QVERIFY(qobject_cast<RemoteNetworkObjectDirectory*>(directory.get())->errorString().isEmpty() == false);
		QCOMPARE(plugin.directoryInstantiations.load(), 0);

		// fail closed
		QVERIFY(directory->tryQueryObjects(NetworkObject::Type::Host, NetworkObject::Attribute::HostAddress,
										   QStringLiteral("pc0-0.example.org"))
					.has_value() == false);
		QVERIFY(directory->tryQueryParents(DummyNetworkObjectDirectory::host(0, 0)).has_value() == false);
		QVERIFY(directory->queryObjects(NetworkObject::Type::Host, NetworkObject::Attribute::None, {}).isEmpty());

		// in-process placement instantiates the backend
		std::unique_ptr<NetworkObjectDirectory> localDirectory(
			NetworkObjectDirectoryManager::createDirectory(&plugin, DirectoryBrokerClient::Placement::InProcess,
														   [client]() { return client; }, nullptr));
		QCOMPARE(plugin.directoryInstantiations.load(), 1);
	}

	void remoteDirectory()
	{
		DummyDirectoryPlugin plugin(2, 3);
		auto service = createService(plugin);
		const auto client = std::make_shared<TestBrokerClient>(service.get());

		RemoteNetworkObjectDirectory directory(client, nullptr);
		directory.update();
		QVERIFY(QTest::qWaitFor([&directory]() { return directory.childCount(directory.rootId()) == 2; }, 2000));
		QVERIFY(directory.errorString().isEmpty());
		QCOMPARE(directory.generation(), 1);

		const auto location = directory.objects(directory.rootObject()).first();
		directory.fetchObjects(location);
		QVERIFY(QTest::qWaitFor([&directory, &location]() { return directory.childCount(location.modelId()) == 3; }, 2000));

		const auto host = DummyNetworkObjectDirectory::host(1, 2);
		const auto hosts = directory.tryQueryObjects(NetworkObject::Type::Host, NetworkObject::Attribute::HostAddress, host.hostAddress());
		QVERIFY(hosts.has_value());
		QCOMPARE(hosts->size(), 1);
		QVERIFY(hosts->first().exactMatch(host));

		const auto parents = directory.tryQueryParents(hosts->first());
		QVERIFY(parents.has_value());
		QCOMPARE(parents->size(), 1);
		QCOMPARE(parents->first().name(), QStringLiteral("Room 1"));

		QCOMPARE(directory.queryObjectAttribute(host.uid(), NetworkObject::Attribute::MacAddress).toString(), host.macAddress());

		// invalid query values are not sent
		const auto requests = client->requestCount;
		QVERIFY(directory.tryQueryObjects(NetworkObject::Type::Host, NetworkObject::Attribute::Name, longString(300)).has_value() == false);
		QCOMPARE(client->requestCount, requests);

		// broker goes down: error label replaces the objects, queries fail
		client->available = false;
		directory.update();
		QVERIFY(QTest::qWaitFor([&directory]() { return directory.errorString().isEmpty() == false; }, 2000));
		QCOMPARE(directory.objects(directory.rootObject()).size(), 1);
		QVERIFY(directory.tryQueryParents(host).has_value() == false);
		QVERIFY(directory.queryObjectAttribute(host.uid(), NetworkObject::Attribute::MacAddress).isValid() == false);

		// and recovers
		client->available = true;
		directory.update();
		QVERIFY(QTest::qWaitFor([&directory]() { return directory.errorString().isEmpty(); }, 2000));
		QVERIFY(QTest::qWaitFor([&directory]() { return directory.childCount(directory.rootId()) == 2; }, 2000));
	}

	void remoteUserGroupsFailClosed()
	{
		DummyDirectoryPlugin plugin(1, 1, 4);
		auto service = createService(plugin);
		const auto client = std::make_shared<TestBrokerClient>(service.get());

		RemoteUserGroupsBackend backend(QStringLiteral("LDAP"), client);
		QCOMPARE(backend.requiresSecrets(), false);

		const auto groups = backend.tryQueryGroupsOfUser(QStringLiteral("teacher"), false);
		QVERIFY(groups.has_value());
		QCOMPARE(groups->size(), 2);
		QCOMPARE(backend.userGroups(false).size(), 4);
		QCOMPARE(backend.userGroupSecurityIdentifier(QStringLiteral("abcd")), QStringLiteral("S-1-5-21-4"));

		// broker down: group memberships are never served from a cache
		client->available = false;
		QVERIFY(backend.tryQueryGroupsOfUser(QStringLiteral("teacher"), false).has_value() == false);
		QVERIFY(backend.groupsOfUser(QStringLiteral("teacher"), false).isEmpty());
		QVERIFY(backend.userGroupSecurityIdentifier(QStringLiteral("abcd")).isEmpty());

		// the UI listing is cached for 60 s
		QCOMPARE(backend.userGroups(false).size(), 4);
		backend.reloadConfiguration();
		QVERIFY(backend.userGroups(false).isEmpty());

		// invalid user names are not sent
		const auto requests = client->requestCount;
		client->available = true;
		QVERIFY(backend.tryQueryGroupsOfUser(QStringLiteral("a\tb"), false).has_value() == false);
		QVERIFY(backend.tryQueryGroupsOfUser(QString(), false).has_value() == false);
		QCOMPARE(client->requestCount, requests);
	}

	void featureAuthorization()
	{
		DirectoryBrokerFeaturePlugin plugin;
		TestServer server;
		QBuffer device;
		device.open(QBuffer::ReadWrite);
		QObject connection;
		const MessageContext context(&device, &connection);

		int forwarded = 0;
		plugin.setForwardFunction([&forwarded](const QString& command, const QVariantMap& arguments,
											   const DirectoryBrokerClient::ReplyHandler& replyHandler) {
			Q_UNUSED(arguments)
			++forwarded;
			replyHandler({777, command, {{Key::Groups, QStringList{QStringLiteral("g")}}, {Key::NextCursor, QString()}}});
		});
		qint64 now = 1000;
		plugin.setClock([&now]() { return now; });

		FeatureMessage message{DirectoryBrokerMessage::featureUid()};
		DirectoryBrokerMessage::toFeatureMessage(Request{11, QString(Command::UserGroupsGroupsOfUser), {{Key::Username, QStringLiteral("teacher")}}}, message);

		// access control not (yet) successful
		for (const auto state : {VncServerClient::AccessControlState::Init, VncServerClient::AccessControlState::Pending,
								 VncServerClient::AccessControlState::Waiting, VncServerClient::AccessControlState::Failed})
		{
			server.client.setAuthType(RfbVeyonAuth::KeyFile);
			server.client.setAccessControlState(state);
			QVERIFY(plugin.handleFeatureMessage(server, context, message));
		}

		// Token connections skip access control
		server.client.setAuthType(RfbVeyonAuth::Token);
		server.client.setAccessControlState(VncServerClient::AccessControlState::Successful);
		QVERIFY(plugin.handleFeatureMessage(server, context, message));

		server.client.setAuthType(RfbVeyonAuth::Invalid);
		QVERIFY(plugin.handleFeatureMessage(server, context, message));

		QCOMPARE(forwarded, 0);
		QCOMPARE(server.replies.size(), 6);
		for (const auto& reply : std::as_const(server.replies))
		{
			QCOMPARE(reply.errorCode(), QString(Error::Denied));
			QCOMPARE(reply.requestId, 11u);
		}

		// key file and logon connections that passed access control are forwarded
		server.replies.clear();
		for (const auto authType : {RfbVeyonAuth::KeyFile, RfbVeyonAuth::Logon})
		{
			server.client.setAuthType(authType);
			QVERIFY(plugin.handleFeatureMessage(server, context, message));
		}
		QCOMPARE(forwarded, 2);
		QCOMPARE(server.replies.size(), 2);
		QVERIFY(server.replies.first().isError() == false);
		// the master's RequestId, not the one of the IPC request
		QCOMPARE(server.replies.first().requestId, 11u);
		QCOMPARE(server.replies.first().arguments.value(Key::Groups).toStringList(), QStringList{QStringLiteral("g")});

		// replies from the service (JSON numbers) pass the feature message encoding
		server.replies.clear();
		const auto host = DummyNetworkObjectDirectory::host(0, 1);
		plugin.setForwardFunction([&host, &forwarded](const QString& command, const QVariantMap& arguments,
													  const DirectoryBrokerClient::ReplyHandler& replyHandler) {
			Q_UNUSED(arguments)
			++forwarded;
			const Reply serviceReply{1, command, {{Key::Objects, QVariantList{DirectoryBrokerMessage::objectToVariant(host)}}, {Key::Generation, 3}}};
			const auto response = IpcMessage::decodeResponse(IpcMessage::encodeResponse(DirectoryBrokerMessage::toIpcResponse(5, serviceReply)));
			replyHandler(DirectoryBrokerMessage::fromIpcResponse(5, command, *response));
		});
		FeatureMessage update{DirectoryBrokerMessage::featureUid()};
		DirectoryBrokerMessage::toFeatureMessage(Request{20, QString(Command::DirectoryUpdate), {}}, update);
		QVERIFY(plugin.handleFeatureMessage(server, context, update));
		QCOMPARE(server.replies.size(), 1);
		QCOMPARE(server.replies.first().requestId, 20u);
		QCOMPARE(server.replies.first().arguments.value(Key::Generation).toInt(), 3);
		const auto receivedObject = DirectoryBrokerMessage::objectFromVariant(server.replies.first().arguments.value(Key::Objects).toList().value(0));
		QVERIFY(receivedObject.has_value());
		QVERIFY(receivedObject->exactMatch(host));
		QCOMPARE(forwarded, 3);

		// invalid requests are not forwarded
		server.replies.clear();
		FeatureMessage invalid{DirectoryBrokerMessage::featureUid()};
		DirectoryBrokerMessage::toFeatureMessage(Request{12, QString(Command::UserGroupsGroupsOfUser), {{Key::Username, longString(300)}}}, invalid);
		QVERIFY(plugin.handleFeatureMessage(server, context, invalid));
		FeatureMessage malformed{DirectoryBrokerMessage::featureUid()};
		malformed.addArgument(DirectoryBrokerMessage::Argument::RequestId, 13);
		QVERIFY(plugin.handleFeatureMessage(server, context, malformed));
		QCOMPARE(forwarded, 3);
		QCOMPARE(server.replies.size(), 2);
		QCOMPARE(server.replies.at(0).errorCode(), QString(Error::BadRequest));
		QCOMPARE(server.replies.at(1).errorCode(), QString(Error::BadRequest));

		// other features are not handled
		QVERIFY(plugin.handleFeatureMessage(server, context, FeatureMessage{Feature::Uid::createUuid()}) == false);
	}

	void featureForwardingLimits()
	{
		DirectoryBrokerFeaturePlugin plugin;
		TestServer server;
		server.client.setAuthType(RfbVeyonAuth::Logon);
		server.client.setAccessControlState(VncServerClient::AccessControlState::Successful);
		QBuffer device;
		device.open(QBuffer::ReadWrite);
		QObject firstConnection;
		QObject secondConnection;
		const MessageContext firstContext(&device, &firstConnection);
		const MessageContext secondContext(&device, &secondConnection);

		QList<DirectoryBrokerClient::ReplyHandler> pendingReplies;
		plugin.setForwardFunction([&pendingReplies](const QString& command, const QVariantMap& arguments,
													const DirectoryBrokerClient::ReplyHandler& replyHandler) {
			Q_UNUSED(command)
			Q_UNUSED(arguments)
			pendingReplies.append(replyHandler);
		});
		qint64 now = 1000;
		plugin.setClock([&now]() { return now; });

		FeatureMessage message{DirectoryBrokerMessage::featureUid()};
		DirectoryBrokerMessage::toFeatureMessage(Request{1, QString(Command::UserGroupsList), {}}, message);

		const auto completePendingReplies = [&pendingReplies]() {
			for (const auto& reply : std::as_const(pendingReplies))
			{
				reply({1, QString(Command::UserGroupsList), {{Key::Groups, QStringList{}}}});
			}
			pendingReplies.clear();
		};

		// per master connection and for all masters together (keeping one outstanding request of
		// the session free for the server's own access control queries)
		for (int index = 0; index < DirectoryBrokerLimits::MaximumOutstandingRequestsPerMaster + 1; ++index)
		{
			QVERIFY(plugin.handleFeatureMessage(server, firstContext, message));
		}
		QCOMPARE(pendingReplies.size(), DirectoryBrokerLimits::MaximumOutstandingRequestsPerMaster);
		for (int index = 0; index < DirectoryBrokerLimits::MaximumOutstandingRequestsPerMaster; ++index)
		{
			QVERIFY(plugin.handleFeatureMessage(server, secondContext, message));
		}
		QCOMPARE(pendingReplies.size(), DirectoryBrokerLimits::MaximumForwardedOutstandingRequests);
		QCOMPARE(server.replies.size(), 2);
		for (const auto& reply : std::as_const(server.replies))
		{
			QCOMPARE(reply.errorCode(), QString(Error::RateLimited));
		}

		completePendingReplies();
		server.replies.clear();

		const auto rateLimitedReplies = [&server]() {
			int count = 0;
			for (const auto& reply : std::as_const(server.replies))
			{
				count += reply.errorCode() == QString(Error::RateLimited) ? 1 : 0;
			}
			return count;
		};

		// request rate per master connection
		now += 1000;
		for (int index = 0; index < DirectoryBrokerLimits::MaximumRequestsPerSecondPerMaster + 1; ++index)
		{
			QVERIFY(plugin.handleFeatureMessage(server, firstContext, message));
			completePendingReplies();
		}
		QCOMPARE(rateLimitedReplies(), 1);
		server.replies.clear();

		// all masters together: the second connection gets the rest of the forwarded rate, then
		// the first one is refused by the global limit without consuming its own rate
		for (int index = 0; index < DirectoryBrokerLimits::MaximumRequestsPerSecondPerMaster; ++index)
		{
			QVERIFY(plugin.handleFeatureMessage(server, secondContext, message));
			completePendingReplies();
		}
		QCOMPARE(rateLimitedReplies(), DirectoryBrokerLimits::MaximumRequestsPerSecondPerMaster * 2 - DirectoryBrokerLimits::MaximumForwardedRequestsPerSecond);
		server.replies.clear();

		now += 600;
		for (int index = 0; index < DirectoryBrokerLimits::MaximumRequestsPerSecondPerMaster; ++index)
		{
			QVERIFY(plugin.handleFeatureMessage(server, firstContext, message));
			completePendingReplies();
		}
		QCOMPARE(rateLimitedReplies(), DirectoryBrokerLimits::MaximumRequestsPerSecondPerMaster);
		server.replies.clear();

		// once the global window has passed, the first connection's own rate is still available
		now += 500;
		for (int index = 0; index < DirectoryBrokerLimits::MaximumRequestsPerSecondPerMaster; ++index)
		{
			QVERIFY(plugin.handleFeatureMessage(server, firstContext, message));
			completePendingReplies();
		}
		QCOMPARE(rateLimitedReplies(), 0);
	}

	// forwarded requests are marked as such, a master can only continue its last few own cursors
	void featureCursorOwnership()
	{
		DirectoryBrokerFeaturePlugin plugin;
		TestServer server;
		server.client.setAuthType(RfbVeyonAuth::KeyFile);
		server.client.setAccessControlState(VncServerClient::AccessControlState::Successful);
		QBuffer device;
		device.open(QBuffer::ReadWrite);
		QObject firstConnection;
		QObject secondConnection;
		const MessageContext firstContext(&device, &firstConnection);
		const MessageContext secondContext(&device, &secondConnection);

		int nextCursor = 0;
		QList<QVariantMap> forwardedArguments;
		plugin.setForwardFunction([&](const QString& command, const QVariantMap& arguments,
									  const DirectoryBrokerClient::ReplyHandler& replyHandler) {
			forwardedArguments.append(arguments);
			replyHandler({1, command, {{Key::Groups, QStringList{}}, {Key::NextCursor, QStringLiteral("%1").arg(++nextCursor, 32, 10, QLatin1Char('0'))}}});
		});
		qint64 now = 1000;
		plugin.setClock([&now]() { return now; });

		const auto send = [&](const MessageContext& context, const QVariantMap& arguments) {
			now += 200;
			FeatureMessage message{DirectoryBrokerMessage::featureUid()};
			DirectoryBrokerMessage::toFeatureMessage(Request{1, QString(Command::UserGroupsList), arguments}, message);
			server.replies.clear();
			(void)plugin.handleFeatureMessage(server, context, message);
			return server.replies.value(0);
		};

		// a master cannot claim to be the session server
		const auto first = send(firstContext, {{Key::Forwarded, false}});
		QCOMPARE(forwardedArguments.last().value(Key::Forwarded).toBool(), true);
		const auto firstCursor = first.arguments.value(Key::NextCursor).toString();

		// other connections cannot use the cursor
		QCOMPARE(send(secondContext, {{Key::Cursor, firstCursor}}).errorCode(), QString(Error::BadRequest));
		QVERIFY(send(firstContext, {{Key::Cursor, firstCursor}}).isError() == false);

		// only the last MaximumCursorsPerMaster cursors of a connection are valid
		QStringList cursors;
		for (int index = 0; index < DirectoryBrokerLimits::MaximumCursorsPerMaster + 1; ++index)
		{
			cursors.append(send(firstContext, {}).arguments.value(Key::NextCursor).toString());
		}
		QCOMPARE(send(firstContext, {{Key::Cursor, cursors.first()}}).errorCode(), QString(Error::BadRequest));
		QVERIFY(send(firstContext, {{Key::Cursor, cursors.last()}}).isError() == false);
	}
};


QTEST_GUILESS_MAIN(DirectoryBrokerTest)
#include "DirectoryBrokerTest.moc"
