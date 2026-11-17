// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QtGlobal>

#include <array>
#include <cstdint>
#include <optional>

// Validation and copy of PipeWire video frames in veyon-capture-helper (02-ipc.md §1a.5). The PipeWire
// peer is the compositor behind the portal, but the frames it shares are untrusted input: nothing in a
// buffer, its chunk or its metadata may make the helper read or write out of bounds. Only plain values
// of spa_data, spa_chunk and the SPA_META_VideoDamage metadata are passed in, so this class needs the
// SPA headers but no PipeWire runtime and can be tested and fuzzed in isolation.
//
// Residual risk (R-19): this rules out out-of-bounds accesses, not every crash. For MemFd buffers
// libpipewire maps maxSize bytes as claimed by the producer without comparing it with the size of
// the memfd, so a producer whose memfd is shorter (or shrinks later) makes the copy fault with
// SIGBUS. That crashes only the dumpable, secret-free helper, which the server restarts with
// backoff; it predates this class. The chunk and metadata pointers are bounded by what
// libpipewire's client-node code set up.
class PipeWireFrame
{
public:
	// the only negotiated format is BGRx (PipeWireFramebuffer::open()), the rfbScreen uses 32 bits too
	static constexpr int BytesPerPixel = 4;

	// 16384 is the usual maximum texture/render target size of current GPUs and therefore bounds what
	// a compositor renders into one screencast stream; RFB itself would allow 65535. The pixel limit
	// (256 MiB per frame) still covers two 8K outputs side by side (15360 x 4320), bounds the
	// framebuffer the helper allocates for a stream, and keeps every byte count of a frame within
	// int32 (SPA_PARAM_BUFFERS_size).
	static constexpr int MaximumDimension = 16384;
	static constexpr qint64 MaximumPixelCount = qint64(1) << 26;

	// damage regions requested per frame (SPA_PARAM_META_size of SPA_META_VideoDamage)
	static constexpr int MaximumDamageRegions = 16;

	enum class Status {
		Valid,
		NoContent,
		InvalidSize,
		UnsupportedFormat,
		UnsupportedDataType,
		NotMapped,
		MissingChunk,
		InvalidStride,
		ChunkOutsideData,
		Truncated,
	};

	// values of spa_chunk
	struct Chunk
	{
		uint32_t offset = 0;
		uint32_t size = 0;
		int32_t stride = 0;
		int32_t flags = 0;
	};

	// values of spa_data (datas[0], mapped by PW_STREAM_FLAG_MAP_BUFFERS) and its chunk, if any
	struct BufferData
	{
		uint32_t type = 0;
		const void* data = nullptr;
		uint32_t maxSize = 0;
		std::optional<Chunk> chunk;
	};

	// pixels and stride are only set for Status::Valid
	struct Source
	{
		Status status = Status::InvalidSize;
		const uchar* pixels = nullptr;
		int stride = 0;
	};

	// half-open rectangle [x1, x2) x [y1, y2) within the frame
	struct Rect
	{
		int x1 = 0;
		int y1 = 0;
		int x2 = 0;
		int y2 = 0;
	};

	// isFullFrame: no usable damage information, everything has to be treated as changed
	struct Damage
	{
		bool isFullFrame = true;
		int count = 0;
		std::array<Rect, MaximumDamageRegions> rects{};
	};

	// pixels with size bytes, stride is rfbScreen::paddedWidthInBytes
	struct Target
	{
		uchar* pixels = nullptr;
		int stride = 0;
		qint64 size = 0;
	};

	[[nodiscard]] static bool isValidSize(qint64 width, qint64 height);
	[[nodiscard]] static bool isSupportedFormat(uint32_t videoFormat);

	[[nodiscard]] static Source validate(int width, int height, uint32_t videoFormat, const BufferData& bufferData);

	[[nodiscard]] static Damage parseDamage(const void* metaData, uint32_t metaSize, int width, int height);

	[[nodiscard]] static bool copy(const Source& source, int width, int height, const Target& target,
								   const Damage& damage, bool forceFullFrame);

	static const char* statusName(Status status);

private:
	static bool isValidRect(const Rect& rect, int width, int height);
	static void copyRect(const Source& source, const Target& target, const Rect& rect);
};
