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
# A) BUILD
# ============================================================================
printf "\n"
printf "==========================================\n"
printf "          A) BUILD\n"
printf "==========================================\n"
printf "\n"

log_info "Copying dylibs to app bundles..."
if [[ -f "${SCRIPT_DIR}/2a_build.sh" ]]; then
    "${SCRIPT_DIR}/2a_build.sh" "${SCRIPT_DIR}/dist"
else
    log_error "2a_build.sh not found"
    exit 1
fi

log_info ""
log_info "✓ Building completed"
log_info ""

# ============================================================================
# B) COPY DYLIBS TO BUNDLES
# ============================================================================
printf "\n"
printf "==========================================\n"
printf "          B) COPY DYLIBS TO BUNDLES\n"
printf "==========================================\n"
printf "\n"


log_info "Copying dylibs to app bundles..."
if [[ -f "${SCRIPT_DIR}/2b_install-dylibs-to-bundles.sh" ]]; then
    "${SCRIPT_DIR}/2b_install-dylibs-to-bundles.sh" "${SCRIPT_DIR}/dist"
else
    log_error "2b_install-dylibs-to-bundles.sh not found"
    exit 1
fi

log_info ""
log_info "✓ Copying dylibs to app bundles completed"
log_info ""

# ============================================================================
# C) PACKAGING
# ============================================================================
printf "\n"
printf "==========================================\n"
printf "          B) PACKAGING\n"
printf "==========================================\n"
printf "\n"

if [[ -f "${SCRIPT_DIR}/2c_package-apps.sh" ]]; then
    "${SCRIPT_DIR}/2c_package-apps.sh"
else
    log_error "2c_package-apps.sh not found"
    exit 1
fi

log_info ""
log_info "✓ Packaging completed"
log_info ""

# ============================================================================
# D) DISTRIBUTION DMG
# ============================================================================
printf "\n"
printf "==========================================\n"
printf "          B) DISTRIBUTION DMG\n"
printf "==========================================\n"
printf "\n"

if [[ -f "${SCRIPT_DIR}/2d_create-distribution.sh" ]]; then
    "${SCRIPT_DIR}/2d_create-distribution.sh"
else
    log_error "2d_create-distribution.sh not found"
    exit 1
fi

log_info ""
log_info "✓ Packaging completed, ready to distribute!"
log_info ""
