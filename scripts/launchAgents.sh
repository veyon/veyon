#!/usr/bin/env bash
#
# launchAgents.sh - Interactive menu for managing Veyon VNC LaunchAgent
#
# This script provides an interactive interface to install, uninstall, and
# check the status of the Veyon VNC LaunchAgent on macOS.
#
# IMPORTANT: Do NOT run this script with sudo. It will request administrator
# privileges when needed for specific operations.
#
# Menu Options:
#   1) Install globally (admin) - Installs LaunchAgent system-wide in /Library/LaunchAgents/
#   2) Install current user     - Installs LaunchAgent for current user in ~/Library/LaunchAgents/
#   3) Check plist file         - Shows if plist files exist and their details
#   4) Check runtime status     - Shows if the service is loaded and running
#   5) Uninstall               - Removes LaunchAgent (global and/or user)
#   0) Exit                    - Exits the script
#

set -euo pipefail

# Security check: Ensure script is not run as root
if [[ "$(id -u)" -eq 0 ]]; then
  echo "[!] Run this helper without sudo/root. Admin actions will request elevation when needed."
  exit 1
fi

# Path configuration
SOURCE_DIR="/Applications/Veyon/veyon-configurator.app/Contents/Resources/Scripts"
PLIST_NAME="com.veyon.vnc.plist"
PLIST="${SOURCE_DIR}/${PLIST_NAME}"
INSTALLER="${SOURCE_DIR}/install_veyon_vnc_agent.sh"
GLOBAL_PLIST="/Library/LaunchAgents/${PLIST_NAME}"
USER_PLIST="${HOME}/Library/LaunchAgents/${PLIST_NAME}"

# ============================================================================
# UTILITY FUNCTIONS
# ============================================================================

# ensure_sources - Verify that required source files exist
# Checks for the plist file and installer script in the Veyon Configurator
ensure_sources() {
  if [[ ! -f "$PLIST" || ! -x "$INSTALLER" ]]; then
    echo "[!] Could not find the original plist/installer in $SOURCE_DIR" >&2
    exit 1
  fi
}

# is_admin - Check if current user is a member of the admin group
# Returns 0 if user is admin, 1 otherwise
is_admin() {
  /usr/sbin/dseditgroup -o checkmember -m "$(id -un)" admin >/dev/null 2>&1
}

# ============================================================================
# MENU OPTION 1: Install globally (admin)
# ============================================================================

# run_admin - Install LaunchAgent system-wide for all users
# This installs the plist to /Library/LaunchAgents/ (requires sudo)
# The agent will be available for all users on the system
run_admin() {
  ensure_sources
  echo "[*] Running global installer via sudo..."
  /usr/bin/sudo "$INSTALLER"
}

# ============================================================================
# MENU OPTION 2: Install current user
# ============================================================================

# run_user - Install LaunchAgent for the current user only
# This installs the plist to ~/Library/LaunchAgents/ (no sudo required)
# Steps:
#   1. Create LaunchAgents directory if needed
#   2. Copy plist file to user's LaunchAgents
#   3. Unload any existing instance (bootout)
#   4. Load the new agent (bootstrap)
#   5. Enable and start the service (kickstart)
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

# ============================================================================
# MENU OPTION 3: Check plist file
# ============================================================================

# show_plist_state - Display status of plist files
# Checks both global and user locations and shows:
#   - Whether the plist file exists
#   - File details (permissions, size, date)
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

# ============================================================================
# MENU OPTION 4: Check runtime status
# ============================================================================

# show_service_state - Display runtime status of the LaunchAgent
# Shows if the service is loaded and running in launchctl
# Handles both normal user context and root context (inspecting console user)
# Displays:
#   - Service listing from launchctl (PID, status, label)
#   - Detailed service info from launchctl print
show_service_state() {
  current_uid="$(id -u)"
  target_uid="$current_uid"

  # If running as root, find the console user to inspect their GUI session
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

  # Query launchctl for the service status
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

  # Print detailed service information
  echo
  /bin/launchctl print "gui/${target_uid}/com.veyon.vnc" || echo "launchctl print failed (service not loaded?)"
}

# ============================================================================
# MENU OPTION 5: Uninstall
# ============================================================================

# uninstall_global - Remove global LaunchAgent (requires sudo)
# Steps:
#   1. Check if global plist exists
#   2. Unload from console user's GUI session (bootout)
#   3. Remove the plist file from /Library/LaunchAgents/
uninstall_global() {
  if [[ ! -f "$GLOBAL_PLIST" ]]; then
    echo "[*] No global plist found."
    return 0
  fi

  # Get console user UID to unload from their session
  console_uid="$(/usr/bin/stat -f %u /dev/console 2>/dev/null || echo "")"
  if [[ -n "$console_uid" && "$console_uid" != "0" ]]; then
    /usr/bin/sudo /bin/launchctl bootout "gui/${console_uid}" "$GLOBAL_PLIST" 2>/dev/null || true
  fi

  # Remove global plist file
  /usr/bin/sudo /bin/rm -f "$GLOBAL_PLIST"
  echo "[*] Removed global plist."
}

# uninstall_user - Remove user LaunchAgent (no sudo required)
# Steps:
#   1. Check if user plist exists
#   2. Unload from current user's GUI session (bootout)
#   3. Remove the plist file from ~/Library/LaunchAgents/
uninstall_user() {
  if [[ ! -f "$USER_PLIST" ]]; then
    echo "[*] No user plist found."
    return 0
  fi

  # Unload from current user's session
  /bin/launchctl bootout "gui/$(id -u)" "$USER_PLIST" 2>/dev/null || true

  # Remove user plist file
  /bin/rm -f "$USER_PLIST"
  echo "[*] Removed user plist."
}

# run_uninstall - Main uninstall function
# Removes both global and user plists if user is admin
# Otherwise only removes user plist
run_uninstall() {
  if is_admin; then
    uninstall_global
  else
    echo "[!] You are not an admin; skipping global removal."
  fi
  uninstall_user
  echo "[*] Uninstall finished."
}

# ============================================================================
# INTERACTIVE MENU LOOP
# ============================================================================

# Main interactive menu loop
# Displays menu options and processes user input
# After each action, shows a submenu to return to main menu or exit
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

  # Process main menu selection
  case "$choice" in
    1) run_admin ;;              # Install system-wide (requires sudo)
    2) run_user ;;               # Install for current user only
    3) show_plist_state ;;       # Show plist file status
    4) show_service_state ;;     # Show runtime status
    5) run_uninstall ;;          # Uninstall LaunchAgent
    0) echo "Bye!"; echo; break ;;  # Exit script
    *) echo "Cancelled." ;;      # Invalid option
  esac

  # Show post-action menu
  echo
  echo "----------------------------------"
  echo
  echo "  Enter) Main menu"
  echo "  0) Exit"
  echo
  read -rp "Select an option: " choice2
  echo

  # Process post-action menu selection
  case "$choice2" in
    0) echo "Bye!"; echo; break ;;  # Exit script
  esac
  # Any other input (including Enter) returns to main menu
done
