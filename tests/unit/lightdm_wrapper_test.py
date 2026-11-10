#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise the installed wrapper's FD inheritance with a fake greeter/daemon.

Run through dbus-run-session; no LightDM, root privileges or desktop is needed.
"""

import json
import os
from pathlib import Path
import select
import socket
import struct
import subprocess
import sys
import tempfile


def number(value):
    return struct.pack(">I", value)


def string(value):
    return number(len(value)) + value


def frame(kind, payload=b""):
    return number(kind) + number(len(payload)) + payload


def receive(fd, count):
    result = bytearray()
    while len(result) < count:
        readable, _, _ = select.select([fd], [], [], 5)
        if not readable:
            raise AssertionError("Timed out waiting for a greeter protocol packet")
        chunk = os.read(fd, count - len(result))
        if not chunk:
            raise AssertionError("Greeter protocol connection closed prematurely")
        result.extend(chunk)
    return bytes(result)


def receive_frame(fd):
    header = receive(fd, 8)
    _, length = struct.unpack(">II", header)
    if length > 1024 * 1024:
        raise AssertionError("Oversized greeter protocol packet")
    return header + receive(fd, length)


def send(fd, payload):
    while payload:
        count = os.write(fd, payload)
        payload = payload[count:]


CONNECT = frame(0, string(b"test-greeter") + number(0) + number(1))
CONNECTED = frame(0, string(b"1.32.0"))
AUTHENTICATE = frame(1, number(1) + string(b"student"))
PROMPT = frame(1, number(1) + string(b"student") + number(1) + number(1) + string(b"Password:"))
RESPONSE = frame(3, number(1) + string(b"test-password"))
AUTHENTICATED = frame(2, number(1) + string(b"student") + number(0))
START_SESSION = frame(4, string(b"plasma"))
SESSION_STARTED = frame(3, number(0))


def fake_greeter():
    assert sys.argv[2:] == ["argument with spaces"]
    assert os.environ.get("TEST_CHAIN_RAN", "") == os.environ.get("TEST_EXPECT_CHAIN", "")
    read_fd = int(os.environ["LIGHTDM_FROM_SERVER_FD"])
    write_fd = int(os.environ["LIGHTDM_TO_SERVER_FD"])
    for fd, device, inode in json.loads(os.environ["TEST_ORIGINAL_FDS"]):
        try:
            stat = os.fstat(fd)
        except OSError:
            continue
        assert (stat.st_dev, stat.st_ino) != (device, inode), "Private daemon FD leaked into the UI"
    send(write_fd, CONNECT)
    assert receive_frame(read_fd) == CONNECTED
    send(write_fd, AUTHENTICATE)
    assert receive_frame(read_fd) == PROMPT
    send(write_fd, RESPONSE)
    assert receive_frame(read_fd) == AUTHENTICATED
    send(write_fd, START_SESSION)
    assert receive_frame(read_fd) == SESSION_STARTED


def test_wrapper(entrypoint, transport, chain=""):
    parent_fds = []
    child_fds = []
    socket_peers = []
    process = None
    try:
        if transport == "socket":
            parent, child = socket.socketpair()
            socket_peers.extend([parent, child])
            read_fd = write_fd = parent.fileno()
            daemon_read = daemon_write = child.fileno()
        else:
            read_fd, daemon_write = os.pipe()
            daemon_read, write_fd = os.pipe()
            parent_fds.extend([read_fd, write_fd])
            child_fds.extend([daemon_read, daemon_write])
        originals = []
        for fd in sorted({daemon_read, daemon_write}):
            stat = os.fstat(fd)
            originals.append([fd, stat.st_dev, stat.st_ino])
        environment = dict(os.environ)
        environment.update(
            LIGHTDM_FROM_SERVER_FD=str(daemon_read),
            LIGHTDM_TO_SERVER_FD=str(daemon_write),
            DBUS_SYSTEM_BUS_ADDRESS=os.environ["DBUS_SESSION_BUS_ADDRESS"],
            TEST_ORIGINAL_FDS=json.dumps(originals),
        )
        environment.pop("LIGHTDM_GREETER_PIPE", None)
        environment.pop("TEST_CHAIN_RAN", None)
        environment["TEST_EXPECT_CHAIN"] = chain
        process = subprocess.Popen(
            [*entrypoint, sys.executable,
             str(Path(__file__).resolve()), "--greeter", "argument with spaces"],
            env=environment,
            pass_fds=sorted({daemon_read, daemon_write}),
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
        )
        if transport == "socket":
            child.close()
        else:
            for fd in child_fds:
                os.close(fd)
            child_fds.clear()
        assert receive_frame(read_fd) == CONNECT
        # Feed one byte at a time to exercise the actual FD transport/parser.
        for byte in CONNECTED:
            send(write_fd, bytes([byte]))
        assert receive_frame(read_fd) == AUTHENTICATE
        send(write_fd, PROMPT)
        assert receive_frame(read_fd) == RESPONSE
        send(write_fd, AUTHENTICATED)
        assert receive_frame(read_fd) == START_SESSION
        send(write_fd, SESSION_STARTED)
        _, stderr = process.communicate(timeout=5)
        assert process.returncode == 0, stderr.decode(errors="replace")
    finally:
        if process is not None and process.poll() is None:
            process.kill()
            process.communicate()
        for fd in parent_fds + child_fds:
            os.close(fd)
        for peer in socket_peers:
            peer.close()


if __name__ == "__main__":
    if len(sys.argv) >= 2 and sys.argv[1] == "--greeter":
        fake_greeter()
    else:
        assert len(sys.argv) == 3, "Pass the helper and installed launcher template paths"
        helper, template = sys.argv[1:]
        for transport in ("socket", "pipes"):
            test_wrapper([helper, "--lightdm-greeter", "--"], transport)
        # LightDM prepends one executable pathname, not a shell command.
        # Execute the real launcher template with only its configured paths
        # changed; spaces in paths/arguments must survive both exec steps.
        with tempfile.TemporaryDirectory(prefix="veyon lightdm launcher ") as directory:
            root = Path(directory)
            bindir, sysconfdir = root / "bin", root / "etc"
            bindir.mkdir()
            sysconfdir.mkdir()
            (bindir / "veyon-login-helper").symlink_to(Path(helper).resolve())
            launcher = root / "veyon-lightdm-greeter"
            launcher.write_text(Path(template).read_text()
                                .replace("@CMAKE_INSTALL_FULL_BINDIR@", str(bindir))
                                .replace("@CMAKE_INSTALL_FULL_SYSCONFDIR@", str(sysconfdir)))
            launcher.chmod(0o755)
            for transport in ("socket", "pipes"):
                test_wrapper([str(launcher)], transport)
            alt_wrapper = sysconfdir / "X11" / "Xgreeter.lightdm"
            alt_wrapper.parent.mkdir()
            alt_wrapper.write_text('#!/bin/sh\nexport TEST_CHAIN_RAN=alt\nexec "$@"\n')
            alt_wrapper.chmod(0o755)
            for transport in ("socket", "pipes"):
                test_wrapper([str(launcher)], transport, "alt")
            custom_wrapper = sysconfdir / "veyon" / "lightdm-greeter-wrapper"
            custom_wrapper.parent.mkdir()
            custom_wrapper.write_text('#!/bin/sh\nexport TEST_CHAIN_RAN=custom\nexec "$@"\n')
            custom_wrapper.chmod(0o755)
            for transport in ("socket", "pipes"):
                test_wrapper([str(launcher)], transport, "custom")
