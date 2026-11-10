/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "LightDmAdapter.h"
#include "LoginSession.h"
#include "FdChannel.h"

#include <QtTest>
#include <QtEndian>
#include <security/pam_appl.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
QByteArray number(quint32 value)
{
	QByteArray result(4, '\0');
	qToBigEndian(value, reinterpret_cast<uchar*>(result.data()));
	return result;
}
QByteArray bytes(const QByteArray& value) { return number(quint32(value.size())) + value; }
QByteArray frame(quint32 type, const QByteArray& payload = {})
{
	return number(type) + number(quint32(payload.size())) + payload;
}
QByteArray authentication(quint32 sequence, const QByteArray& user)
{
	return frame(1, number(sequence) + bytes(user));
}
QByteArray prompt(quint32 sequence, quint32 style)
{
	return frame(1, number(sequence) + bytes(QByteArrayLiteral("student")) + number(1) + number(style) + bytes(QByteArrayLiteral("Prompt")));
}
QByteArray end(quint32 sequence, quint32 result)
{
	return frame(2, number(sequence) + bytes(QByteArrayLiteral("student")) + number(result));
}
constexpr quint32 LocalSequence = 1;
constexpr quint32 RemoteSequence = LocalSequence ^ 0x80000000U;

void ready(LightDmAdapter& adapter)
{
	adapter.receiveFromDaemon(frame(0));
	adapter.receiveFromGreeter(authentication(LocalSequence, QByteArrayLiteral("previous-user")));
}
}

class LightDmLoginTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void manualTrafficIsPreserved();
	void fragmentedAndCoalescedPackets();
	void passwordOnlyPrompt();
	void usernameThenPassword();
	void staleUiDoesNotAnswerRemotePrompts();
	void authenticationFailureRestoresManualLogin();
	void cancelledAttemptCanBeRetried();
	void extraSecretIsNotGivenThePassword();
	void sessionStartFailure();
	void failedRemoteSessionCannotBeStartedByTheUi();
	void uiCannotReuseRemoteSequence();
	void obsoleteRepliesNeverReachManualLogin();
	void repeatedUiSequenceGetsFreshWireSequence();
	void cancellationDuringSessionStartRetiresConnection();
	void duplicateAuthenticationSuccessCannotStartTwice();
	void daemonResetCancelsRemoteAuthentication();
	void manualSessionStartIsTracked();
	void unsolicitedSessionResultFailsClosed();
	void malformedMessagesFailClosed();
	void loginValidationAndBusyState();
	void sessionRouting();
	void nonblockingTransport();
};

void LightDmLoginTest::manualTrafficIsPreserved()
{
	LightDmAdapter adapter;
	QSignalSpy daemon(&adapter, &LightDmAdapter::writeToDaemon);
	QSignalSpy ui(&adapter, &LightDmAdapter::writeToGreeter);
	const auto connect = frame(0, bytes(QByteArrayLiteral("test")) + number(0));
	adapter.receiveFromGreeter(connect);
	QCOMPARE(daemon.takeFirst().at(0).toByteArray(), connect);
	adapter.receiveFromDaemon(frame(0));
	QCOMPARE(ui.takeFirst().at(0).toByteArray(), frame(0));
	const auto login = authentication(1, QByteArrayLiteral("local"));
	adapter.receiveFromGreeter(login);
	QCOMPARE(daemon.takeFirst().at(0).toByteArray(), login);
	adapter.receiveFromDaemon(prompt(1, PAM_PROMPT_ECHO_OFF));
	QCOMPARE(ui.takeFirst().at(0).toByteArray(), prompt(1, PAM_PROMPT_ECHO_OFF));
	const auto reply = frame(3, number(1) + bytes(QByteArrayLiteral("manual-password")));
	adapter.receiveFromGreeter(reply);
	QCOMPARE(daemon.takeFirst().at(0).toByteArray(), reply);
}

void LightDmLoginTest::fragmentedAndCoalescedPackets()
{
	LightDmAdapter adapter;
	adapter.receiveFromGreeter(authentication(LocalSequence, QByteArrayLiteral("student")));
	QSignalSpy ui(&adapter, &LightDmAdapter::writeToGreeter);
	const QByteArray packets = frame(0) + prompt(1, PAM_PROMPT_ECHO_OFF);
	for (int i = 0; i < packets.size(); ++i)
	{
		adapter.receiveFromDaemon(packets.mid(i, 1));
	}
	QCOMPARE(ui.size(), 2);
	QCOMPARE(ui.at(0).at(0).toByteArray(), frame(0));
	QCOMPARE(ui.at(1).at(0).toByteArray(), prompt(1, PAM_PROMPT_ECHO_OFF));
	ui.clear();
	adapter.receiveFromDaemon(packets);
	QCOMPARE(ui.size(), 2);
}

void LightDmLoginTest::passwordOnlyPrompt()
{
	LightDmAdapter adapter;
	ready(adapter);
	QSignalSpy daemon(&adapter, &LightDmAdapter::writeToDaemon);
	QSignalSpy finished(&adapter, &LightDmAdapter::finished);
	QVERIFY(adapter.login(QStringLiteral("student"), QByteArrayLiteral("p@ss"), QStringLiteral("plasma")));
	QCOMPARE(daemon.takeFirst().at(0).toByteArray(), authentication(RemoteSequence, QByteArrayLiteral("student")));
	adapter.receiveFromDaemon(prompt(RemoteSequence, PAM_PROMPT_ECHO_OFF));
	QCOMPARE(daemon.takeFirst().at(0).toByteArray(), frame(3, number(1) + bytes(QByteArrayLiteral("p@ss"))));
	adapter.receiveFromDaemon(end(RemoteSequence, PAM_SUCCESS));
	QCOMPARE(daemon.takeFirst().at(0).toByteArray(), frame(4, bytes(QByteArrayLiteral("plasma"))));
	QVERIFY(finished.isEmpty());
	adapter.receiveFromDaemon(frame(3, number(0)));
	QCOMPARE(finished.size(), 1);
	QVERIFY(finished.first().at(0).toBool());
	QVERIFY(adapter.isBusy()); // do not let stale UI input reset the starting session
	adapter.receiveFromGreeter(authentication(2, QByteArrayLiteral("local")));
	QVERIFY(daemon.isEmpty());
}

void LightDmLoginTest::usernameThenPassword()
{
	LightDmAdapter adapter;
	ready(adapter);
	QSignalSpy daemon(&adapter, &LightDmAdapter::writeToDaemon);
	QVERIFY(adapter.login(QStringLiteral("DOMAIN\\student"), QByteArrayLiteral("secret"), {}));
	daemon.clear();
	const auto batch = frame(1, number(RemoteSequence) + bytes(QByteArrayLiteral("student")) + number(3) +
		number(PAM_TEXT_INFO) + bytes(QByteArrayLiteral("Welcome")) +
		number(PAM_PROMPT_ECHO_ON) + bytes(QByteArrayLiteral("User")) +
		number(PAM_PROMPT_ECHO_OFF) + bytes(QByteArrayLiteral("Password")));
	adapter.receiveFromDaemon(batch);
	QCOMPARE(daemon.size(), 1);
	QCOMPARE(daemon.first().at(0).toByteArray(), frame(3, number(2) +
		bytes(QByteArrayLiteral("DOMAIN\\student")) + bytes(QByteArrayLiteral("secret"))));
}

void LightDmLoginTest::staleUiDoesNotAnswerRemotePrompts()
{
	LightDmAdapter adapter;
	ready(adapter);
	QSignalSpy daemon(&adapter, &LightDmAdapter::writeToDaemon);
	QSignalSpy ui(&adapter, &LightDmAdapter::writeToGreeter);
	QVERIFY(adapter.login(QStringLiteral("student"), QByteArrayLiteral("remote"), {}));
	daemon.clear();
	adapter.receiveFromDaemon(prompt(LocalSequence, PAM_PROMPT_ECHO_OFF));
	adapter.receiveFromDaemon(end(LocalSequence, PAM_AUTH_ERR));
	adapter.receiveFromGreeter(frame(3, number(1) + bytes(QByteArrayLiteral("stale-local"))));
	adapter.receiveFromGreeter(frame(5));
	adapter.receiveFromGreeter(authentication(2, QByteArrayLiteral("different-local")));
	QVERIFY(daemon.isEmpty());
	QVERIFY(ui.isEmpty());
	adapter.receiveFromDaemon(end(RemoteSequence, PAM_AUTH_ERR));
	const auto restored = frame(2, number(2) + bytes(QByteArrayLiteral("different-local")) + number(PAM_AUTH_ERR));
	QCOMPARE(ui.size(), 1);
	QCOMPARE(ui.first().at(0).toByteArray(), restored);
}

void LightDmLoginTest::authenticationFailureRestoresManualLogin()
{
	LightDmAdapter adapter;
	ready(adapter);
	// The original UI may already consider a passwordless account authenticated.
	adapter.receiveFromDaemon(end(LocalSequence, PAM_SUCCESS));
	QSignalSpy finished(&adapter, &LightDmAdapter::finished);
	QSignalSpy ui(&adapter, &LightDmAdapter::writeToGreeter);
	QSignalSpy daemon(&adapter, &LightDmAdapter::writeToDaemon);
	QVERIFY(adapter.login(QStringLiteral("student"), QByteArrayLiteral("wrong"), {}));
	adapter.receiveFromDaemon(end(RemoteSequence, PAM_AUTH_ERR));
	QCOMPARE(finished.size(), 1);
	QVERIFY(!finished.first().at(0).toBool());
	QVERIFY(!adapter.isBusy());
	QCOMPARE(ui.size(), 1);
	daemon.clear();
	const auto manual = authentication(2, QByteArrayLiteral("manual"));
	adapter.receiveFromGreeter(manual);
	QCOMPARE(daemon.first().at(0).toByteArray(), manual);
}

void LightDmLoginTest::cancelledAttemptCanBeRetried()
{
	LightDmAdapter adapter;
	ready(adapter);
	QSignalSpy finished(&adapter, &LightDmAdapter::finished);
	QVERIFY(adapter.login(QStringLiteral("student"), QByteArrayLiteral("secret"), {}));
	adapter.cancel();
	QCOMPARE(finished.size(), 1);
	QVERIFY(!finished.first().at(0).toBool());
	adapter.cancel();
	QCOMPARE(finished.size(), 1);
	// Cancellation replies must not be confused with the next remote attempt.
	adapter.receiveFromDaemon(end(RemoteSequence, PAM_AUTH_ERR));
	QVERIFY(adapter.login(QStringLiteral("student"), QByteArrayLiteral("secret"), {}));
	adapter.receiveFromDaemon(end(RemoteSequence, PAM_SUCCESS));
	QCOMPARE(finished.size(), 1);
	QVERIFY(adapter.isBusy());
	adapter.receiveFromDaemon(end(RemoteSequence + 1, PAM_AUTH_ERR));
	QCOMPARE(finished.size(), 2);
	QVERIFY(!finished.last().at(0).toBool());
}

void LightDmLoginTest::extraSecretIsNotGivenThePassword()
{
	LightDmAdapter adapter;
	ready(adapter);
	QSignalSpy daemon(&adapter, &LightDmAdapter::writeToDaemon);
	QSignalSpy finished(&adapter, &LightDmAdapter::finished);
	QVERIFY(adapter.login(QStringLiteral("student"), QByteArrayLiteral("secret"), {}));
	adapter.receiveFromDaemon(prompt(RemoteSequence, PAM_PROMPT_ECHO_OFF));
	daemon.clear();
	adapter.receiveFromDaemon(prompt(RemoteSequence, PAM_PROMPT_ECHO_OFF));
	QCOMPARE(daemon.size(), 1);
	QCOMPARE(daemon.first().at(0).toByteArray(), frame(5));
	QCOMPARE(finished.size(), 1);
	QVERIFY(!finished.first().at(0).toBool());
}

void LightDmLoginTest::sessionStartFailure()
{
	LightDmAdapter adapter;
	ready(adapter);
	QSignalSpy finished(&adapter, &LightDmAdapter::finished);
	QVERIFY(adapter.login(QStringLiteral("student"), QByteArrayLiteral("secret"), {}));
	adapter.receiveFromDaemon(end(RemoteSequence, PAM_SUCCESS));
	adapter.receiveFromDaemon(frame(3, number(1)));
	QCOMPARE(finished.size(), 1);
	QVERIFY(!finished.first().at(0).toBool());
	QVERIFY(!adapter.isBusy());
}

void LightDmLoginTest::failedRemoteSessionCannotBeStartedByTheUi()
{
	LightDmAdapter adapter;
	ready(adapter);
	QSignalSpy daemon(&adapter, &LightDmAdapter::writeToDaemon);
	QSignalSpy ui(&adapter, &LightDmAdapter::writeToGreeter);
	QVERIFY(adapter.login(QStringLiteral("student"), QByteArrayLiteral("secret"), {}));
	adapter.receiveFromDaemon(end(RemoteSequence, PAM_SUCCESS));
	adapter.receiveFromDaemon(frame(3, number(1)));
	daemon.clear();
	ui.clear();
	adapter.receiveFromGreeter(frame(4, bytes(QByteArrayLiteral("plasma"))));
	QVERIFY(daemon.isEmpty());
	QCOMPARE(ui.size(), 1);
	QCOMPARE(ui.first().at(0).toByteArray(), frame(3, number(1)));
	adapter.receiveFromGreeter(frame(3, number(1) + bytes(QByteArrayLiteral("stale-password"))));
	QVERIFY(daemon.isEmpty());
	const auto fresh = authentication(2, QByteArrayLiteral("manual"));
	adapter.receiveFromGreeter(fresh);
	QCOMPARE(daemon.size(), 1);
	QCOMPARE(daemon.first().at(0).toByteArray(), fresh);
}

void LightDmLoginTest::uiCannotReuseRemoteSequence()
{
	LightDmAdapter adapter;
	ready(adapter);
	QVERIFY(adapter.login(QStringLiteral("student"), QByteArrayLiteral("secret"), {}));
	adapter.cancel();
	QSignalSpy daemon(&adapter, &LightDmAdapter::writeToDaemon);
	QSignalSpy ui(&adapter, &LightDmAdapter::writeToGreeter);
	adapter.receiveFromGreeter(authentication(RemoteSequence, QByteArrayLiteral("manual")));
	QCOMPARE(daemon.takeFirst().at(0).toByteArray(), authentication(2, QByteArrayLiteral("manual")));
	adapter.receiveFromDaemon(end(RemoteSequence, PAM_SUCCESS));
	adapter.receiveFromDaemon(prompt(RemoteSequence, PAM_PROMPT_ECHO_OFF));
	QVERIFY(ui.isEmpty());
	adapter.receiveFromDaemon(end(2, PAM_AUTH_ERR));
	QCOMPARE(ui.size(), 1);
	QCOMPARE(ui.first().at(0).toByteArray(), end(RemoteSequence, PAM_AUTH_ERR));
}

void LightDmLoginTest::obsoleteRepliesNeverReachManualLogin()
{
	LightDmAdapter adapter;
	ready(adapter);
	QVERIFY(adapter.login(QStringLiteral("student"), QByteArrayLiteral("first"), {}));
	adapter.cancel();
	QVERIFY(adapter.login(QStringLiteral("student"), QByteArrayLiteral("second"), {}));
	adapter.cancel();
	adapter.receiveFromGreeter(authentication(2, QByteArrayLiteral("manual")));
	QSignalSpy ui(&adapter, &LightDmAdapter::writeToGreeter);
	adapter.receiveFromDaemon(end(RemoteSequence, PAM_SUCCESS));
	adapter.receiveFromDaemon(end(RemoteSequence + 1, PAM_SUCCESS));
	adapter.receiveFromDaemon(prompt(LocalSequence, PAM_PROMPT_ECHO_OFF));
	QVERIFY(ui.isEmpty());
	adapter.receiveFromDaemon(prompt(2, PAM_PROMPT_ECHO_OFF));
	QCOMPARE(ui.size(), 1);
	QCOMPARE(ui.first().at(0).toByteArray(), prompt(2, PAM_PROMPT_ECHO_OFF));
}

void LightDmLoginTest::repeatedUiSequenceGetsFreshWireSequence()
{
	LightDmAdapter adapter;
	ready(adapter);
	QSignalSpy daemon(&adapter, &LightDmAdapter::writeToDaemon);
	QSignalSpy ui(&adapter, &LightDmAdapter::writeToGreeter);
	adapter.receiveFromGreeter(authentication(LocalSequence, QByteArrayLiteral("manual")));
	QCOMPARE(daemon.takeFirst().at(0).toByteArray(), authentication(2, QByteArrayLiteral("manual")));
	adapter.receiveFromDaemon(end(LocalSequence, PAM_SUCCESS));
	QVERIFY(ui.isEmpty());
	adapter.receiveFromDaemon(prompt(2, PAM_PROMPT_ECHO_OFF));
	QCOMPARE(ui.takeFirst().at(0).toByteArray(), prompt(LocalSequence, PAM_PROMPT_ECHO_OFF));
}

void LightDmLoginTest::cancellationDuringSessionStartRetiresConnection()
{
	LightDmAdapter adapter;
	ready(adapter);
	QSignalSpy errors(&adapter, &LightDmAdapter::protocolError);
	QSignalSpy finished(&adapter, &LightDmAdapter::finished);
	QVERIFY(adapter.login(QStringLiteral("student"), QByteArrayLiteral("secret"), {}));
	adapter.receiveFromDaemon(end(RemoteSequence, PAM_SUCCESS));
	adapter.cancel();
	QCOMPARE(errors.size(), 1);
	QCOMPARE(finished.size(), 1);
	QVERIFY(!finished.first().at(0).toBool());
	QVERIFY(!adapter.login(QStringLiteral("other"), QByteArrayLiteral("secret"), {}));
	QSignalSpy daemon(&adapter, &LightDmAdapter::writeToDaemon);
	QSignalSpy ui(&adapter, &LightDmAdapter::writeToGreeter);
	adapter.receiveFromDaemon(frame(3, number(0))); // unsequenced late success
	adapter.receiveFromGreeter(authentication(2, QByteArrayLiteral("manual")));
	adapter.receiveFromGreeter(frame(4, bytes(QByteArrayLiteral("plasma"))));
	QVERIFY(daemon.isEmpty());
	QVERIFY(ui.isEmpty());
	QCOMPARE(finished.size(), 1);
}

void LightDmLoginTest::duplicateAuthenticationSuccessCannotStartTwice()
{
	LightDmAdapter adapter;
	ready(adapter);
	QSignalSpy daemon(&adapter, &LightDmAdapter::writeToDaemon);
	QSignalSpy errors(&adapter, &LightDmAdapter::protocolError);
	QVERIFY(adapter.login(QStringLiteral("student"), QByteArrayLiteral("secret"), {}));
	adapter.receiveFromDaemon(end(RemoteSequence, PAM_SUCCESS));
	daemon.clear();
	adapter.receiveFromDaemon(end(RemoteSequence, PAM_SUCCESS));
	QCOMPARE(errors.size(), 1);
	QCOMPARE(daemon.size(), 1);
	QCOMPARE(daemon.first().at(0).toByteArray(), frame(5)); // cancel, never a second start
}

void LightDmLoginTest::daemonResetCancelsRemoteAuthentication()
{
	LightDmAdapter adapter;
	ready(adapter);
	QSignalSpy finished(&adapter, &LightDmAdapter::finished);
	QVERIFY(adapter.login(QStringLiteral("student"), QByteArrayLiteral("secret"), {}));
	adapter.receiveFromDaemon(frame(5));
	QCOMPARE(finished.size(), 1);
	QVERIFY(!finished.first().at(0).toBool());
	QSignalSpy daemon(&adapter, &LightDmAdapter::writeToDaemon);
	QSignalSpy ui(&adapter, &LightDmAdapter::writeToGreeter);
	adapter.receiveFromDaemon(prompt(RemoteSequence, PAM_PROMPT_ECHO_OFF));
	QVERIFY(daemon.isEmpty());
	QVERIFY(ui.isEmpty());
	adapter.receiveFromGreeter(authentication(2, QByteArrayLiteral("manual")));
	QCOMPARE(daemon.first().at(0).toByteArray(), authentication(2, QByteArrayLiteral("manual")));
}

void LightDmLoginTest::manualSessionStartIsTracked()
{
	LightDmAdapter adapter;
	ready(adapter);
	adapter.receiveFromDaemon(end(LocalSequence, PAM_SUCCESS));
	QSignalSpy daemon(&adapter, &LightDmAdapter::writeToDaemon);
	QSignalSpy ui(&adapter, &LightDmAdapter::writeToGreeter);
	const auto start = frame(4, bytes(QByteArrayLiteral("plasma")));
	adapter.receiveFromGreeter(start);
	QCOMPARE(daemon.takeFirst().at(0).toByteArray(), start);
	QVERIFY(!adapter.login(QStringLiteral("student"), QByteArrayLiteral("secret"), {}));
	adapter.receiveFromGreeter(start); // never leave two unsequenced replies in flight
	QVERIFY(daemon.isEmpty());
	QCOMPARE(ui.takeFirst().at(0).toByteArray(), frame(3, number(1)));
	adapter.receiveFromDaemon(frame(3, number(1)));
	QCOMPARE(ui.takeFirst().at(0).toByteArray(), frame(3, number(1)));
	QVERIFY(!adapter.isBusy());
	QVERIFY(adapter.login(QStringLiteral("student"), QByteArrayLiteral("secret"), {}));
}

void LightDmLoginTest::unsolicitedSessionResultFailsClosed()
{
	LightDmAdapter adapter;
	ready(adapter);
	QSignalSpy errors(&adapter, &LightDmAdapter::protocolError);
	QSignalSpy ui(&adapter, &LightDmAdapter::writeToGreeter);
	adapter.receiveFromDaemon(frame(3, number(0)));
	QCOMPARE(errors.size(), 1);
	QVERIFY(ui.isEmpty());
	QVERIFY(!adapter.login(QStringLiteral("student"), QByteArrayLiteral("secret"), {}));
}

void LightDmLoginTest::malformedMessagesFailClosed()
{
	LightDmAdapter adapter;
	ready(adapter);
	QSignalSpy errors(&adapter, &LightDmAdapter::protocolError);
	QSignalSpy finished(&adapter, &LightDmAdapter::finished);
	QVERIFY(adapter.login(QStringLiteral("student"), QByteArrayLiteral("secret"), {}));
	adapter.receiveFromDaemon(frame(1, number(RemoteSequence) + number(100) + QByteArrayLiteral("truncated")));
	QCOMPARE(errors.size(), 1);
	QCOMPARE(finished.size(), 1);
	QVERIFY(!finished.first().at(0).toBool());
	QVERIFY(!adapter.login(QStringLiteral("student"), QByteArrayLiteral("secret"), {}));
	LightDmAdapter tooLarge;
	QSignalSpy sizeErrors(&tooLarge, &LightDmAdapter::protocolError);
	tooLarge.receiveFromDaemon(number(1) + number(1024 * 1024 + 1));
	QCOMPARE(sizeErrors.size(), 1);
}

void LightDmLoginTest::loginValidationAndBusyState()
{
	LightDmAdapter adapter;
	QVERIFY(!adapter.login(QStringLiteral("student"), QByteArrayLiteral("secret"), {}));
	ready(adapter);
	QVERIFY(!adapter.login({}, QByteArrayLiteral("secret"), {}));
	QVERIFY(!adapter.login(QStringLiteral("student"), QByteArray("a\0b", 3), {}));
	QVERIFY(adapter.login(QStringLiteral("student"), QByteArrayLiteral("secret"), {}));
	QVERIFY(!adapter.login(QStringLiteral("other"), QByteArrayLiteral("secret"), {}));
}

void LightDmLoginTest::sessionRouting()
{
	LoginSession session;
	session.service = QStringLiteral("lightdm-greeter");
	session.sessionClass = QStringLiteral("greeter");
	session.state = QStringLiteral("active");
	session.type = QStringLiteral("x11");
	session.seat = QStringLiteral("seat0");
	QVERIFY(session.isLightDmGreeter());
	QVERIFY(!session.isUser());
	session.sessionClass = QStringLiteral("user");
	QVERIFY(session.isLightDmGreeter());
	QVERIFY(!session.isUser());
	session.type = QStringLiteral("wayland");
	QVERIFY(session.isLightDmGreeter());
	// Some greeter PAM stacks omit XDG_SESSION_TYPE. The running LightDM
	// wrapper still proves greeter identity; the server probes its environment.
	session.type = QStringLiteral("unspecified");
	QVERIFY(session.isLightDmGreeter());
	session.type.clear();
	QVERIFY(session.isLightDmGreeter());
	session.type = QStringLiteral("tty");
	QVERIFY(!session.isLightDmGreeter());
	session.type = QStringLiteral("wayland");
	session.service = QStringLiteral("lightdm");
	QVERIFY(!session.isLightDmGreeter());
	QVERIFY(session.isUser());
	session.service = QStringLiteral("sddm-greeter");
	session.sessionClass = QStringLiteral("greeter");
	QVERIFY(!session.isLightDmGreeter());
	QVERIFY(!session.isUser());
	session.service = QStringLiteral("lightdm-greeter");
	session.state = QStringLiteral("closing");
	QVERIFY(!session.isLightDmGreeter());
}

void LightDmLoginTest::nonblockingTransport()
{
	int sockets[2];
	QVERIFY(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
	FdChannel channel(sockets[0], sockets[0]);
	QVERIFY(channel.isValid());
	QSignalSpy incoming(&channel, &FdChannel::received);
	QSignalSpy closed(&channel, &FdChannel::disconnected);
	QVERIFY(::write(sockets[1], "hello", 5) == 5);
	QTRY_VERIFY(!incoming.isEmpty());
	QCOMPARE(incoming.first().at(0).toByteArray(), QByteArrayLiteral("hello"));
	QVERIFY(::fcntl(sockets[1], F_SETFL, O_NONBLOCK) == 0);
	const QByteArray payload(512 * 1024, 'x');
	channel.send(payload);
	QByteArray received;
	QElapsedTimer timer;
	timer.start();
	while (received.size() < payload.size() && timer.elapsed() < 3000)
	{
		char buffer[8192];
		const auto count = ::read(sockets[1], buffer, sizeof(buffer));
		if (count > 0)
		{
			received.append(buffer, int(count));
		}
		QCoreApplication::processEvents();
	}
	QCOMPARE(received, payload);
	::close(sockets[1]);
	QTRY_COMPARE(closed.size(), 1);
}

QTEST_GUILESS_MAIN(LightDmLoginTest)
#include "LightDmLoginTest.moc"
