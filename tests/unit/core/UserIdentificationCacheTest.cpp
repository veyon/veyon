// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QBuffer>
#include <QtTest>

#include "UserIdentificationCache.h"

class UserIdentificationCacheTest : public QObject
{
	Q_OBJECT

	using RequestResult = UserIdentificationCache::RequestResult;

private Q_SLOTS:
	void cachesEnteredIdentity();
	void answersAllPendingRequestsOnce();
	void emptyIdentityIsNotCached();
	void unsolicitedReplyIsIgnored();
	void sessionUserChangeInvalidatesIdentity();
	void restartsDialogIfWorkerIsGone();
	void dropsClosedConnections();
	void forceAsksAgainDespiteCachedIdentity();
	void forceIsIgnoredForUnidentifiedSession();
	void forceWhileDialogIsOpen();
	void forceCancelledLeavesNothingCached();
};



void UserIdentificationCacheTest::cachesEnteredIdentity()
{
	UserIdentificationCache cache;
	QBuffer master1;
	QBuffer master2;

	QCOMPARE(cache.addRequest(MessageContext{&master1}, QStringLiteral("guest"), false), RequestResult::StartDialog);
	QVERIFY(cache.isRequestPending());
	QVERIFY(cache.requestId().isNull() == false);

	const auto requests = cache.complete(QStringLiteral("Jane Doe"), QStringLiteral("guest"));
	QCOMPARE(requests.size(), 1);
	QCOMPARE(requests.first().ioDevice(), &master1);
	QVERIFY(cache.isRequestPending() == false);

	// a new connection (e.g. after restarting the master) gets the cached identity
	QCOMPARE(cache.addRequest(MessageContext{&master2}, QStringLiteral("guest"), false), RequestResult::Cached);
	QCOMPARE(cache.identity(), QStringLiteral("Jane Doe"));
	QCOMPARE(cache.addRequest(MessageContext{&master1}, QStringLiteral("guest"), false), RequestResult::Cached);
	QVERIFY(cache.isRequestPending() == false);
}



void UserIdentificationCacheTest::answersAllPendingRequestsOnce()
{
	UserIdentificationCache cache;
	QBuffer master1;
	QBuffer master2;

	QCOMPARE(cache.addRequest(MessageContext{&master1}, QStringLiteral("guest"), false), RequestResult::StartDialog);
	const auto requestId = cache.requestId();

	// while the dialog is open, further requests do not open another one
	QCOMPARE(cache.addRequest(MessageContext{&master2}, QStringLiteral("guest"), true), RequestResult::Pending);
	QCOMPARE(cache.addRequest(MessageContext{&master1}, QStringLiteral("guest"), true), RequestResult::Pending);
	QCOMPARE(cache.addRequest(MessageContext{&master2}, QStringLiteral("guest"), true), RequestResult::Pending);
	QCOMPARE(cache.requestId(), requestId);

	const auto requests = cache.complete(QStringLiteral("Jane Doe"), QStringLiteral("guest"));
	QCOMPARE(requests.size(), 2);
	QCOMPARE(requests.at(0).ioDevice(), &master1);
	QCOMPARE(requests.at(1).ioDevice(), &master2);

	QVERIFY(cache.complete(QStringLiteral("Jane Doe"), QStringLiteral("guest")).isEmpty());
}



void UserIdentificationCacheTest::emptyIdentityIsNotCached()
{
	UserIdentificationCache cache;
	QBuffer master;

	QCOMPARE(cache.addRequest(MessageContext{&master}, QStringLiteral("guest"), false), RequestResult::StartDialog);
	QCOMPARE(cache.complete(QString{}, QStringLiteral("guest")).size(), 1);
	QVERIFY(cache.identity().isEmpty());

	QCOMPARE(cache.addRequest(MessageContext{&master}, QStringLiteral("guest"), false), RequestResult::StartDialog);
}



void UserIdentificationCacheTest::unsolicitedReplyIsIgnored()
{
	UserIdentificationCache cache;
	QBuffer master;

	QVERIFY(cache.complete(QStringLiteral("Mallory"), QStringLiteral("guest")).isEmpty());
	QVERIFY(cache.identity().isEmpty());
	QCOMPARE(cache.addRequest(MessageContext{&master}, QStringLiteral("guest"), false), RequestResult::StartDialog);
}



void UserIdentificationCacheTest::sessionUserChangeInvalidatesIdentity()
{
	UserIdentificationCache cache;
	QBuffer master;

	cache.addRequest(MessageContext{&master}, QStringLiteral("guest"), false);
	cache.complete(QStringLiteral("Jane Doe"), QStringLiteral("guest"));

	QCOMPARE(cache.addRequest(MessageContext{&master}, QStringLiteral("guest2"), false), RequestResult::StartDialog);
	QVERIFY(cache.identity().isEmpty());

	// identity is not returned for the previous user either
	cache.complete(QString{}, QStringLiteral("guest2"));
	QCOMPARE(cache.addRequest(MessageContext{&master}, QStringLiteral("guest"), false), RequestResult::StartDialog);
}



void UserIdentificationCacheTest::restartsDialogIfWorkerIsGone()
{
	UserIdentificationCache cache;
	QBuffer master1;
	QBuffer master2;

	QCOMPARE(cache.addRequest(MessageContext{&master1}, QStringLiteral("guest"), false), RequestResult::StartDialog);
	const auto requestId = cache.requestId();

	QCOMPARE(cache.addRequest(MessageContext{&master2}, QStringLiteral("guest"), false), RequestResult::StartDialog);
	QVERIFY(cache.requestId() != requestId);

	QCOMPARE(cache.complete(QStringLiteral("Jane Doe"), QStringLiteral("guest")).size(), 2);
}



void UserIdentificationCacheTest::dropsClosedConnections()
{
	UserIdentificationCache cache;
	QBuffer master1;
	auto master2 = new QBuffer;

	cache.addRequest(MessageContext{master2}, QStringLiteral("guest"), false);
	delete master2;

	QCOMPARE(cache.addRequest(MessageContext{&master1}, QStringLiteral("guest"), true), RequestResult::Pending);
	QCOMPARE(cache.addRequest(MessageContext{nullptr}, QStringLiteral("guest"), true), RequestResult::Pending);

	const auto requests = cache.complete(QStringLiteral("Jane Doe"), QStringLiteral("guest"));
	QCOMPARE(requests.size(), 1);
	QCOMPARE(requests.first().ioDevice(), &master1);
}



void UserIdentificationCacheTest::forceAsksAgainDespiteCachedIdentity()
{
	UserIdentificationCache cache;
	QBuffer master1;
	QBuffer master2;

	cache.addRequest(MessageContext{&master1}, QStringLiteral("guest"), false);
	cache.complete(QStringLiteral("Jane Doe"), QStringLiteral("guest"));

	QCOMPARE(cache.addRequest(MessageContext{&master2}, QStringLiteral("guest"), false, true),
			 RequestResult::StartDialog);
	QVERIFY(cache.identity().isEmpty());

	// automatic identification by another master meanwhile waits for the dialog as well
	QCOMPARE(cache.addRequest(MessageContext{&master1}, QStringLiteral("guest"), true), RequestResult::Pending);

	QCOMPARE(cache.complete(QStringLiteral("John Doe"), QStringLiteral("guest")).size(), 2);
	QCOMPARE(cache.addRequest(MessageContext{&master1}, QStringLiteral("guest"), false), RequestResult::Cached);
	QCOMPARE(cache.identity(), QStringLiteral("John Doe"));
}



void UserIdentificationCacheTest::forceIsIgnoredForUnidentifiedSession()
{
	UserIdentificationCache cache;
	QBuffer master;

	QCOMPARE(cache.addRequest(MessageContext{&master}, QStringLiteral("user"), false, true), RequestResult::Ignored);
	QVERIFY(cache.isRequestPending() == false);
	QVERIFY(cache.complete(QStringLiteral("Mallory"), QStringLiteral("user")).isEmpty());

	// a session identified before is no longer identified once the session user changes
	cache.addRequest(MessageContext{&master}, QStringLiteral("guest"), false);
	cache.complete(QStringLiteral("Jane Doe"), QStringLiteral("guest"));
	QCOMPARE(cache.addRequest(MessageContext{&master}, QStringLiteral("user"), false, true), RequestResult::Ignored);
	QVERIFY(cache.identity().isEmpty());
}



void UserIdentificationCacheTest::forceWhileDialogIsOpen()
{
	UserIdentificationCache cache;
	QBuffer master1;
	QBuffer master2;

	QCOMPARE(cache.addRequest(MessageContext{&master1}, QStringLiteral("guest"), false), RequestResult::StartDialog);
	const auto requestId = cache.requestId();

	// no second dialog - the open one answers the forced request as well
	QCOMPARE(cache.addRequest(MessageContext{&master2}, QStringLiteral("guest"), true, true), RequestResult::Pending);
	QCOMPARE(cache.requestId(), requestId);

	QCOMPARE(cache.complete(QStringLiteral("Jane Doe"), QStringLiteral("guest")).size(), 2);
	QCOMPARE(cache.identity(), QStringLiteral("Jane Doe"));
}



void UserIdentificationCacheTest::forceCancelledLeavesNothingCached()
{
	UserIdentificationCache cache;
	QBuffer master;

	cache.addRequest(MessageContext{&master}, QStringLiteral("guest"), false);
	cache.complete(QStringLiteral("Jane Doe"), QStringLiteral("guest"));

	QCOMPARE(cache.addRequest(MessageContext{&master}, QStringLiteral("guest"), false, true),
			 RequestResult::StartDialog);
	QCOMPARE(cache.complete(QString{}, QStringLiteral("guest")).size(), 1);

	QCOMPARE(cache.addRequest(MessageContext{&master}, QStringLiteral("guest"), false), RequestResult::StartDialog);

	// still a known guest session, so re-identification remains possible
	cache.complete(QString{}, QStringLiteral("guest"));
	QCOMPARE(cache.addRequest(MessageContext{&master}, QStringLiteral("guest"), false, true),
			 RequestResult::StartDialog);
}



QTEST_GUILESS_MAIN(UserIdentificationCacheTest)
#include "UserIdentificationCacheTest.moc"
