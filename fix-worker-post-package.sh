#!/bin/bash
# Script to fix veyon-worker after packaging

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PACKAGE_DIR="$SCRIPT_DIR/veyon-macos-package"
BUILD_WORKER="$SCRIPT_DIR/build/worker/veyon-worker"
DIST_WORKER="$SCRIPT_DIR/dist/bin/veyon-worker"
SERVER_APP="$PACKAGE_DIR/veyon-server.app"
WORKER_DEST="$SERVER_APP/Contents/MacOS/veyon-worker"

echo "=== Fixing veyon-worker for veyon-server.app ==="

# Determine worker source (prefer BUILD, fallback to DIST)
WORKER_SOURCE=""
if [[ -f "$BUILD_WORKER" ]]; then
    WORKER_SOURCE="$BUILD_WORKER"
    echo "✓ Using veyon-worker from BUILD directory (most recent)"
elif [[ -f "$DIST_WORKER" ]]; then
    WORKER_SOURCE="$DIST_WORKER"
    echo "⚠ Using veyon-worker from DIST directory (may be old)"
else
    echo "❌ Error: veyon-worker not found in BUILD or DIST"
    exit 1
fi

# Copy worker to server bundle
echo "Copying worker to server bundle..."
cp "$WORKER_SOURCE" "$WORKER_DEST"
chmod +x "$WORKER_DEST"

# Fix Qt framework paths
echo "Fixing Qt framework paths..."
for qt_fw in QtConcurrent QtNetwork QtWidgets QtGui QtCore; do
  install_name_tool -change \
    "/usr/local/opt/qt@5/lib/$qt_fw.framework/Versions/5/$qt_fw" \
    "@rpath/$qt_fw.framework/Versions/5/$qt_fw" \
    "$WORKER_DEST"
done

# Fix QCA framework path
echo "Fixing QCA framework path..."
install_name_tool -change \
  "/usr/local/lib/qca-qt5.framework/Versions/2/qca-qt5" \
  "@rpath/qca-qt5.framework/Versions/2/qca-qt5" \
  "$WORKER_DEST"

# Fix OpenSSL paths
echo "Fixing OpenSSL paths..."
install_name_tool -change \
  "/opt/local/libexec/openssl3/lib/libssl.3.dylib" \
  "@rpath/openssl/lib/libssl.3.dylib" \
  "$WORKER_DEST"

install_name_tool -change \
  "/opt/local/libexec/openssl3/lib/libcrypto.3.dylib" \
  "@rpath/openssl/lib/libcrypto.3.dylib" \
  "$WORKER_DEST"

# Add rpath for Frameworks directory
echo "Adding rpath..."
install_name_tool -add_rpath "@executable_path/../Frameworks" "$WORKER_DEST" 2>/dev/null || true

# Verify
echo ""
echo "=== Verification ==="
echo "Worker dependencies:"
otool -L "$WORKER_DEST" | grep -E "@rpath|/usr/local" | head -10

echo ""
echo "=== Testing worker launch ==="
"$WORKER_DEST" --help 2>&1 | head -3 || echo "Worker test completed (expected to show usage or error)"

echo ""
echo "✅ Worker fix completed!"
