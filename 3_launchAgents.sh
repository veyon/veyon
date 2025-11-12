#!/usr/bin/env bash
set -euo pipefail

if [[ "$(id -u)" -eq 0 ]]; then
  echo "[!] Run this helper without sudo/root. Admin actions will request elevation when needed."
  exit 1
fi

SOURCE_DIR="/Applications/Veyon/veyon-configurator.app/Contents/Resources/Scripts"
PLIST_NAME="com.veyon.vnc.plist"
PLIST="${SOURCE_DIR}/${PLIST_NAME}"
INSTALLER="${SOURCE_DIR}/install_veyon_vnc_agent.sh"
GLOBAL_PLIST="/Library/LaunchAgents/${PLIST_NAME}"
USER_PLIST="${HOME}/Library/LaunchAgents/${PLIST_NAME}"

ensure_sources() {
  if [[ ! -f "$PLIST" || ! -x "$INSTALLER" ]]; then
    echo "[!] Could not find the original plist/installer in $SOURCE_DIR" >&2
    exit 1
  fi
}

is_admin() {
  /usr/sbin/dseditgroup -o checkmember -m "$(id -un)" admin >/dev/null 2>&1
}

run_admin() {
  ensure_sources
  echo "[*] Running global installer via sudo..."
  /usr/bin/sudo "$INSTALLER"
}

run_user() {
  ensure_sources
  mkdir -p "${HOME}/Library/LaunchAgents"
  /bin/cp "$PLIST" "$USER_PLIST"
  echo "[*] Copied plist to ${USER_PLIST}"
  uid="$(id -u)"
  /bin/launchctl bootout "gui/${uid}" "$USER_PLIST" 2>/dev/null || true
  /bin/launchctl bootstrap "gui/${uid}" "$USER_PLIST"
  /bin/launchctl enable "gui/${uid}/com.veyon.vnc"
  /bin/launchctl kickstart -k "gui/${uid}/com.veyon.vnc"
  echo "[*] LaunchAgent loaded for UID ${uid}"
}

show_plist_state() {
  echo "--- LaunchAgent plist state ---"
  if [[ -f "$GLOBAL_PLIST" ]]; then
    echo "Global plist present at $GLOBAL_PLIST"
    /bin/ls -l "$GLOBAL_PLIST"
  else
    echo "Global plist missing."
  fi
  if [[ -f "$USER_PLIST" ]]; then
    echo "User plist present at $USER_PLIST"
    /bin/ls -l "$USER_PLIST"
  else
    echo "User plist missing."
  fi
}

show_service_state() {
  current_uid="$(id -u)"
  target_uid="$current_uid"
  if [[ "$current_uid" == "0" ]]; then
    console_uid="$(/usr/bin/stat -f %u /dev/console 2>/dev/null || echo "")"
    if [[ -n "$console_uid" && "$console_uid" != "0" ]]; then
      target_uid="$console_uid"
      echo "[*] Running as root: inspecting GUI user $target_uid"
    else
      echo "[!] Running as root but no GUI user detected; nothing to inspect."
      return
    fi
  fi

  echo "--- launchctl state (uid ${target_uid}) ---"
  if [[ "$current_uid" == "$target_uid" ]]; then
    service_line="$(/bin/launchctl list | /usr/bin/grep -F com.veyon.vnc || true)"
  else
    service_line="$(/bin/launchctl asuser "$target_uid" /bin/launchctl list | /usr/bin/grep -F com.veyon.vnc || true)"
  fi

  if [[ -n "$service_line" ]]; then
    echo "$service_line"
  else
    echo "Service not listed."
  fi

  echo
  /bin/launchctl print "gui/${target_uid}/com.veyon.vnc" || echo "launchctl print failed (service not loaded?)"
}

uninstall_global() {
  if [[ ! -f "$GLOBAL_PLIST" ]]; then
    echo "[*] No global plist found."
    return 0
  fi
  console_uid="$(/usr/bin/stat -f %u /dev/console 2>/dev/null || echo "")"
  if [[ -n "$console_uid" && "$console_uid" != "0" ]]; then
    /usr/bin/sudo /bin/launchctl bootout "gui/${console_uid}" "$GLOBAL_PLIST" 2>/dev/null || true
  fi
  /usr/bin/sudo /bin/rm -f "$GLOBAL_PLIST"
  echo "[*] Removed global plist."
}

uninstall_user() {
  if [[ ! -f "$USER_PLIST" ]]; then
    echo "[*] No user plist found."
    return 0
  fi
  /bin/launchctl bootout "gui/$(id -u)" "$USER_PLIST" 2>/dev/null || true
  /bin/rm -f "$USER_PLIST"
  echo "[*] Removed user plist."
}

run_uninstall() {
  if is_admin; then
    uninstall_global
  else
    echo "[!] You are not an admin; skipping global removal."
  fi
  uninstall_user
  echo "[*] Uninstall finished."
}

while true; do
  clear
  echo "----------------------------------"
  echo "       Veyon agents helper"
  echo "----------------------------------"
  echo ""
  echo "  1) Install globally (admin)"
  echo "  2) Install current user"
  echo "  3) Check plist file"
  echo "  4) Check runtime status"
  echo "  5) Uninstall"
  echo "  0) Exit"
  echo ""
  read -rp "Select an option: " choice
  echo

  case "$choice" in
    1) run_admin ;;
    2) run_user ;;
    3) show_plist_state ;;
    4) show_service_state ;;
    5) run_uninstall ;;
    0) echo "Bye!"; echo; break ;;
    *) echo "Cancelled." ;;
  esac

  echo
  echo "----------------------------------"
  echo
  echo "  Enter) Main menu"
  echo "  0) Exit"
  echo
  read -rp "Select an option: " choice2
  echo

  case "$choice2" in
    0) echo "Bye!"; echo; break ;;
  esac
done
