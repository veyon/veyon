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

# Method 2: call the DBus PermissionStore directly via busctl (from systemd)
# busctl uses an explicit D-Bus type-signature syntax that avoids the array
# formatting ambiguities that affect some versions of dbus-send.
echo "flatpak CLI not found; using busctl to call DBus PermissionStore directly ..."

if ! command -v busctl >/dev/null 2>&1; then
    echo "ERROR: neither 'flatpak' nor 'busctl' found. Cannot set permission." >&2
    exit 1
fi

# SetPermission signature: sbssas
#   s  = table
#   b  = create (false = do not create the table if it does not exist)
#   s  = id (permission type)
#   s  = app (application identifier)
#   as = data (1-element array: "yes")
busctl call --user \
    org.freedesktop.impl.portal.PermissionStore \
    /org/freedesktop/impl/portal/PermissionStore \
    org.freedesktop.impl.portal.PermissionStore \
    SetPermission \
    sbssas \
    "${TABLE}" false "${PERM_TYPE}" "${APP_ID}" 1 yes

echo "Done."
