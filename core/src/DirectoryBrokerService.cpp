// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDeadlineTimer>
#include <QElapsedTimer>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMutex>
#include <QPointer>
#include <QRandomGenerator>
#include <QTimer>

#include "DirectoryBrokerService.h"
#include "NetworkObjectDirectory.h"
#include "UserGroupsBackendInterface.h"

#include <algorithm>
#include <deque>


using namespace DirectoryBrokerProtocol;

// cancellation flag and execution deadline of the request being executed in the directory worker
// thread
static thread_local const std::atomic<bool>* currentRequestCancelled = nullptr;
static thread_local QDeadlineTimer currentRequestDeadline;



// Objects returned to one client (session server, own or forwarded requests), addressable by UID in
// its later requests; least recently used objects are dropped first
class DirectoryBrokerKnownObjects
{
public:
	explicit DirectoryBrokerKnownObjects(int maximumObjects = 0) :
		m_maximumObjects(maximumObjects)
	{
	}

	void insert(const NetworkObject& object)
	{
		const auto it = m_objects.find(object.uid());
		if (it != m_objects.end())
		{
			m_useOrder.erase(it->second.lastUse);
			m_objects.erase(it);
		}

		while (int(m_objects.size()) >= m_maximumObjects && m_useOrder.empty() == false)
		{
			m_objects.erase(m_useOrder.begin()->second);
			m_useOrder.erase(m_useOrder.begin());
		}

		const auto use = m_nextUse++;
		m_objects[object.uid()] = {object, use};
		m_useOrder[use] = object.uid();
	}

	std::optional<NetworkObject> find(NetworkObject::Uid uid)
	{
		const auto it = m_objects.find(uid);
		if (it == m_objects.end())
		{
			return {};
		}

		m_useOrder.erase(it->second.lastUse);
		it->second.lastUse = m_nextUse++;
		m_useOrder[it->second.lastUse] = uid;

		return it->second.object;
	}

private:
	struct Entry
	{
		NetworkObject object;
		quint64 lastUse;
	};

	int m_maximumObjects;
	std::map<NetworkObject::Uid, Entry> m_objects;
	std::map<quint64, NetworkObject::Uid> m_useOrder;
	quint64 m_nextUse = 0;
};



// Owns the directory and the user-groups backend and executes the queued requests in the directory
// worker thread. Requests are queued per client, own requests of all clients are taken round-robin
// before the forwarded ones.
class DirectoryBrokerWorker : public QObject
{
public:
	using Reply = DirectoryBrokerMessage::Reply;
	using ResultHandler = std::function<void(quint64 jobId, const Reply& result)>;

	struct Job
	{
		quint64 jobId = 0;
		QString clientId;
		DirectoryBrokerMessage::Request request;
		std::shared_ptr<std::atomic<bool>> cancelled;
		quint64 serial = 0;
	};

	DirectoryBrokerWorker(const DirectoryBrokerService::DirectoryFactory& directoryFactory,
						  const DirectoryBrokerService::UserGroupsBackendFactory& userGroupsBackendFactory,
						  const ResultHandler& resultHandler) :
		m_directoryFactory(directoryFactory),
		m_userGroupsBackendFactory(userGroupsBackendFactory),
		m_resultHandler(resultHandler)
	{
	}

	~DirectoryBrokerWorker() override
	{
		delete m_directory;
	}

	// called in the service thread
	void enqueue(Job job)
	{
		const auto forwarded = DirectoryBrokerMessage::isForwarded(job.request);

		m_queueMutex.lock();
		job.serial = m_nextSerial++;
		auto& queues = forwarded ? m_forwardedQueues : m_ownQueues;
		auto& order = forwarded ? m_forwardedOrder : m_ownOrder;
		auto& queue = queues[job.clientId];
		if (queue.empty())
		{
			order.push_back(job.clientId);
		}
		queue.push_back(job);
		m_queueMutex.unlock();

		QMetaObject::invokeMethod(this, [this]() { processNextJob(); }, Qt::QueuedConnection);
	}

	// called in the service thread
	void removeClient(const QString& clientId)
	{
		QMutexLocker locker(&m_queueMutex);
		for (auto* queues : {&m_ownQueues, &m_forwardedQueues})
		{
			queues->remove(clientId);
		}
		for (auto* order : {&m_ownOrder, &m_forwardedOrder})
		{
			order->erase(std::remove(order->begin(), order->end(), clientId), order->end());
		}
		m_removedClients.append(clientId);
	}

	void setBackendRetryInterval(int interval)
	{
		m_backendRetryInterval = interval;
	}

	void setForwardedExecutionBudget(int budget)
	{
		m_forwardedExecutionBudget = budget;
	}

private:
	using Queues = QHash<QString, std::deque<Job>>;

	static bool takeJob(Queues& queues, std::deque<QString>& order, Job& job)
	{
		if (order.empty())
		{
			return false;
		}

		const auto clientId = order.front();
		order.pop_front();
		auto& queue = queues[clientId]; // clazy:exclude=detaching-member
		job = queue.front();
		queue.pop_front();
		if (queue.empty())
		{
			queues.remove(clientId);
		}
		else
		{
			// round-robin: the next job of this client after the jobs of all other clients
			order.push_back(clientId);
		}

		return true;
	}

	void processNextJob()
	{
		Job job;

		m_queueMutex.lock();
		for (const auto& clientId : std::as_const(m_removedClients))
		{
			m_knownObjects.remove(knownObjectsKey(clientId, false));
			m_knownObjects.remove(knownObjectsKey(clientId, true));
		}
		m_removedClients.clear();
		const auto hasJob = takeJob(m_ownQueues, m_ownOrder, job) || takeJob(m_forwardedQueues, m_forwardedOrder, job);
		m_queueMutex.unlock();

		if (hasJob == false || job.cancelled->load())
		{
			return;
		}

		// forwarded requests get a shorter execution budget, as they delay the session servers'
		// own requests (access control) queued behind them
		currentRequestCancelled = job.cancelled.get();
		currentRequestDeadline = DirectoryBrokerMessage::isForwarded(job.request) ? QDeadlineTimer(m_forwardedExecutionBudget) : QDeadlineTimer(QDeadlineTimer::Forever);
		const auto result = execute(job);
		currentRequestCancelled = nullptr;

		m_resultHandler(job.jobId, result);
	}

	Reply execute(const Job& job)
	{
		const auto& request = job.request;

		if (request.command.startsWith(QLatin1String("UserGroups.")))
		{
			if (m_userGroupsBackend == nullptr && isRetryDue(m_userGroupsBackendFailure))
			{
				m_userGroupsBackend = m_userGroupsBackendFactory ? m_userGroupsBackendFactory() : nullptr;
				if (m_userGroupsBackend == nullptr)
				{
					m_userGroupsBackendFailure.start();
				}
			}

			if (m_userGroupsBackend == nullptr)
			{
				return unavailable(request, QStringLiteral("no user groups backend"));
			}

			return executeUserGroupsRequest(request);
		}

		if (m_directory == nullptr && isRetryDue(m_directoryFailure))
		{
			m_directory = m_directoryFactory ? m_directoryFactory() : nullptr;
			if (m_directory == nullptr)
			{
				m_directoryFailure.start();
			}
		}

		if (m_directory == nullptr)
		{
			return unavailable(request, QStringLiteral("no directory"));
		}

		return executeDirectoryRequest(job);
	}

	bool isRetryDue(const QElapsedTimer& failure) const
	{
		return failure.isValid() == false || failure.hasExpired(m_backendRetryInterval);
	}

	Reply executeUserGroupsRequest(const DirectoryBrokerMessage::Request& request)
	{
		const auto& arguments = request.arguments;
		const auto queryDomainGroups = arguments.value(Key::QueryDomainGroups).toBool();

		if (request.command == Command::UserGroupsList)
		{
			return DirectoryBrokerMessage::success(request, {{Key::Groups, m_userGroupsBackend->userGroups(queryDomainGroups)}});
		}

		if (request.command == Command::UserGroupsGroupsOfUser)
		{
			const auto groups = m_userGroupsBackend->tryQueryGroupsOfUser(arguments.value(Key::Username).toString(),
																		  queryDomainGroups);
			if (groups.has_value() == false)
			{
				return unavailable(request, QStringLiteral("groups of user could not be determined"));
			}
			return DirectoryBrokerMessage::success(request, {{Key::Groups, *groups}});
		}

		if (request.command == Command::UserGroupsGroupSecurityIdentifier)
		{
			return DirectoryBrokerMessage::success(
				request, {{Key::Sid, m_userGroupsBackend->userGroupSecurityIdentifier(arguments.value(Key::GroupName).toString())}});
		}

		return DirectoryBrokerMessage::error(request.requestId, Error::BadRequest, QStringLiteral("unknown command"));
	}

	Reply executeDirectoryRequest(const Job& job)
	{
		const auto& request = job.request;
		const auto& arguments = request.arguments;
		const auto forwarded = DirectoryBrokerMessage::isForwarded(request);
		auto& knownObjects = this->knownObjects(job.clientId, forwarded);

		if (request.command == Command::DirectoryUpdate)
		{
			return updateReply(job, knownObjects, true);
		}

		if (request.command == Command::DirectoryQueryObjects)
		{
			const auto type = DirectoryBrokerMessage::typeFromName(arguments.value(Key::Type));
			const auto attribute = DirectoryBrokerMessage::attributeFromName(arguments.value(Key::Attribute));
			// the value is used literally (e.g. host names are never resolved in the service)
			const auto objects = m_directory->tryQueryObjects(*type, *attribute, arguments.value(Key::Value).toString());
			if (objects.has_value() == false)
			{
				return unavailable(request, QStringLiteral("directory could not be queried"));
			}
			return objectsReply(request, *objects, knownObjects);
		}

		// the remaining commands address an object returned to this client before
		const auto objectUid = *DirectoryBrokerMessage::uidFromVariant(arguments.value(Key::ObjectUid));
		const auto object = objectUid == m_directory->rootObject().uid() ? m_directory->rootObject() : knownObjects.find(objectUid);
		if (object.has_value() == false)
		{
			return DirectoryBrokerMessage::error(request.requestId, Error::BadRequest, QStringLiteral("unknown object"));
		}

		if (request.command == Command::DirectoryFetchObjects)
		{
			if (object->type() == NetworkObject::Type::Root)
			{
				return updateReply(job, knownObjects, false);
			}
			if (object->isContainer())
			{
				m_directory->fetchObjects(*object);
				if (m_directory->hasUpdateFailed())
				{
					return unavailable(request, QStringLiteral("objects could not be fetched"));
				}
			}
			return objectsReply(request, m_directory->objects(*object), knownObjects);
		}

		if (request.command == Command::DirectoryQueryParents)
		{
			const auto parents = m_directory->tryQueryParents(*object);
			if (parents.has_value() == false)
			{
				return unavailable(request, QStringLiteral("directory could not be queried"));
			}
			return objectsReply(request, *parents, knownObjects);
		}

		if (request.command == Command::DirectoryQueryObjectAttribute)
		{
			const auto attribute = *DirectoryBrokerMessage::attributeFromName(arguments.value(Key::Attribute));
			if (attribute != NetworkObject::Attribute::Name && attribute != NetworkObject::Attribute::HostAddress &&
				attribute != NetworkObject::Attribute::MacAddress &&
				(attribute != NetworkObject::Attribute::DirectoryAddress || forwarded))
			{
				return DirectoryBrokerMessage::error(request.requestId, Error::BadRequest, QStringLiteral("unsupported attribute"));
			}

			auto value = m_directory->queryObjectAttribute(objectUid, attribute);
			if (value.isValid() == false)
			{
				value = object->attributeValue(attribute);
			}
			return DirectoryBrokerMessage::success(request, {{Key::Value, value.toString()}});
		}

		return DirectoryBrokerMessage::error(request.requestId, Error::BadRequest, QStringLiteral("unknown command"));
	}

	// requests that were queued while an update ran are answered with its result
	Reply updateReply(const Job& job, DirectoryBrokerKnownObjects& knownObjects, bool withGeneration)
	{
		if (job.serial >= m_lastUpdateFinishSerial)
		{
			m_directory->update();

			// an aborted or failed update is incomplete: neither reply with it nor reuse it for
			// coalesced requests (masters would drop the missing computers)
			if (DirectoryBrokerService::isCurrentRequestCancelled())
			{
				return DirectoryBrokerMessage::error(job.request.requestId, Error::Timeout, QStringLiteral("update aborted"));
			}
			if (m_directory->hasUpdateFailed())
			{
				return unavailable(job.request, QStringLiteral("directory update failed"));
			}
			m_lastUpdateObjects = m_directory->objects(m_directory->rootObject());
			++m_generation;

			QMutexLocker locker(&m_queueMutex);
			m_lastUpdateFinishSerial = m_nextSerial;
		}

		auto reply = objectsReply(job.request, m_lastUpdateObjects, knownObjects);
		if (withGeneration)
		{
			reply.arguments[Key::Generation] = m_generation;
		}

		return reply;
	}

	static Reply objectsReply(const DirectoryBrokerMessage::Request& request, const NetworkObjectList& objects,
							  DirectoryBrokerKnownObjects& knownObjects)
	{
		if (objects.size() > DirectoryBrokerLimits::MaximumResultItems)
		{
			return DirectoryBrokerMessage::error(request.requestId, Error::TooLarge, QStringLiteral("result too large"));
		}

		const auto forwarded = DirectoryBrokerMessage::isForwarded(request);

		QVariantList list;
		list.reserve(objects.size());
		for (const auto& object : objects)
		{
			knownObjects.insert(object);

			auto map = DirectoryBrokerMessage::objectToVariant(object).toMap();
			if (forwarded)
			{
				// masters address objects by UID, full DNs are not needed there
				map.remove(QStringLiteral("DirectoryAddress"));
			}
			list.append(map);
		}

		return DirectoryBrokerMessage::success(request, {{Key::Objects, list}});
	}

	static QString knownObjectsKey(const QString& clientId, bool forwarded)
	{
		return forwarded ? QStringLiteral("F:") + clientId : QStringLiteral("O:") + clientId;
	}

	DirectoryBrokerKnownObjects& knownObjects(const QString& clientId, bool forwarded)
	{
		const auto key = knownObjectsKey(clientId, forwarded);
		auto it = m_knownObjects.find(key);
		if (it == m_knownObjects.end())
		{
			it = m_knownObjects.insert(key, DirectoryBrokerKnownObjects(forwarded ? DirectoryBrokerLimits::MaximumKnownForwardedObjects : DirectoryBrokerLimits::MaximumKnownOwnObjects));
		}
		return it.value();
	}

	static Reply unavailable(const DirectoryBrokerMessage::Request& request, const QString& message)
	{
		vWarning() << request.command << "failed:" << message;
		return DirectoryBrokerMessage::error(request.requestId, Error::Unavailable, message);
	}

	const DirectoryBrokerService::DirectoryFactory m_directoryFactory;
	const DirectoryBrokerService::UserGroupsBackendFactory m_userGroupsBackendFactory;
	const ResultHandler m_resultHandler;
	int m_backendRetryInterval = DirectoryBrokerLimits::BackendRetryInterval;
	int m_forwardedExecutionBudget = DirectoryBrokerLimits::ForwardedExecutionBudget;

	QMutex m_queueMutex;
	Queues m_ownQueues;
	Queues m_forwardedQueues;
	std::deque<QString> m_ownOrder;
	std::deque<QString> m_forwardedOrder;
	QStringList m_removedClients;
	quint64 m_nextSerial = 1;
	quint64 m_lastUpdateFinishSerial = 0;

	// accessed in the worker thread only
	NetworkObjectDirectory* m_directory = nullptr;
	QElapsedTimer m_directoryFailure;
	UserGroupsBackendInterface* m_userGroupsBackend = nullptr;
	QElapsedTimer m_userGroupsBackendFailure;
	QHash<QString, DirectoryBrokerKnownObjects> m_knownObjects;
	NetworkObjectList m_lastUpdateObjects;
	int m_generation = 0;
};



DirectoryBrokerService::DirectoryBrokerService(const DirectoryFactory& directoryFactory,
											   const UserGroupsBackendFactory& userGroupsBackendFactory,
											   const Clock& clock, QObject* parent) :
	QObject(parent),
	m_clock(clock),
	m_worker(nullptr)
{
	const QPointer<DirectoryBrokerService> service(this);

	m_worker = new DirectoryBrokerWorker(directoryFactory, userGroupsBackendFactory,
										 [service](quint64 jobId, const Reply& result) {
		if (service)
		{
			QMetaObject::invokeMethod(service, [service, jobId, result]() {
				if (service)
				{
					service->finishRequest(jobId, result);
				}
			}, Qt::QueuedConnection);
		}
	});

	m_workerThread.setObjectName(QStringLiteral("DirectoryWorker"));
	m_worker->moveToThread(&m_workerThread);
	connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
	m_workerThread.start();
}



DirectoryBrokerService::~DirectoryBrokerService()
{
	for (auto& pendingRequest : m_pendingRequests)
	{
		pendingRequest.second.cancelled->store(true);
	}

	m_workerThread.quit();
	m_workerThread.wait();
}



void DirectoryBrokerService::setBackendRetryInterval(int interval)
{
	QMetaObject::invokeMethod(m_worker, [this, interval]() { m_worker->setBackendRetryInterval(interval); },
							  Qt::BlockingQueuedConnection);
}



void DirectoryBrokerService::handleRequest(const QString& clientId, const Request& request, const ReplyHandler& replyHandler)
{
	auto& client = m_clients[clientId];
	const auto forwarded = DirectoryBrokerMessage::isForwarded(request);
	auto& limiter = client.limiter(forwarded);

	if (limiter.acceptRequest(m_clock()) == false)
	{
		vWarning() << "request rate of" << clientId << (forwarded ? "(forwarded)" : "") << "exceeded";
		replyHandler(DirectoryBrokerMessage::error(request.requestId, Error::RateLimited, QStringLiteral("request rate exceeded")));
		return;
	}

	const auto validationError = DirectoryBrokerMessage::validateRequest(request);
	if (validationError.has_value())
	{
		vWarning() << "invalid request" << request.command << "from" << clientId << validationError->errorMessage();
		replyHandler(*validationError);
		return;
	}

	vDebug() << clientId << request.command << (forwarded ? "(forwarded)" : "");

	if (request.arguments.value(Key::Cursor).toString().isEmpty() == false)
	{
		continueWithCursor(client.cursorPool(forwarded), request, replyHandler);
		return;
	}

	if (limiter.acquireOutstandingRequest() == false)
	{
		vWarning() << "too many outstanding requests of" << clientId << (forwarded ? "(forwarded)" : "");
		replyHandler(DirectoryBrokerMessage::error(request.requestId, Error::RateLimited, QStringLiteral("too many outstanding requests")));
		return;
	}

	const auto jobId = m_nextJobId++;
	const auto cancelled = std::make_shared<std::atomic<bool>>(false);

	m_pendingRequests[jobId] = PendingRequest{clientId, request, replyHandler, cancelled};

	QTimer::singleShot(m_requestTimeout, this, [this, jobId]() { timeoutRequest(jobId); });

	m_worker->enqueue({jobId, clientId, request, cancelled, 0});
}



void DirectoryBrokerService::removeClient(const QString& clientId)
{
	m_worker->removeClient(clientId);
	m_clients.erase(clientId);

	for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end();)
	{
		if (it->second.clientId == clientId)
		{
			it->second.cancelled->store(true);
			it = m_pendingRequests.erase(it);
		}
		else
		{
			++it;
		}
	}
}



bool DirectoryBrokerService::isCurrentRequestCancelled()
{
	return currentRequestCancelled && (currentRequestCancelled->load() || currentRequestDeadline.hasExpired());
}



void DirectoryBrokerService::setForwardedExecutionBudget(int budget)
{
	QMetaObject::invokeMethod(m_worker, [this, budget]() { m_worker->setForwardedExecutionBudget(budget); },
							  Qt::BlockingQueuedConnection);
}



void DirectoryBrokerService::continueWithCursor(CursorPool& pool, const Request& request, const ReplyHandler& replyHandler)
{
	removeExpiredCursors(pool);

	const auto cursorId = request.arguments.value(Key::Cursor).toString();
	const auto it = pool.cursors.find(cursorId);
	if (it == pool.cursors.end() || it->second.command != request.command)
	{
		replyHandler(DirectoryBrokerMessage::error(request.requestId, Error::BadRequest, QStringLiteral("unknown or expired cursor")));
		return;
	}

	replyHandler(nextPage(pool, cursorId, it->second, request.requestId));
}



void DirectoryBrokerService::finishRequest(quint64 jobId, const Reply& result)
{
	const auto it = m_pendingRequests.find(jobId);
	if (it == m_pendingRequests.end())
	{
		// timed out or client removed: discard the late result
		return;
	}

	const auto pendingRequest = it->second;
	m_pendingRequests.erase(it);

	const auto clientIt = m_clients.find(pendingRequest.clientId);
	if (clientIt == m_clients.end())
	{
		return;
	}

	const auto forwarded = DirectoryBrokerMessage::isForwarded(pendingRequest.request);
	clientIt->second.limiter(forwarded).releaseOutstandingRequest();

	pendingRequest.replyHandler(result.isError() ? result : createPagedReply(clientIt->second.cursorPool(forwarded), pendingRequest.request, result));
}



void DirectoryBrokerService::timeoutRequest(quint64 jobId)
{
	const auto it = m_pendingRequests.find(jobId);
	if (it == m_pendingRequests.end())
	{
		return;
	}

	const auto pendingRequest = it->second;
	m_pendingRequests.erase(it);
	pendingRequest.cancelled->store(true);

	vWarning() << "request" << pendingRequest.request.command << "of" << pendingRequest.clientId << "timed out";

	const auto clientIt = m_clients.find(pendingRequest.clientId);
	if (clientIt != m_clients.end())
	{
		clientIt->second.limiter(DirectoryBrokerMessage::isForwarded(pendingRequest.request)).releaseOutstandingRequest();
	}

	pendingRequest.replyHandler(DirectoryBrokerMessage::error(pendingRequest.request.requestId, Error::Timeout));
}



DirectoryBrokerService::Reply DirectoryBrokerService::createPagedReply(CursorPool& pool, const Request& request, const Reply& result)
{
	const auto listKey = DirectoryBrokerMessage::listKey(request.command);
	if (result.arguments.contains(listKey) == false)
	{
		return result;
	}

	Cursor cursor;
	cursor.command = request.command;
	cursor.extraArguments = result.arguments;
	cursor.extraArguments.remove(listKey);
	cursor.items = result.arguments.value(listKey).toList();
	if (cursor.items.size() > DirectoryBrokerLimits::MaximumResultItems)
	{
		return DirectoryBrokerMessage::error(request.requestId, Error::TooLarge, QStringLiteral("result too large"));
	}

	cursor.itemSizes.reserve(cursor.items.size());
	for (const auto& item : std::as_const(cursor.items))
	{
		const auto size = encodedSize(item);
		cursor.itemSizes.append(size);
		cursor.size += size;
	}
	cursor.expiryTime = m_clock() + DirectoryBrokerLimits::CursorLifetime;

	return nextPage(pool, {}, cursor, request.requestId);
}



DirectoryBrokerService::Reply DirectoryBrokerService::nextPage(CursorPool& pool, const QString& cursorId, Cursor& cursor, quint32 requestId)
{
	const auto listKey = DirectoryBrokerMessage::listKey(cursor.command);
	const auto maximumItems = DirectoryBrokerMessage::maximumItemsPerPage(cursor.command);
	const auto availableSize = DirectoryBrokerLimits::MaximumReplyPageSize -
		encodedSize(cursor.extraArguments) - DirectoryBrokerLimits::CursorSize * 2 - 64;

	int itemCount = 0;
	qint64 pageSize = 0;
	while (itemCount < cursor.items.size() && itemCount < maximumItems &&
		   pageSize + cursor.itemSizes.at(itemCount) <= availableSize)
	{
		pageSize += cursor.itemSizes.at(itemCount);
		++itemCount;
	}

	if (itemCount == 0 && cursor.items.isEmpty() == false)
	{
		vWarning() << "directory entry too large for a reply page";
		if (cursorId.isEmpty() == false)
		{
			removeCursor(pool, cursorId);
		}
		return DirectoryBrokerMessage::error(requestId, Error::TooLarge, QStringLiteral("directory entry too large"));
	}

	const auto command = cursor.command;
	auto arguments = cursor.extraArguments;
	arguments[listKey] = cursor.items.mid(0, itemCount);

	const auto remainingItems = cursor.items.mid(itemCount);
	const auto remainingSize = cursor.size - pageSize;

	if (remainingItems.isEmpty())
	{
		if (cursorId.isEmpty() == false)
		{
			removeCursor(pool, cursorId);
		}
		arguments[Key::NextCursor] = QString{};
		return {requestId, command, arguments};
	}

	if (cursorId.isEmpty() == false)
	{
		cursor.items = remainingItems;
		cursor.itemSizes = cursor.itemSizes.mid(itemCount);
		cursor.size = remainingSize;
		pool.memory -= pageSize;

		arguments[Key::NextCursor] = cursorId;
		return {requestId, command, arguments};
	}

	// new cursor for the remaining items of a fresh result
	if (remainingSize > pool.maximumMemory)
	{
		return DirectoryBrokerMessage::error(requestId, Error::TooLarge, QStringLiteral("result too large"));
	}

	removeExpiredCursors(pool);

	// a full pool drops its oldest cursor (e.g. one abandoned by a master) instead of failing
	while (pool.cursors.empty() == false &&
		   (int(pool.cursors.size()) >= pool.maximumCursors || pool.memory + remainingSize > pool.maximumMemory))
	{
		const auto oldest = std::min_element(pool.cursors.begin(), pool.cursors.end(),
											 [](const auto& a, const auto& b) { return a.second.serial < b.second.serial; });
		vDebug() << "evicting oldest cursor";
		removeCursor(pool, oldest->first);
	}

	QString newCursorId;
	do
	{
		QByteArray random(DirectoryBrokerLimits::CursorSize, Qt::Uninitialized);
		QRandomGenerator::system()->generate(random.begin(), random.end());
		newCursorId = QString::fromLatin1(random.toHex());
	} while (pool.cursors.find(newCursorId) != pool.cursors.end());

	pool.cursors[newCursorId] = Cursor{command, cursor.extraArguments, remainingItems, cursor.itemSizes.mid(itemCount),
									   remainingSize, cursor.expiryTime, m_nextCursorSerial++};
	pool.memory += remainingSize;

	arguments[Key::NextCursor] = newCursorId;

	return {requestId, command, arguments};
}



void DirectoryBrokerService::removeExpiredCursors(CursorPool& pool)
{
	const auto now = m_clock();

	for (auto it = pool.cursors.begin(); it != pool.cursors.end();)
	{
		if (now >= it->second.expiryTime)
		{
			pool.memory -= it->second.size;
			it = pool.cursors.erase(it);
		}
		else
		{
			++it;
		}
	}
}



void DirectoryBrokerService::removeCursor(CursorPool& pool, const QString& cursorId)
{
	const auto it = pool.cursors.find(cursorId);
	if (it != pool.cursors.end())
	{
		pool.memory -= it->second.size;
		pool.cursors.erase(it);
	}
}



int DirectoryBrokerService::encodedSize(const QVariant& item)
{
	// size within the JSON reply incl. the separating comma
	return int(QJsonDocument(QJsonArray{QJsonValue::fromVariant(item)}).toJson(QJsonDocument::Compact).size()) - 1;
}
