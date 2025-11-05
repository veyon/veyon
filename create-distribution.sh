#!/bin/bash
# create-distribution.sh - Crear paquete de distribución único de Veyon para macOS

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info()  { printf "${GREEN}[INFO]${NC} %s\n" "$*"; }
log_error() { printf "${RED}[ERROR]${NC} %s\n" "$*"; }

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PACKAGE_DIR="${SCRIPT_DIR}/veyon-macos-package"
DIST_OUTPUT="${SCRIPT_DIR}/veyon-macos-distribution"
DMG_TEMP="${SCRIPT_DIR}/dmg-temp"

if [[ ! -d "$PACKAGE_DIR" ]]; then
    log_error "No existe ${PACKAGE_DIR}. Ejecuta package-veyon-macos-v3.sh primero."
    exit 1
fi

log_info "=== Creando paquete de distribución Veyon para macOS ==="

rm -rf "$DIST_OUTPUT" "$DMG_TEMP"
mkdir -p "$DIST_OUTPUT" "$DMG_TEMP"

log_info "Copiando aplicaciones al DMG temporal..."
cp -R "$PACKAGE_DIR/veyon-configurator.app" "$DMG_TEMP/"
cp -R "$PACKAGE_DIR/veyon-master.app" "$DMG_TEMP/"

if [[ -f "$PACKAGE_DIR/README.txt" ]]; then
    cp "$PACKAGE_DIR/README.txt" "$DMG_TEMP/"
fi

log_info "Creando imagen DMG..."
hdiutil create -volname "Veyon macOS" -srcfolder "$DMG_TEMP" -ov -format UDZO \
    "$DIST_OUTPUT/Veyon-macOS.dmg"

rm -rf "$DMG_TEMP"

DMG_SIZE=$(du -sh "$DIST_OUTPUT/Veyon-macOS.dmg" | cut -f1)

log_info ""
log_info "✓ Distribución completada"
log_info ""
log_info "Archivo creado:"
log_info "  Veyon-macOS.dmg (${DMG_SIZE})"
log_info ""
log_info "Ubicación: ${DIST_OUTPUT}/Veyon-macOS.dmg"
log_info ""
log_info "INSTRUCCIONES DE DISTRIBUCIÓN:"
log_info "  1. Distribuye el archivo Veyon-macOS.dmg"
log_info "  2. Los usuarios deben montar el DMG (doble clic)"
log_info "  3. Arrastrar las aplicaciones a /Applications"
log_info ""
log_info "IMPORTANTE: NO copies las .app directamente desde Finder,"
log_info "siempre usa el DMG para distribuir."
