#!/bin/bash
# install-dylibs-to-bundles.sh - Copia dylibs a los app bundles después del install

set -e

DIST_DIR="${1:-dist}"
echo "Installing dylibs to app bundles in $DIST_DIR..."

# Copiar dylibs a cada app bundle
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
