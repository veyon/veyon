# Greeter capture protocol bindings

`zkde-screencast-unstable-v1.xml` is the version-3 wire-compatible subset of
[KDE plasma-wayland-protocols](https://invent.kde.org/libraries/plasma-wayland-protocols/-/blob/master/src/protocols/zkde-screencast-unstable-v1.xml).
Requests and events added after version 3 are omitted; the client always binds
version 3, including when talking to newer KWin. This includes `stream_region`
and the `created` PipeWire node ID event supported by KWin 6.1.

`xdg-output-unstable-v1.xml` comes from
[wayland-protocols](https://gitlab.freedesktop.org/wayland/wayland-protocols/-/blob/main/unstable/xdg-output/xdg-output-unstable-v1.xml).
The client binds version 2, whose `done` event delimits logical output geometry.
This handles fractional scaling without deriving logical size from pixel size.

Copyright and license notices are retained in each XML file. Bindings are
produced by the build host's `wayland-scanner`; no generated C is checked in.

`fake-input.xml` comes from
[KDE plasma-wayland-protocols](https://invent.kde.org/libraries/plasma-wayland-protocols/-/blob/master/src/protocols/fake-input.xml).
The client binds version 5 on KWin 6.1 and version 6 where advertised. Version 5
uses evdev keycodes and a keymap read through `wl_keyboard`; version 6 adds native
keysyms. The interface must be declared in the installed server desktop file.

`keystate.xml` comes from
[KDE plasma-wayland-protocols](https://invent.kde.org/libraries/plasma-wayland-protocols/-/blob/master/src/protocols/keystate.xml).
The client binds version 1 and requests the initial lock state with
`fetchStates`, then follows `stateChanged` events. This provides CapsLock state
without a focused surface; ordinary `wl_keyboard.modifiers` events are sent
only to a focused client. Version 1 has no destructor request, so its proxy is
destroyed locally when the separate input connection is closed.
