/*
 * PipeWireFramebuffer.cpp - PipeWire stream consumer that fills an rfbScreen framebuffer
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

#include "PipeWireFramebuffer.h"

#include <VeyonCore.h>

#include <cstring>

extern "C" {
#include <spa/pod/builder.h>
}

PipeWireFramebuffer::PipeWireFramebuffer(QObject* parent)
	: QObject(parent)
{
}

PipeWireFramebuffer::~PipeWireFramebuffer()
{
	close();
}

bool PipeWireFramebuffer::open(int fd, quint32 nodeId, rfbScreenInfoPtr screen)
{
	m_rfbScreen = screen;
	m_frameSize = {};

	pw_init(nullptr, nullptr);

	m_loop = pw_main_loop_new(nullptr);
	if (!m_loop)
	{
		vCritical() << "Failed to create PipeWire main loop";
		return false;
	}

	m_context = pw_context_new(pw_main_loop_get_loop(m_loop), nullptr, 0);
	if (!m_context)
	{
		vCritical() << "Failed to create PipeWire context";
		pw_main_loop_destroy(m_loop);
		m_loop = nullptr;
		return false;
	}

	// pw_context_connect_fd takes ownership of the fd
	m_core = pw_context_connect_fd(m_context, fd, nullptr, 0);
	if (!m_core)
	{
		vCritical() << "Failed to connect PipeWire context via FD";
		pw_context_destroy(m_context);
		m_context = nullptr;
		pw_main_loop_destroy(m_loop);
		m_loop = nullptr;
		return false;
	}

	static const pw_core_events coreEvents = {
		.version = PW_VERSION_CORE_EVENTS,
		.error = &PipeWireFramebuffer::onCoreError,
	};
	pw_core_add_listener(m_core, &m_coreListener, &coreEvents, this);

	pw_properties* props = pw_properties_new(
		PW_KEY_MEDIA_TYPE, "Video",
		PW_KEY_MEDIA_CATEGORY, "Capture",
		PW_KEY_MEDIA_ROLE, "Screen",
		nullptr
	);

	m_stream = pw_stream_new(m_core, "veyon-screen-capture", props);
	if (!m_stream)
	{
		vCritical() << "Failed to create PipeWire stream";
		spa_hook_remove(&m_coreListener);
		pw_core_disconnect(m_core);
		m_core = nullptr;
		pw_context_destroy(m_context);
		m_context = nullptr;
		pw_main_loop_destroy(m_loop);
		m_loop = nullptr;
		return false;
	}

	static const pw_stream_events streamEvents = {
		.version = PW_VERSION_STREAM_EVENTS,
		.state_changed = &PipeWireFramebuffer::onStreamStateChanged,
		.param_changed = &PipeWireFramebuffer::onStreamParamChanged,
		.process = &PipeWireFramebuffer::onStreamProcess,
	};
	pw_stream_add_listener(m_stream, &m_streamListener, &streamEvents, this);

	// Negotiate BGRx format (4 bytes/pixel, no alpha, matches rfbScreen 32-bit layout)
	static constexpr int ParamBufSize = 1024;
	uint8_t paramBuffer[ParamBufSize];
	spa_pod_builder builder;
	spa_pod_builder_init(&builder, paramBuffer, sizeof(paramBuffer));

	spa_video_info_raw videoInfo{};
	videoInfo.format = SPA_VIDEO_FORMAT_BGRx;

	const spa_pod* params[1];
	params[0] = spa_format_video_raw_build(&builder, SPA_PARAM_EnumFormat, &videoInfo);

	const auto flags = static_cast<pw_stream_flags>(
		PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS
	);

	const int ret = pw_stream_connect(
		m_stream,
		PW_DIRECTION_INPUT,
		nodeId,
		flags,
		params, 1
	);

	if (ret < 0)
	{
		vCritical() << "pw_stream_connect failed:" << ret;
		spa_hook_remove(&m_streamListener);
		pw_stream_destroy(m_stream);
		m_stream = nullptr;
		spa_hook_remove(&m_coreListener);
		pw_core_disconnect(m_core);
		m_core = nullptr;
		pw_context_destroy(m_context);
		m_context = nullptr;
		pw_main_loop_destroy(m_loop);
		m_loop = nullptr;
		return false;
	}

	m_running = true;

	// Run the PipeWire main loop in a dedicated thread so it doesn't block Qt
	connect(&m_loopThread, &QThread::started, this, [this]() {
		pw_main_loop_run(m_loop);
	}, Qt::DirectConnection);
	m_loopThread.start();

	vDebug() << "PipeWire stream opened for node" << nodeId;
	return true;
}

void PipeWireFramebuffer::close()
{
	m_running = false;

	if (m_loop)
	{
		pw_main_loop_quit(m_loop);
	}

	if (m_loopThread.isRunning())
	{
		m_loopThread.quit();
		m_loopThread.wait(3000);
	}

	if (m_stream)
	{
		spa_hook_remove(&m_streamListener);
		pw_stream_disconnect(m_stream);
		pw_stream_destroy(m_stream);
		m_stream = nullptr;
	}

	if (m_core)
	{
		spa_hook_remove(&m_coreListener);
		pw_core_disconnect(m_core);
		m_core = nullptr;
	}

	if (m_context)
	{
		pw_context_destroy(m_context);
		m_context = nullptr;
	}

	if (m_loop)
	{
		pw_main_loop_destroy(m_loop);
		m_loop = nullptr;
	}
}

// ---------------------------------------------------------------------------
// PipeWire C callbacks (called from PipeWire loop thread)
// ---------------------------------------------------------------------------

void PipeWireFramebuffer::onCoreError(void* data, uint32_t /*id*/, int /*seq*/,
                                       int res, const char* message)
{
	auto* self = static_cast<PipeWireFramebuffer*>(data);
	vCritical() << "PipeWire core error" << res << (message ? message : "");
	self->m_running = false;
	if (self->m_loop)
	{
		pw_main_loop_quit(self->m_loop);
	}
	Q_EMIT self->streamEnded();
}

void PipeWireFramebuffer::onStreamStateChanged(void* data,
                                                pw_stream_state /*old*/,
                                                pw_stream_state state,
                                                const char* error)
{
	auto* self = static_cast<PipeWireFramebuffer*>(data);
	vDebug() << "PipeWire stream state:" << pw_stream_state_as_string(state);

	if (state == PW_STREAM_STATE_ERROR || state == PW_STREAM_STATE_UNCONNECTED)
	{
		if (error)
		{
			vCritical() << "PipeWire stream error:" << error;
		}
		self->m_running = false;
		if (self->m_loop)
		{
			pw_main_loop_quit(self->m_loop);
		}
		Q_EMIT self->streamEnded();
	}
}

void PipeWireFramebuffer::onStreamParamChanged(void* data, uint32_t id,
                                                const spa_pod* param)
{
	auto* self = static_cast<PipeWireFramebuffer*>(data);

	if (id != SPA_PARAM_Format || param == nullptr)
	{
		return;
	}

	spa_video_info_raw videoInfo{};
	if (spa_format_video_raw_parse(param, &videoInfo) < 0)
	{
		return;
	}

	const int width  = static_cast<int>(videoInfo.size.width);
	const int height = static_cast<int>(videoInfo.size.height);

	if (width <= 0 || height <= 0)
	{
		return;
	}

	vDebug() << "PipeWire stream format:" << width << "x" << height
	         << "format=" << videoInfo.format;

	self->m_frameSize = QSize(width, height);

	// Accept the negotiated format by calling pw_stream_update_params with buffer params
	static constexpr int ParamBufSize = 1024;
	uint8_t paramBuffer[ParamBufSize];
	spa_pod_builder builder;
	spa_pod_builder_init(&builder, paramBuffer, sizeof(paramBuffer));

	const spa_pod* bufferParams[1];
	bufferParams[0] = static_cast<const spa_pod*>(spa_pod_builder_add_object(
		&builder,
		SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
		SPA_PARAM_BUFFERS_buffers,  SPA_POD_CHOICE_RANGE_Int(2, 1, 32),
		SPA_PARAM_BUFFERS_blocks,   SPA_POD_Int(1),
		SPA_PARAM_BUFFERS_size,     SPA_POD_Int(width * height * 4),
		SPA_PARAM_BUFFERS_stride,   SPA_POD_Int(width * 4),
		SPA_PARAM_BUFFERS_dataType, SPA_POD_CHOICE_FLAGS_Int(1 << SPA_DATA_MemPtr)
	));

	pw_stream_update_params(self->m_stream, bufferParams, 1);
}

void PipeWireFramebuffer::onStreamProcess(void* data)
{
	static_cast<PipeWireFramebuffer*>(data)->processFrame();
}

// ---------------------------------------------------------------------------
// Frame processing (runs in PipeWire loop thread)
// ---------------------------------------------------------------------------

void PipeWireFramebuffer::processFrame()
{
	if (!m_stream || !m_rfbScreen)
	{
		return;
	}

	pw_buffer* pwBuf = pw_stream_dequeue_buffer(m_stream);
	if (!pwBuf)
	{
		return;
	}

	spa_buffer* buf = pwBuf->buffer;
	if (!buf || buf->n_datas == 0 || !buf->datas[0].data)
	{
		pw_stream_queue_buffer(m_stream, pwBuf);
		return;
	}

	const int width  = m_frameSize.width();
	const int height = m_frameSize.height();

	if (width <= 0 || height <= 0)
	{
		pw_stream_queue_buffer(m_stream, pwBuf);
		return;
	}

	// Resize rfbScreen framebuffer if the stream dimensions differ
	if (m_rfbScreen->width != width || m_rfbScreen->height != height)
	{
		const int newSize = width * height * 4;
		char* newBuf = new char[newSize];
		std::memset(newBuf, 0, static_cast<size_t>(newSize));
		char* oldBuf = m_rfbScreen->frameBuffer;
		rfbNewFramebuffer(m_rfbScreen, newBuf, width, height, 8, 3, 4);
		delete[] oldBuf;
	}

	spa_data& d = buf->datas[0];
	const int srcStride = d.chunk->stride > 0 ? d.chunk->stride : width * 4;
	const int dstStride = width * 4;
	const char* src = static_cast<const char*>(d.data) + d.chunk->offset;
	char* dst = m_rfbScreen->frameBuffer;

	for (int y = 0; y < height; ++y)
	{
		std::memcpy(dst + y * dstStride, src + y * srcStride,
		            static_cast<size_t>(dstStride));
	}

	rfbMarkRectAsModified(m_rfbScreen, 0, 0, width, height);

	pw_stream_queue_buffer(m_stream, pwBuf);
}
