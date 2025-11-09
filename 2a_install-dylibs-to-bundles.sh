#!/bin/bash
# 2a_install-dylibs-to-bundles.sh - Copy dylibs to app bundles after install

set -e

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

echo "Done!"
