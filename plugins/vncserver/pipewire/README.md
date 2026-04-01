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
io.veyon.Veyon.Server
```

This identifier is used by the XDG Desktop Portal permission store.
A consistent, non-Flatpak identifier is needed so that permissions granted by
an administrator survive service restarts.

## Pre-authorization for KDE Plasma 6.3+

By default the portal will display a consent dialog the first time the Veyon
Server requests screen access.  In a classroom or kiosk scenario this dialog
must be suppressed so that the server daemon can start without user
interaction.

KDE Plasma 6.3+ supports permanent pre-authorization via the portal permission
store table `kde-authorized`.

### Method 1: `flatpak permission-set` (recommended)

```bash
flatpak permission-set kde-authorized screencast io.veyon.Veyon.Server yes
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
  string:io.veyon.Veyon.Server \
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
  types via `SelectDevices`.  Actual input injection (sending key/mouse events
  back through the portal) is not yet implemented and is documented as a
  future enhancement.

* **Multi-monitor**: `SelectSources` currently requests a single monitor.
  Multi-monitor support can be added by iterating stream node IDs returned by
  the portal `Start` response.
