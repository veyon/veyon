// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "TextMessageSanitizer.h"

extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	const auto input = QString::fromUtf8(data, static_cast<int>(size));

	const auto sanitized = TextMessageSanitizer::sanitizeHtml(input);

	// sanitizing must be idempotent, otherwise the allowlist is not a fixed point
	if (TextMessageSanitizer::sanitizeHtml(sanitized) != sanitized)
	{
		__builtin_trap();
	}

	(void)TextMessageSanitizer::sanitizeTitle(input);
	(void)TextMessageSanitizer::isAllowedLink(input);

	return 0;
}
