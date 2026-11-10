/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <QObject>
#include <QByteArray>

// A greeter-wrapper owns LightDM's private greeter connection and proxies the
// existing GTK/KDE UI. Only a remote login temporarily takes over this channel.
class LightDmAdapter : public QObject
{
	Q_OBJECT
public:
	explicit LightDmAdapter(QObject* parent = nullptr);
	~LightDmAdapter() override;
	void receiveFromDaemon(const QByteArray& data);
	void receiveFromGreeter(const QByteArray& data);
	bool login(const QString& username, const QByteArray& password, const QString& session);
	void cancel();
	bool isReady() const { return m_connected; }
	bool isBusy() const { return m_remote || m_startingSession || m_manualSessionStarting; }

Q_SIGNALS:
	void ready();
	void writeToDaemon(const QByteArray& data);
	void writeToGreeter(const QByteArray& data);
	void finished(bool success, const QString& error);
	void protocolError(const QString& error);

private:
	void consume(QByteArray& buffer, bool fromDaemon);
	void daemonMessage(quint32 type, const QByteArray& payload, const QByteArray& frame);
	void greeterMessage(quint32 type, const QByteArray& payload, const QByteArray& frame);
	void complete(bool success, const QString& error = {});
	void failProtocol();
	void clearPassword();

	QByteArray m_daemonBuffer;
	QByteArray m_greeterBuffer;
	QString m_username;
	QByteArray m_password;
	QString m_session;
	quint32 m_localSequence = 0;
	quint32 m_localWireSequence = 0;
	quint32 m_localCounter = 0;
	quint32 m_remoteSequence = 0;
	quint32 m_remoteCounter = 0x80000000U;
	QString m_localUsername;
	bool m_localConversation = false;
	bool m_connected = false;
	bool m_remote = false;
	bool m_failed = false;
	bool m_passwordUsed = false;
	bool m_usernameUsed = false;
	bool m_startingSession = false;
	bool m_manualSessionStarting = false;
	bool m_sessionResultPending = false;
	bool m_requireNewAuthentication = false;
};
