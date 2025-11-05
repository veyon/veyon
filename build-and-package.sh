#!/bin/bash
# build-and-package.sh - Script completo: compilar, empaquetar y crear DMG de distribución

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
# PASO 1: COMPILACIÓN
# ============================================================================

log_step "=== PASO 1: Compilación de Veyon ==="
log_info ""

if [[ ! -d "${SCRIPT_DIR}/build" ]]; then
    log_info "Configurando cmake por primera vez..."
    cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${SCRIPT_DIR}/dist" \
        -DCMAKE_PREFIX_PATH=/usr/local/opt/qt@5 \
        -DVEYON_BUILD_LINUX=OFF \
        -DVEYON_BUILD_WIN32=OFF \
        -DVEYON_BUILD_MACOS=ON
else
    log_info "Directorio build existente, omitiendo configuración cmake"
fi

log_info "Compilando..."
cmake --build build --parallel

log_info "Instalando en dist/..."
cmake --build build --target install

log_info ""
log_info "✓ Compilación completada"
log_info ""

# ============================================================================
# PASO 2: EMPAQUETADO
# ============================================================================

log_step "=== PASO 2: Empaquetado de aplicaciones ==="
log_info ""

if [[ -f "${SCRIPT_DIR}/package-veyon-macos-v3.sh" ]]; then
    "${SCRIPT_DIR}/package-veyon-macos-v3.sh"
else
    log_error "No se encontró package-veyon-macos-v3.sh"
    exit 1
fi

log_info ""
log_info "✓ Empaquetado completado"
log_info ""

# ============================================================================
# PASO 3: CREAR DISTRIBUCIÓN DMG
# ============================================================================

log_step "=== PASO 3: Creación de DMG de distribución ==="
log_info ""

if [[ -f "${SCRIPT_DIR}/create-distribution.sh" ]]; then
    "${SCRIPT_DIR}/create-distribution.sh"
else
    log_error "No se encontró create-distribution.sh"
    exit 1
fi

log_info ""
log_info "=========================================="
log_info "=== PROCESO COMPLETO FINALIZADO ✓ ==="
log_info "=========================================="
log_info ""
log_info "DMG de distribución listo en:"
log_info "  ${SCRIPT_DIR}/veyon-macos-distribution/Veyon-macOS.dmg"
log_info ""
log_info "Este DMG contiene:"
log_info "  - veyon-configurator.app"
log_info "  - veyon-master.app"
log_info "  - README.txt con instrucciones"
log_info ""
log_info "¡Listo para distribuir!"
log_info ""
