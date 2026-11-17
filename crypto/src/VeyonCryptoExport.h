// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QtGlobal>

#if defined(veyon_crypto_EXPORTS)
#define VEYON_CRYPTO_EXPORT Q_DECL_EXPORT
#else
#define VEYON_CRYPTO_EXPORT Q_DECL_IMPORT
#endif
