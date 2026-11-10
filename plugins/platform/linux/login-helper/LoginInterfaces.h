/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

namespace LoginHelperInterface
{
inline constexpr auto Service = "io.veyon.LoginHelper";
inline constexpr auto Path = "/io/veyon/LoginHelper";
inline constexpr auto Interface = "io.veyon.LoginHelper";
inline constexpr auto AgentPath = "/io/veyon/LoginAgent";
inline constexpr auto AgentInterface = "io.veyon.LoginAgent";
inline constexpr auto VeyonService = "io.veyon.veyon-service";
inline constexpr int AuthenticationTimeout = 45000;
inline constexpr int LoginTimeout = 60000;
inline constexpr int ServiceTimeout = 65000;
inline constexpr int ClientTimeout = 70000;
}
