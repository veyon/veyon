// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PipeWireFrame.h"

#include <algorithm>
#include <cstring>

extern "C" {
#include <spa/buffer/buffer.h>
#include <spa/buffer/meta.h>
#include <spa/param/video/raw.h>
}

namespace {

#ifdef SPA_CHUNK_FLAG_EMPTY
constexpr uint32_t PipeWireChunkFlagEmpty = SPA_CHUNK_FLAG_EMPTY;
#else
// older SPA headers lack the name; the bit is ABI and simply never set by older producers
constexpr uint32_t PipeWireChunkFlagEmpty = 1u << 1;
#endif

// damage covering more than 3/5 of the frame is cheaper to copy as a whole than rect by rect
constexpr qint64 FullFrameDamageNumerator = 3;
constexpr qint64 FullFrameDamageDenominator = 5;

}



bool PipeWireFrame::isValidSize(qint64 width, qint64 height)
{
	return width > 0 && height > 0 &&
		width <= MaximumDimension && height <= MaximumDimension &&
		width * height <= MaximumPixelCount;
}



bool PipeWireFrame::isSupportedFormat(uint32_t videoFormat)
{
	return videoFormat == SPA_VIDEO_FORMAT_BGRx;
}



PipeWireFrame::Source PipeWireFrame::validate(int width, int height, uint32_t videoFormat,
											  const BufferData& bufferData)
{
	if (isValidSize(width, height) == false)
	{
		return {Status::InvalidSize};
	}

	if (isSupportedFormat(videoFormat) == false)
	{
		return {Status::UnsupportedFormat};
	}

	// both are plain memory mapped by PipeWire (PW_STREAM_FLAG_MAP_BUFFERS); a DMA-BUF is rejected:
	// no modifier was negotiated, so its layout is unknown, and reading it would need DMA_BUF_IOCTL_SYNC
	if (bufferData.type != SPA_DATA_MemPtr && bufferData.type != SPA_DATA_MemFd)
	{
		return {Status::UnsupportedDataType};
	}

	if (bufferData.data == nullptr)
	{
		return {Status::NotMapped};
	}

	if (bufferData.chunk.has_value() == false)
	{
		return {Status::MissingChunk};
	}

	const auto& chunk = *bufferData.chunk;
	const auto chunkFlags = static_cast<uint32_t>(chunk.flags);

	// a buffer without image content is a regular producer signal, e.g. for cursor-only updates:
	// mutter 46 and KWin 5.27 send SPA_CHUNK_FLAG_CORRUPTED with size 0, KWin 6 the flag alone
	// ("do not look at the frame contents"), others size 0 or SPA_CHUNK_FLAG_EMPTY; producers report
	// the damage of skipped content with the next frame
	if (chunk.size == 0 || (chunkFlags & (SPA_CHUNK_FLAG_CORRUPTED | PipeWireChunkFlagEmpty)))
	{
		return {Status::NoContent};
	}

	// stride 0 means tightly packed rows; a negative (bottom-up) stride is never negotiated
	const auto rowBytes = qint64(width) * BytesPerPixel;
	const auto stride = chunk.stride == 0 ? rowBytes : qint64(chunk.stride);
	if (stride < rowBytes)
	{
		return {Status::InvalidStride};
	}

	// offset and size are producer-controlled, maxSize is the size of the mapping
	if (chunk.offset > bufferData.maxSize || chunk.size > bufferData.maxSize - chunk.offset)
	{
		return {Status::ChunkOutsideData};
	}

	// all values are < 2^32 (stride, size) and <= 2^14 (height), so this cannot overflow; the last
	// row has to be complete including its padding as producers always report stride * height
	if (stride * height > qint64(chunk.size))
	{
		return {Status::Truncated};
	}

	// stride is an int32 or rowBytes (<= 2^16)
	return {Status::Valid, static_cast<const uchar*>(bufferData.data) + chunk.offset, int(stride)};
}



PipeWireFrame::Damage PipeWireFrame::parseDamage(const void* metaData, uint32_t metaSize, int width, int height)
{
	Damage damage;

	if (metaData == nullptr || isValidSize(width, height) == false)
	{
		return damage;
	}

	// the metadata size bounds the array, an invalid (empty) entry marks its end (spa/buffer/meta.h)
	const auto entryCount = metaSize / sizeof(spa_meta_region);
	const auto* entries = static_cast<const uchar*>(metaData);

	const auto readEntry = [entries](size_t index) {
		spa_meta_region region;
		std::memcpy(&region, entries + index * sizeof(spa_meta_region), sizeof(region));
		return region;
	};

	size_t index = 0;
	for (; index < entryCount && index < size_t(MaximumDamageRegions); ++index)
	{
		const auto region = readEntry(index);
		if (spa_meta_region_is_valid(&region) == false)
		{
			break;
		}

		// clip in 64 bit: position is int32, size uint32
		const auto x = qint64(region.region.position.x);
		const auto y = qint64(region.region.position.y);
		const Rect rect{
			int(std::clamp<qint64>(x, 0, width)),
			int(std::clamp<qint64>(y, 0, height)),
			int(std::clamp<qint64>(x + region.region.size.width, 0, width)),
			int(std::clamp<qint64>(y + region.region.size.height, 0, height)),
		};

		// entirely outside of the frame
		if (rect.x2 <= rect.x1 || rect.y2 <= rect.y1)
		{
			continue;
		}

		damage.rects[size_t(damage.count)] = rect;
		++damage.count;
	}

	// no damage reported at all: treat as unknown, like a missing metadata block
	if (index == 0)
	{
		return damage;
	}

	// more regions than requested: the ones beyond the limit are not inspected, so they are unknown
	if (index == size_t(MaximumDamageRegions) && index < entryCount)
	{
		const auto region = readEntry(index);
		if (spa_meta_region_is_valid(&region))
		{
			return Damage{};
		}
	}

	damage.isFullFrame = false;
	return damage;
}



bool PipeWireFrame::copy(const Source& source, int width, int height, const Target& target,
						 const Damage& damage, bool forceFullFrame)
{
	if (source.status != Status::Valid || source.pixels == nullptr || isValidSize(width, height) == false)
	{
		return false;
	}

	const auto rowBytes = qint64(width) * BytesPerPixel;
	if (source.stride < rowBytes ||
		target.pixels == nullptr || target.stride < rowBytes ||
		target.size < qint64(target.stride) * (height - 1) + rowBytes)
	{
		return false;
	}

	if (forceFullFrame || damage.isFullFrame)
	{
		copyRect(source, target, Rect{0, 0, width, height});
		return true;
	}

	if (damage.count < 0 || damage.count > MaximumDamageRegions)
	{
		return false;
	}

	qint64 damagedArea = 0;
	for (int i = 0; i < damage.count; ++i)
	{
		const auto& rect = damage.rects[size_t(i)];
		if (isValidRect(rect, width, height) == false)
		{
			return false;
		}
		damagedArea += qint64(rect.x2 - rect.x1) * (rect.y2 - rect.y1);
	}

	// at most 16 rects of at most 2^26 pixels each, no overflow
	if (damagedArea * FullFrameDamageDenominator > qint64(width) * height * FullFrameDamageNumerator)
	{
		copyRect(source, target, Rect{0, 0, width, height});
		return true;
	}

	for (int i = 0; i < damage.count; ++i)
	{
		copyRect(source, target, damage.rects[size_t(i)]);
	}

	return true;
}



const char* PipeWireFrame::statusName(Status status)
{
	switch (status)
	{
	case Status::Valid: return "valid";
	case Status::NoContent: return "no image content";
	case Status::InvalidSize: return "invalid frame size";
	case Status::UnsupportedFormat: return "unsupported video format";
	case Status::UnsupportedDataType: return "missing or unsupported buffer data type";
	case Status::NotMapped: return "buffer data not mapped";
	case Status::MissingChunk: return "missing chunk";
	case Status::InvalidStride: return "invalid stride";
	case Status::ChunkOutsideData: return "chunk outside of buffer data";
	case Status::Truncated: return "truncated frame";
	}

	return "unknown";
}



bool PipeWireFrame::isValidRect(const Rect& rect, int width, int height)
{
	return rect.x1 >= 0 && rect.y1 >= 0 &&
		rect.x1 < rect.x2 && rect.y1 < rect.y2 &&
		rect.x2 <= width && rect.y2 <= height;
}



// BGRx to the R,G,B,x byte order of the rfbScreen (PipeWireCaptureServer::initVncServer())
void PipeWireFrame::copyRect(const Source& source, const Target& target, const Rect& rect)
{
	const auto pixelCount = size_t(rect.x2 - rect.x1);

	for (int y = rect.y1; y < rect.y2; ++y)
	{
		const auto* sourcePixel = source.pixels + qint64(y) * source.stride + qint64(rect.x1) * BytesPerPixel;
		auto* targetPixel = target.pixels + qint64(y) * target.stride + qint64(rect.x1) * BytesPerPixel;

		for (size_t x = 0; x < pixelCount; ++x)
		{
			targetPixel[0] = sourcePixel[2];
			targetPixel[1] = sourcePixel[1];
			targetPixel[2] = sourcePixel[0];
			targetPixel[3] = sourcePixel[3];
			sourcePixel += BytesPerPixel;
			targetPixel += BytesPerPixel;
		}
	}
}
