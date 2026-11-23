// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtTest>

#include "AuthenticationLimiter.h"

#include <thread>
#include <vector>

// Limits of the password checks of the service's D-Bus method Authenticate
class AuthenticationLimiterTest : public QObject
{
	Q_OBJECT
private:
	using Rejection = AuthenticationLimiter::Rejection;

	qint64 m_now = 0;

	std::shared_ptr<AuthenticationLimiter> createLimiter()
	{
		return AuthenticationLimiter::create([this]() { return m_now; });
	}

	static Rejection rejectionOf(AuthenticationLimiter& limiter, const QString& key,
								 const QString& username = QStringLiteral("user"))
	{
		auto rejection = Rejection::Admitted;
		const auto permit = limiter.admit(key, username, &rejection);
		return permit ? Rejection::Admitted : rejection;
	}

	// one check of username by key with the given result
	static void check(AuthenticationLimiter& limiter, const QString& key, const QString& username, bool authenticated)
	{
		const auto permit = limiter.admit(key, username);
		QVERIFY(permit);
		permit->finish(authenticated);
	}

	static void fail(AuthenticationLimiter& limiter, const QString& key, const QString& username = QStringLiteral("user"))
	{
		check(limiter, key, username, false);
	}

private Q_SLOTS:
	void init()
	{
		m_now = 100000;
	}

	void oneOutstandingRequestPerCaller()
	{
		const auto limiter = createLimiter();
		const auto key = QStringLiteral("uid:1000");

		auto permit = limiter->admit(key, QStringLiteral("user"));
		QVERIFY(permit);
		QCOMPARE(limiter->runningCount(), 1);

		auto rejection = Rejection::Admitted;
		QVERIFY(limiter->admit(key, QStringLiteral("user"), &rejection) == nullptr);
		QCOMPARE(rejection, Rejection::Busy);

		// other callers are independent
		QVERIFY(limiter->admit(QStringLiteral("uid:1001"), QStringLiteral("user")));

		permit->finish(true);
		QCOMPARE(limiter->runningCount(), 0);
		QVERIFY(limiter->admit(key, QStringLiteral("user")));
	}

	void exponentialBackoffAfterFailures()
	{
		const auto limiter = createLimiter();
		const auto key = QStringLiteral("uid:0");

		const QList<qint64> expectedBackoffs{1000, 2000, 4000, 8000, 16000, 30000, 30000, 30000};
		for (const auto backoff : expectedBackoffs)
		{
			fail(*limiter, key);

			m_now += backoff - 1;
			QCOMPARE(rejectionOf(*limiter, key), Rejection::BackingOff);

			m_now += 1;
			QCOMPARE(rejectionOf(*limiter, key), Rejection::Admitted);
		}

		// many failures: no overflow of the exponent, still capped
		for (int i = 0; i < 100; ++i)
		{
			fail(*limiter, key);
			m_now += AuthenticationLimiter::MaximumFailureBackoff;
		}
		fail(*limiter, key);
		m_now += AuthenticationLimiter::MaximumFailureBackoff - 1;
		QCOMPARE(rejectionOf(*limiter, key), Rejection::BackingOff);
	}

	void successResetsBackoff()
	{
		const auto limiter = createLimiter();
		const auto key = QStringLiteral("uid:1000");

		fail(*limiter, key);
		m_now += 1000;
		fail(*limiter, key);
		m_now += 2000;
		fail(*limiter, key);
		m_now += 4000;

		auto permit = limiter->admit(key, QStringLiteral("user"));
		QVERIFY(permit);
		permit->finish(true);

		// back to the initial backoff
		fail(*limiter, key);
		m_now += AuthenticationLimiter::InitialFailureBackoff;
		QCOMPARE(rejectionOf(*limiter, key), Rejection::Admitted);
	}

	// F1: a caller who knows one valid account alternates guesses for a victim with successful
	// checks of its own account; the successes must not reset the backoff of the guesses
	void successForOtherAccountDoesNotReset()
	{
		const auto limiter = createLimiter();
		const auto key = QStringLiteral("uid:0");
		const auto victim = QStringLiteral("victim");
		const auto self = QStringLiteral("student");

		qint64 backoff = AuthenticationLimiter::InitialFailureBackoff;
		for (int round = 0; round < 10; ++round)
		{
			fail(*limiter, key, victim);
			m_now += backoff - 1;
			QCOMPARE(rejectionOf(*limiter, key, self), Rejection::BackingOff);
			m_now += 1;

			check(*limiter, key, self, true);

			// still backing off at the current level: the next guess doubles it
			backoff = qMin<qint64>(backoff * 2, AuthenticationLimiter::MaximumFailureBackoff);
		}

		QCOMPARE(backoff, qint64(AuthenticationLimiter::MaximumFailureBackoff));
		fail(*limiter, key, victim);
		m_now += AuthenticationLimiter::MaximumFailureBackoff - 1;
		QCOMPARE(rejectionOf(*limiter, key, victim), Rejection::BackingOff);
	}

	// the same with the own account checked first: a success before any failure resets nothing
	// that matters, failures for two names are never reset by a success
	void mixedFailuresAreNotReset()
	{
		const auto limiter = createLimiter();
		const auto key = QStringLiteral("uid:1000");

		fail(*limiter, key, QStringLiteral("a"));
		m_now += AuthenticationLimiter::InitialFailureBackoff;
		fail(*limiter, key, QStringLiteral("b"));
		m_now += 2 * AuthenticationLimiter::InitialFailureBackoff;

		// "a" succeeds, but "b" failed as well
		check(*limiter, key, QStringLiteral("a"), true);
		fail(*limiter, key, QStringLiteral("a"));
		m_now += 4 * AuthenticationLimiter::InitialFailureBackoff - 1;
		QCOMPARE(rejectionOf(*limiter, key, QStringLiteral("a")), Rejection::BackingOff);

		// decays only through FailureResetInterval
		m_now += AuthenticationLimiter::FailureResetInterval;
		fail(*limiter, key, QStringLiteral("a"));
		m_now += AuthenticationLimiter::InitialFailureBackoff;
		QCOMPARE(rejectionOf(*limiter, key, QStringLiteral("a")), Rejection::Admitted);
	}

	// a typo followed by the right password of the same account still resets
	void typoThenCorrectResets()
	{
		const auto limiter = createLimiter();
		const auto key = QStringLiteral("uid:0");
		const auto teacher = QStringLiteral("teacher");

		fail(*limiter, key, teacher);
		m_now += AuthenticationLimiter::InitialFailureBackoff;
		fail(*limiter, key, teacher);
		m_now += 2 * AuthenticationLimiter::InitialFailureBackoff;
		check(*limiter, key, teacher, true);

		// reset: the next failure starts with the initial backoff, for any name
		fail(*limiter, key, QStringLiteral("other"));
		m_now += AuthenticationLimiter::InitialFailureBackoff;
		QCOMPARE(rejectionOf(*limiter, key, teacher), Rejection::Admitted);
	}

	// F4: a check without verdict (helper error, timeout) is neither failure nor success
	void finishWithoutResult()
	{
		const auto limiter = createLimiter();
		const auto key = QStringLiteral("uid:1000");

		auto permit = limiter->admit(key, QStringLiteral("user"));
		QVERIFY(permit);
		permit->finishWithoutResult();
		QCOMPARE(limiter->runningCount(), 0);
		QCOMPARE(rejectionOf(*limiter, key), Rejection::Admitted);

		// no reset of earlier failures either
		fail(*limiter, key);
		m_now += AuthenticationLimiter::InitialFailureBackoff;
		permit = limiter->admit(key, QStringLiteral("user"));
		permit->finishWithoutResult();
		permit->finish(true);
		permit.reset();
		fail(*limiter, key);
		m_now += AuthenticationLimiter::InitialFailureBackoff;
		QCOMPARE(rejectionOf(*limiter, key), Rejection::BackingOff);
	}

	void rejectionsDoNotExtendBackoff()
	{
		const auto limiter = createLimiter();
		const auto key = QStringLiteral("uid:1000");

		fail(*limiter, key);
		for (int i = 0; i < 10; ++i)
		{
			m_now += 50;
			QCOMPARE(rejectionOf(*limiter, key), Rejection::BackingOff);
		}
		m_now += AuthenticationLimiter::InitialFailureBackoff - 500;
		QCOMPARE(rejectionOf(*limiter, key), Rejection::Admitted);
	}

	void globalConcurrencyLimit()
	{
		const auto limiter = createLimiter();

		std::vector<std::shared_ptr<AuthenticationLimiter::Permit>> permits;
		for (int i = 0; i < AuthenticationLimiter::MaximumConcurrentAuthentications; ++i)
		{
			permits.push_back(limiter->admit(QStringLiteral("caller%1").arg(i), QStringLiteral("user")));
			QVERIFY(permits.back());
		}

		QCOMPARE(rejectionOf(*limiter, QStringLiteral("another")), Rejection::Overloaded);
		QCOMPARE(limiter->runningCount(), AuthenticationLimiter::MaximumConcurrentAuthentications);

		// a released (unanswered) permit frees its slot without counting as failure
		permits.front().reset();
		QCOMPARE(limiter->runningCount(), AuthenticationLimiter::MaximumConcurrentAuthentications - 1);
		QCOMPARE(rejectionOf(*limiter, QStringLiteral("caller0")), Rejection::Admitted);
	}

	void droppedPermitKeepsBackoff()
	{
		const auto limiter = createLimiter();
		const auto key = QStringLiteral("uid:1000");

		fail(*limiter, key);
		m_now += AuthenticationLimiter::InitialFailureBackoff;

		limiter->admit(key, QStringLiteral("user")).reset();

		// neither success nor failure: the next failure doubles the backoff
		fail(*limiter, key);
		m_now += AuthenticationLimiter::InitialFailureBackoff;
		QCOMPARE(rejectionOf(*limiter, key), Rejection::BackingOff);
	}

	void finishOnce()
	{
		const auto limiter = createLimiter();
		auto permit = limiter->admit(QStringLiteral("a"), QStringLiteral("user"));
		QVERIFY(permit);
		permit->finish(false);
		permit->finish(true);
		permit.reset();
		QCOMPARE(limiter->runningCount(), 0);

		// the first result counts
		QCOMPARE(rejectionOf(*limiter, QStringLiteral("a")), Rejection::BackingOff);
	}

	void permitKeepsLimiterAlive()
	{
		auto limiter = createLimiter();
		auto permit = limiter->admit(QStringLiteral("a"), QStringLiteral("user"));
		limiter.reset();
		permit->finish(true);
		permit.reset();
	}

	void idleCallersAreForgotten()
	{
		const auto limiter = createLimiter();
		fail(*limiter, QStringLiteral("a"));
		auto outstanding = limiter->admit(QStringLiteral("b"), QStringLiteral("user"));
		QCOMPARE(limiter->trackedCallerCount(), 2);

		m_now += AuthenticationLimiter::FailureResetInterval;
		QVERIFY(limiter->admit(QStringLiteral("c"), QStringLiteral("user")));

		// "b" is outstanding and stays
		QCOMPARE(limiter->trackedCallerCount(), 2);

		// "a" starts with the initial backoff again
		fail(*limiter, QStringLiteral("a"));
		m_now += AuthenticationLimiter::InitialFailureBackoff;
		QCOMPARE(rejectionOf(*limiter, QStringLiteral("a")), Rejection::Admitted);
	}

	void rejectionLogging()
	{
		const auto limiter = createLimiter();
		const auto key = QStringLiteral("uid:1000");

		int suppressed = -1;
		QVERIFY(limiter->shouldLogRejection(key, &suppressed));
		QCOMPARE(suppressed, 0);

		for (int i = 0; i < 5; ++i)
		{
			m_now += 1000;
			QVERIFY(limiter->shouldLogRejection(key) == false);
		}

		// other callers have their own window
		QVERIFY(limiter->shouldLogRejection(QStringLiteral("uid:1001")));

		m_now += AuthenticationLimiter::RejectionLogInterval;
		QVERIFY(limiter->shouldLogRejection(key, &suppressed));
		QCOMPARE(suppressed, 5);
		QVERIFY(limiter->shouldLogRejection(key) == false);
	}

	void requestValidation_data()
	{
		QTest::addColumn<QString>("username");
		QTest::addColumn<QByteArray>("password");
		QTest::addColumn<bool>("valid");

		QTest::newRow("valid") << QStringLiteral("teacher") << QByteArray("secret") << true;
		QTest::newRow("non-ASCII") << QStringLiteral("léa") << QByteArray("p\xc3\xa4ss") << true;
		QTest::newRow("maximum sizes")
			<< QString(AuthenticationLimiter::MaximumUsernameSize, QLatin1Char('u'))
			<< QByteArray(AuthenticationLimiter::MaximumPasswordSize, 'p') << true;
		QTest::newRow("empty username") << QString() << QByteArray("secret") << false;
		QTest::newRow("empty password") << QStringLiteral("teacher") << QByteArray() << false;
		QTest::newRow("long username")
			<< QString(AuthenticationLimiter::MaximumUsernameSize + 1, QLatin1Char('u')) << QByteArray("secret") << false;
		// the limit is in UTF-8 bytes
		QTest::newRow("long UTF-8 username")
			<< QString(AuthenticationLimiter::MaximumUsernameSize / 2 + 1, QChar(0xe9)) << QByteArray("secret") << false;
		QTest::newRow("long password")
			<< QStringLiteral("teacher") << QByteArray(AuthenticationLimiter::MaximumPasswordSize + 1, 'p') << false;
		QTest::newRow("NUL in password") << QStringLiteral("teacher") << QByteArray("sec\0ret", 7) << false;
		QTest::newRow("NUL in username") << QString::fromLatin1("tea\0cher", 8) << QByteArray("secret") << false;
		QTest::newRow("newline in username") << QStringLiteral("teacher\n") << QByteArray("secret") << false;
		QTest::newRow("DEL in username") << QStringLiteral("teacher\x7f") << QByteArray("secret") << false;
		QTest::newRow("C1 control in username") << QStringLiteral("teacher\u0085") << QByteArray("secret") << false;
	}

	void requestValidation()
	{
		QFETCH(QString, username);
		QFETCH(QByteArray, password);
		QFETCH(bool, valid);

		QCOMPARE(AuthenticationLimiter::isValidRequest(username, password.constData(), password.size()), valid);
	}

	void nullPassword()
	{
		QVERIFY(AuthenticationLimiter::isValidRequest(QStringLiteral("teacher"), nullptr, 5) == false);
	}

	void rejectionNames()
	{
		QCOMPARE(AuthenticationLimiter::rejectionName(Rejection::Admitted), QLatin1String("none"));
		QCOMPARE(AuthenticationLimiter::rejectionName(Rejection::Busy), QLatin1String("busy"));
		QCOMPARE(AuthenticationLimiter::rejectionName(Rejection::BackingOff), QLatin1String("backing off"));
		QCOMPARE(AuthenticationLimiter::rejectionName(Rejection::Overloaded), QLatin1String("overloaded"));
	}

	// the D-Bus interface and the threads releasing permits share one limiter
	void concurrentUse()
	{
		const auto limiter = AuthenticationLimiter::create();
		std::atomic<int> maximumRunning{0};

		std::vector<std::thread> threads;
		for (int t = 0; t < 8; ++t)
		{
			threads.emplace_back([&limiter, &maximumRunning, t]() {
				for (int i = 0; i < 2000; ++i)
				{
					auto permit = limiter->admit(QStringLiteral("caller%1-%2").arg(t).arg(i % 3), QStringLiteral("user"));
					if (permit)
					{
						const auto running = limiter->runningCount();
						auto current = maximumRunning.load();
						while (running > current && maximumRunning.compare_exchange_weak(current, running) == false)
						{
						}
						permit->finish(true);
					}
				}
			});
		}
		for (auto& thread : threads)
		{
			thread.join();
		}

		QCOMPARE(limiter->runningCount(), 0);
		QVERIFY(maximumRunning.load() <= AuthenticationLimiter::MaximumConcurrentAuthentications);
	}

	// default clock
	void steadyClock()
	{
		const auto limiter = AuthenticationLimiter::create();
		fail(*limiter, QStringLiteral("a"));
		QCOMPARE(rejectionOf(*limiter, QStringLiteral("a")), Rejection::BackingOff);
	}
};


QTEST_GUILESS_MAIN(AuthenticationLimiterTest)
#include "AuthenticationLimiterTest.moc"
