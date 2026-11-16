// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>
#include <QUrl>

// Pure helper functions for hardening text messages received from a master
// (PLAN.md 5.7c). They depend on QtCore only.
class TextMessageSanitizer
{
public:
	static constexpr int MaxTextLength = 64 * 1024;
	static constexpr int MaxTitleLength = 256;
	static constexpr int MaxNestingDepth = 32;

	// Rewrites rich text so that only an allowlist of tags and attributes survives.
	// The result is well-formed (balanced tags, escaped text) and contains links
	// with allowed schemes only. Input longer than MaxTextLength is truncated.
	[[nodiscard]] static QString sanitizeHtml(const QString& html);

	// Returns the title as plain text: control characters removed, limited to MaxTitleLength.
	[[nodiscard]] static QString sanitizeTitle(const QString& title);

	// Returns true for valid http(s) URLs with a host and mailto URLs with an address.
	[[nodiscard]] static bool isAllowedLink(const QUrl& url);

	// Parses and checks a link as found in an href attribute or a linkActivated() signal.
	[[nodiscard]] static bool isAllowedLink(const QString& link);
};
