// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QPoint>
#include <QPointF>
#include <QSize>

#include <optional>

/**
 * @brief Geometry of a ScreenCast stream as reported in the portal's Start response.
 *
 * @c size is the stream size the portal checks pointer positions against. On scaled outputs both
 * KDE Plasma and mutter report the logical size (2560x1440 for a 4K output at 150 %), which
 * differs from the size of the PipeWire buffers. @c position is the stream's position in the
 * compositor's global coordinate space. Both are optional; old portals don't report them.
 */
struct PortalStreamGeometry
{
	QPoint position;
	QSize size;
};

/**
 * @brief How the compositor interprets the positions passed to NotifyPointerMotionAbsolute().
 */
enum class PortalPointerMappingMode {
	/**
	 * Positions are in the stream's coordinate space, i.e. in the stream size from the Start
	 * response, which differs from the buffer size on scaled outputs (KDE Plasma: logical pixels
	 * of the output). Positions are scaled by stream size / framebuffer size.
	 */
	Scale,
	/**
	 * Positions are in buffer pixels (mutter: it divides by the output's scale itself if stage
	 * views are scaled), while xdg-desktop-portal still limits them to the stream size, which
	 * mutter reports as the logical size. Positions are passed unscaled and clamped to the
	 * stream, so with fractional scaling on GNOME the area beyond the logical size can't be
	 * reached through the portal's Notify* methods (an upstream limitation, ConnectToEIS would
	 * be needed).
	 */
	ClampOnly,
};

/**
 * @brief Maps a pointer position of a VNC client to the coordinate space of a portal stream.
 *
 * VNC clients send positions in framebuffer pixels, i.e. in the size of the PipeWire buffers.
 * xdg-desktop-portal checks the positions passed to NotifyPointerMotionAbsolute() against the
 * stream size from the Start response (0 <= x < width, 0 <= y < height, error "Invalid position"
 * otherwise), and what the compositor makes of a position depends on @p mode.
 *
 * The position is clamped to the framebuffer first, and the result is always inside the stream.
 * Without a stream size the framebuffer size is used (portals < 1.14 don't check positions and
 * may not report a size; newer ones reject every position then). The stream's position is not
 * added: positions passed to the portal are relative to the stream, the compositor adds its
 * offset in the global space itself.
 *
 * @return the position to pass to NotifyPointerMotionAbsolute() or nothing if no framebuffer
 * size is known
 */
inline std::optional<QPointF> mapPointerToPortalStream(QPoint framebufferPosition, QSize framebufferSize,
													   const PortalStreamGeometry& stream,
													   PortalPointerMappingMode mode)
{
	if (framebufferSize.isEmpty())
	{
		return std::nullopt;
	}

	const auto streamSize = stream.size.isEmpty() ? framebufferSize : stream.size;

	const auto map = [mode](int position, int framebufferExtent, int streamExtent) {
		const auto pixel = qBound(0, position, framebufferExtent - 1);
		if (mode == PortalPointerMappingMode::ClampOnly)
		{
			return double(qMin(pixel, streamExtent - 1));
		}
		// at most (extent - 1) * streamExtent / extent, which is below streamExtent
		return double(pixel) * streamExtent / framebufferExtent;
	};

	return QPointF(map(framebufferPosition.x(), framebufferSize.width(), streamSize.width()),
				   map(framebufferPosition.y(), framebufferSize.height(), streamSize.height()));
}
