#!/bin/bash
# Quick server test with debug output

echo "=== Quick VNC Server Test ==="

# Kill existing
pkill -9 veyon-server
pkill -9 veyon-worker
sleep 2

# Fix worker
./fix-worker-post-package.sh > /dev/null 2>&1

# Launch server in background with log capture
cd veyon-macos-package
./veyon-server.app/Contents/MacOS/veyon-server > /tmp/veyon-test.log 2>&1 &
SERVER_PID=$!

echo "Server PID: $SERVER_PID"
echo "Waiting 5 seconds for startup..."
sleep 5

# Check if running
if ps -p $SERVER_PID > /dev/null; then
    echo "✅ Server is running"

    # Show key logs
    echo ""
    echo "=== [PUMP] Logs (should appear every ~1 second) ==="
    grep "\[PUMP\]" /tmp/veyon-test.log | tail -5

    echo ""
    echo "=== ScreenCapturer Logs ==="
    grep -E "\[DEBUG\] ScreenCapturer" /tmp/veyon-test.log | tail -10

    echo ""
    echo "=== Frame Capture Logs ==="
    grep -E "didOutput|frameHandler" /tmp/veyon-test.log | tail -10 || echo "NO FRAME LOGS FOUND"

    echo ""
    echo "=== Port Status ==="
    lsof -nP -i4TCP:11200 | grep LISTEN || echo "Port 11200 not listening"

    echo ""
    echo "Tail logs with: tail -f /tmp/veyon-test.log"
else
    echo "❌ Server failed to start!"
    echo "Last 20 lines of log:"
    tail -20 /tmp/veyon-test.log
fi
