// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QPoint>
#include <QPointF>
#include <QSize>

#include <cmath>
#include <optional>

/**
 * @brief Geometry of a ScreenCast stream as reported in the portal's Start response.
 *
 * @c size is the stream's size in the compositor's coordinate space, e.g. the logical size of a
 * scaled output (2560x1440 for a 4K output at 150 %), which may differ from the size of the
 * PipeWire buffers. @c position is the stream's position in the compositor's global coordinate
 * space. Both are optional; old portals don't report them.
 */
struct PortalStreamGeometry
{
	QPoint position;
	QSize size;
};

/**
 * @brief Maps a pointer position of a VNC client to the coordinate space of a portal stream.
 *
 * VNC clients send positions in framebuffer pixels, i.e. in the size of the PipeWire buffers,
 * while NotifyPointerMotionAbsolute() takes positions in the stream's coordinate space, which
 * xdg-desktop-portal checks against the stream size from the Start response (0 <= x < width,
 * 0 <= y < height, error "Invalid position" otherwise). The position is therefore scaled by
 * stream size / framebuffer size. Without a stream size the framebuffer size is used, which is
 * what the portal and the compositor expect if both sizes are the same (e.g. no scaling).
 *
 * The position is clamped to the framebuffer first, so the result is always inside the stream.
 * The stream's position is not added: positions passed to the portal are relative to the stream,
 * the compositor adds its offset in the global space itself.
 *
 * @return the position to pass to NotifyPointerMotionAbsolute() or nothing if no framebuffer
 * size is known
 */
inline std::optional<QPointF> mapPointerToPortalStream(QPoint framebufferPosition, QSize framebufferSize,
													   const PortalStreamGeometry& stream)
{
	if (framebufferSize.isEmpty())
	{
		return std::nullopt;
	}

	const auto streamSize = stream.size.isEmpty() ? framebufferSize : stream.size;

	const auto map = [](int position, int framebufferExtent, int streamExtent) {
		const auto pixel = qBound(0, position, framebufferExtent - 1);
		const auto mapped = double(pixel) * streamExtent / framebufferExtent;
		// (extent - 1) * streamExtent / extent < streamExtent, but never rely on rounding here
		return std::fmin(mapped, std::nextafter(double(streamExtent), 0.0));
	};

	return QPointF(map(framebufferPosition.x(), framebufferSize.width(), streamSize.width()),
				   map(framebufferPosition.y(), framebufferSize.height(), streamSize.height()));
}
