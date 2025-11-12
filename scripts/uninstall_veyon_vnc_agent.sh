#!/usr/bin/env bash
set -euo pipefail

PLIST_DST="/Library/LaunchAgents/com.veyon.vnc.plist"

# Note: This script is run with administrator privileges via osascript
# so we don't need to use sudo

UID_CONSOLE="$(/usr/bin/stat -f %u /dev/console 2>/dev/null || echo "")"
if [[ -n "$UID_CONSOLE" && "$UID_CONSOLE" != "0" ]]; then
  echo "[*] Booting out agent for uid=$UID_CONSOLE"
  /bin/launchctl bootout "gui/$UID_CONSOLE" "$PLIST_DST" 2>/dev/null || true
fi

if [[ -f "$PLIST_DST" ]]; then
  echo "[*] Removing $PLIST_DST"
  /bin/rm -f "$PLIST_DST"
fi

echo "[*] Uninstalled."
