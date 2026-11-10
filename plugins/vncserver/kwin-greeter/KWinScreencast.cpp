/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "KWinScreencast.h"
#include "KWinInput.h"
#include "wayland-kwin-screencast-client.h"
#include "wayland-xdg-output-client.h"

#include <QRect>
#include <QPointer>
#include <QSocketNotifier>
#include <QTimer>
#include <cerrno>
#include <cstring>
#include <map>
#include <sys/socket.h>
#include <unistd.h>
#include <wayland-client.h>

namespace
{
QString connectionError(wl_display* display, const QString& message)
{
	const auto error = wl_display_get_error(display);
	if (error == EPROTO)
	{
		const wl_interface* interface = nullptr;
		uint32_t object = 0;
		const auto code = wl_display_get_protocol_error(display, &interface, &object);
		return QStringLiteral("%1 (Wayland protocol %2, object %3, code %4)")
			.arg(message, QString::fromLatin1(interface ? interface->name : "unknown")).arg(object).arg(code);
	}
	return QStringLiteral("%1 (error %2: %3)").arg(message).arg(error)
		.arg(QString::fromLocal8Bit(std::strerror(error)));
}
}

class KWinScreencast::InputConnection : public QObject
{
public:
	explicit InputConnection(KWinScreencast* owner) : owner(owner),
		input([this]() { flush(); }, [this]() {
			if (this->owner && !retiring)
			{
				Q_EMIT this->owner->inputAvailabilityChanged(!finished && input.pointerAvailable(), !finished && input.keyboardAvailable());
			}
		})
	{
		deadline.setSingleShot(true);
		QObject::connect(&deadline, &QTimer::timeout, this, [this]() {
			fail(QStringLiteral("KWin greeter input negotiation timed out"));
		});
	}
	~InputConnection()
	{
		input.stop();
		readNotifier.reset();
		writeNotifier.reset();
		if (sync) { wl_callback_destroy(sync); }
		if (registry) { wl_registry_destroy(registry); }
		if (display) { wl_display_flush(display); wl_display_disconnect(display); }
	}
	bool start(int fd)
	{
		ucred peer{};
		socklen_t size = sizeof(peer);
		if (fd < 0 || ::getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &peer, &size) != 0 ||
			size != sizeof(peer) || peer.uid != ::getuid())
		{
			if (fd >= 0) { ::close(fd); }
			fail(QStringLiteral("Input Wayland socket does not belong to the greeter account"));
			return false;
		}
		display = wl_display_connect_to_fd(fd);
		if (!display) { fail(QStringLiteral("Could not connect to the greeter input compositor")); return false; }
		registry = wl_display_get_registry(display);
		static const wl_registry_listener listener = {
			.global = [](void* data, wl_registry* registry, uint32_t id, const char* name, uint32_t version) {
				static_cast<InputConnection*>(data)->input.announce(registry, id, name, version);
			},
			.global_remove = [](void* data, wl_registry*, uint32_t id) {
				static_cast<InputConnection*>(data)->input.removeGlobal(id);
			},
		};
		wl_registry_add_listener(registry, &listener, this);
		readNotifier = std::make_unique<QSocketNotifier>(wl_display_get_fd(display), QSocketNotifier::Read);
		QObject::connect(readNotifier.get(), &QSocketNotifier::activated, this, [this]() {
			if (wl_display_dispatch(display) < 0) { fail(connectionError(display, QStringLiteral("Greeter input Wayland connection closed"))); }
			else { flush(); }
		});
		writeNotifier = std::make_unique<QSocketNotifier>(wl_display_get_fd(display), QSocketNotifier::Write);
		writeNotifier->setEnabled(false);
		QObject::connect(writeNotifier.get(), &QSocketNotifier::activated, this, [this]() { flush(); });
		sync = wl_display_sync(display);
		static const wl_callback_listener syncListener = {
			.done = [](void* data, wl_callback* callback, uint32_t) {
				auto* self = static_cast<InputConnection*>(data);
				wl_callback_destroy(callback);
				self->sync = nullptr;
				self->deadline.stop();
				if (!self->finished) { self->input.registryReady(); }
			},
		};
		wl_callback_add_listener(sync, &syncListener, this);
		deadline.start(owner->m_timeoutMs);
		return flush();
	}
	void retire()
	{
		retiring = true;
		input.stop();
		if (finished || !display) { deleteLater(); return; }
		if (sync) { wl_callback_destroy(sync); }
		sync = wl_display_sync(display);
		static const wl_callback_listener listener = {
			.done = [](void* data, wl_callback* callback, uint32_t) {
				auto* self = static_cast<InputConnection*>(data);
				wl_callback_destroy(callback);
				self->sync = nullptr;
				self->deleteLater();
			},
		};
		wl_callback_add_listener(sync, &listener, this);
		deadline.start(1000);
		flush();
	}
	bool flush()
	{
		if (finished || !display) { return false; }
		if (wl_display_flush(display) < 0)
		{
			if (errno == EAGAIN) { writeNotifier->setEnabled(true); return true; }
			fail(connectionError(display, QStringLiteral("Could not send greeter input Wayland requests")));
			return false;
		}
		writeNotifier->setEnabled(false);
		return true;
	}
	void fail(const QString& error)
	{
		if (retiring) { deleteLater(); return; }
		if (finished) { return; }
		finished = true;
		deadline.stop();
		if (readNotifier) { readNotifier->setEnabled(false); }
		if (writeNotifier) { writeNotifier->setEnabled(false); }
		input.stop();
		if (owner)
		{
			Q_EMIT owner->inputAvailabilityChanged(false, false);
			Q_EMIT owner->inputFailed(error);
		}
	}
	QPointer<KWinScreencast> owner;
	KWinInput input;
	wl_display* display{nullptr};
	wl_registry* registry{nullptr};
	wl_callback* sync{nullptr};
	std::unique_ptr<QSocketNotifier> readNotifier;
	std::unique_ptr<QSocketNotifier> writeNotifier;
	QTimer deadline;
	bool finished{false};
	bool retiring{false};
};

class KWinScreencast::Connection
{
public:
	explicit Connection(KWinScreencast* owner) : owner(owner)
	{
		deadline.setSingleShot(true);
		QObject::connect(&deadline, &QTimer::timeout, owner, [this]() {
			fail(QStringLiteral("KWin screencast negotiation timed out"));
		});
	}
	~Connection()
	{
		readNotifier.reset();
		writeNotifier.reset();
		if (sync) { wl_callback_destroy(sync); }
		if (stream) { zkde_screencast_stream_unstable_v1_close(stream); }
		for (auto& [id, output] : outputs)
		{
			if (output.logical) { zxdg_output_v1_destroy(output.logical); }
			wl_output_destroy(output.output);
		}
		if (outputManager) { zxdg_output_manager_v1_destroy(outputManager); }
		if (manager) { zkde_screencast_unstable_v1_destroy(manager); }
		if (registry) { wl_registry_destroy(registry); }
		if (display)
		{
			wl_display_flush(display);
			wl_display_disconnect(display);
		}
	}

	bool start(int fd)
	{
		ucred peer{};
		socklen_t size = sizeof(peer);
		if (fd < 0 || ::getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &peer, &size) != 0 ||
			size != sizeof(peer) || peer.uid != ::getuid())
		{
			if (fd >= 0) { ::close(fd); }
			fail(QStringLiteral("Wayland socket does not belong to the greeter account"));
			return false;
		}
		display = wl_display_connect_to_fd(fd); // owns fd, including on failure
		if (!display)
		{
			fail(QStringLiteral("Could not connect to the greeter Wayland compositor"));
			return false;
		}
		registry = wl_display_get_registry(display);
		static const wl_registry_listener listener = {
			.global = [](void* data, wl_registry* registry, uint32_t id, const char* name, uint32_t version) {
				auto* self = static_cast<Connection*>(data);
				if (std::strcmp(name, zkde_screencast_unstable_v1_interface.name) == 0 && version >= 3)
				{
					self->manager = static_cast<zkde_screencast_unstable_v1*>(wl_registry_bind(registry, id, &zkde_screencast_unstable_v1_interface, 3));
				}
				else if (std::strcmp(name, zxdg_output_manager_v1_interface.name) == 0 && version >= 2)
				{
					self->outputManager = static_cast<zxdg_output_manager_v1*>(wl_registry_bind(registry, id, &zxdg_output_manager_v1_interface, 2));
				}
				else if (std::strcmp(name, wl_output_interface.name) == 0)
				{
					if (self->geometryRequested || self->outputs.size() >= 32)
					{
						self->fail(QStringLiteral("Greeter output layout changed or exceeds capture limits"));
						return;
					}
					// Geometry comes from xdg-output; consume legacy events too.
					auto* output = static_cast<wl_output*>(wl_registry_bind(registry, id, &wl_output_interface, 1));
					static const wl_output_listener outputListener = {
						.geometry = [](void*, wl_output*, int32_t, int32_t, int32_t, int32_t, int32_t, const char*, const char*, int32_t) {},
						.mode = [](void*, wl_output*, uint32_t, int32_t, int32_t, int32_t) {},
						.done = nullptr,
						.scale = nullptr,
					};
					wl_output_add_listener(output, &outputListener, nullptr);
					self->outputs.emplace(id, Output{self, output, nullptr, {}, false});
				}
			},
			.global_remove = [](void* data, wl_registry*, uint32_t id) {
				auto* self = static_cast<Connection*>(data);
				if (self->outputs.count(id)) { self->fail(QStringLiteral("Greeter output was removed; capture must restart")); }
			},
		};
		wl_registry_add_listener(registry, &listener, this);
		readNotifier = std::make_unique<QSocketNotifier>(wl_display_get_fd(display), QSocketNotifier::Read);
		QObject::connect(readNotifier.get(), &QSocketNotifier::activated, owner, [this]() {
			if (wl_display_dispatch(display) < 0) { fail(connectionError(display, QStringLiteral("Greeter capture Wayland connection closed"))); }
			else { flush(); }
		});
		writeNotifier = std::make_unique<QSocketNotifier>(wl_display_get_fd(display), QSocketNotifier::Write);
		writeNotifier->setEnabled(false);
		QObject::connect(writeNotifier.get(), &QSocketNotifier::activated, owner, [this]() { flush(); });
		synchronize();
		deadline.start(owner->m_timeoutMs);
		return flush();
	}

	void synchronize()
	{
		sync = wl_display_sync(display);
		static const wl_callback_listener listener = {
			.done = [](void* data, wl_callback* callback, uint32_t) {
				auto* self = static_cast<Connection*>(data);
				wl_callback_destroy(callback);
				self->sync = nullptr;
				if (self->finished) { return; }
				if (!self->geometryRequested) { self->requestGeometry(); }
				else { self->requestStream(); }
			},
		};
		wl_callback_add_listener(sync, &listener, this);
	}

	void requestGeometry()
	{
		if (!manager || !outputManager || outputs.empty())
		{
			fail(QStringLiteral("KWin screencast or logical-output protocol is unavailable; check X-KDE-Wayland-Interfaces"));
			return;
		}
		geometryRequested = true;
		static const zxdg_output_v1_listener listener = {
			.logical_position = [](void* data, zxdg_output_v1*, int32_t x, int32_t y) {
				auto* output = static_cast<Output*>(data);
				if (x < -32768 || x > 32768 || y < -32768 || y > 32768)
				{
					output->connection->fail(QStringLiteral("Invalid greeter output position")); return;
				}
				if (output->complete && output->geometry.topLeft() != QPoint(x, y))
				{
					output->connection->fail(QStringLiteral("Greeter output layout changed; capture must restart")); return;
				}
				output->geometry.moveTopLeft(QPoint(x, y));
			},
			.logical_size = [](void* data, zxdg_output_v1*, int32_t width, int32_t height) {
				auto* output = static_cast<Output*>(data);
				if (width <= 0 || width > 16384 || height <= 0 || height > 16384)
				{
					output->connection->fail(QStringLiteral("Invalid greeter output size")); return;
				}
				if (output->complete && output->geometry.size() != QSize(width, height))
				{
					output->connection->fail(QStringLiteral("Greeter output layout changed; capture must restart")); return;
				}
				output->geometry.setSize(QSize(width, height));
			},
			.done = [](void* data, zxdg_output_v1*) { static_cast<Output*>(data)->complete = true; },
			.name = [](void*, zxdg_output_v1*, const char*) {},
			.description = [](void*, zxdg_output_v1*, const char*) {},
		};
		for (auto& [id, output] : outputs)
		{
			output.logical = zxdg_output_manager_v1_get_xdg_output(outputManager, output.output);
			zxdg_output_v1_add_listener(output.logical, &listener, &output);
		}
		synchronize();
	}

	void requestStream()
	{
		QRect workspace;
		for (const auto& [id, output] : outputs)
		{
			const auto& geometry = output.geometry;
			if (!output.complete || !geometry.isValid() || geometry.width() > 16384 || geometry.height() > 16384 ||
				geometry.x() < -32768 || geometry.x() > 32768 || geometry.y() < -32768 || geometry.y() > 32768)
			{
				fail(QStringLiteral("Invalid greeter logical output geometry"));
				return;
			}
			workspace |= geometry;
		}
		if (workspace.width() > 16384 || workspace.height() > 16384 ||
			quint64(workspace.width()) * quint64(workspace.height()) * 4 > 128 * 1024 * 1024)
		{
			fail(QStringLiteral("Greeter workspace exceeds capture size limits"));
			return;
		}
		// Logical resolution includes every monitor and handles fractional scaling.
		this->workspace = workspace;
		stream = zkde_screencast_unstable_v1_stream_region(manager, workspace.x(), workspace.y(),
			uint32_t(workspace.width()), uint32_t(workspace.height()), wl_fixed_from_int(1),
			ZKDE_SCREENCAST_UNSTABLE_V1_POINTER_EMBEDDED);
		static const zkde_screencast_stream_unstable_v1_listener listener = {
			.closed = [](void* data, zkde_screencast_stream_unstable_v1*) {
				static_cast<Connection*>(data)->fail(QStringLiteral("KWin screencast stream closed"));
			},
			.created = [](void* data, zkde_screencast_stream_unstable_v1*, uint32_t node) {
				auto* self = static_cast<Connection*>(data);
				if (self->finished) { return; }
				if (self->created) { self->fail(QStringLiteral("KWin returned a duplicate screencast node")); return; }
				if (node == uint32_t(-1)) { self->fail(QStringLiteral("KWin returned an invalid PipeWire node")); return; }
				self->created = true;
				self->deadline.stop();
				Q_EMIT self->owner->streamCreated(node);
			},
			.failed = [](void* data, zkde_screencast_stream_unstable_v1*, const char* error) {
				static_cast<Connection*>(data)->fail(QStringLiteral("KWin screencast failed: ") + QString::fromUtf8(error));
			},
		};
		zkde_screencast_stream_unstable_v1_add_listener(stream, &listener, this);
	}

	bool flush()
	{
		if (finished) { return false; }
		if (wl_display_flush(display) < 0)
		{
			if (errno == EAGAIN) { writeNotifier->setEnabled(true); return true; }
			fail(connectionError(display, QStringLiteral("Could not send greeter capture Wayland requests")));
			return false;
		}
		writeNotifier->setEnabled(false);
		return true;
	}
	void fail(const QString& error)
	{
		if (finished) { return; }
		finished = true;
		deadline.stop();
		if (readNotifier) { readNotifier->setEnabled(false); }
		if (writeNotifier) { writeNotifier->setEnabled(false); }
		Q_EMIT owner->failed(error);
	}

	struct Output { Connection* connection; wl_output* output; zxdg_output_v1* logical; QRect geometry; bool complete; };
	KWinScreencast* owner;
	QRect workspace;
	wl_display* display{nullptr};
	wl_registry* registry{nullptr};
	wl_callback* sync{nullptr};
	zkde_screencast_unstable_v1* manager{nullptr};
	zxdg_output_manager_v1* outputManager{nullptr};
	zkde_screencast_stream_unstable_v1* stream{nullptr};
	std::map<uint32_t, Output> outputs;
	std::unique_ptr<QSocketNotifier> readNotifier;
	std::unique_ptr<QSocketNotifier> writeNotifier;
	QTimer deadline;
	bool geometryRequested{false};
	bool created{false};
	bool finished{false};
};

KWinScreencast::KWinScreencast(int timeoutMs, QObject* parent) : QObject(parent), m_timeoutMs(timeoutMs) {}
KWinScreencast::~KWinScreencast() { stop(); }

bool KWinScreencast::start(int waylandFd)
{
	stop();
	m_connection = std::make_unique<Connection>(this);
	return m_connection->start(waylandFd);
}

bool KWinScreencast::startInput(int waylandFd)
{
	retireInput();
	if (!m_connection || m_connection->finished || !m_connection->created)
	{
		if (waylandFd >= 0) { ::close(waylandFd); }
		return false;
	}
	m_inputConnection = std::make_unique<InputConnection>(this);
	return m_inputConnection->start(waylandFd);
}

void KWinScreencast::stop()
{
	retireInput();
	m_connection.reset();
	if (m_retiringInput == 0) { Q_EMIT inputStopped(); }
}

void KWinScreencast::retireInput()
{
	if (!m_inputConnection) { return; }
	auto* connection = m_inputConnection.release();
	++m_retiringInput;
	connect(connection, &QObject::destroyed, this, [this]() {
		if (--m_retiringInput == 0 && !m_inputConnection) { Q_EMIT inputStopped(); }
	});
	connection->retire();
}

void KWinScreencast::keyEvent(quintptr client, quint32 symbol, bool down)
{
	if (keyboardInputAvailable()) { m_inputConnection->input.keyEvent(client, symbol, down); }
}

void KWinScreencast::pointerEvent(quintptr client, int buttons, int x, int y, const QSize& frameSize)
{
	if (pointerInputAvailable())
	{
		m_inputConnection->input.pointerEvent(client, buttons, x, y, m_connection->workspace, frameSize);
	}
}

void KWinScreencast::releaseClient(quintptr client)
{
	if (m_inputConnection && !m_inputConnection->finished) { m_inputConnection->input.releaseClient(client); }
}

bool KWinScreencast::pointerInputAvailable() const
{
	return m_connection && !m_connection->finished && m_connection->created &&
		m_inputConnection && !m_inputConnection->finished && m_inputConnection->input.pointerAvailable();
}

bool KWinScreencast::keyboardInputAvailable() const
{
	return m_connection && !m_connection->finished && m_connection->created &&
		m_inputConnection && !m_inputConnection->finished && m_inputConnection->input.keyboardAvailable();
}
