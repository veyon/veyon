# LightDM remote login

`veyon-login-helper` implements the **Log in** action for LightDM without
keyboard emulation. It has two modes:

* `--service`: a root-owned system service, started with `veyon.service` and
  also available through D-Bus activation.
* `--lightdm-greeter -- <command> [arguments]`: a wrapper around the existing
  GTK/KDE greeter, running with the greeter's usual UID.

GDM and other display managers continue to use Veyon's existing login path.
No GDM adapter is included.

## Installation

Run administration commands on the student computer as root. Install the
updated Linux platform plugin and Veyon service together with these files
(paths below assume a `/usr` prefix and `/etc` configuration directory):

| File | Purpose |
| --- | --- |
| `/usr/bin/veyon-login-helper` | System helper and greeter bridge |
| `/usr/bin/veyon-lightdm-greeter` | LightDM launcher |
| `veyon-login-helper.service` in the system unit directory | Helper service |
| `/usr/share/dbus-1/system-services/io.veyon.LoginHelper.service` | D-Bus activation |
| `/usr/share/dbus-1/system.d/io.veyon.LoginHelper.conf` | System bus permissions |
| `/usr/share/lightdm/lightdm.conf.d/90-veyon-login.conf` | LightDM integration |

CMake installs the LightDM snippet into
`${datadir}/lightdm/lightdm.conf.d/` with this setting:

```ini
[Seat:*]
greeter-wrapper=/usr/bin/veyon-lightdm-greeter
```

The executable path is generated from the installation prefix; the example
above uses `/usr`. Keep the existing `greeter-session` value. There is no need
to replace `lightdm-gtk-greeter` or `lightdm-kde-greeter`.

LightDM 1.32 treats `greeter-wrapper` as **one executable pathname**, without
shell parsing or argument splitting. Do not put helper flags or a second
wrapper on that configuration line. LightDM appends the original greeter
command and arguments to the launcher; the launcher uses `exec` to start the
bridge with `--lightdm-greeter --`, preserving the process identity expected
by the system helper.

The launcher automatically preserves ALT Linux's `/etc/X11/Xgreeter.lightdm`
when that executable exists. The resulting chain is LightDM -> bridge ->
Xgreeter.lightdm -> original GTK/KDE greeter. Both paths are generated from
the installation directories; these examples assume `/usr` and `/etc`.

For another existing wrapper, create an executable, root-owned
`/etc/veyon/lightdm-greeter-wrapper` which delegates to it, for example:

```sh
#!/bin/sh
exec /path/to/existing-wrapper --its-option "$@"
```

This optional hook takes precedence over the automatic ALT wrapper. Keep
the `greeter-wrapper` configuration set to the single Veyon launcher path.
Check the effective configuration with `lightdm --show-config`. Save the
previous `greeter-wrapper` value before removing or commenting it out in
`/etc/lightdm/lightdm.conf`: that setting overrides the packaged snippet.
Within one configuration directory, `.conf` files are read in filename order;
administrator snippets in `/etc/lightdm/lightdm.conf.d/` override packaged
snippets in `/usr/share`, and the main `lightdm.conf` is read last. These
overrides apply to the same option in the same section.

Activate the new configuration by restarting LightDM or rebooting after logout;
restarting LightDM ends running desktop sessions.
`veyon.service` pulls in `veyon-login-helper.service` before the display manager
starts. The helper does not need a separate `systemctl enable`; the Veyon
service must be enabled by the package or administrator. Both services restart
after an unexpected exit. Startup does not wait for `network-online.target`;
domain authentication may still require a working network.

The helper unit and the updated Veyon unit must both be included in the RPM;
copying only the new executable is insufficient. After updating systemd units,
the package must reload the service manager with `systemctl daemon-reload`.
After installing or changing D-Bus policy, reload the system bus configuration
with `systemctl reload dbus.service`. The wrapper configuration takes effect
after restarting LightDM. It does not override administrator settings in
`/etc/lightdm` or retrofit a bridge into an already running greeter.

If Veyon and LightDM use different installation prefixes (for example Veyon
under `/usr/local` and LightDM under `/usr`), copy the generated configuration
to `/etc/lightdm/lightdm.conf.d/90-veyon-login.conf` so LightDM reads it.

For NixOS, the module defines the D-Bus system service. Configure
`services.xserver.displayManager.lightdm.extraSeatDefaults` with the wrapper
path above (using `${config.services.veyon.package}/bin/veyon-lightdm-greeter`);
NixOS generates LightDM's own configuration.

## Routing and authentication

Veyon queries logind for the **currently running** session. An online/active
greeter on a seat with PAM service `lightdm-greeter` uses the new API.
This also handles distributions that assign the greeter `Class=user`.
An omitted/`unspecified` greeter type is accepted only for this LightDM PAM
service; the server probes its display environment and agent registration
still requires the trusted wrapper/LightDM process relationship. TTY sessions
are excluded. A LightDM greeter with `Class=user` is not counted as a logged-in
student by Veyon's access checks or tray notifications.
The installed greeter packages and the desktop selected for the next login
do not determine routing. Custom LightDM installations that rename the
`lightdm-greeter` PAM service require corresponding detection changes.

Other display managers keep the legacy keyboard-based login path and
`Linux/UserLoginKeySequence`. If LightDM is detected but its wrapper
is unavailable, Veyon reports an error and does not type the password into an
unknown UI state.

The service periodically reconciles graphical sessions in both single- and
multi-session modes. It retries after missing startup metadata, refreshes the
display environment after server crashes, and follows an existing greeter when
it becomes active again without a new `SessionNew` signal. It retains the
current server until a replacement session's environment is ready. For X11
greeters, a regular root-owned `XAUTHORITY` file is permitted because the server
runs as root; group/world-writable files are still rejected. The greeter server
is launched directly, without a Plasma/GNOME user-session launcher.

## Greeter server mode

The Linux service automatically passes `--greeter` to `veyon-server` for a
logind greeter, including LightDM greeters reported as `Class=user`. No extra
service or manual server configuration is needed. The greeter does not wait
for `MinimumUserSessionLifetime`. After login, the ordinary server starts with
the existing user-session behavior and configured VNC backend.

The authenticated control endpoint runs independently of display capture.
During boot it can accept a Master connection before the internal capture
listener is ready. A refused internal connection is retried every 250 ms within
the original 30-second RFB handshake deadline, retaining the authenticated
external connection. This does not retry authentication failures or bypass
access control. Disconnecting Master stops further connection attempts.
For an X11 greeter it launches the built-in X11 capture backend in a child
of the same executable (`--greeter-vnc x11 <internal-port>`), independently of the desktop backend
selected in Veyon. Desktop `X11Vnc/ExtraArguments` are not applied in this
mode. The greeter's X11 `Display` property from logind takes precedence over
an inherited `DISPLAY`. The service validates `XAUTHORITY`; if unavailable,
it checks the greeter's `$HOME/.Xauthority`, then the standard LightDM
`/run/lightdm/root/:N` cookie for that exact local display. These fallbacks
retain the same ownership and permission checks. A custom LightDM
`run-directory` requires the correct `XAUTHORITY` in the greeter environment.
Capture uses explicit `-display`/`-auth`, disables shared memory, and ignores
`~/.x11vncrc`. GUI feature workers still inherit the real display environment.
X11 runs as root;
a Wayland greeter server runs under the greeter account's UID.

An X11 or KWin backend which fails before becoming ready is attempted up to three
times, one second apart, with the display environment refreshed before each
attempt. Each authenticated RFB startup probe has a 10-second limit. If all
attempts fail, or a backend exits after becoming ready,
the parent starts the headless backend on the same internal port with the same
random VNC password. The teacher sees a placeholder instead of the greeter,
but can reconnect and send an authenticated login command. Existing VNC
connections may disconnect when switching to the fallback. While the placeholder
is running, the supervisor checks the greeter environment once per second.
A newly available display or changed display/socket/authority metadata triggers
another bounded native startup attempt. Unchanged failing environments do not
continuously restart capture. The old placeholder is stopped before the native
listener reuses its port; viewers may reconnect, while the authenticated login
channel stays available. If the fallback also fails, the server exits and the
service's existing restart mechanism retries it. The `builtin-x11vnc-server`,
`kwin-greeter-vnc-server` and `headless-vnc-server` plugins must be included in
the installed package.

A native **KDE/KWin Wayland greeter** launches `--greeter-vnc kwin` under the
greeter's UID. The separate `kwin-greeter-vnc-server` plugin requests a continuous
PipeWire stream directly through KWin's
[`zkde_screencast_unstable_v1`](https://invent.kde.org/libraries/plasma-wayland-protocols/-/blob/master/src/protocols/zkde-screencast-unstable-v1.xml)
Wayland protocol (version 3). It captures the union of logical output geometries
at logical resolution, including the cursor, so fractional scaling and multiple
monitors do not require guessing pixel-to-logical coordinate ratios. There is
no screenshot polling, desktop portal or consent dialog. The normal desktop
backend retains its portal-based setup. Remote account login still uses the
LightDM bridge; interactive control of the greeter uses KWin's input interface.
Other Wayland compositors, including GNOME/Mutter, retain the headless fallback.
The compositor must already run in the greeter session. Selecting a future
Plasma Wayland desktop does not turn an X11 LightDM greeter into a Wayland one.

KWin authorizes the installed `veyon-server` executable through
`X-KDE-Wayland-Interfaces=zkde_screencast_unstable_v1,org_kde_kwin_fake_input`
in the installed `io.veyon.veyon-server.desktop`. Include that desktop file even when splitting
the native greeter plugin into a separate RPM. This KDE-specific list uses
commas, not the semicolons used by standard desktop-entry list fields such as
`Categories`. A semicolon-separated value does not grant either interface.
After correcting an installed file, reboot or restart the greeter/compositor
after ending any user session to refresh KDE's application metadata and restart
the capture backend. Do not disable KWin's permission
checks. This is separate from the system-bus `LoginAgent.Login`/`Cancel`
permissions below. Capture connects to the greeter's `WAYLAND_DISPLAY`, resolved
relative to `XDG_RUNTIME_DIR` if needed, and verifies the socket peer's UID.

Remote control forwards authenticated internal RFB key and pointer events to
[`org_kde_kwin_fake_input`](https://invent.kde.org/libraries/plasma-wayland-protocols/-/blob/master/src/protocols/fake-input.xml)
on a separate verified Wayland connection, opened only after the first captured
frame. Input protocol errors or disconnections disable remote input without
closing the capture connection or switching the image to a placeholder.
Absolute pointer coordinates are mapped
from the framebuffer to the captured logical workspace, including negative
monitor origins. Left/middle/right buttons, side buttons, dragging, and vertical
and horizontal wheel events are supported. The existing Master remote-control
window is used; no additional network listener or privileged input daemon is
introduced. View-only connections do not send input events.

KWin 6.1 exposes fake-input version 5, which takes physical evdev keycodes.
The backend reads the greeter's `wl_keyboard` keymap (bounded to 1 MiB), uses
libxkbcommon to map RFB keysyms, and supplies Shift/level-3 modifiers when needed.
Unicode RFB symbols can be matched to equivalent legacy keysyms in the keymap.
The backend subscribes to KWin `org_kde_kwin_keystate` version 1 for CapsLock
state without taking keyboard focus. Modifier selection accounts for that
state, including changes made locally while remote control is connected.
Legacy keyboard input waits for the initial CapsLock state; if this protocol
is unavailable, the pointer and video remain usable but v5 keyboard input is
disabled. No additional daemon or service is required.
The active layout and lock states remain those selected in the greeter: select
the matching layout there before typing. A symbol absent from the configured
keymap is ignored. This path does not switch layouts or insert arbitrary Unicode
text. Keypad keysyms map to physical keypad keys, including NumLock-dependent
levels; set the greeter's NumLock state for the desired keypad behavior.
The mapper does not toggle lock keys. With fake-input version 6 the backend
sends keysyms directly instead.
Clipboard exchange remains unavailable in the greeter's QCoreApplication mode.

Held keys and buttons are tracked per RFB client, with shared references when
multiple clients hold the same input. Disconnecting a client releases only its
own references; stopping capture releases all remaining input and keeps the
input connection alive until a Wayland sync confirms processing or a one-second
deadline expires. Duplicate key presses and unmatched releases are ignored.
Typed characters and login credentials are never logged. If fake-input is not
advertised, capture stays available in view-only mode. A missing/invalid keymap
on version 5 disables keyboard input while preserving the pointer and video.

The backend reuses PipeWire on that account's socket. If no daemon is listening,
it starts and supervises `pipewire -c <datadir>/veyon/greeter-pipewire.conf`
under the greeter account, then stops that child when capture ends. It never
stops an existing daemon or removes another instance's socket. This minimal
configuration loads native transport, client nodes, link factory and access
checks, with no audio devices or PulseAudio server. The consumer creates its
video link explicitly, so WirePlumber and pipewire-media-session are unnecessary.
PipeWire must be available to KWin on the same socket: `PIPEWIRE_RUNTIME_DIR`
(default `XDG_RUNTIME_DIR`) and `PIPEWIRE_REMOTE` (default `pipewire-0`) must
match the compositor's environment. The runtime directory must belong to the
greeter and have no group/other permissions; socket connections are checked
against that UID. A custom remote name must be a basename, not a path.

Packaging requires `kwin-greeter-vnc-server.so`, the server desktop file,
`<datadir>/veyon/greeter-pipewire.conf`, and the PipeWire daemon with the four
modules listed above. Building this optional backend (`WITH_KWIN_GREETER=ON`,
the default) requires PipeWire/SPA development headers, `wayland-client >= 1.18`
and `wayland-scanner`, plus `xkbcommon >= 1.0` development files.
`WITH_KWIN_GREETER=OFF` keeps the remaining backends available, with the
placeholder on native Wayland greeters. No extra LightDM
configuration is required beyond the existing login wrapper integration.

Startup has an 8.5-second first-frame deadline within the supervisor's 10-second
RFB probe. PipeWire startup is limited to three seconds; Wayland negotiation to
five seconds. Image dimensions, stride, mapped-buffer bounds and bounded damage
metadata are validated (maximum 128 MiB per frame). The internal RFB listener
opens only after a complete actual frame. Capture failure, stream closure or an
output removal falls back to the placeholder while preserving the login channel.
Capture has no fixed five-frame limit; the compositor produces frames as the
screen changes. Output layout changes require restarting the capture backend.
This does not add a GDM login adapter.

The capture child serves password-protected RFB on loopback. The random
internal VNC password is separate from the student's login password and is
passed through stdin; external x11vnc builds use their existing temporary
password-file mechanism for the final capture process. Veyon authentication
and access rules apply to every remote connection, including the placeholder.
The greeter exposes its complete framebuffer; per-monitor Qt screen metadata
and clipboard synchronization are unavailable in this mode.

## Login authorization

The remote command follows the normal authenticated Veyon feature channel:

* The Veyon service authorizes only the server process it manages for the
  target session, checking its executable, UID, PID and start time. Capture
  children and independently launched servers cannot request API login.
* The helper accepts registrations only from the installed bridge executable,
  directly parented by the root LightDM process recorded as the session's
  logind `Leader`. Its executable and directory ancestors must be root-owned
  and not writable by group/others. Sharing the greeter's UID is insufficient.
* The helper accepts login requests only from the root owner of the Veyon
  service's D-Bus name; the bridge accepts `Login` and `Cancel` only from the
  root owner of the helper's name. System bus policy must allow those calls
  separately; see [D-Bus permissions](#d-bus-permissions).
* Overlapping requests and an existing user desktop on the same seat are
  rejected. Core dumps and same-UID process inspection are disabled in both
  helper modes. These checks do not protect against compromised root or code
  already running inside the trusted bridge/greeter.

A custom launcher placed **before** the bridge, or before the managed server,
must `exec` it rather than fork another process. Wrappers chained **after**
the bridge are unaffected.

The bridge speaks LightDM greeter protocol v1. It isolates manual and remote
authentication conversations and drops obsolete replies. Remote login answers
username/password prompts and requests session start after authentication.
It does not change PAM, autologin settings or LightDM configuration at login
time. Verify compatibility with the distribution's LightDM/greeter builds.

Cancelling, timing out or resetting a pending session start closes the bridge
connection to prevent late replies from affecting another attempt; LightDM
must recover the greeter. If the helper stops during authentication, the bridge
cancels that attempt before registering with the replacement service.

Success is confirmed by a graphical user session with the requested account's
UID on the same seat in logind. Accounts must be resolvable through NSS,
including domain accounts provided by SSSD. Authentication is performed by
LightDM using its configured PAM stack, not by `veyon-auth-helper`.

On the LightDM API path, the student's password is not written to persistent
service data, files or command arguments. Extra authentication prompts (OTP,
password change, additional secrets) return an explicit error; the password
is never reused as a second secret.

The bridge has a 45-second deadline covering authentication and the
session-start response. In parallel, the helper allows 60 seconds for the
operation, including confirmation in logind. These are overlapping deadlines,
not an additional 60 seconds after authentication. Preliminary checks and the
synchronous NSS account lookup happen before the helper's timer starts, so
these limits do not bound the entire request when NSS is slow or unavailable.

## Desktop session selection

An empty `Linux/UserLoginSession` lets LightDM choose the default
desktop. A nonempty value explicitly selects a session ID, for example:

```sh
veyon-cli config set Linux/UserLoginSession plasma
```

Use the basename of the appropriate `.desktop` file in the distribution's
session directories, without `.desktop`. An explicit ID must have an installed
session definition with a nonempty `Exec` in LightDM's configured
`sessions-directory`; otherwise the helper rejects it before authentication.
The helper reads LightDM's standard configuration snippets and main file,
including a custom main file selected with the daemon's `--config` option.
It resolves the trusted main daemon from the greeter's root-owned LightDM
session child, and resolves relative config paths against the main daemon's
working directory. If that relationship cannot be verified, explicit session
selection is rejected before authentication.
An empty ID keeps LightDM's default selection. The remote path does not read a
session selection that exists only in the greeter's UI.

To return to LightDM's default selection:

```sh
veyon-cli config unset Linux/UserLoginSession
```

Run these commands as root on the student computer. Restart `veyon.service`
after changing the setting so the running server loads it; existing Veyon
connections will disconnect. The helper does not need a restart for this change.

## Disabling, removing and recovering the integration

Stopping Veyon services does not remove the LightDM wrapper setting. The
bridge still starts the original greeter and permits manual login when the
helper is unavailable. Without a running Veyon service, remote monitoring and
login are unavailable. `systemctl disable` only removes autostart links; it
does not stop running units or prevent the helper's D-Bus activation. An enabled
Veyon service also pulls in the helper through its `Wants` dependency.

To bypass the bridge while keeping Veyon installed, restore the saved
`greeter-wrapper` value in the appropriate section of
`/etc/lightdm/lightdm.conf`. For the ALT setup using the standard wrapper:

```ini
[Seat:*]
greeter-wrapper=/etc/X11/Xgreeter.lightdm
```

Use your previous wrapper path if it differs. If there was no wrapper before
Veyon, set `greeter-wrapper=` explicitly to override the packaged snippet.
Keep the existing `greeter-session`. This is also the recovery procedure if
the Veyon launcher prevents the greeter from starting; edit the configuration
from a text console or another administrator session.

Before removing the package, restore the original setting, remove any manually
copied Veyon snippet from `/etc/lightdm/lightdm.conf.d/`, and check
`lightdm --show-config`. The effective wrapper must no longer refer to Veyon.
Package removal must remove its own snippet from `/usr/share` together with
the executables, units and D-Bus files. It does not undo your manual edits to
`lightdm.conf` or remove manually created configuration copies.

Changes apply at the next LightDM startup, not to an already running bridge.
After logout, restart LightDM or reboot to verify manual login. Stopping Veyon
services alone does not require restarting LightDM.

## Collecting a failed login

After installing the updated package and booting to the login screen, check
that `veyon.service` and `veyon-login-helper.service` are active and that the
helper logs `LightDM bridge registered`. There must also be a `veyon-server`
process with `--greeter` and a capture child for the greeter. The main log is
named `GreeterServer`, and capture logs use `GreeterCapture` (both may have a
session-ID suffix). Capture the following after one Master login attempt:

```sh
systemctl status veyon.service veyon-login-helper.service lightdm.service --no-pager -l
journalctl -b -u veyon.service -u veyon-login-helper.service -u lightdm.service --no-pager -l
loginctl list-sessions
lightdm --show-config
```

On installations naming the unit `veyon-service.service`, use that unit instead
of `veyon.service`. Also collect the configured Veyon service/server log files
and LightDM's greeter log (commonly `/var/log/lightdm/seat0-greeter.log`). The
greeter server's stderr is forwarded to the parent service so display and Qt
startup failures can be seen in its journal.

The helper logs readiness, bridge registration/rejection, requested seat and
account UID, and the final outcome. The wrapper logs authentication start, PAM
result and session-start result without passwords or prompt text.

* No `LightDM bridge registered`: check the effective wrapper configuration,
  installed executable permissions and process/session identity errors.
* Registration followed by `Forwarding login to LightDM bridge` and
  `Rejected send message` for `io.veyon.LoginAgent`: check the system bus policy
  described below. The command was blocked before reaching PAM.
* A failed `LightDM PAM authentication result`: inspect the account, credentials
  and LightDM PAM stack. A successful PAM result followed by a failed
  session-start result points to desktop session startup.
* `Greeter capture unavailable`: inspect `Greeter capture session type`,
  `Greeter X11 capture using display`, `DISPLAY`/`XAUTHORITY` and capture logs.
  `Greeter X11 capture requires DISPLAY and a validated XAUTHORITY file` means
  no usable display address or cookie file was found. `Retrying greeter capture`
  reports a fresh startup attempt and its backend.
  The placeholder preserves the remote login channel; capture failure alone
  does not imply a failed login.
* `KWin screencast or logical-output protocol is unavailable`: check that the
  greeter really runs under KWin Wayland and that the installed server desktop
  file declares `X-KDE-Wayland-Interfaces=zkde_screencast_unstable_v1`.
* `Cannot connect to the greeter-owned Wayland socket`: check the greeter's
  `WAYLAND_DISPLAY` and `XDG_RUNTIME_DIR`; a desktop's socket is not sufficient.
* `Greeter PipeWire daemon failed/exited` or a startup timeout: check that
  `pipewire` and the installed `greeter-pipewire.conf` modules are present.
  Check the daemon's forwarded errors in the Veyon journal. Never start the
  greeter's PipeWire as root.
* `KWin greeter stream did not deliver a first frame before the deadline`:
  inspect KWin/PipeWire errors and verify both processes use the same runtime
  directory and PipeWire socket. `KWin greeter PipeWire stream ready with actual
  frame` confirms that the consumer received real compositor pixels.
* `KWin fake-input is unavailable; greeter capture remains view-only`: verify
  `org_kde_kwin_fake_input` is present in the installed desktop file's
  `X-KDE-Wayland-Interfaces` list. Do not disable KWin's permission checks.
* `KWin greeter remote input disabled; capture continues`: input negotiation or
  its connection failed. The log includes the Wayland interface, object ID and
  protocol error code when available. Capture uses a different connection;
  inspect separate capture/PipeWire errors if a placeholder is shown.
* `KWin greeter remote input availability: pointer true keyboard true` confirms
  that input interfaces and the required keymap are available; it does not
  prove a particular greeter consumed an event. If only the keyboard is false,
  inspect the keymap warning. If characters differ, check the greeter's selected
  layout and Caps Lock/Num Lock state.

### D-Bus permissions

The helper sends `Login` and `Cancel` on interface `io.veyon.LoginAgent`, object
path `/io/veyon/LoginAgent`, to the bridge's unique bus connection name.
Permission to send to `io.veyon.LoginHelper` does not cover this destination.
These are system bus permissions, not file permissions or PAM settings.

The installed `/usr/share/dbus-1/system.d/io.veyon.LoginHelper.conf` already
contains both permissions. Normally, install that policy with the helper and
reload the system bus configuration. For an older package missing these rules,
create `/etc/dbus-1/system.d/veyon-login-agent.conf` as root:

```sh
cat > /etc/dbus-1/system.d/veyon-login-agent.conf <<'EOF'
<busconfig>
  <policy user="root">
    <allow send_type="method_call"
           send_interface="io.veyon.LoginAgent"
           send_path="/io/veyon/LoginAgent"
           send_member="Login"/>
    <allow send_type="method_call"
           send_interface="io.veyon.LoginAgent"
           send_path="/io/veyon/LoginAgent"
           send_member="Cancel"/>
  </policy>
</busconfig>
EOF
systemctl reload dbus.service
```

Retry the Master login after reloading; this policy change does not require
restarting LightDM. The rules grant only root access to these two methods at
this path/interface. The bridge additionally verifies that the sender owns the
helper's bus name. Do not widen the rules to ordinary users or all methods.
After installing a corrected package, the temporary local policy may be
removed; reload the bus configuration again. Remove that local file as well
when uninstalling the integration.

## Verification for developers and packagers

Configure with `WITH_TESTS=ON`, build the test targets and run:

```sh
cmake --build <build-directory> --target LightDmLoginTest LoginProcessIdentityTest SessionFileAccessTest GreeterEnvironmentTest GreeterVncServerTest KWinScreencastTest KWinKeyboardTest PipeWireFrameTest VncProxyConnectionTest
ctest --test-dir <build-directory> -R '^(LightDm(Login|Wrapper)|LoginProcessIdentity|SessionFileAccess|GreeterEnvironment|GreeterVncServer|KWinScreencast|KWinKeyboard|PipeWireFrame|VncProxyConnection)Test$' --output-on-failure
```

These tests cover protocol framing, manual/remote conversation isolation,
authentication failures, cancellation, process identity, authority-file checks
and capture fallback/retries. `GreeterEnvironmentTest` covers display selection
and authority-file checks. With `wayland-server` development files,
`KWinScreencastTest` talks to a fake compositor over a real Wayland socket and
checks logical workspace selection, embedded cursor requests, protocol errors,
closure, negotiation timeout, and local socket peer ownership. Input tests cover
fake-input versions 5 and 6, key/button ownership across clients, disconnect
cleanup, pointer coordinate scaling, and wheel transitions. It never starts a
live compositor or PipeWire daemon. `KWinKeyboardTest` covers symbol mapping,
Shift, Unicode equivalence and rejected keymaps. `PipeWireFrameTest` covers
padded rows, truncated chunks, mapped-memory bounds and oversized frames.
`VncProxyConnectionTest` exercises real loopback sockets and the production
protocol parsers: a late capture listener must serve a frame through the same
external connection after repeated refusals, and an external disconnect must
cancel further connection attempts.
With Python 3 and `dbus-run-session`, `LightDmWrapperTest`
also checks the helper/launcher and ALT/custom wrapper chains with a fake
LightDM daemon and greeter. No live display manager is required. The isolated
bus test does not exercise the installed system bus policy or the complete
service/helper authorization chain.

On a disposable ALT machine with a root-owned installation, verify both GTK
and KDE greeters:

1. Check `loginctl show-session <ID> -p Service -p Class -p Type -p State -p Name`
   for `Service=lightdm-greeter`, confirm bridge registration, and check manual
   login/logout first.
2. Log in local and domain accounts from Master, including a wrong password
   followed by a manual and a remote retry. Change the preselected UI account
   and keyboard layout; remote credentials must be unaffected.
3. Check rejection of overlapping requests, an existing user desktop on the
   same seat and an invalid `UserLoginSession`. Restart the helper during an
   idle greeter and during authentication; check re-registration and recovery.
4. Select Veyon's Wayland desktop backend while using an X11 greeter. Confirm
   `veyon-server --greeter` and `--greeter-vnc x11`, monitoring and remote login.
   Terminate the capture child or use invalid X authority: the headless fallback
   must permit a new connection and login.
5. For a native KWin Wayland greeter, confirm `--greeter-vnc kwin` and the
   `KWin greeter PipeWire stream ready with actual frame` log. Check image updates,
   cursor, fractional scaling and multiple monitors. Check capture first with an
   existing greeter PipeWire instance, then with no instance or session manager.
   Confirm that only a Veyon-started daemon is stopped when capture ends. Deny
   capture permission or remove an output and check the fallback and remote
   LightDM login. Then log in and confirm the ordinary desktop backend starts.
6. Open remote control during a native KWin greeter. Check clicking/focusing the
   username and password fields, typing, Shift/Ctrl/Alt shortcuts, dragging and
   wheel scrolling. Check view-only mode does not move the pointer. Match the
   greeter's layout when using fake-input version 5, then test layout changes
   through the normal greeter UI. Disconnect while holding a key or button;
   verify no stuck input, including with two controlling connections. Remove
   only input permission and confirm video and the LightDM API login still work.
7. Keep Master monitoring and the remote-view window open on a logged-in
   student desktop, reboot the student, and check automatic reconnection to
   the greeter without restarting Master. Then log in and check reconnection
   to the desktop again. Repeat with logout instead of reboot.
8. Check the legacy path on another display manager, then disabling and
   removing the LightDM integration with restoration of the original wrapper.

Verify denial of `RegisterAgent` from ordinary accounts, other processes with
the greeter's UID and a second bridge launched by the greeter UI. An independently
started `veyon-server` must not be authorized to request API login. Interrupt a
pending session start and verify that a late reply cannot start another
account, and that LightDM restores a usable greeter. These installed-system
checks complement the unit tests.
