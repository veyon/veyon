/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "KWinScreencast.h"
#include "GreeterPipeWire.h"
#include "wayland-kwin-screencast-server.h"
#include "wayland-xdg-output-server.h"

#include <QLocalServer>
#include <QSignalSpy>
#include <QSocketNotifier>
#include <QTemporaryDir>
#include <QtTest>
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <wayland-server.h>

// This is a wire-protocol peer, not a replacement for the capture client.
// It lets the real client discover outputs and negotiate a stream without KWin.
class Compositor
{
public:
	Compositor(bool advertiseCapture = true, bool failStream = false) : reject(failStream)
	{
		display = wl_display_create();
		for (auto& geometry : outputGeometries)
		{
			wl_global_create(display, &wl_output_interface, 2, &geometry,
				[](wl_client* client, void* data, uint32_t version, uint32_t id) {
					auto* output = wl_resource_create(client, &wl_output_interface, int(version), id);
					wl_resource_set_user_data(output, data);
					const auto* geometry = static_cast<QRect*>(data);
					wl_output_send_geometry(output, geometry->x(), geometry->y(), 0, 0, WL_OUTPUT_SUBPIXEL_UNKNOWN, "test", "test", WL_OUTPUT_TRANSFORM_NORMAL);
					// Pixel mode intentionally differs from logical geometry (fractional scale).
					wl_output_send_mode(output, WL_OUTPUT_MODE_CURRENT, geometry->width() * 3 / 2, geometry->height() * 3 / 2, 60000);
					if (version >= 2) { wl_output_send_scale(output, 2); wl_output_send_done(output); }
				});
		}
		wl_global_create(display, &zxdg_output_manager_v1_interface, 2, this,
			[](wl_client* client, void* data, uint32_t version, uint32_t id) {
				auto* manager = wl_resource_create(client, &zxdg_output_manager_v1_interface, int(version), id);
				static const struct zxdg_output_manager_v1_interface implementation = {
					.destroy = [](wl_client*, wl_resource* resource) { wl_resource_destroy(resource); },
					.get_xdg_output = [](wl_client* peer, wl_resource* manager, uint32_t object, wl_resource* legacyOutput) {
						auto* output = wl_resource_create(peer, &zxdg_output_v1_interface, 2, object);
						static_cast<Compositor*>(wl_resource_get_user_data(manager))->logicalOutputs.append(output);
						static const struct zxdg_output_v1_interface outputImplementation = {
							.destroy = [](wl_client*, wl_resource* resource) { wl_resource_destroy(resource); },
						};
						wl_resource_set_implementation(output, &outputImplementation, nullptr, nullptr);
						const auto* geometry = static_cast<QRect*>(wl_resource_get_user_data(legacyOutput));
						zxdg_output_v1_send_logical_position(output, geometry->x(), geometry->y());
						zxdg_output_v1_send_logical_size(output, geometry->width(), geometry->height());
						zxdg_output_v1_send_done(output);
					},
				};
				wl_resource_set_implementation(manager, &implementation, data, nullptr);
			});
		if (advertiseCapture)
		{
			wl_global_create(display, &zkde_screencast_unstable_v1_interface, 3, this,
				[](wl_client* client, void* data, uint32_t version, uint32_t id) {
					auto* manager = wl_resource_create(client, &zkde_screencast_unstable_v1_interface, int(version), id);
					static const struct zkde_screencast_unstable_v1_interface implementation = {
						.stream_output = nullptr,
						.stream_window = nullptr,
						.destroy = [](wl_client*, wl_resource* resource) { wl_resource_destroy(resource); },
						.stream_virtual_output = nullptr,
						.stream_region = [](wl_client* peer, wl_resource* resource, uint32_t id,
							int32_t x, int32_t y, uint32_t width, uint32_t height, wl_fixed_t scale, uint32_t cursor) {
							auto* self = static_cast<Compositor*>(wl_resource_get_user_data(resource));
							self->region = QRect(x, y, int(width), int(height));
							self->cursorMode = cursor;
							self->requestedScale = wl_fixed_to_double(scale);
							self->stream = wl_resource_create(peer, &zkde_screencast_stream_unstable_v1_interface, 3, id);
							static const struct zkde_screencast_stream_unstable_v1_interface streamImplementation = {
								.close = [](wl_client*, wl_resource* stream) { wl_resource_destroy(stream); },
							};
							wl_resource_set_implementation(self->stream, &streamImplementation, nullptr, nullptr);
							if (self->reject) { zkde_screencast_stream_unstable_v1_send_failed(self->stream, "denied"); }
							else { zkde_screencast_stream_unstable_v1_send_created(self->stream, 42); }
						},
					};
					wl_resource_set_implementation(manager, &implementation, data, nullptr);
				});
		}
		auto* eventLoop = wl_display_get_event_loop(display);
		notifier = std::make_unique<QSocketNotifier>(wl_event_loop_get_fd(eventLoop), QSocketNotifier::Read);
		QObject::connect(notifier.get(), &QSocketNotifier::activated, notifier.get(), [this, eventLoop]() {
			wl_event_loop_dispatch(eventLoop, 0);
			wl_display_flush_clients(display);
		});
	}
	~Compositor()
	{
		notifier.reset();
		wl_display_destroy_clients(display);
		wl_display_destroy(display);
	}
	int connectClient()
	{
		int sockets[2];
		if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sockets) != 0) { return -1; }
		wl_client_create(display, sockets[0]);
		return sockets[1];
	}
	void closeStream()
	{
		zkde_screencast_stream_unstable_v1_send_closed(stream);
		wl_display_flush_clients(display);
	}
	std::array<QRect, 2> outputGeometries{QRect(-640, 0, 640, 480), QRect(0, 0, 960, 540)};
	wl_display* display{nullptr};
	wl_resource* stream{nullptr};
	QList<wl_resource*> logicalOutputs;
	QRect region;
	uint32_t cursorMode{0};
	double requestedScale{0};
	bool reject;
	std::unique_ptr<QSocketNotifier> notifier;
};

class KWinScreencastTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void usesLogicalWorkspaceAndEmbeddedCursor()
	{
		Compositor compositor;
		KWinScreencast capture;
		QSignalSpy created(&capture, &KWinScreencast::streamCreated);
		QSignalSpy failed(&capture, &KWinScreencast::failed);
		QVERIFY(capture.start(compositor.connectClient()));
		QTRY_COMPARE(created.size(), 1);
		QCOMPARE(created.first().first().toUInt(), uint(42));
		QCOMPARE(compositor.region, QRect(-640, 0, 1600, 540));
		QCOMPARE(compositor.cursorMode, uint(2));
		QCOMPARE(compositor.requestedScale, 1.0);
		QVERIFY(failed.isEmpty());
		compositor.closeStream();
		QTRY_COMPARE(failed.size(), 1);
	}
	void layoutChangeEndsTheOldCapture()
	{
		Compositor compositor;
		KWinScreencast capture;
		QSignalSpy created(&capture, &KWinScreencast::streamCreated);
		QSignalSpy failed(&capture, &KWinScreencast::failed);
		QVERIFY(capture.start(compositor.connectClient()));
		QTRY_COMPARE(created.size(), 1);
		zxdg_output_v1_send_logical_size(compositor.logicalOutputs.first(), 800, 600);
		zxdg_output_v1_send_done(compositor.logicalOutputs.first());
		wl_display_flush_clients(compositor.display);
		QTRY_COMPARE(failed.size(), 1);
	}
	void invalidGeometryNeverCreatesAStream()
	{
		Compositor compositor;
		compositor.outputGeometries[0].setWidth(20000);
		KWinScreencast capture;
		QSignalSpy created(&capture, &KWinScreencast::streamCreated);
		QSignalSpy failed(&capture, &KWinScreencast::failed);
		QVERIFY(capture.start(compositor.connectClient()));
		QTRY_COMPARE(failed.size(), 1);
		QVERIFY(created.isEmpty());
	}
	void missingProtocolFailsWithoutStartingAStream()
	{
		Compositor compositor(false);
		KWinScreencast capture;
		QSignalSpy created(&capture, &KWinScreencast::streamCreated);
		QSignalSpy failed(&capture, &KWinScreencast::failed);
		QVERIFY(capture.start(compositor.connectClient()));
		QTRY_COMPARE(failed.size(), 1);
		QVERIFY(created.isEmpty());
	}
	void compositorRejectionIsReported()
	{
		Compositor compositor(true, true);
		KWinScreencast capture;
		QSignalSpy created(&capture, &KWinScreencast::streamCreated);
		QSignalSpy failed(&capture, &KWinScreencast::failed);
		QVERIFY(capture.start(compositor.connectClient()));
		QTRY_COMPARE(failed.size(), 1);
		QVERIFY(created.isEmpty());
	}
	void unresponsiveCompositorTimesOut()
	{
		int sockets[2];
		QVERIFY(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sockets) == 0);
		KWinScreencast capture(100);
		QSignalSpy failed(&capture, &KWinScreencast::failed);
		QVERIFY(capture.start(sockets[0]));
		QTRY_COMPARE(failed.size(), 1);
		::close(sockets[1]);
	}
	void pipeWireSocketChecksActualPeerUid()
	{
		QTemporaryDir directory;
		QLocalServer server;
		const auto path = directory.filePath(QStringLiteral("pipewire-0"));
		QVERIFY(server.listen(path));
		const auto fd = GreeterPipeWire::connectSocket(path, uint(::getuid()));
		QVERIFY(fd >= 0);
		::close(fd);
		QVERIFY(GreeterPipeWire::connectSocket(path, uint(::getuid()) + 1) < 0);
	}
};

QTEST_GUILESS_MAIN(KWinScreencastTest)
#include "KWinScreencastTest.moc"
