// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QDBusError>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QObject>
#include <QQueue>
#include <QTimer>
#include <functional>

// Portal methods may run concurrently. Keep all input in one FIFO so modifier
// transitions and pointer clicks cannot overtake the keys that depend on them.
class PortalInputDispatcher : public QObject
{
public:
	using Send = std::function<QDBusPendingCall()>;
	explicit PortalInputDispatcher(QObject* parent = nullptr) : QObject(parent)
	{
		m_deadline.setSingleShot(true);
		connect(&m_deadline, &QTimer::timeout, this, [this]() {
			fail(QDBusError(QDBusError::NoReply, QStringLiteral("Portal input request timed out")));
		});
	}
	std::function<void(const QDBusError&)> onError;
	void enqueue(Send send)
	{
		if (m_failed) return;
		if (m_pending.size() >= MaximumPendingEvents) {
			fail(QDBusError(QDBusError::LimitsExceeded, QStringLiteral("Portal input queue limit exceeded")));
			return;
		}
		m_pending.enqueue(std::move(send));
		sendNext();
	}
	void clear(bool resetFailure = true)
	{
		m_pending.clear();m_deadline.stop();
		if (resetFailure) m_failed = false;
		if (m_inFlight) {
			m_inFlight->disconnect(this);
			m_inFlight->deleteLater();m_inFlight = nullptr;
		}
	}
private:
	void sendNext()
	{
		if (m_inFlight || m_pending.isEmpty() || m_failed) return;
		auto send = m_pending.dequeue();
		auto watcher = new QDBusPendingCallWatcher(send(), this);
		m_inFlight = watcher;
		connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
			m_deadline.stop();m_inFlight = nullptr;
			const QDBusPendingReply<> reply = *watcher;
			watcher->deleteLater();
			if (reply.isError()) fail(reply.error());
			else sendNext();
		});
		m_deadline.start(ReplyTimeoutMs);
	}
	void fail(const QDBusError& error)
	{
		clear();m_failed = true;
		if (onError) onError(error);
	}
	static constexpr int MaximumPendingEvents = 4096;
	static constexpr int ReplyTimeoutMs = 5000;
	QQueue<Send> m_pending;
	QDBusPendingCallWatcher* m_inFlight{nullptr};
	QTimer m_deadline{this};
	bool m_failed{false};
};
