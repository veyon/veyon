/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "LightDmAdapter.h"

#include <QtEndian>
#include <QDebug>
#include <security/pam_appl.h>
#include <cstring>
#include <limits>

namespace
{
constexpr quint32 MaxPayload = 1024 * 1024;

QByteArray integer(quint32 value)
{
	QByteArray data(4, '\0');
	qToBigEndian(value, reinterpret_cast<uchar*>(data.data()));
	return data;
}

QByteArray string(const QByteArray& value)
{
	return integer(quint32(value.size())) + value;
}

QByteArray packet(quint32 type, const QByteArray& payload = {})
{
	return integer(type) + integer(quint32(payload.size())) + payload;
}

class Reader
{
public:
	explicit Reader(const QByteArray& data) : m_data(data) {}
	quint32 number()
	{
		if (m_offset + 4 > m_data.size())
		{
			valid = false;
			return 0;
		}
		const auto value = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(m_data.constData() + m_offset));
		m_offset += 4;
		return value;
	}
	QByteArray bytes()
	{
		const auto length = number();
		if (!valid || length > MaxPayload || length > quint32(m_data.size() - m_offset))
		{
			valid = false;
			return {};
		}
		const auto value = m_data.mid(m_offset, int(length));
		m_offset += int(length);
		return value;
	}
	bool atEnd() const { return valid && m_offset == m_data.size(); }
	bool valid = true;
private:
	const QByteArray& m_data;
	int m_offset = 0;
};
}

LightDmAdapter::LightDmAdapter(QObject* parent) : QObject(parent) {}
LightDmAdapter::~LightDmAdapter() { clearPassword(); }

void LightDmAdapter::clearPassword()
{
	if (!m_password.isEmpty())
	{
		explicit_bzero(m_password.data(), size_t(m_password.size()));
		m_password.clear();
	}
}

void LightDmAdapter::receiveFromDaemon(const QByteArray& data)
{
	if (m_failed)
	{
		return;
	}
	m_daemonBuffer += data;
	consume(m_daemonBuffer, true);
}

void LightDmAdapter::receiveFromGreeter(const QByteArray& data)
{
	if (m_failed)
	{
		return;
	}
	m_greeterBuffer += data;
	consume(m_greeterBuffer, false);
}

void LightDmAdapter::consume(QByteArray& buffer, bool fromDaemon)
{
	while (!m_failed && buffer.size() >= 8)
	{
		Reader reader(buffer);
		const auto type = reader.number();
		const auto length = reader.number();
		if (length > MaxPayload)
		{
			failProtocol();
			return;
		}
		if (buffer.size() < 8 + int(length))
		{
			return;
		}
		const auto frame = buffer.left(8 + int(length));
		buffer.remove(0, frame.size());
		if (fromDaemon)
		{
			daemonMessage(type, frame.mid(8), frame);
		}
		else
		{
			greeterMessage(type, frame.mid(8), frame);
		}
	}
}

void LightDmAdapter::greeterMessage(quint32 type, const QByteArray& payload, const QByteArray& frame)
{
	const bool authenticating = type == 1 || type == 2 || type == 7;
	if (authenticating)
	{
		Reader reader(payload);
		m_localSequence = reader.number();
		if (type == 7)
		{
			reader.bytes(); // remote session type
		}
		m_localUsername = type == 2 ? QString() : QString::fromUtf8(reader.bytes());
		if (!reader.atEnd())
		{
			failProtocol();
			return;
		}
		m_localConversation = true;
	}
	// Track even suppressed UI attempts: after a failure the synthetic end
	// message must use the sequence the UI is currently waiting for.
	if ((m_remote || m_startingSession) && type != 0 && type != 8)
	{
		if (type == 4)
		{
			// Unblock greeters using synchronous start_session(), without ever
			// allowing them to start the remotely authenticated account.
			Q_EMIT writeToGreeter(packet(3, integer(1)));
		}
		return;
	}
	if (m_requireNewAuthentication)
	{
		if (type == 1 || type == 2 || type == 7)
		{
			m_requireNewAuthentication = false;
		}
		else if (type == 3 || type == 4 || type == 6)
		{
			// A failed session start can leave the remote account authenticated
			// inside LightDM. A local UI must begin its own authentication before
			// it is allowed to start anything on this connection again.
			if (type == 4)
			{
				Q_EMIT writeToGreeter(packet(3, integer(1)));
			}
			return;
		}
	}
	if (type == 4)
	{
		if (m_manualSessionStarting)
		{
			Q_EMIT writeToGreeter(packet(3, integer(1)));
			return;
		}
		m_manualSessionStarting = true;
		m_sessionResultPending = true;
	}
	if (authenticating)
	{
		// The UI supplies arbitrary sequence numbers. Allocate our own lower-half
		// IDs so it cannot collide with any remote attempt or reuse a cancelled
		// manual attempt's ID. Never wrap either namespace.
		if (m_localCounter == 0x7fffffffU)
		{
			failProtocol();
			return;
		}
		m_localWireSequence = ++m_localCounter;
		Q_EMIT writeToDaemon(packet(type, integer(m_localWireSequence) + payload.mid(4)));
		return;
	}
	Q_EMIT writeToDaemon(frame);
}

bool LightDmAdapter::login(const QString& username, const QByteArray& password, const QString& session)
{
	if (!m_connected || m_failed || isBusy() || username.isEmpty() || username.size() > 1024 ||
		username.contains(QLatin1Char('\0')) || password.size() > 4096 || password.contains('\0') || session.size() > 256)
	{
		return false;
	}
	if (m_remoteCounter == std::numeric_limits<quint32>::max())
	{
		failProtocol();
		return false;
	}
	m_username = username;
	m_password = password;
	m_session = session;
	m_passwordUsed = false;
	m_usernameUsed = false;
	m_startingSession = false;
	m_remote = true;
	// handle_authenticate() in LightDM resets/cancels its previous conversation.
	// A separate sequence keeps already queued UI prompts out of this request.
	m_remoteSequence = ++m_remoteCounter;
	qInfo() << "LightDM remote authentication started; sequence" << m_remoteSequence;
	Q_EMIT writeToDaemon(packet(1, integer(m_remoteSequence) + string(username.toUtf8())));
	return true;
}

void LightDmAdapter::daemonMessage(quint32 type, const QByteArray& payload, const QByteArray& frame)
{
	if (type == 0 || type == 7)
	{
		const bool wasConnected = m_connected;
		m_connected = true;
		if (!wasConnected)
		{
			Q_EMIT ready();
		}
	}
	if ((type == 5 || type == 6) && m_sessionResultPending)
	{
		failProtocol();
		return;
	}
	if ((type == 5 || type == 6) && m_remote)
	{
		complete(false, QStringLiteral("LightDM reset the authentication conversation"));
	}
	if (!m_remote)
	{
		if (type == 1 || type == 2)
		{
			Reader reader(payload);
			const auto sequence = reader.number();
			if (!reader.valid)
			{
				failProtocol();
				return;
			}
			if (!m_localConversation || sequence != m_localWireSequence)
			{
				return; // remote or obsolete manual replies never reach the UI
			}
			Q_EMIT writeToGreeter(packet(type, integer(m_localSequence) + payload.mid(4)));
			return;
		}
		else if (type == 3)
		{
			Reader reader(payload);
			const auto result = reader.number();
			if (!reader.atEnd() || !m_sessionResultPending || !m_manualSessionStarting)
			{
				failProtocol();
				return;
			}
			m_sessionResultPending = false;
			if (result != 0)
			{
				m_manualSessionStarting = false;
			}
		}
		else if (type == 5 || type == 6)
		{
			m_startingSession = false;
			m_manualSessionStarting = false;
			m_localConversation = false;
			m_localWireSequence = 0;
		}
		Q_EMIT writeToGreeter(frame);
		return;
	}
	if (type != 1 && type != 2 && type != 3)
	{
		Q_EMIT writeToGreeter(frame);
		return;
	}
	Reader reader(payload);
	if (type == 3)
	{
		const auto result = reader.number();
		if (!reader.atEnd() || !m_startingSession || !m_sessionResultPending)
		{
			failProtocol();
			return;
		}
		m_sessionResultPending = false;
		qInfo() << "LightDM session-start result:" << result;
		complete(result == 0, result == 0 ? QString() : QStringLiteral("LightDM could not start the session"));
		return;
	}
	const auto sequence = reader.number();
	reader.bytes(); // canonical authentication username
	if (!reader.valid)
	{
		failProtocol();
		return;
	}
	if (sequence != m_remoteSequence)
	{
		return; // queued response to the previous local UI authentication
	}
	if (m_startingSession)
	{
		// No further prompt/end message belongs to an authentication which has
		// already requested session start. In particular, never issue it twice.
		failProtocol();
		return;
	}
	if (type == 2)
	{
		const auto result = reader.number();
		if (!reader.atEnd())
		{
			failProtocol();
			return;
		}
		qInfo() << "LightDM PAM authentication result:" << result;
		if (result != PAM_SUCCESS)
		{
			complete(false, QStringLiteral("LightDM authentication failed"));
			return;
		}
		clearPassword();
		m_startingSession = true;
		m_sessionResultPending = true;
		qInfo() << "LightDM authentication succeeded; requesting session start";
		Q_EMIT writeToDaemon(packet(4, string(m_session.toUtf8())));
		return;
	}
	const auto count = reader.number();
	if (count > 128)
	{
		failProtocol();
		return;
	}
	QByteArray responses;
	quint32 responseCount = 0;
	for (quint32 i = 0; i < count; ++i)
	{
		const auto style = reader.number();
		reader.bytes(); // prompt text is not logged or matched by translated text
		if (!reader.valid)
		{
			failProtocol();
			return;
		}
		if (style == PAM_PROMPT_ECHO_OFF && !m_passwordUsed)
		{
			responses += string(m_password);
			m_passwordUsed = true;
			clearPassword();
			++responseCount;
		}
		else if (style == PAM_PROMPT_ECHO_ON && !m_usernameUsed && !m_passwordUsed)
		{
			responses += string(m_username.toUtf8());
			m_usernameUsed = true;
			++responseCount;
		}
		else if (style != PAM_TEXT_INFO && style != PAM_ERROR_MSG)
		{
			complete(false, QStringLiteral("Additional authentication input is required"));
			return;
		}
	}
	if (!reader.atEnd())
	{
		failProtocol();
		return;
	}
	if (responseCount > 0)
	{
		qInfo() << "Answering LightDM authentication prompts; count" << responseCount;
		Q_EMIT writeToDaemon(packet(3, integer(responseCount) + responses));
	}
}

void LightDmAdapter::complete(bool success, const QString& error)
{
	m_remote = false;
	clearPassword();
	if (!success)
	{
		m_startingSession = false;
		m_requireNewAuthentication = true;
		Q_EMIT writeToDaemon(packet(5));
	}
	if (!success && m_localConversation)
	{
		// Finish the UI's displaced conversation so GTK/KDE can accept a new
		// manual attempt. Never report remote authentication success to the UI.
		m_localConversation = false;
		Q_EMIT writeToGreeter(packet(2, integer(m_localSequence) + string(m_localUsername.toUtf8()) + integer(PAM_AUTH_ERR)));
	}
	Q_EMIT finished(success, error);
}

void LightDmAdapter::cancel()
{
	if (m_remote)
	{
		const bool sessionResultPending = m_startingSession;
		if (sessionResultPending)
		{
			m_failed = true;
		}
		complete(false, QStringLiteral("Login cancelled or timed out"));
		if (sessionResultPending)
		{
			// START_SESSION replies carry no sequence. Close this connection rather
			// than let a late reply complete a different login or expose the still
			// authenticated account to the original UI. LightDM owns recovery.
			Q_EMIT protocolError(QStringLiteral("LightDM session start was cancelled; closing the greeter connection"));
		}
	}
}

void LightDmAdapter::failProtocol()
{
	m_failed = true;
	if (m_remote)
	{
		complete(false, QStringLiteral("Invalid LightDM greeter protocol message"));
	}
	Q_EMIT protocolError(QStringLiteral("Invalid LightDM greeter protocol message"));
}
