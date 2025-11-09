#!/bin/bash
# 2_build-package-distribution.sh - Complete script: build, package and create distribution DMG
# This is the main script that orchestrates the entire build and packaging process

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

log_info "Building..."
cmake --build build --parallel

log_info "Installing to dist/..."
cmake --build build --target install

log_info "Copying dylibs to app bundles..."
if [[ -f "${SCRIPT_DIR}/2a_install-dylibs-to-bundles.sh" ]]; then
    "${SCRIPT_DIR}/2a_install-dylibs-to-bundles.sh" "${SCRIPT_DIR}/dist"
else
    log_error "2a_install-dylibs-to-bundles.sh not found"
    exit 1
fi

log_info ""
log_info "✓ Build completed"
log_info ""

# ============================================================================
# STEP 2: PACKAGING
# ============================================================================

log_step "=== STEP 2: Packaging applications ==="
log_info ""

if [[ -f "${SCRIPT_DIR}/2b_package-apps.sh" ]]; then
    "${SCRIPT_DIR}/2b_package-apps.sh"
else
    log_error "2b_package-apps.sh not found"
    exit 1
fi

log_info ""
log_info "✓ Packaging completed"
log_info ""

# ============================================================================
# STEP 3: CREATE DISTRIBUTION DMG
# ============================================================================

log_step "=== STEP 3: Creating distribution DMG ==="
log_info ""

if [[ -f "${SCRIPT_DIR}/2c_create-distribution.sh" ]]; then
    "${SCRIPT_DIR}/2c_create-distribution.sh"
else
    log_error "2c_create-distribution.sh not found"
    exit 1
fi

log_info ""
log_info "=========================================="
log_info "=== COMPLETE PROCESS FINISHED ✓ ==="
log_info "=========================================="
log_info ""
log_info "Distribution DMG ready at:"
log_info "  ${SCRIPT_DIR}/veyon-macos-distribution/Veyon-macOS.dmg"
log_info ""
log_info "This DMG contains:"
log_info "  - veyon-configurator.app"
log_info "  - veyon-master.app"
log_info "  - veyon-server.app"
log_info "  - README.txt with instructions"
log_info ""
log_info "Ready to distribute!"
log_info ""
