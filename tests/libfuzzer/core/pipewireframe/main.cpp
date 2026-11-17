// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PipeWireFrame.h"

#include <algorithm>
#include <cstring>
#include <vector>

// PipeWire frames in veyon-capture-helper (02-ipc.md §1a.5): stream size and format, spa_data,
// spa_chunk and SPA_META_VideoDamage are all chosen by the producer. The buffer data and the
// damage metadata are allocated with exactly the sizes the producer claims, so ASan catches any
// read beyond them, and the target with exactly the size of the frame.
//
// Input: 32 byte header (little-endian), damage metadata, pixel bytes (repeated to fill maxSize)
//   0 u16 width       2 u16 height     4 u32 videoFormat   8 u8 dataType   9 u8 flags
//   10 u16 metaSize   12 u32 maxSize   16 u32 offset       20 u32 size     24 i32 stride
//   28 i32 chunkFlags
// flags: bit 0 chunk present, bit 1 data not mapped, bit 2 force full frame, bits 3-7 target padding

namespace {

constexpr size_t HeaderSize = 32;
constexpr uint32_t MaximumDataSize = 1 << 18;

template<typename T>
T read(const uint8_t* data, size_t offset)
{
	T value;
	std::memcpy(&value, data + offset, sizeof(value));
	return value;
}

bool isConverted(const uchar* source, const uchar* target)
{
	return target[0] == source[2] && target[1] == source[1] && target[2] == source[0] && target[3] == source[3];
}

}



extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	if (size < HeaderSize)
	{
		return 0;
	}

	const int width = read<uint16_t>(data, 0);
	const int height = read<uint16_t>(data, 2);
	const auto videoFormat = read<uint32_t>(data, 4);
	const auto flags = read<uint8_t>(data, 9);
	const auto forceFullFrame = (flags & 4) != 0;
	const int targetPadding = flags >> 3;

	const auto* meta = data + HeaderSize;
	const auto metaSize = std::min<size_t>(read<uint16_t>(data, 10), size - HeaderSize);
	const std::vector<uint8_t> damageMeta(meta, meta + metaSize);

	const auto* pixels = meta + metaSize;
	const auto pixelsSize = size - HeaderSize - metaSize;

	const auto maxSize = read<uint32_t>(data, 12) % (MaximumDataSize + 1);
	std::vector<uchar> bufferData(maxSize);
	for (size_t filled = 0; filled < bufferData.size() && pixelsSize > 0; filled += pixelsSize)
	{
		std::memcpy(bufferData.data() + filled, pixels, std::min(pixelsSize, bufferData.size() - filled));
	}

	PipeWireFrame::BufferData frameData;
	frameData.type = read<uint8_t>(data, 8);
	frameData.data = (flags & 2) ? nullptr : bufferData.data();
	frameData.maxSize = maxSize;
	if (flags & 1)
	{
		frameData.chunk = PipeWireFrame::Chunk{read<uint32_t>(data, 16), read<uint32_t>(data, 20),
											   read<int32_t>(data, 24), read<int32_t>(data, 28)};
	}

	const auto source = PipeWireFrame::validate(width, height, videoFormat, frameData);
	(void)PipeWireFrame::statusName(source.status);

	// an empty std::vector has no heap block, so ASan would not see a read of it
	const auto damage = PipeWireFrame::parseDamage(metaSize > 0 ? damageMeta.data() : nullptr,
												   uint32_t(metaSize), width, height);
	if (damage.isFullFrame == false)
	{
		if (damage.count < 0 || damage.count > PipeWireFrame::MaximumDamageRegions)
		{
			__builtin_trap();
		}
		for (int i = 0; i < damage.count; ++i)
		{
			const auto& rect = damage.rects[size_t(i)];
			if (rect.x1 < 0 || rect.y1 < 0 || rect.x1 >= rect.x2 || rect.y1 >= rect.y2 ||
				rect.x2 > width || rect.y2 > height)
			{
				__builtin_trap();
			}
		}
	}

	if (source.status != PipeWireFrame::Status::Valid)
	{
		std::vector<uchar> target(16);
		if (PipeWireFrame::copy(source, 1, 1, {target.data(), 4, qint64(target.size())}, damage, true))
		{
			__builtin_trap();
		}
		return 0;
	}

	// the validated frame lies within the mapping
	const auto rowBytes = qint64(width) * PipeWireFrame::BytesPerPixel;
	const auto* begin = bufferData.data();
	if (source.pixels < begin || source.stride < rowBytes ||
		(source.pixels - begin) + qint64(source.stride) * (height - 1) + rowBytes > qint64(maxSize))
	{
		__builtin_trap();
	}

	const auto targetStride = int(rowBytes) + targetPadding;
	std::vector<uchar> target(size_t(qint64(targetStride) * (height - 1) + rowBytes), 0);
	if (PipeWireFrame::copy(source, width, height, {target.data(), targetStride, qint64(target.size())},
							damage, forceFullFrame) == false)
	{
		__builtin_trap();
	}

	const auto checkRect = [&](const PipeWireFrame::Rect& rect) {
		for (int y = rect.y1; y < rect.y2; ++y)
		{
			for (int x = rect.x1; x < rect.x2; ++x)
			{
				if (isConverted(source.pixels + qint64(y) * source.stride + x * 4,
								target.data() + qint64(y) * targetStride + x * 4) == false)
				{
					__builtin_trap();
				}
			}
		}
	};

	if (forceFullFrame || damage.isFullFrame)
	{
		checkRect({0, 0, width, height});
	}
	else
	{
		for (int i = 0; i < damage.count; ++i)
		{
			checkRect(damage.rects[size_t(i)]);
		}
	}

	return 0;
}
