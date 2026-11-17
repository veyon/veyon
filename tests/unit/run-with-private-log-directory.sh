#!/bin/sh
# SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Runs a test that creates a VeyonCore with logging. As root, Veyon writes its log files to
# /var/log/veyon (WP23), so the test then runs in a private mount namespace with a tmpfs on
# /var/log and never touches the real one (exit code 77, i.e. skipped, if that's not possible).

if [ "$(id -u)" != 0 ] ; then
	exec "$@"
fi

# root without CAP_SYS_ADMIN (e.g. in a container) can't create a mount namespace
unshare --mount true 2>/dev/null || exit 77

exec unshare --mount sh -c 'mount -t tmpfs tmpfs /var/log || exit 77; exec "$@"' sh "$@"
