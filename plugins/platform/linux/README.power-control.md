# Linux session power control

Reboot and power-off requests first use logind's `Reboot(false)` and
`PowerOff(false)` on the system bus, then the existing ConsoleKit fallback.
This preserves the privileged server behavior on X11 and the login screen.
The log now records the name and message of a failed logind request; the old
code discarded this error before invoking desktop fallbacks.

If the system interfaces fail, KDE session actions use these no-argument
methods on the session bus only when the current desktop is KDE/Plasma:

| Service | Object | Interface | Method | Action |
| --- | --- | --- | --- | --- |
| `org.kde.Shutdown` | `/Shutdown` | `org.kde.Shutdown` | `logoutAndReboot` | Reboot |
| `org.kde.Shutdown` | `/Shutdown` | `org.kde.Shutdown` | `logoutAndShutdown` | Power off |
| `org.kde.Shutdown` | `/Shutdown` | `org.kde.Shutdown` | `logout` | Log out |

These are the methods used by
[Plasma session management](https://github.com/KDE/plasma-workspace/blob/master/libkworkspace/sessionmanagement.cpp).
The service may be D-Bus activated. No new Veyon service or polkit rule is
installed, and session power operations retain the desktop's authorization and
shutdown handling.

The desktop is selected from `XDG_CURRENT_DESKTOP`, then
`XDG_SESSION_DESKTOP`/`DESKTOP_SESSION`, with `KDE_FULL_SESSION` used only
when all desktop identifiers are empty. Installing an activatable KDE service
does not select it for an Xfce, GNOME or other desktop session; those sessions
retain their existing fallbacks.

The legacy `/KSMServer` `org.kde.KSMServerInterface.logout` method is used only
when the new service/interface/method is explicitly unavailable or unsupported.
Its arguments are D-Bus integers: `(0, 1, 2)` for reboot, `(0, 2, 2)` for power
off and `(0, 3, 2)` for logout. Permission denial or a timeout from the modern
KDE service is reported without a second request through the legacy API.
Desktop fallbacks are checked in sequence and stop after an accepted request.

`veyon-cli feature` reporting `[OK]` does not confirm that the remote operating
system completed shutdown: feature requests have no power-operation result
acknowledgement. Check the student's Veyon log if the machine stays running.
An accepted D-Bus request also does not prove eventual shutdown; the desktop
can still handle application/session shutdown asynchronously.

## Verification

`KdeSessionControlTest` runs under `dbus-run-session` when `WITH_TESTS=ON` and
`dbus-run-session` is available. It checks modern method names with zero
arguments, legacy integer signatures, and no legacy retry after AccessDenied
or an ambiguous NoReply error.
Both KDE service names are owned by fake test objects on an isolated session
bus; the tests do not call the system bus or start a real shutdown service.

On the installed system, check reboot and power-off in a Plasma Wayland
session, then repeat in Xfce/X11 and at the login screen. Also check KDE logout
and that a denied action produces a useful error rather than calls to
PowerDevil's unsupported `Reboot`/`Shutdown` methods.
