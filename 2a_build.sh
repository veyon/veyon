#!/bin/bash
# 2a_build.sh - Build Veyon for macOS

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info()  { printf "${GREEN}[INFO]${NC} %s\n" "$*"; }
log_step()  { printf "${BLUE}[STEP]${NC} %s\n" "$*"; }
log_error() { printf "${RED}[ERROR]${NC} %s\n" "$*"; }

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# ============================================================================
# STEP 1: BUILD
# ============================================================================

log_step "=== STEP 1: Building Veyon ==="
log_info ""

if [[ ! -d "${SCRIPT_DIR}/build" ]]; then
    log_info "Configuring cmake for the first time..."
    cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${SCRIPT_DIR}/dist" \
        -DCMAKE_PREFIX_PATH=/usr/local/opt/qt@5 \
        -DVEYON_BUILD_LINUX=OFF \
        -DVEYON_BUILD_WIN32=OFF \
        -DVEYON_BUILD_MACOS=ON
else
    log_info "Build directory exists, skipping cmake configuration"
fi

# Force rebuild of macvnc if it's a submodule with uncommitted changes
if [[ -d "${SCRIPT_DIR}/3rdparty/macvnc/.git" ]]; then
    cd "${SCRIPT_DIR}/3rdparty/macvnc"
    if git diff-index --quiet HEAD -- 2>/dev/null; then
        log_info "macvnc submodule has no changes"
    else
        log_info "macvnc submodule has uncommitted changes - forcing rebuild"
        rm -f "${SCRIPT_DIR}/build/3rdparty/macvnc/libmacvnc.a"
        rm -f "${SCRIPT_DIR}/build/3rdparty/macvnc/CMakeFiles/macvnc.dir/src/"*.o
        log_info "Cleaned macvnc build artifacts"
    fi
    cd "${SCRIPT_DIR}"
fi

log_info "Building..."
cmake --build build --parallel

log_info "Installing to dist/..."
cmake --build build --target install