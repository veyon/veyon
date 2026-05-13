/*
 * PipeWireFramebuffer.h - PipeWire stream consumer that fills an rfbScreen framebuffer
 *
 * Copyright (c) 2024-2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include <QObject>
#include <QSize>
#include <QThread>

extern "C" {
#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>
#include <rfb/rfb.h>
}

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
	explicit PipeWireFramebuffer(QObject* parent = nullptr);
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

	pw_main_loop*    m_loop{nullptr};
	pw_context*      m_context{nullptr};
	pw_core*         m_core{nullptr};
	pw_stream*       m_stream{nullptr};

	spa_hook m_coreListener{};
	spa_hook m_streamListener{};

	rfbScreenInfoPtr m_rfbScreen{nullptr};
	QSize            m_frameSize;
	uint32_t         m_videoFormat{0};
	bool             m_running{false};

	QThread m_loopThread;
};
