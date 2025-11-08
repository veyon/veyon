#!/bin/bash
# Test veyon-server directly from build directory

set -e

BUILD_DIR="/Users/pablo/GitHub/veyon/build"
SERVER_APP="$BUILD_DIR/server/veyon-server.app"
SERVER_BIN="$SERVER_APP/Contents/MacOS/veyon-server"

echo "=== Testing veyon-server directly from build ==="
echo ""

# Check if server exists
if [ ! -f "$SERVER_BIN" ]; then
    echo "Error: Server not found at $SERVER_BIN"
    exit 1
fi

echo "Server found: $SERVER_BIN"
echo ""

# Kill any running instances
pkill -9 veyon-server 2>/dev/null || true
pkill -9 veyon-worker 2>/dev/null || true
sleep 1

echo "Starting server..."
echo ""

# Run server with full output
cd "$BUILD_DIR/server"
"$SERVER_BIN" 2>&1 | head -200
