#!/bin/bash
# 1_configure-cmake.sh - CMake configuration for Veyon macOS
# This script saves the correct configuration to be able to recreate the build directory

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info()  { printf "${GREEN}[INFO]${NC} %s\n" "$*"; }
log_error() { printf "${RED}[ERROR]${NC} %s\n" "$*"; }
log_warn()  { printf "${YELLOW}[WARN]${NC} %s\n" "$*"; }

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

log_info "=== Configuring CMake for Veyon macOS ==="
log_info ""

# Check if build exists and warn
if [[ -d "${SCRIPT_DIR}/build" ]]; then
    log_warn "The build/ directory already exists."
    read -p "Do you want to remove it and reconfigure? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        log_info "Removing build/..."
        rm -rf "${SCRIPT_DIR}/build"
    else
        log_info "Keeping existing build/. Aborting."
        exit 0
    fi
fi

log_info "Configuring CMake with Qt5..."
log_info ""

# CMake configuration with all necessary parameters
cmake -S . -B build \
  -DWITH_QT6=OFF \
  -DCMAKE_PREFIX_PATH="/usr/local/opt/qt@5/lib/cmake;/usr/local/opt/qthttpserver/lib/cmake" \
  -DLdap_INCLUDE_DIRS="/usr/local/opt/openldap/include" \
  -DLdap_LIBRARIES="/usr/local/opt/openldap/lib/libldap.dylib;/usr/local/opt/openldap/lib/liblber.dylib" \
  -DCMAKE_INSTALL_PREFIX="$PWD/dist" \
  -DCMAKE_BUILD_TYPE=Release \
  -DVEYON_BUILD_LINUX=OFF \
  -DVEYON_BUILD_WIN32=OFF \
  -DVEYON_BUILD_MACOS=ON

log_info ""
log_info "✓ Configuration completed"
log_info ""
log_info "Now you can build with:"
log_info "  cmake --build build --parallel"
log_info ""
log_info "Or use the complete script:"
log_info "  ./2_build-package-distribution.sh"
log_info ""
