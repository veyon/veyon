#!/bin/bash
# 2b_install-dylibs-to-bundles.sh - Copy dylibs to app bundles after install

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info()  { printf "${GREEN}[INFO]${NC} %s\n" "$*"; }
log_step()  { printf "${BLUE}[STEP]${NC} %s\n" "$*"; }
log_error() { printf "${RED}[ERROR]${NC} %s\n" "$*"; }

# ============================================================================
# B) COPY DYLIBS TO BUNDLES
# ============================================================================

printf "\n"
printf "==========================================\n"
printf "          B) COPY DYLIBS TO BUNDLES\n"
printf "==========================================\n"
printf "\n"

DIST_DIR="${1:-dist}"
echo "Installing dylibs to app bundles in $DIST_DIR..."

# Copy dylibs to each app bundle
for app in veyon-master veyon-configurator veyon-server; do
	app_path="$DIST_DIR/Applications/Veyon/${app}.app"
	if [[ -d "$app_path" ]]; then
		echo "  Installing dylibs to ${app}.app..."
		mkdir -p "$app_path/Contents/lib/veyon"
		cp -f "$DIST_DIR/lib/veyon"/*.dylib "$app_path/Contents/lib/veyon/" 2>/dev/null || true
		echo "  ✓ ${app}.app"
	fi
done

log_info ""
log_info "✓ Copy dylibs to bundles completed"
log_info ""
log_info ""
