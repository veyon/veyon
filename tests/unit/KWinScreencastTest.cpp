/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "KWinScreencast.h"
#include "GreeterPipeWire.h"
#include "wayland-kwin-screencast-server.h"
#include "wayland-xdg-output-server.h"
#include "wayland-fake-input-server.h"
#include "wayland-keystate-server.h"

#include <QLocalServer>
#include <QSignalSpy>
#include <QSocketNotifier>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <linux/input-event-codes.h>
#include <cstdlib>
#include <cstring>
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
	Compositor(bool advertiseCapture = true, bool failStream = false, uint32_t inputVersion = 0, bool provideKeymap = true, bool failInput = false, bool provideLockState = true) : reject(failStream), rejectInput(failInput)
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
		if (inputVersion)
		{
			if (provideLockState)
			{
				wl_global_create(display, &org_kde_kwin_keystate_interface, 1, this,
					[](wl_client* client, void* data, uint32_t version, uint32_t id) {
						auto* resource = wl_resource_create(client, &org_kde_kwin_keystate_interface, int(version), id);
						static const struct org_kde_kwin_keystate_interface implementation = {
							.fetchStates = [](wl_client*, wl_resource* resource) {
								auto* self = static_cast<Compositor*>(wl_resource_get_user_data(resource));
								org_kde_kwin_keystate_send_stateChanged(resource, ORG_KDE_KWIN_KEYSTATE_KEY_CAPSLOCK,
									self->capsLock ? ORG_KDE_KWIN_KEYSTATE_STATE_LOCKED : ORG_KDE_KWIN_KEYSTATE_STATE_UNLOCKED);
							},
							.destroy = [](wl_client*, wl_resource* resource) { wl_resource_destroy(resource); },
						};
						auto* self = static_cast<Compositor*>(data);
						self->lockStates.append(resource);
						wl_resource_set_implementation(resource, &implementation, data, [](wl_resource* resource) {
							static_cast<Compositor*>(wl_resource_get_user_data(resource))->lockStates.removeAll(resource);
						});
					});
			}

			if (provideKeymap)
			{
				auto* context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
				xkb_rule_names names{};
				names.rules = "evdev";
				names.model = "pc105";
				names.layout = "us,ru";
				auto* map = xkb_keymap_new_from_names(context, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
				if (map)
				{
					auto* text = xkb_keymap_get_as_string(map, XKB_KEYMAP_FORMAT_TEXT_V1);
					if (!keyboardMap.open()) { qFatal("Could not open the test keymap file"); }
					keyboardMap.write(text, qint64(std::strlen(text)) + 1);
					keyboardMap.flush(); // leave the FD at EOF to exercise receiver seeking
					std::free(text);
					xkb_keymap_unref(map);
				}
				xkb_context_unref(context);
			}
			wl_global_create(display, &wl_seat_interface, 5, this,
				[](wl_client* client, void* data, uint32_t version, uint32_t id) {
					auto* seat = wl_resource_create(client, &wl_seat_interface, int(version), id);
					static const struct wl_seat_interface implementation = {
						.get_pointer = nullptr,
						.get_keyboard = [](wl_client* client, wl_resource* seat, uint32_t id) {
							auto* self = static_cast<Compositor*>(wl_resource_get_user_data(seat));
							auto* keyboard = wl_resource_create(client, &wl_keyboard_interface, wl_resource_get_version(seat), id);
							static const struct wl_keyboard_interface implementation = {
								.release = [](wl_client*, wl_resource* resource) { wl_resource_destroy(resource); },
							};
							wl_resource_set_implementation(keyboard, &implementation, nullptr, nullptr);
							wl_keyboard_send_keymap(keyboard, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, self->keyboardMap.handle(), uint32_t(self->keyboardMap.size()));
						},
						.get_touch = nullptr,
						.release = [](wl_client*, wl_resource* resource) { wl_resource_destroy(resource); },
					};
					wl_resource_set_implementation(seat, &implementation, data, nullptr);
					const auto* self = static_cast<Compositor*>(data);
					wl_seat_send_capabilities(seat, self->keyboardMap.isOpen() ? WL_SEAT_CAPABILITY_KEYBOARD : 0);
				});
			wl_global_create(display, &org_kde_kwin_fake_input_interface, int(inputVersion), this,
				[](wl_client* client, void* data, uint32_t version, uint32_t id) {
					auto* input = wl_resource_create(client, &org_kde_kwin_fake_input_interface, int(version), id);
					static const struct org_kde_kwin_fake_input_interface implementation = {
						.authenticate = [](wl_client*, wl_resource* resource, const char*, const char*) {
							auto* self = static_cast<Compositor*>(wl_resource_get_user_data(resource));
							if (self->rejectInput) { wl_resource_post_error(resource, 0, "input rejected"); return; }
							self->authenticated = true;
						},
						.pointer_motion = nullptr,
						.button = [](wl_client*, wl_resource* resource, uint32_t button, uint32_t state) {
							auto* self = static_cast<Compositor*>(wl_resource_get_user_data(resource));
							if (self->authenticated) { self->buttons.append({button, state}); }
						},
						.axis = [](wl_client*, wl_resource* resource, uint32_t axis, wl_fixed_t delta) {
							auto* self = static_cast<Compositor*>(wl_resource_get_user_data(resource));
							if (self->authenticated) { self->axes.append(QPointF(axis, wl_fixed_to_double(delta))); }
						},
						.touch_down = nullptr,
						.touch_motion = nullptr,
						.touch_up = nullptr,
						.touch_cancel = nullptr,
						.touch_frame = nullptr,
						.pointer_motion_absolute = [](wl_client*, wl_resource* resource, wl_fixed_t x, wl_fixed_t y) {
							auto* self = static_cast<Compositor*>(wl_resource_get_user_data(resource));
							if (self->authenticated) { self->positions.append(QPointF(wl_fixed_to_double(x), wl_fixed_to_double(y))); }
						},
						.keyboard_key = [](wl_client*, wl_resource* resource, uint32_t code, uint32_t state) {
							auto* self = static_cast<Compositor*>(wl_resource_get_user_data(resource));
							if (self->authenticated) { self->keys.append({code, state}); }
						},
						.destroy = [](wl_client*, wl_resource* resource) { wl_resource_destroy(resource); },
						.keyboard_keysym = [](wl_client*, wl_resource* resource, uint32_t symbol, uint32_t state) {
							auto* self = static_cast<Compositor*>(wl_resource_get_user_data(resource));
							if (self->authenticated) { self->symbols.append({symbol, state}); }
						},
					};
					wl_resource_set_implementation(input, &implementation, data, nullptr);
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
	void setCapsLock(bool enabled)
	{
		capsLock = enabled;
		for (auto* resource : lockStates)
		{
			org_kde_kwin_keystate_send_stateChanged(resource, ORG_KDE_KWIN_KEYSTATE_KEY_CAPSLOCK,
				capsLock ? ORG_KDE_KWIN_KEYSTATE_STATE_LOCKED : ORG_KDE_KWIN_KEYSTATE_STATE_UNLOCKED);
		}
		wl_display_flush_clients(display);
	}
	QVector<wl_resource*> lockStates;
	bool capsLock{false};
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
	bool rejectInput;
	bool authenticated{false};
	QTemporaryFile keyboardMap;
	QVector<QPointF> positions;
	QVector<QPointF> axes;
	using Event = QPair<uint32_t, uint32_t>;
	QVector<Event> keys;
	QVector<Event> symbols;
	QVector<Event> buttons;
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
	void translatesPointerAndWheelFromFramebufferCoordinates()
	{
		Compositor compositor(true, false, 5);
		KWinScreencast capture;
		QSignalSpy created(&capture, &KWinScreencast::streamCreated);
		QVERIFY(capture.start(compositor.connectClient()));
		QTRY_COMPARE(created.size(), 1);
		QVERIFY(capture.startInput(compositor.connectClient()));
		QTRY_VERIFY(capture.pointerInputAvailable());
		capture.pointerEvent(1, 1, 50, 25, QSize(800, 270));
		capture.pointerEvent(1, 1 | 8, 50, 25, QSize(800, 270));
		capture.pointerEvent(1, 1 | 8, 51, 25, QSize(800, 270));
		capture.pointerEvent(1, 0, -10, 999, QSize(800, 270));
		capture.pointerEvent(1, 32, -10, 999, QSize(800, 270));
		capture.pointerEvent(1, 0, -10, 999, QSize(800, 270));
		QTRY_COMPARE(compositor.positions.size(), 6);
		QCOMPARE(compositor.positions.first(), QPointF(-540, 50));
		QCOMPARE(compositor.positions.last(), QPointF(-640, 538));
		QCOMPARE(compositor.buttons, (QVector<Compositor::Event>{{BTN_LEFT, 1}, {BTN_LEFT, 0}}));
		QCOMPARE(compositor.axes, (QVector<QPointF>{QPointF(0, -15), QPointF(1, -15)}));
	}
	void disconnectReleasesOnlyThatClientsHeldKeysAndButtons()
	{
		Compositor compositor(true, false, 5);
		KWinScreencast capture;
		QSignalSpy created(&capture, &KWinScreencast::streamCreated);
		QVERIFY(capture.start(compositor.connectClient()));
		QTRY_COMPARE(created.size(), 1);
		QVERIFY(capture.startInput(compositor.connectClient()));
		QTRY_VERIFY(capture.keyboardInputAvailable());
		capture.keyEvent(1, XKB_KEY_Control_L, true);
		capture.keyEvent(2, XKB_KEY_Control_L, true);
		capture.keyEvent(1, XKB_KEY_A, true);
		capture.keyEvent(1, XKB_KEY_A, true); // duplicate press must not leak a reference
		capture.pointerEvent(1, 1, 0, 0, QSize(1600, 540));
		capture.pointerEvent(2, 1, 0, 0, QSize(1600, 540));
		capture.releaseClient(1);
		QTRY_COMPARE(compositor.keys.size(), 5);
		QCOMPARE(compositor.keys, (QVector<Compositor::Event>{{KEY_LEFTCTRL, 1}, {KEY_LEFTSHIFT, 1}, {KEY_A, 1}, {KEY_A, 0}, {KEY_LEFTSHIFT, 0}}));
		QCOMPARE(compositor.buttons, (QVector<Compositor::Event>{{BTN_LEFT, 1}}));
		capture.keyEvent(3, XKB_KEY_Control_L, false); // unmatched release must not release client 2
		capture.releaseClient(2);
		QTRY_COMPARE(compositor.keys.size(), 6);
		QCOMPARE(compositor.keys.last(), (Compositor::Event{KEY_LEFTCTRL, 0}));
		QCOMPARE(compositor.buttons, (QVector<Compositor::Event>{{BTN_LEFT, 1}, {BTN_LEFT, 0}}));
	}
	void stoppingCaptureReleasesInputBeforeClosingWayland()
	{
		Compositor compositor(true, false, 5);
		KWinScreencast capture;
		QSignalSpy created(&capture, &KWinScreencast::streamCreated);
		QVERIFY(capture.start(compositor.connectClient()));
		QTRY_COMPARE(created.size(), 1);
		QVERIFY(capture.startInput(compositor.connectClient()));
		QTRY_VERIFY(capture.keyboardInputAvailable());
		capture.keyEvent(1, XKB_KEY_Alt_L, true);
		capture.pointerEvent(1, 4, 0, 0, QSize(1600, 540));
		QTRY_COMPARE(compositor.keys.size(), 1);
		QTRY_COMPARE(compositor.buttons.size(), 1);
		capture.stop();
		QTRY_COMPARE(compositor.keys.size(), 2);
		QCOMPARE(compositor.keys, (QVector<Compositor::Event>{{KEY_LEFTALT, 1}, {KEY_LEFTALT, 0}}));
		QCOMPARE(compositor.buttons, (QVector<Compositor::Event>{{BTN_RIGHT, 1}, {BTN_RIGHT, 0}}));
	}
	void destructionDeliversHeldInputReleases()
	{
		Compositor compositor(true, false, 5);
		auto capture = std::make_unique<KWinScreencast>();
		QSignalSpy created(capture.get(), &KWinScreencast::streamCreated);
		QVERIFY(capture->start(compositor.connectClient()));
		QTRY_COMPARE(created.size(), 1);
		QVERIFY(capture->startInput(compositor.connectClient()));
		QTRY_VERIFY(capture->keyboardInputAvailable());
		capture->keyEvent(1, XKB_KEY_Control_L, true);
		QTRY_COMPARE(compositor.keys.size(), 1);
		capture.reset();
		QTRY_COMPARE(compositor.keys.size(), 2);
		QCOMPARE(compositor.keys.last(), (Compositor::Event{KEY_LEFTCTRL, 0}));
	}
	void stoppingInputDoesNotWaitForeverForTheCompositor()
	{
		Compositor compositor(true, false, 5);
		KWinScreencast capture;
		QSignalSpy created(&capture, &KWinScreencast::streamCreated);
		QSignalSpy stopped(&capture, &KWinScreencast::inputStopped);
		QVERIFY(capture.start(compositor.connectClient()));
		QTRY_COMPARE(created.size(), 1);
		QVERIFY(capture.startInput(compositor.connectClient()));
		QTRY_VERIFY(capture.keyboardInputAvailable());
		stopped.clear();
		compositor.notifier->setEnabled(false);
		capture.stop();
		QTRY_COMPARE_WITH_TIMEOUT(stopped.size(), 1, 3000);
	}
	void changedReleaseKeysymDoesNotLeaveAKeyPressed()
	{
		Compositor compositor(true, false, 5);
		KWinScreencast capture;
		QSignalSpy created(&capture, &KWinScreencast::streamCreated);
		QVERIFY(capture.start(compositor.connectClient()));
		QTRY_COMPARE(created.size(), 1);
		QVERIFY(capture.startInput(compositor.connectClient()));
		QTRY_VERIFY(capture.keyboardInputAvailable());
		capture.keyEvent(1, XKB_KEY_Shift_L, true);
		capture.keyEvent(1, XKB_KEY_A, true);
		capture.keyEvent(1, XKB_KEY_Shift_L, false);
		capture.keyEvent(1, XKB_KEY_a, false);
		capture.keyEvent(1, XKB_KEY_a, true);
		capture.keyEvent(1, XKB_KEY_a, false);
		QTRY_COMPARE(compositor.keys.size(), 6);
		QCOMPARE(compositor.keys, (QVector<Compositor::Event>{{KEY_LEFTSHIFT, 1}, {KEY_A, 1}, {KEY_A, 0}, {KEY_LEFTSHIFT, 0}, {KEY_A, 1}, {KEY_A, 0}}));
	}
	void legacyInputUsesCapsLockWithoutKeyboardFocus()
	{
		Compositor compositor(true, false, 5);
		compositor.setCapsLock(true);
		KWinScreencast capture;
		QSignalSpy created(&capture, &KWinScreencast::streamCreated);
		QSignalSpy availability(&capture, &KWinScreencast::inputAvailabilityChanged);
		QVERIFY(capture.start(compositor.connectClient()));
		QTRY_COMPARE(created.size(), 1);
		QVERIFY(capture.startInput(compositor.connectClient()));
		QTRY_VERIFY(capture.keyboardInputAvailable());
		capture.keyEvent(1, XKB_KEY_A, true);
		capture.keyEvent(1, XKB_KEY_A, false);
		capture.keyEvent(1, XKB_KEY_a, true);
		capture.keyEvent(1, XKB_KEY_a, false);
		QTRY_COMPARE(compositor.keys.size(), 6);
		QCOMPARE(compositor.keys, (QVector<Compositor::Event>{{KEY_A, 1}, {KEY_A, 0},
			{KEY_LEFTSHIFT, 1}, {KEY_A, 1}, {KEY_A, 0}, {KEY_LEFTSHIFT, 0}}));
		const auto previous = availability.size();
		compositor.setCapsLock(false);
		QTRY_VERIFY(availability.size() > previous);
		compositor.keys.clear();
		capture.keyEvent(1, XKB_KEY_A, true);
		capture.keyEvent(1, XKB_KEY_A, false);
		QTRY_COMPARE(compositor.keys.size(), 4);
		QCOMPARE(compositor.keys, (QVector<Compositor::Event>{{KEY_LEFTSHIFT, 1}, {KEY_A, 1}, {KEY_A, 0}, {KEY_LEFTSHIFT, 0}}));
	}
	void missingLockStatePreservesPointerAndVideo()
	{
		Compositor compositor(true, false, 5, true, false, false);
		KWinScreencast capture;
		QSignalSpy created(&capture, &KWinScreencast::streamCreated);
		QVERIFY(capture.start(compositor.connectClient()));
		QTRY_COMPARE(created.size(), 1);
		QVERIFY(capture.startInput(compositor.connectClient()));
		QTRY_VERIFY(capture.pointerInputAvailable());
		QVERIFY(!capture.keyboardInputAvailable());
		capture.keyEvent(1, XKB_KEY_A, true);
		capture.pointerEvent(1, 0, 0, 0, QSize(1600, 540));
		QTRY_COMPARE(compositor.positions.size(), 1);
		QVERIFY(compositor.keys.isEmpty());
	}
	void newerKWinAcceptsNativeKeysymsWithoutAKeymap()
	{
		Compositor compositor(true, false, 6, false);
		KWinScreencast capture;
		QSignalSpy created(&capture, &KWinScreencast::streamCreated);
		QVERIFY(capture.start(compositor.connectClient()));
		QTRY_COMPARE(created.size(), 1);
		QVERIFY(capture.startInput(compositor.connectClient()));
		QTRY_VERIFY(capture.keyboardInputAvailable());
		capture.keyEvent(1, 0x01000444, true);
		capture.releaseClient(1);
		QTRY_COMPARE(compositor.symbols.size(), 2);
		QCOMPARE(compositor.symbols, (QVector<Compositor::Event>{{0x01000444, 1}, {0x01000444, 0}}));
		QVERIFY(compositor.keys.isEmpty());
		capture.keyEvent(1, XKB_KEY_A, true);
		capture.keyEvent(1, XKB_KEY_a, true); // same held letter, different case
		capture.keyEvent(1, XKB_KEY_a, false);
		QTRY_COMPARE(compositor.symbols.size(), 4);
		QCOMPARE(compositor.symbols.at(2), (Compositor::Event{XKB_KEY_A, 1}));
		QCOMPARE(compositor.symbols.at(3), (Compositor::Event{XKB_KEY_A, 0}));
	}
	void unavailableInputPreservesScreenCapture()
	{
		Compositor compositor;
		KWinScreencast capture;
		QSignalSpy created(&capture, &KWinScreencast::streamCreated);
		QSignalSpy failed(&capture, &KWinScreencast::failed);
		QVERIFY(capture.start(compositor.connectClient()));
		QTRY_COMPARE(created.size(), 1);
		QVERIFY(capture.startInput(compositor.connectClient()));
		QVERIFY(!capture.pointerInputAvailable());
		QVERIFY(!capture.keyboardInputAvailable());
		capture.keyEvent(1, XKB_KEY_a, true);
		capture.pointerEvent(1, 1, 0, 0, QSize(1600, 540));
		capture.releaseClient(1);
		QVERIFY(failed.isEmpty());
	}
	void inputProtocolErrorPreservesCaptureConnection()
	{
		Compositor compositor(true, false, 5, true, true);
		KWinScreencast capture;
		QSignalSpy created(&capture, &KWinScreencast::streamCreated);
		QSignalSpy failed(&capture, &KWinScreencast::failed);
		QSignalSpy inputFailed(&capture, &KWinScreencast::inputFailed);
		QVERIFY(capture.start(compositor.connectClient()));
		QTRY_COMPARE(created.size(), 1);
		QVERIFY(capture.startInput(compositor.connectClient()));
		QTRY_COMPARE(inputFailed.size(), 1);
		QVERIFY(inputFailed.first().first().toString().contains(QStringLiteral("org_kde_kwin_fake_input")));
		QVERIFY(!capture.pointerInputAvailable());
		QVERIFY(!capture.keyboardInputAvailable());
		QVERIFY(failed.isEmpty());
		// The capture connection still receives compositor events after input fails.
		compositor.closeStream();
		QTRY_COMPARE(failed.size(), 1);
	}
	void inputDisconnectPreservesCaptureConnection()
	{
		Compositor compositor;
		KWinScreencast capture;
		QSignalSpy created(&capture, &KWinScreencast::streamCreated);
		QSignalSpy failed(&capture, &KWinScreencast::failed);
		QSignalSpy inputFailed(&capture, &KWinScreencast::inputFailed);
		QVERIFY(capture.start(compositor.connectClient()));
		QTRY_COMPARE(created.size(), 1);
		int sockets[2];
		QVERIFY(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sockets) == 0);
		QVERIFY(capture.startInput(sockets[0]));
		::close(sockets[1]);
		QTRY_COMPARE(inputFailed.size(), 1);
		QVERIFY(failed.isEmpty());
		compositor.closeStream();
		QTRY_COMPARE(failed.size(), 1);
	}
	void missingKeyboardPreservesPointerAndVideo()
	{
		Compositor compositor(true, false, 5, false);
		KWinScreencast capture;
		QSignalSpy created(&capture, &KWinScreencast::streamCreated);
		QSignalSpy failed(&capture, &KWinScreencast::failed);
		QVERIFY(capture.start(compositor.connectClient()));
		QTRY_COMPARE(created.size(), 1);
		QVERIFY(capture.startInput(compositor.connectClient()));
		QTRY_VERIFY(capture.pointerInputAvailable());
		QVERIFY(!capture.keyboardInputAvailable());
		capture.keyEvent(1, XKB_KEY_a, true);
		capture.pointerEvent(1, 0, 0, 0, QSize(1600, 540));
		QTRY_COMPARE(compositor.positions.size(), 1);
		QCOMPARE(compositor.positions.first(), QPointF(-640, 0));
		QVERIFY(compositor.keys.isEmpty());
		QVERIFY(failed.isEmpty());
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
