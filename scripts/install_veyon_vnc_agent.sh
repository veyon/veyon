#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PLIST_SRC="${SCRIPT_DIR}/com.veyon.vnc.plist"
PLIST_DST="/Library/LaunchAgents/com.veyon.vnc.plist"

if [[ ! -f "$PLIST_SRC" ]]; then
  echo "[!] LaunchAgent template not found at $PLIST_SRC" >&2
  exit 1
fi

echo "[*] Installing LaunchAgent to $PLIST_DST"
/usr/bin/sudo /usr/bin/install -m 0644 -o root -g wheel "$PLIST_SRC" "$PLIST_DST"

UID_CONSOLE="$(/usr/bin/stat -f %u /dev/console 2>/dev/null || echo "")"
if [[ -n "$UID_CONSOLE" && "$UID_CONSOLE" != "0" ]]; then
  echo "[*] Bootstrapping for current console user (uid=$UID_CONSOLE)"
  /usr/bin/sudo /bin/launchctl bootout "gui/$UID_CONSOLE" "$PLIST_DST" 2>/dev/null || true
  /usr/bin/sudo /bin/launchctl bootstrap "gui/$UID_CONSOLE" "$PLIST_DST"
  /usr/bin/sudo /bin/launchctl enable "gui/$UID_CONSOLE/com.veyon.vnc"
  /usr/bin/sudo /bin/launchctl kickstart -k "gui/$UID_CONSOLE/com.veyon.vnc"
else
  echo "[!] No console user detected. Agent will start on next user login."
fi

echo "[*] Done."
