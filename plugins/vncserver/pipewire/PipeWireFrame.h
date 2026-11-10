/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once
#include <cstdint>

namespace PipeWireFrame
{
inline bool validSize(int width, int height)
{
	return width > 0 && height > 0 && width <= 16384 && height <= 16384 &&
		uint64_t(width) * uint64_t(height) * 4 <= 128 * 1024 * 1024;
}

// A mapped chunk may have padding between rows and an offset into its buffer.
inline bool fits(int width, int height, int32_t stride, uint32_t offset, uint32_t size, uint32_t maxSize)
{
	if (!validSize(width, height) || stride < 0 || offset > maxSize || size > maxSize - offset) { return false; }
	const auto rowBytes = uint64_t(width) * 4;
	const auto rowStride = stride > 0 ? uint64_t(stride) : rowBytes;
	// QImage conversion may copy row padding, including the final scanline.
	return rowStride >= rowBytes && rowStride * uint64_t(height) <= size;
}
}
