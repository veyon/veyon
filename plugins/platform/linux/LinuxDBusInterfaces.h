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

namespace DBusDaemonInterface {
inline constexpr auto ServiceName = QLatin1String("org.freedesktop.DBus");
inline constexpr auto ObjectPath = QLatin1String("/org/freedesktop/DBus");
inline constexpr auto InterfaceName = QLatin1String("org.freedesktop.DBus");
inline constexpr auto NameHasOwnerMethod = QLatin1String("NameHasOwner");
}

namespace LogindManagerInterface {
inline constexpr auto ServiceName = QLatin1String("org.freedesktop.login1");
inline constexpr auto ObjectPath = QLatin1String("/org/freedesktop/login1");
inline constexpr auto InterfaceName = QLatin1String("org.freedesktop.login1.Manager");
inline constexpr auto TerminateSessionMethod = QLatin1String("TerminateSession");
}

namespace ConsoleKitManagerInterface {
inline constexpr auto ServiceName = QLatin1String("org.freedesktop.ConsoleKit");
inline constexpr auto ObjectPath = QLatin1String("/org/freedesktop/ConsoleKit/Manager");
inline constexpr auto InterfaceName = QLatin1String("org.freedesktop.ConsoleKit.Manager");
inline constexpr auto CloseSessionMethod = QLatin1String("CloseSession");
}

// session management of current Plasma versions (D-Bus activated, methods without arguments),
// as used by plasma-workspace's libkworkspace (SessionManagement)
namespace KdeShutdownInterface {
inline constexpr auto ServiceName = QLatin1String("org.kde.Shutdown");
inline constexpr auto ObjectPath = QLatin1String("/Shutdown");
inline constexpr auto InterfaceName = QLatin1String("org.kde.Shutdown");
inline constexpr auto LogoutMethod = QLatin1String("logout");
inline constexpr auto RebootMethod = QLatin1String("logoutAndReboot");
inline constexpr auto PowerOffMethod = QLatin1String("logoutAndShutdown");
}

// session management of older Plasma versions: logout(int confirm, int type, int mode) with
// the values of LinuxDesktopIntegration::KDE
namespace KsmServerInterface {
inline constexpr auto ServiceName = QLatin1String("org.kde.ksmserver");
inline constexpr auto ObjectPath = QLatin1String("/KSMServer");
inline constexpr auto InterfaceName = QLatin1String("org.kde.KSMServerInterface");
inline constexpr auto LogoutMethod = QLatin1String("logout");
}

namespace GnomeSessionManagerInterface {
inline constexpr auto ServiceName = QLatin1String("org.gnome.SessionManager");
inline constexpr auto ObjectPath = QLatin1String("/org/gnome/SessionManager");
inline constexpr auto InterfaceName = QLatin1String("org.gnome.SessionManager");
}

namespace MateSessionManagerInterface {
inline constexpr auto ServiceName = QLatin1String("org.mate.SessionManager");
inline constexpr auto ObjectPath = QLatin1String("/org/mate/SessionManager");
inline constexpr auto InterfaceName = QLatin1String("org.mate.SessionManager");
}

// power manager of Xfce and LXDE
namespace PowerManagementInterface {
inline constexpr auto ServiceName = QLatin1String("org.freedesktop.PowerManagement");
inline constexpr auto ObjectPath = QLatin1String("/org/freedesktop/PowerManagement");
inline constexpr auto InterfaceName = QLatin1String("org.freedesktop.PowerManagement");
}
