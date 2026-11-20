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

### Process layout: `veyon-capture-helper`

`veyon-server` makes itself non-dumpable (`PR_SET_DUMPABLE 0`) so that other
processes of the session user cannot read its memory (IPC keys, host key).
xdg-desktop-portal rejects such callers (`Unable to open /proc/<pid>/root`), so
every portal call and the PipeWire stream run in the separate executable
`veyon-capture-helper` (`PipeWireCaptureServer`, `PortalSession`,
`PipeWireFramebuffer`):

* `veyon-server` (this plugin, `CaptureHelperProcess`) starts the helper as its
  direct child, i.e. as the session user in the same systemd scope/cgroup, so
  the portal derives the same app ID as for the server.
* The only data passed is one stdin line `VEYONCAP1 <hex64> <socketName>`: a
  random 32-byte VNC password generated per server start and the name of the
  helper's socket `capture-<sessionId>-<random>.sock`. The helper never
  receives the IPC token, the host key, logon credentials or session data.
* The helper listens on no TCP/UDP port at all (so other local users can't
  reach it, and there are no WebSockets): it serves RFB on a Unix domain socket
  in the private directory `$XDG_RUNTIME_DIR/veyon` (mode 0700, socket 0600)
  and hands only connections of its own uid (`SO_PEERCRED`) to LibVNCServer, at
  most 16 at a time, each blocking it for at most 2 s.
* `veyon-server` connects only if `SO_PEERCRED` of the listener is its current
  helper process (the demo server worker: a child of the server) and never
  without VNC authentication. VNC authentication uses only the first 8 password
  bytes (56 bits) and is defence in depth.
* The helper refuses to run as root, stays dumpable (only `RLIMIT_CORE` 0),
  loads no plugins besides the platform plugin, has no service IPC and dies
  with the server (`PR_SET_PDEATHSIG`); when it exits, the server restarts it
  with an increasing delay (1 s … 30 s).
* Residual risk: the session user can kill, trace or replace the helper (or
  take its socket path while it restarts, which the server rejects) and thus
  show fake screen content or drop input of their own session; they cannot read
  the server's memory.

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

xdg-desktop-portal derives the app ID of a host application from the process
behind the calling D-Bus connection: from its systemd unit or scope
(`app-io.veyon.veyon\x2dserver-<random>.scope`, see `LinuxServerProcess`) or,
with xdg-desktop-portal ≥ 1.20, from a registration via
`org.freedesktop.host.portal.Registry`. Well-known bus names are not used, so
`veyon-capture-helper` does not register one, and there is no D-Bus activation
file (`veyon-server` must only be started by `veyon-service`).

The **XDG `.desktop` file** `io.veyon.veyon-server.desktop` is installed to
`/usr/share/applications/`. The portal reads `Name=Veyon Server` from it to
display "Veyon Server" in the permission dialog.

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
  `NotifyPointerMotionAbsolute` however takes positions in the coordinate space
  of the stream, which is the `size` the portal reports for the stream in the
  `Start` response. With display scaling this is the logical size of the
  output (e.g. 2560×1440 for a 4K output at 150 % on KDE Plasma), and
  xdg-desktop-portal rejects positions outside of it ("Invalid position").
  Positions are therefore scaled by stream size / framebuffer size and clamped
  to the stream, using the current framebuffer size so that a resize at
  runtime is taken into account. If the portal doesn't report a size (old
  portals), positions are passed unscaled. The stream's `position` is not
  added, since positions are relative to the stream.

* **Multi-monitor**: `SelectSources` currently requests a single monitor.
  Multi-monitor support can be added by iterating stream node IDs returned by
  the portal `Start` response.
