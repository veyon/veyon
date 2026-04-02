#!/bin/sh
# veyon-preauth-kde.sh – Pre-authorize Veyon Server for unattended KDE Plasma screen access
#
# Copyright (c) 2024-2026 Tobias Junghans <tobydox@veyon.io>
#
# This file is part of Veyon - https://veyon.io
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Usage:
#   Run this script as the *target user* (the user whose desktop will be captured),
#   or from a post-install hook that has access to that user's session bus.
#
#   sudo -u <target-user> DBUS_SESSION_BUS_ADDRESS="$(cat /run/user/<uid>/bus)" \
#       /usr/share/veyon/veyon-preauth-kde.sh
#
# Requirements:
#   - KDE Plasma 6.3 or later
#   - xdg-desktop-portal-kde installed and running
#   - Either 'flatpak' CLI or direct DBus access to the PermissionStore
#
# What this script does:
#   Grants "io.veyon.Veyon.Server" permanent permission for RemoteDesktop portal
#   access under KDE.  After running this script the Veyon Server daemon will be
#   able to capture the screen and receive input without any interactive portal
#   consent dialog.
#
# Stable app_id used: io.veyon.Veyon.Server
# Permission table  : kde-authorized
# Permission type   : screencast

APP_ID="io.veyon.Veyon.Server"
TABLE="kde-authorized"
# KDE Plasma's PermissionStore uses "screencast" (not "remote-desktop") as the
# permission ID for the kde-authorized screen-capture pre-authorization table.
PERM_TYPE="screencast"

set -e

echo "Veyon KDE pre-authorization script"
echo "  App ID : ${APP_ID}"
echo "  Table  : ${TABLE}"
echo "  Type   : ${PERM_TYPE}"
echo ""

# Method 1: use the flatpak CLI if available (simplest)
if command -v flatpak >/dev/null 2>&1; then
    echo "Using 'flatpak permission-set' ..."
    flatpak permission-set "${TABLE}" "${PERM_TYPE}" "${APP_ID}" yes
    echo "Done."
    exit 0
fi

# Method 2: call the DBus PermissionStore directly
echo "flatpak CLI not found; using DBus PermissionStore directly ..."

if ! command -v dbus-send >/dev/null 2>&1; then
    echo "ERROR: neither 'flatpak' nor 'dbus-send' found. Cannot set permission." >&2
    exit 1
fi

dbus-send \
    --session \
    --print-reply \
    --dest=org.freedesktop.impl.portal.PermissionStore \
    /org/freedesktop/impl/portal/PermissionStore \
    org.freedesktop.impl.portal.PermissionStore.SetPermission \
    "string:${TABLE}" \
    "boolean:false" \
    "string:${PERM_TYPE}" \
    "string:${APP_ID}" \
    "array:string:yes"

echo "Done."
