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

#include <QImage>
#include <QMutex>
#include <QScopeGuard>
#include <unistd.h>

#include "PipeWireFramebuffer.h"
#include "PipeWireFrame.h"

#include "VeyonCore.h"

#include <cstring>

extern "C" {
#include <spa/buffer/meta.h>
#include <spa/pod/builder.h>
#include <spa/utils/string.h>
}

PipeWireFramebuffer::PipeWireFramebuffer(QMutex* screenMutex, QObject* parent)
	: QObject(parent)
	, m_screenMutex(screenMutex)
{
}

PipeWireFramebuffer::~PipeWireFramebuffer()
{
	close();
}

bool PipeWireFramebuffer::open(int fd, quint32 nodeId, rfbScreenInfoPtr screen, bool linkDirectly)
{
	QMutexLocker lock(m_screenMutex);
	const auto closeFd = qScopeGuard([&]() { if (fd >= 0) { ::close(fd); } });

	m_rfbScreen = screen;
	m_sourceNode = nodeId;
	m_linkDirectly = linkDirectly;
	m_firstFrame = true;
	m_frameSize = {};
	// a (re)opened stream starts from scratch while rfbScreen may keep its size and stale content
	m_forceFullFrame = true;

	pw_init(nullptr, nullptr);

	m_loop = pw_thread_loop_new("veyon-screen-capture", nullptr);
	if (!m_loop)
	{
		vCritical() << "Failed to create PipeWire main loop";
		return false;
	}

	m_context = pw_context_new(pw_thread_loop_get_loop(m_loop), nullptr, 0);
	if (!m_context)
	{
		vCritical() << "Failed to create PipeWire context";
		pw_thread_loop_destroy(m_loop);
		m_loop = nullptr;
		return false;
	}

	// pw_context_connect_fd takes ownership of the fd
	m_core = pw_context_connect_fd(m_context, fd, nullptr, 0);
	fd = -1; // ownership transferred to PipeWire, including on failure
	if (!m_core)
	{
		vCritical() << "Failed to connect PipeWire context via FD";
		pw_context_destroy(m_context);
		m_context = nullptr;
		pw_thread_loop_destroy(m_loop);
		m_loop = nullptr;
		return false;
	}

	static const pw_core_events coreEvents = {
		.version = PW_VERSION_CORE_EVENTS,
		.error = &PipeWireFramebuffer::onCoreError,
	};
	pw_core_add_listener(m_core, &m_coreListener, &coreEvents, this);
	if (m_linkDirectly)
	{
		m_registry = pw_core_get_registry(m_core, PW_VERSION_REGISTRY, 0);
		if (!m_registry)
		{
			vCritical() << "Could not create the PipeWire registry proxy";
			close();
			return false;
		}
		static const pw_registry_events registryEvents = {
			.version = PW_VERSION_REGISTRY_EVENTS,
			.global = &PipeWireFramebuffer::onRegistryGlobal,
			.global_remove = &PipeWireFramebuffer::onRegistryGlobalRemove,
		};
		pw_registry_add_listener(m_registry, &m_registryListener, &registryEvents, this);
	}

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
		if (m_registry) { spa_hook_remove(&m_registryListener); pw_proxy_destroy(reinterpret_cast<pw_proxy*>(m_registry)); m_registry = nullptr; }
		spa_hook_remove(&m_coreListener);
		pw_core_disconnect(m_core);
		m_core = nullptr;
		pw_context_destroy(m_context);
		m_context = nullptr;
		pw_thread_loop_destroy(m_loop);
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
		(m_linkDirectly ? 0 : PW_STREAM_FLAG_AUTOCONNECT) | PW_STREAM_FLAG_MAP_BUFFERS
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
		if (m_registry) { spa_hook_remove(&m_registryListener); pw_proxy_destroy(reinterpret_cast<pw_proxy*>(m_registry)); m_registry = nullptr; }
		spa_hook_remove(&m_coreListener);
		pw_core_disconnect(m_core);
		m_core = nullptr;
		pw_context_destroy(m_context);
		m_context = nullptr;
		pw_thread_loop_destroy(m_loop);
		m_loop = nullptr;
		return false;
	}

	m_running = true;

	// PipeWire owns and synchronizes its worker thread, including startup/stop.
	if (pw_thread_loop_start(m_loop) < 0)
	{
		vCritical() << "Could not start the PipeWire capture thread";
		close();
		return false;
	}

	vDebug() << "PipeWire stream opened for node" << nodeId;
	return true;
}

void PipeWireFramebuffer::close()
{
	m_running = false;

	if (m_loop) { pw_thread_loop_stop(m_loop); }

	if (m_link) { pw_proxy_destroy(m_link); m_link = nullptr; }
	if (m_registry)
	{
		spa_hook_remove(&m_registryListener);
		pw_proxy_destroy(reinterpret_cast<pw_proxy*>(m_registry));
		m_registry = nullptr;
	}
	m_ports.clear();
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
		pw_thread_loop_destroy(m_loop);
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
	Q_EMIT self->streamEnded();
}

void PipeWireFramebuffer::onStreamStateChanged(void* data,
												pw_stream_state oldState,
												pw_stream_state state,
												const char* error)
{
	auto* self = static_cast<PipeWireFramebuffer*>(data);
	vDebug() << "PipeWire stream state:" << pw_stream_state_as_string(state);
	if (state == PW_STREAM_STATE_PAUSED && self->m_linkDirectly) { self->linkPorts(); }

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

	if (!PipeWireFrame::validSize(width, height) ||
		videoInfo.format != SPA_VIDEO_FORMAT_BGRx)
	{
		self->m_frameSize = {};
		vCritical() << "Invalid PipeWire video format or frame size";
		Q_EMIT self->streamEnded();
		return;
	}

	vDebug() << "PipeWire stream format:" << width << "x" << height
			 << "format=" << videoInfo.format;

	self->m_frameSize = QSize(width, height);
	self->m_videoFormat = videoInfo.format;

	// Accept the negotiated format by calling pw_stream_update_params with buffer
	// params and a request for SPA_META_VideoDamage so the compositor can tell us
	// which regions actually changed on each frame.
	static constexpr int ParamBufSize = 1024;
	uint8_t paramBuffer[ParamBufSize];
	spa_pod_builder builder;
	spa_pod_builder_init(&builder, paramBuffer, sizeof(paramBuffer));

	// Allow up to MaxDamageRects damage rectangles per frame.
	static constexpr int MaxDamageRects = 16;

	const spa_pod* params[2];
	params[0] = static_cast<const spa_pod*>(spa_pod_builder_add_object(
		&builder,
		SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
		SPA_PARAM_BUFFERS_buffers,  SPA_POD_CHOICE_RANGE_Int(2, 1, 32),
		SPA_PARAM_BUFFERS_blocks,   SPA_POD_Int(1),
		SPA_PARAM_BUFFERS_size,     SPA_POD_Int(width * height * 4),
		SPA_PARAM_BUFFERS_stride,   SPA_POD_Int(width * 4),
		SPA_PARAM_BUFFERS_dataType, SPA_POD_CHOICE_FLAGS_Int((1 << SPA_DATA_MemPtr) | (1 << SPA_DATA_MemFd))
	));
	params[1] = static_cast<const spa_pod*>(spa_pod_builder_add_object(
		&builder,
		SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta,
		SPA_PARAM_META_type, SPA_POD_Id(SPA_META_VideoDamage),
		SPA_PARAM_META_size, SPA_POD_Int(static_cast<int>(sizeof(spa_meta_region)) * MaxDamageRects)
	));

	pw_stream_update_params(self->m_stream, params, 2);
}

void PipeWireFramebuffer::onStreamProcess(void* data)
{
	static_cast<PipeWireFramebuffer*>(data)->processFrame();
}

void PipeWireFramebuffer::onRegistryGlobal(void* data, uint32_t id, uint32_t,
	const char* type, uint32_t, const spa_dict* properties)
{
	auto* self = static_cast<PipeWireFramebuffer*>(data);
	if (std::strcmp(type, PW_TYPE_INTERFACE_Port) != 0 || !properties) { return; }
	uint32_t node = SPA_ID_INVALID;
	const auto* nodeString = spa_dict_lookup(properties, PW_KEY_NODE_ID);
	const auto* direction = spa_dict_lookup(properties, PW_KEY_PORT_DIRECTION);
	if (!nodeString || !direction || !spa_atou32(nodeString, &node, 10)) { return; }
	if (std::strcmp(direction, "in") != 0 && std::strcmp(direction, "out") != 0) { return; }
	if (self->m_ports.size() >= 8192) { return; }
	self->m_ports[id] = {node, std::strcmp(direction, "in") == 0};
	self->linkPorts();
}

void PipeWireFramebuffer::onRegistryGlobalRemove(void* data, uint32_t id)
{
	auto* self = static_cast<PipeWireFramebuffer*>(data);
	self->m_ports.erase(id);
	if (id == self->m_sourceNode) { Q_EMIT self->streamEnded(); }
}

void PipeWireFramebuffer::linkPorts()
{
	if (m_link || !m_stream) { return; }
	const auto inputNode = pw_stream_get_node_id(m_stream);
	if (inputNode == SPA_ID_INVALID) { return; }
	uint32_t inputPort = SPA_ID_INVALID;
	uint32_t outputPort = SPA_ID_INVALID;
	for (const auto& [id, port] : m_ports)
	{
		if (port.input && port.node == inputNode) { inputPort = id; }
		if (!port.input && port.node == m_sourceNode) { outputPort = id; }
	}
	if (inputPort == SPA_ID_INVALID || outputPort == SPA_ID_INVALID) { return; }
	// Explicit linking works without WirePlumber or a portal in the greeter.
	auto* properties = pw_properties_new(nullptr, nullptr);
	pw_properties_setf(properties, PW_KEY_LINK_OUTPUT_NODE, "%u", m_sourceNode);
	pw_properties_setf(properties, PW_KEY_LINK_OUTPUT_PORT, "%u", outputPort);
	pw_properties_setf(properties, PW_KEY_LINK_INPUT_NODE, "%u", inputNode);
	pw_properties_setf(properties, PW_KEY_LINK_INPUT_PORT, "%u", inputPort);
	m_link = static_cast<pw_proxy*>(pw_core_create_object(m_core, "link-factory",
		PW_TYPE_INTERFACE_Link, PW_VERSION_LINK, &properties->dict, 0));
	pw_properties_free(properties);
	if (!m_link)
	{
		vCritical() << "Could not link the KWin producer to the greeter capture stream";
		Q_EMIT streamEnded();
	}
}


void PipeWireFramebuffer::convertFrame(char* dst, const char* src, int srcStride, int dstStride,
										int width, int height, const spa_meta_region* damage, size_t damageCount, bool forceFullFrame)
{
	const auto convertRect = [](char* dst, int dstStride,
							 const char* src, int srcStride,
							 int w, int h, uint32_t videoFormat)
	{
		const size_t rowBytes = static_cast<size_t>(w) * 4;

		if (videoFormat == SPA_VIDEO_FORMAT_BGRx)
		{
			const QImage srcImage(reinterpret_cast<const uchar*>(src), w, h,
								  srcStride, QImage::Format_RGB32); // ARGB32 layout matches BGRx-in-memory as ARGB32 on little-endian
			const auto swapped = srcImage.rgbSwapped(); // allocates its own buffer, SIMD-accelerated internally

			for (int y = 0; y < h; ++y)
			{
				std::memcpy(dst + y * dstStride, swapped.constScanLine(y), rowBytes);
			}
		}
		else
		{
			for (int y = 0; y < h; ++y)
			{
				std::memcpy(dst + y * dstStride, src + y * srcStride, rowBytes);
			}
		}
	};

	// Fall back to a single full-frame conversion when there's no usable damage
	// info, or when the caller forces it (e.g. framebuffer was just resized).
	if (forceFullFrame || !damage || damageCount == 0 || !spa_meta_region_is_valid(damage))
	{
		convertRect(dst, dstStride, src, srcStride, width, height, m_videoFormat);
		return;
	}

	// Sum up the damaged area first so we can bail out to a full-frame
	// conversion if damage covers most of the screen anyway - iterating many
	// rects individually only pays off when a meaningful portion of the frame
	// is actually unchanged.
	static constexpr double FullFrameFallbackThreshold = 0.6;

	int64_t damagedArea = 0;
	int rectCount = 0;
	for (size_t i = 0; i < damageCount && spa_meta_region_is_valid(&damage[i]); ++i)
	{
		const auto* r = &damage[i];
		damagedArea += int64_t(qMin<uint32_t>(r->region.size.width, uint32_t(width))) *
			qMin<uint32_t>(r->region.size.height, uint32_t(height));
		++rectCount;
	}

	if (rectCount == 0 ||
		damagedArea > static_cast<int64_t>(FullFrameFallbackThreshold * width * height))
	{
		convertRect(dst, dstStride, src, srcStride, width, height, m_videoFormat);
		return;
	}

	for (size_t i = 0; i < damageCount && spa_meta_region_is_valid(&damage[i]); ++i)
	{
		const auto* r = &damage[i];
		// Clamp to framebuffer bounds
		const int x1 = qBound(0, static_cast<int>(r->region.position.x), width);
		const int y1 = qBound(0, static_cast<int>(r->region.position.y), height);
		const int x2 = int(qBound(qint64(0), qint64(r->region.position.x) + qint64(r->region.size.width), qint64(width)));
		const int y2 = int(qBound(qint64(0), qint64(r->region.position.y) + qint64(r->region.size.height), qint64(height)));

		if (x2 > x1 && y2 > y1)
		{
			convertRect(dst + y1 * dstStride + x1 * 4, dstStride,
						src + y1 * srcStride + x1 * 4, srcStride,
						x2 - x1, y2 - y1, m_videoFormat);
		}
	}
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

	spa_data& d = buf->datas[0];
	const auto* chunk = d.chunk;
	const int srcStride = chunk && chunk->stride > 0 ? chunk->stride : width * 4;
	if (!chunk || (chunk->flags & SPA_CHUNK_FLAG_CORRUPTED) ||
		!PipeWireFrame::fits(width, height, chunk->stride, chunk->offset, chunk->size, d.maxsize))
	{
		pw_stream_queue_buffer(m_stream, pwBuf);
		m_forceFullFrame = true;
		return;
	}
	const bool sizeChanged = (m_rfbScreen->width != width || m_rfbScreen->height != height);
	if (sizeChanged)
	{
		const int newSize = width * height * 4;
		auto* newBuf = new char[newSize]{};
		auto* oldBuf = m_rfbScreen->frameBuffer;
		rfbNewFramebuffer(m_rfbScreen, newBuf, width, height, 8, 3, 4);
		delete[] oldBuf;
	}
	const int dstStride = m_rfbScreen->paddedWidthInBytes;
	const char* src = static_cast<const char*>(d.data) + chunk->offset;
	char* dst = m_rfbScreen->frameBuffer;

	// Damage metadata drives both which regions we mark modified for VNC
	// clients AND (as an optimization) which regions we actually need to
	// convert/copy - falling back to a full-frame conversion when unavailable,
	// when the framebuffer was just resized, or when damage covers most of
	// the frame anyway.
	const auto* metadata = spa_buffer_find_meta(buf, SPA_META_VideoDamage);
	const auto* damage = metadata ? static_cast<const spa_meta_region*>(metadata->data) : nullptr;
	const size_t damageCount = metadata && metadata->data && metadata->size % sizeof(spa_meta_region) == 0 &&
		metadata->size / sizeof(spa_meta_region) <= 16 ? metadata->size / sizeof(spa_meta_region) : 0;

	// the first frame after (re)opening the stream or after a skipped frame must be complete:
	// otherwise areas never reported as damaged keep stale/blank content
	const bool fullFrame = sizeChanged || m_forceFullFrame;
	m_forceFullFrame = false;

	convertFrame(dst, src, srcStride, dstStride, width, height, damage, damageCount, fullFrame);

	if (damageCount > 0 && damage && spa_meta_region_is_valid(damage) && !fullFrame)
	{
		for (size_t i = 0; i < damageCount && spa_meta_region_is_valid(&damage[i]); ++i)
		{
			const auto* r = &damage[i];
			// Clamp to framebuffer bounds
			const int x1 = qBound(0, static_cast<int>(r->region.position.x), width);
			const int y1 = qBound(0, static_cast<int>(r->region.position.y), height);
			const int x2 = int(qBound(qint64(0), qint64(r->region.position.x) + qint64(r->region.size.width), qint64(width)));
			const int y2 = int(qBound(qint64(0), qint64(r->region.position.y) + qint64(r->region.size.height), qint64(height)));
			if (x2 > x1 && y2 > y1)
			{
				rfbMarkRectAsModified(m_rfbScreen, x1, y1, x2, y2);
			}
		}
	}
	else
	{
		rfbMarkRectAsModified(m_rfbScreen, 0, 0, width, height);
	}

	pw_stream_queue_buffer(m_stream, pwBuf);
	if (m_firstFrame)
	{
		m_firstFrame = false;
		Q_EMIT firstFrameReady();
	}
}
