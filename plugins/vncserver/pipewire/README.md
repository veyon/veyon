# Veyon PipeWire VNC Server Plugin

Wayland VNC server backend for the Veyon Server using
**PipeWire** and the **XDG Desktop Portal RemoteDesktop** API.

## Overview

Under Wayland, X11-based screen capture (e.g., x11vnc) is not available.
This plugin uses the standard `org.freedesktop.portal.RemoteDesktop` D-Bus
interface to request screen content and input device access via
[xdg-desktop-portal](https://github.com/flatpak/xdg-desktop-portal),
then receives the video stream through a PipeWire remote and feeds it into
LibVNCServer so that VNC clients can connect.

### Portal flow

```
CreateSession  →  SelectDevices  →  SelectSources  →  Start  →  OpenPipeWireRemote
```

All steps are asynchronous via the portal `Response` signal.
If the stream ends (e.g., compositor restart) the session is automatically
restarted after a short delay.

## Dependencies

| Dependency | Debian package |
|---|---|
| PipeWire ≥ 0.3 | `libpipewire-0.3-dev` |
| xdg-desktop-portal | `xdg-desktop-portal` |
| xdg-desktop-portal-kde (KDE) | `xdg-desktop-portal-kde` |
| LibVNCServer ≥ 0.9.8 | `libvncserver-dev` |
| Qt5 DBus | `qtbase5-dev` (included) |

## Application identifier

The plugin registers itself under the stable application identifier:

```
io.veyon.veyon-server
```

This identifier is used by the XDG Desktop Portal permission store.
A consistent, non-Flatpak identifier is needed so that permissions granted by
an administrator survive service restarts.

The identifier is registered in three complementary ways:

1. **Host app registry** — with xdg-desktop-portal ≥ 1.19.4, `PortalSession`
   calls `org.freedesktop.host.portal.Registry.Register("io.veyon.veyon-server")`
   on a session bus connection of its own whenever it starts a session. All
   other portal calls go to the portal instance which answered, so a restarted
   portal sees `Register` first. Older portals derive the app_id from the
   systemd scope of the process. The portal doesn't take the app_id from
   well-known bus names.

2. **XDG `.desktop` file** — `io.veyon.veyon-server.desktop` is installed to
   `/usr/share/applications/`.  The portal reads `Name=Veyon Server` from it
   to display "Veyon Server" (instead of the terminal emulator or process name)
   in the KDE permission dialog.

3. **D-Bus session service file** — `io.veyon.veyon-server.service` is
   installed to `/usr/share/dbus-1/services/`.  It maps the well-known bus name
   to the `veyon-server` executable so dbus-daemon and xdg-desktop-portal can
   follow the full `bus_name → desktop_file → display_name` chain.

## Pre-authorization for KDE Plasma 6.3+

By default the portal will display a consent dialog the first time the Veyon
Server requests screen access.  In a classroom or kiosk scenario this dialog
must be suppressed so that the server daemon can start without user
interaction.

KDE Plasma 6.3+ supports permanent pre-authorization via the portal permission
store table `kde-authorized`.

### Method 1: `flatpak permission-set` (recommended)

```bash
flatpak permission-set kde-authorized screencast io.veyon.veyon-server yes
```

This command must be run **as the target user** (the user whose desktop is to
be captured), not as root.

### Method 2: D-Bus PermissionStore

If the `flatpak` CLI is not installed, you can use `dbus-send` directly:

```bash
dbus-send \
  --session \
  --print-reply \
  --dest=org.freedesktop.impl.portal.PermissionStore \
  /org/freedesktop/impl/portal/PermissionStore \
  org.freedesktop.impl.portal.PermissionStore.SetPermission \
  string:kde-authorized \
  boolean:false \
  string:screencast \
  string:io.veyon.veyon-server \
  array:string:yes
```

### Helper script

A convenience script is installed to `/usr/share/veyon/veyon-preauth-kde.sh`.
It tries Method 1 first and falls back to Method 2:

```bash
sudo -u <desktop-user> \
  DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$(id -u <desktop-user>)/bus" \
  /usr/share/veyon/veyon-preauth-kde.sh
```

### Debian post-install integration

In a Debian package, add a `postinst` snippet that runs the helper for each
auto-login user:

```sh
# debian/veyon-server.postinst
VEYON_USER="student"  # adjust to the auto-login user

if [ -S "/run/user/$(id -u ${VEYON_USER})/bus" ]; then
    sudo -u "${VEYON_USER}" \
        DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$(id -u ${VEYON_USER})/bus" \
        /usr/share/veyon/veyon-preauth-kde.sh || true
fi
```

### systemd service considerations

The Veyon Server is typically started as a system-level service (`root` or a
dedicated system user).  Under Wayland the portal, however, runs inside the
**user session** of the logged-in user whose screen is being shared.

The recommended setup is:

1. Run `veyon-server` as the **desktop user** (not root), or use `systemd --user`
   with `WantedBy=graphical-session.target`.
2. Ensure `DBUS_SESSION_BUS_ADDRESS` and `WAYLAND_DISPLAY` are forwarded to
   the service environment (see `ImportCredential=` / `PassEnvironment=`).
3. Run the pre-authorization script once during provisioning (post-install or
   cloud-init).

Example `veyon-server.service` override (`/etc/systemd/user/veyon-server.service`):

```ini
[Unit]
Description=Veyon Server (Wayland/PipeWire)
PartOf=graphical-session.target
After=graphical-session.target

[Service]
Type=simple
ExecStart=/usr/sbin/veyon-server
Restart=on-failure
RestartSec=5s

[Install]
WantedBy=graphical-session.target
```

Enable it as the desktop user:

```bash
systemctl --user enable --now veyon-server
```

## Limitations and known issues

* **GNOME**: GNOME's portal backend does not currently support the
  `kde-authorized` pre-authorization table.  An interactive consent dialog
  will appear on first use.  Persistent `restore_token` support (portal v2+)
  reduces subsequent prompts to zero after the first manual approval.

* **Input forwarding**: The plugin requests both keyboard and pointer device
  types via `SelectDevices`.  Key presses and releases are forwarded using
  `NotifyKeyboardKeysym`.  Pointer motion is forwarded as absolute coordinates
  via `NotifyPointerMotionAbsolute`, button presses/releases via
  `NotifyPointerButton`, and scroll wheel events via
  `NotifyPointerAxisDiscrete`.  All input events are ignored until the portal
  session is in the `Running` state.

* **Pointer coordinates and display scaling**: VNC clients send pointer
  positions in framebuffer pixels, i.e. in the size of the PipeWire buffers.
  xdg-desktop-portal (1.14 and later) only accepts positions for
  `NotifyPointerMotionAbsolute` inside the `size` the portal reports for the
  stream in the `Start` response and rejects all others ("Invalid position").
  With display scaling, KDE Plasma and mutter both report the logical size of
  the output (e.g. 2560×1440 for a 4K output at 150 %), but they interpret the
  positions differently:

  * **KDE Plasma** takes positions in logical pixels of the output, so they
    are scaled by stream size / framebuffer size and clamped to the stream.
  * **mutter (GNOME)** takes positions in buffer pixels and divides them by
    the output's scale itself if stage views are scaled (fractional scaling,
    `scale-monitor-framebuffer`). Positions are therefore passed unscaled and
    clamped to the stream. Since the portal limits them to the logical size,
    the area beyond it (right and bottom part of the screen) can't be reached
    with fractional scaling. This is an upstream limitation of the portal's
    `Notify*` methods; reaching it would need input via `ConnectToEIS`. Without
    scaled stage views stream and buffer size are the same and the pointer
    covers the whole screen.

  mutter is detected by its `org.gnome.Mutter.RemoteDesktop` service on the
  session bus (`XDG_CURRENT_DESKTOP` is only used if the bus can't be asked).
  If the portal doesn't report a size, positions are passed unscaled, which
  only works with portals older than 1.14; a warning is logged. The stream's
  `position` is not added, since positions are relative to the stream.

  The stream size is read once per portal session. If the size of the
  PipeWire buffers changes (e.g. after a change of the output's resolution),
  the portal session is restarted to read it again. A change of the scale
  alone leaves the buffers as they are and can't be noticed; the pointer is
  off then until the session is restarted (e.g. by restarting the Veyon
  Server).

* **Input errors**: An error reply to a single input request only drops that
  request and is logged at most every 10 s. A request without reply within
  5 s drops the pending input but keeps the session; key and button releases
  and the last pointer position are never dropped. A release the portal
  doesn't reply to is sent once more (an error reply would only be repeated),
  a motion without reply is sent again with the next pointer event, and keys
  and buttons still pressed are released when the session is closed or
  restarted, waiting at most 500 ms for the replies before the session is
  closed. The session is closed
  and restarted (which also restarts capture) only if the portal is gone,
  after 8 error replies in a row (pointer positions rejected by the portal
  don't count) or after two requests in a row without reply.

* **Multi-monitor**: `SelectSources` currently requests a single monitor.
  Multi-monitor support can be added by iterating stream node IDs returned by
  the portal `Start` response.
