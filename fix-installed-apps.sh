#!/bin/bash

echo "=== Fixing RPATH in installed Veyon apps ==="
echo ""

APP_DIR="/Applications/veyon-configurator.app"

if [ ! -d "$APP_DIR" ]; then
    echo "Error: $APP_DIR not found"
    exit 1
fi

echo "Fixing RPATHs in $APP_DIR..."

# Fix all binaries in MacOS/
for binary in "$APP_DIR/Contents/MacOS"/*; do
    if [ -f "$binary" ] && file "$binary" | grep -q "Mach-O"; then
        echo "  Fixing $(basename $binary)..."

        # Remove build directory RPATH
        install_name_tool -delete_rpath "/Users/pablo/GitHub/veyon/build/core" "$binary" 2>/dev/null || true
        install_name_tool -delete_rpath "/Users/pablo/GitHub/veyon/build/plugins" "$binary" 2>/dev/null || true

        # Add correct RPATH
        install_name_tool -add_rpath "@executable_path/../Frameworks" "$binary" 2>/dev/null || true
        install_name_tool -add_rpath "@executable_path/../lib/veyon" "$binary" 2>/dev/null || true
    fi
done

# Do the same for veyon-master.app
APP_DIR="/Applications/veyon-master.app"

if [ -d "$APP_DIR" ]; then
    echo ""
    echo "Fixing RPATHs in $APP_DIR..."

    for binary in "$APP_DIR/Contents/MacOS"/*; do
        if [ -f "$binary" ] && file "$binary" | grep -q "Mach-O"; then
            echo "  Fixing $(basename $binary)..."

            install_name_tool -delete_rpath "/Users/pablo/GitHub/veyon/build/core" "$binary" 2>/dev/null || true
            install_name_tool -delete_rpath "/Users/pablo/GitHub/veyon/build/plugins" "$binary" 2>/dev/null || true

            install_name_tool -add_rpath "@executable_path/../Frameworks" "$binary" 2>/dev/null || true
            install_name_tool -add_rpath "@executable_path/../lib/veyon" "$binary" 2>/dev/null || true
        fi
    done
fi

echo ""
echo "=== Done! ==="
echo ""
echo "Now try running:"
echo "  /Applications/veyon-configurator.app/Contents/MacOS/veyon-service"
