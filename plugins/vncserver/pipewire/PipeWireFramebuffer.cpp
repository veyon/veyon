// SPDX-FileCopyrightText: 2024-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QMutex>
#include <QScopeGuard>

#include "PipeWireFrame.h"
#include "PipeWireFramebuffer.h"
#include "VeyonCore.h"

#include <cstring>
#include <limits>

extern "C" {
#include <spa/buffer/meta.h>
#include <spa/pod/builder.h>
}

PipeWireFramebuffer::PipeWireFramebuffer(QMutex* screenMutex, QObject* parent) :
	QObject(parent), m_screenMutex(screenMutex)
{
}

PipeWireFramebuffer::~PipeWireFramebuffer()
{
	close();
}

bool PipeWireFramebuffer::open(int fd, quint32 nodeId, rfbScreenInfoPtr screen)
{
	QMutexLocker lock(m_screenMutex);

	m_rfbScreen = screen;
	m_frameSize = {};
	m_acceptedFrameSize = {};
	// a (re)opened stream starts from scratch while rfbScreen may keep its size and stale content
	m_forceFullFrame = true;

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
		nullptr);

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
		PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS);

	const int ret = pw_stream_connect(
		m_stream,
		PW_DIRECTION_INPUT,
		nodeId,
		flags,
		params, 1);

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
	m_loopThread.reset(QThread::create([this]() {
		pw_main_loop_run(m_loop);
	}));
	m_loopThread->start();

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

	if (m_loopThread && m_loopThread->isRunning())
	{
		m_loopThread->wait(3000);
	}
	m_loopThread.reset();

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
											   pw_stream_state oldState,
											   pw_stream_state state,
											   const char* error)
{
	auto* self = static_cast<PipeWireFramebuffer*>(data);
	vDebug() << "PipeWire stream state:" << pw_stream_state_as_string(state);

	if (state == PW_STREAM_STATE_ERROR ||
		state == PW_STREAM_STATE_UNCONNECTED ||
		(state == PW_STREAM_STATE_PAUSED && oldState == PW_STREAM_STATE_STREAMING))
	{
		if (error)
		{
			vCritical() << "PipeWire stream error:" << error;
		}

		if (state == PW_STREAM_STATE_PAUSED)
		{
			// Some compositors pause the stream (rather than erroring out)
			// during output reconfiguration, e.g. switching between internal
			// and external monitors. Reactivating in place is unreliable
			// across compositors, so force a clean teardown/reconnect via
			// the existing recovery path instead of trying to resume.
			vWarning() << "PipeWire stream paused – forcing clean reconnect";
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

	// a rejected format leaves no frame size, so processFrame() drops every buffer of the stream
	self->m_frameSize = {};
	self->m_videoFormat = SPA_VIDEO_FORMAT_UNKNOWN;

	if (PipeWireFrame::isValidSize(videoInfo.size.width, videoInfo.size.height) == false ||
		PipeWireFrame::isSupportedFormat(videoInfo.format) == false)
	{
		self->logRejectedFormat(videoInfo.size.width, videoInfo.size.height, videoInfo.format);
		return;
	}

	const int width = static_cast<int>(videoInfo.size.width);
	const int height = static_cast<int>(videoInfo.size.height);

	vDebug() << "PipeWire stream format:" << width << "x" << height
			 << "format=" << videoInfo.format;

	const QSize frameSize(width, height);
	const auto sizeChanged = self->m_acceptedFrameSize.isValid() && self->m_acceptedFrameSize != frameSize;
	self->m_frameSize = frameSize;
	self->m_acceptedFrameSize = frameSize;
	self->m_videoFormat = videoInfo.format;
	if (sizeChanged)
	{
		Q_EMIT self->frameSizeChanged(frameSize);
	}

	// Accept the negotiated format by calling pw_stream_update_params with buffer
	// params and a request for SPA_META_VideoDamage so the compositor can tell us
	// which regions actually changed on each frame.
	static constexpr int ParamBufSize = 1024;
	uint8_t paramBuffer[ParamBufSize];
	spa_pod_builder builder;
	spa_pod_builder_init(&builder, paramBuffer, sizeof(paramBuffer));

	static_assert(PipeWireFrame::MaximumPixelCount * PipeWireFrame::BytesPerPixel <= std::numeric_limits<int>::max());
	const int rowBytes = width * PipeWireFrame::BytesPerPixel;

	const spa_pod* params[2];
	params[0] = static_cast<const spa_pod*>(spa_pod_builder_add_object(
		&builder,
		SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
		SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(2, 1, 32),
		SPA_PARAM_BUFFERS_blocks, SPA_POD_Int(1),
		SPA_PARAM_BUFFERS_size, SPA_POD_Int(rowBytes * height),
		SPA_PARAM_BUFFERS_stride, SPA_POD_Int(rowBytes),
		SPA_PARAM_BUFFERS_dataType, SPA_POD_CHOICE_FLAGS_Int(1 << SPA_DATA_MemPtr)));
	params[1] = static_cast<const spa_pod*>(spa_pod_builder_add_object(
		&builder,
		SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta,
		SPA_PARAM_META_type, SPA_POD_Id(SPA_META_VideoDamage),
		SPA_PARAM_META_size, SPA_POD_Int(static_cast<int>(sizeof(spa_meta_region)) * PipeWireFrame::MaximumDamageRegions)));

	pw_stream_update_params(self->m_stream, params, 2);
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
	QMutexLocker lock(m_screenMutex);

	if (!m_stream || !m_rfbScreen)
	{
		return;
	}

	pw_buffer* pwBuf = pw_stream_dequeue_buffer(m_stream);
	if (!pwBuf)
	{
		return;
	}

	const auto queueBuffer = qScopeGuard([this, pwBuf]() {
		pw_stream_queue_buffer(m_stream, pwBuf);
	});

	const spa_buffer* buf = pwBuf->buffer;

	PipeWireFrame::BufferData bufferData;
	if (buf && buf->n_datas > 0 && buf->datas)
	{
		const spa_data& data = buf->datas[0];
		bufferData.type = data.type;
		bufferData.data = data.data;
		bufferData.maxSize = data.maxsize;
		if (data.chunk)
		{
			bufferData.chunk = PipeWireFrame::Chunk{data.chunk->offset, data.chunk->size,
													data.chunk->stride, data.chunk->flags};
		}
	}

	const int width = m_frameSize.width();
	const int height = m_frameSize.height();

	const auto source = PipeWireFrame::validate(width, height, m_videoFormat, bufferData);
	if (source.status == PipeWireFrame::Status::NoContent)
	{
		// nothing is lost: producers report the damage with the next frame
		return;
	}
	if (source.status != PipeWireFrame::Status::Valid)
	{
		// its damage is lost, so the next frame has to be copied and sent in full
		m_forceFullFrame = true;
		logDroppedFrame(PipeWireFrame::statusName(source.status));
		return;
	}

	// Resize rfbScreen framebuffer if the stream dimensions differ (bounded by PipeWireFrame::isValidSize())
	const bool sizeChanged = (m_rfbScreen->width != width || m_rfbScreen->height != height);
	if (sizeChanged)
	{
		auto* newBuf = new char[size_t(width) * size_t(height) * PipeWireFrame::BytesPerPixel]{};
		char* oldBuf = m_rfbScreen->frameBuffer;
		rfbNewFramebuffer(m_rfbScreen, newBuf, width, height, 8, 3, 4);
		delete[] oldBuf;
	}

	// Damage metadata drives both which regions we mark modified for VNC
	// clients AND (as an optimization) which regions we actually need to
	// copy - falling back to a full-frame copy when unavailable, when the
	// framebuffer was just resized, or when damage covers most of the frame anyway.
	const auto* damageMeta = spa_buffer_find_meta(buf, SPA_META_VideoDamage);
	const auto damage = damageMeta ? PipeWireFrame::parseDamage(damageMeta->data, damageMeta->size, width, height) :
									 PipeWireFrame::Damage{};

	// the first frame after (re)opening the stream or after a skipped frame must be complete:
	// otherwise areas never reported as damaged keep stale/blank content
	const bool fullFrame = sizeChanged || m_forceFullFrame;

	const PipeWireFrame::Target target{reinterpret_cast<uchar*>(m_rfbScreen->frameBuffer),
									   m_rfbScreen->paddedWidthInBytes,
									   qint64(m_rfbScreen->paddedWidthInBytes) * m_rfbScreen->height};
	if (PipeWireFrame::copy(source, width, height, target, damage, fullFrame) == false)
	{
		m_forceFullFrame = true;
		logDroppedFrame("framebuffer does not match the stream");
		return;
	}

	m_forceFullFrame = false;

	if (fullFrame || damage.isFullFrame)
	{
		rfbMarkRectAsModified(m_rfbScreen, 0, 0, width, height);
	}
	else
	{
		for (int i = 0; i < damage.count; ++i)
		{
			const auto& rect = damage.rects[size_t(i)];
			rfbMarkRectAsModified(m_rfbScreen, rect.x1, rect.y1, rect.x2, rect.y2);
		}
	}
}



// Frames are untrusted input, a producer may send invalid ones at frame rate: log the first one
// immediately, a change of the reason after a short interval (bounded even if the producer alternates
// between reasons) and repeats at most once per interval with the number of frames dropped meanwhile
void PipeWireFramebuffer::logDroppedFrame(const char* reason)
{
	++m_droppedFrameCount;

	if (m_droppedFrameLogTimer.isValid())
	{
		const auto reasonChanged = std::strcmp(m_lastDroppedFrameReason, reason) != 0;
		const auto interval = reasonChanged ? DroppedFrameReasonChangeLogInterval : DroppedFrameLogInterval;
		if (m_droppedFrameLogTimer.hasExpired(interval) == false)
		{
			return;
		}
	}

	vWarning() << "dropping PipeWire frame:" << reason << "-" << m_droppedFrameCount << "dropped since last report";

	m_lastDroppedFrameReason = reason;
	m_droppedFrameCount = 0;
	m_droppedFrameLogTimer.start();
}



// the producer decides how often param_changed fires, so rejections are rate-limited like frames
void PipeWireFramebuffer::logRejectedFormat(uint32_t width, uint32_t height, uint32_t videoFormat)
{
	++m_rejectedFormatCount;

	if (m_rejectedFormatLogTimer.isValid() &&
		m_rejectedFormatLogTimer.hasExpired(DroppedFrameLogInterval) == false)
	{
		return;
	}

	vWarning() << "rejecting PipeWire stream format" << width << "x" << height << "format=" << videoFormat
			   << "-" << m_rejectedFormatCount << "rejected since last report";

	m_rejectedFormatCount = 0;
	m_rejectedFormatLogTimer.start();
}
