// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>

namespace VeyonServiceInterface {
inline constexpr auto ServiceName = QLatin1String("io.veyon.veyon-service");
inline constexpr auto ObjectPath = QLatin1String("/io/veyon/VeyonService");
inline constexpr auto InterfaceName = QLatin1String("io.veyon.VeyonService");
inline constexpr auto AuthenticateMethod = QLatin1String("Authenticate");
}

namespace AuthHelperInterface {
inline constexpr auto ServiceName = QLatin1String("io.veyon.AuthHelper");
inline constexpr auto ObjectPath = QLatin1String("/io/veyon/AuthHelper");
inline constexpr auto InterfaceName = QLatin1String("io.veyon.AuthHelper");
inline constexpr auto AuthenticateMethod = QLatin1String("Authenticate");
}
