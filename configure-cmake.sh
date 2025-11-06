#!/bin/bash
# configure-cmake.sh - Configuración de CMake para Veyon macOS
# Este script guarda la configuración correcta para poder recrear el build directory

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info()  { printf "${GREEN}[INFO]${NC} %s\n" "$*"; }
log_error() { printf "${RED}[ERROR]${NC} %s\n" "$*"; }
log_warn()  { printf "${YELLOW}[WARN]${NC} %s\n" "$*"; }

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

log_info "=== Configurando CMake para Veyon macOS ==="
log_info ""

# Verificar si build existe y avisar
if [[ -d "${SCRIPT_DIR}/build" ]]; then
    log_warn "El directorio build/ ya existe."
    read -p "¿Deseas eliminarlo y reconfigurar? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        log_info "Eliminando build/..."
        rm -rf "${SCRIPT_DIR}/build"
    else
        log_info "Manteniendo build/ existente. Abortando."
        exit 0
    fi
fi

log_info "Configurando CMake con Qt5..."
log_info ""

# Configuración CMake con todos los parámetros necesarios
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
log_info "✓ Configuración completada"
log_info ""
log_info "Ahora puedes compilar con:"
log_info "  cmake --build build --parallel"
log_info ""
log_info "O usar el script completo:"
log_info "  ./build-and-package.sh"
log_info ""
