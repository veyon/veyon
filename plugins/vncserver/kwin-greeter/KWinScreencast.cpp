/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "KWinScreencast.h"
#include "wayland-kwin-screencast-client.h"
#include "wayland-xdg-output-client.h"

#include <QRect>
#include <QSocketNotifier>
#include <QTimer>
#include <cerrno>
#include <cstring>
#include <map>
#include <sys/socket.h>
#include <unistd.h>
#include <wayland-client.h>

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
			if (wl_display_dispatch(display) < 0) { fail(QStringLiteral("Greeter Wayland connection closed")); }
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
			fail(QStringLiteral("Could not send greeter Wayland requests"));
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
KWinScreencast::~KWinScreencast() = default;

bool KWinScreencast::start(int waylandFd)
{
	stop();
	m_connection = std::make_unique<Connection>(this);
	return m_connection->start(waylandFd);
}

void KWinScreencast::stop() { m_connection.reset(); }
