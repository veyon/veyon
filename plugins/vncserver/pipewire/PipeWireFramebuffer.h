// SPDX-FileCopyrightText: 2024-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QSize>
#include <QThread>

#include <memory>

extern "C" {
#include <pipewire/pipewire.h>
#include <rfb/rfb.h>
#include <spa/param/video/format-utils.h>
}

class QMutex;

/**
 * @brief Connects to a PipeWire node and copies frames into an rfbScreen framebuffer.
 *
 * The PipeWire main loop runs in a dedicated QThread so it does not block Qt's
 * event loop.  Format parameters are captured via the param_changed callback
 * before frame processing begins.
 *
 * Typical use:
 *   1. Call open() with a valid PipeWire remote FD and node ID.
 *   2. Connect streamEnded() to handle reconnection.
 *   3. Call close() to stop and release all resources.
 */
class PipeWireFramebuffer : public QObject
{
	Q_OBJECT
public:
	explicit PipeWireFramebuffer(QMutex* screenMutex, QObject* parent = nullptr);
	~PipeWireFramebuffer() override;

	/**
	 * @brief Open a PipeWire connection and start streaming into the rfbScreen.
	 * @param fd      PipeWire remote FD (takes ownership)
	 * @param nodeId  PipeWire node ID from portal Start response
	 * @param screen  rfbScreen to write frames into (must outlive close())
	 * @return true on success
	 */
	bool open(int fd, quint32 nodeId, rfbScreenInfoPtr screen);

	/** @brief Stop the PipeWire loop and free all resources. */
	void close();

	bool isRunning() const { return m_running; }

Q_SIGNALS:
	void streamEnded();

private:
	// PipeWire C callbacks (called from PipeWire loop thread)
	static void onCoreError(void* data, uint32_t id, int seq, int res, const char* message);
	static void onStreamStateChanged(void* data, pw_stream_state old,
									 pw_stream_state state, const char* error);
	static void onStreamParamChanged(void* data, uint32_t id, const spa_pod* param);
	static void onStreamProcess(void* data);

	void processFrame();
	void logDroppedFrame(const char* reason);
	void logRejectedFormat(uint32_t width, uint32_t height, uint32_t videoFormat);

	static constexpr qint64 DroppedFrameLogInterval = 10000;
	static constexpr qint64 DroppedFrameReasonChangeLogInterval = 1000;

	pw_main_loop* m_loop{nullptr};
	pw_context* m_context{nullptr};
	pw_core* m_core{nullptr};
	pw_stream* m_stream{nullptr};

	spa_hook m_coreListener{};
	spa_hook m_streamListener{};

	QMutex* m_screenMutex;
	rfbScreenInfoPtr m_rfbScreen{nullptr};
	QSize m_frameSize;
	uint32_t m_videoFormat{SPA_VIDEO_FORMAT_UNKNOWN};
	bool m_running{false};
	bool m_forceFullFrame{true};
	qint64 m_droppedFrameCount{0};
	const char* m_lastDroppedFrameReason{nullptr};
	QElapsedTimer m_droppedFrameLogTimer;
	qint64 m_rejectedFormatCount{0};
	QElapsedTimer m_rejectedFormatLogTimer;

	std::unique_ptr<QThread> m_loopThread;
};
