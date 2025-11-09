#!/bin/bash
# 2c_create-distribution.sh - Create single distribution package of Veyon for macOS

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info()  { printf "${GREEN}[INFO]${NC} %s\n" "$*"; }
log_error() { printf "${RED}[ERROR]${NC} %s\n" "$*"; }

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PACKAGE_DIR="${SCRIPT_DIR}/veyon-macos-package"
DIST_OUTPUT="${SCRIPT_DIR}/veyon-macos-distribution"
DMG_TEMP="${SCRIPT_DIR}/dmg-temp"

if [[ ! -d "$PACKAGE_DIR" ]]; then
    log_error "${PACKAGE_DIR} does not exist. Run 2b_package-apps.sh first."
    exit 1
fi

log_info "=== Creating Veyon distribution package for macOS ==="

rm -rf "$DIST_OUTPUT" "$DMG_TEMP"
mkdir -p "$DIST_OUTPUT" "$DMG_TEMP"

log_info "Creating Veyon folder structure in DMG..."
mkdir -p "$DMG_TEMP/Veyon"

log_info "Copying applications to Veyon folder..."
cp -R "$PACKAGE_DIR/veyon-configurator.app" "$DMG_TEMP/Veyon/"
cp -R "$PACKAGE_DIR/veyon-master.app" "$DMG_TEMP/Veyon/"
cp -R "$PACKAGE_DIR/veyon-server.app" "$DMG_TEMP/Veyon/"

if [[ -f "$PACKAGE_DIR/README.txt" ]]; then
    cp "$PACKAGE_DIR/README.txt" "$DMG_TEMP/Veyon/"
fi

log_info "Creating symbolic link to Applications..."
ln -s /Applications "$DMG_TEMP/Applications"

log_info "Creating DMG image..."
hdiutil create -volname "Veyon macOS" -srcfolder "$DMG_TEMP" -ov -format UDZO \
    "$DIST_OUTPUT/Veyon-macOS.dmg"

rm -rf "$DMG_TEMP"

DMG_SIZE=$(du -sh "$DIST_OUTPUT/Veyon-macOS.dmg" | cut -f1)

log_info ""
log_info "✓ Distribution completed"
log_info ""
log_info "File created:"
log_info "  Veyon-macOS.dmg (${DMG_SIZE})"
log_info ""
log_info "Location: ${DIST_OUTPUT}/Veyon-macOS.dmg"
log_info ""
log_info "DISTRIBUTION INSTRUCTIONS:"
log_info "  1. Distribute the Veyon-macOS.dmg file"
log_info "  2. Users must mount the DMG (double click)"
log_info "  3. Drag the 'Veyon' folder to the 'Applications' shortcut"
log_info "  4. The entire Veyon folder will be installed in /Applications/Veyon/"
log_info ""
log_info "DMG Structure:"
log_info "  - Veyon/ (folder with all apps and README)"
log_info "  - Applications (shortcut for easy drag-and-drop installation)"
log_info ""
log_info "IMPORTANT: DO NOT copy .app files directly from Finder,"
log_info "always use the DMG for distribution."
