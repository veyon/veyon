#!/bin/bash
# package-veyon-macos-v2.sh - Empaquetado avanzado de Veyon para macOS
# Replica la distribución funcional anterior incluyendo frameworks y librerías auxiliares.

set -euo pipefail
IFS=$'\n\t'

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info()  { printf "${GREEN}[INFO]${NC} %s\n" "$*"; }
log_warn()  { printf "${YELLOW}[WARN]${NC} %s\n" "$*"; }
log_error() { printf "${RED}[ERROR]${NC} %s\n" "$*"; }

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DIST_DIR="${SCRIPT_DIR}/dist/Applications/Veyon"
PACKAGE_DIR="${SCRIPT_DIR}/veyon-macos-package"
FIX_SCRIPT="${SCRIPT_DIR}/tools/fix_bundle_deps.py"

MAIN_APPS=(veyon-configurator veyon-master)

QT5_CELLAR="/usr/local/Cellar/qt@5/5.15.13"
QT5_PLUGINS="/usr/local/opt/qt@5/plugins"
QT5_FRAMEWORKS_DIR="${QT5_CELLAR}/lib"

QT_HTTP_FRAMEWORK_SOURCE="$HOME/GitHub/qt5httpserver-build/lib"

QT_EXTRA_FRAMEWORKS=(QtDBus QtPrintSupport QtSvg QtQml QtQmlModels QtQuick QtPdf)
QT_HTTP_FRAMEWORKS=(QtHttpServer.framework QtSslServer.framework)

BREW_LIBS=(
	"/usr/local/opt/libpng/lib/libpng16.16.dylib"
	"/usr/local/opt/jpeg-turbo/lib/libjpeg.8.dylib"
	"/usr/local/opt/libtiff/lib/libtiff.6.dylib"
	"/usr/local/opt/webp/lib/libwebp.7.dylib"
	"/usr/local/opt/webp/lib/libwebpdemux.2.dylib"
	"/usr/local/opt/webp/lib/libwebpmux.3.dylib"
	"/usr/local/opt/xz/lib/liblzma.5.dylib"
	"/usr/local/opt/nspr/lib/libnspr4.dylib"
	"/usr/local/opt/nspr/lib/libplc4.dylib"
	"/usr/local/opt/nspr/lib/libplds4.dylib"
	"/usr/local/opt/nss/lib/libnss3.dylib"
	"/usr/local/opt/nss/lib/libnssutil3.dylib"
	"/usr/local/opt/nss/lib/libsmime3.dylib"
	"/usr/local/opt/nss/lib/libssl3.dylib"
)

OPENSSL_LIB_DIR="/opt/local/libexec/openssl3/lib"
OPENSSL_LIBS=(libssl.3.dylib libcrypto.3.dylib)
OPENSSL_MODULES_DIR="${OPENSSL_LIB_DIR}/ossl-modules"

QCA_FRAMEWORK="/usr/local/lib/qca-qt5.framework"

require_path() {
	local path="$1"
	if [[ ! -e "$path" ]]; then
		log_warn "No se encontró ${path}"
		return 1
	fi
	return 0
}

copy_if_exists() {
	local source="$1"
	local dest_dir="$2"
	if [[ -e "$source" ]]; then
		mkdir -p "$dest_dir"
		cp -R "$source" "$dest_dir/"
	else
		log_warn "No se pudo copiar ${source} (no existe)"
	fi
}

ensure_command() {
	if ! command -v "$1" >/dev/null 2>&1; then
		log_error "Comando requerido no encontrado: $1"
		exit 1
	fi
}

if [[ ! -d "$DIST_DIR" ]]; then
	log_error "No existe ${DIST_DIR}. Ejecuta 'cmake --build build --target install' antes."
	exit 1
fi

ensure_command macdeployqt
ensure_command install_name_tool
ensure_command python3

log_info "=== Empaquetado Veyon macOS (v2) ==="
log_info "Directorio de distribución: ${DIST_DIR}"

rm -rf "$PACKAGE_DIR"
mkdir -p "$PACKAGE_DIR"

package_app() {
	local app="$1"
	local source_app="${DIST_DIR}/${app}.app"
	local target_app="${PACKAGE_DIR}/${app}.app"

	if [[ ! -d "$source_app" ]]; then
		log_warn "No se encontró ${source_app}, se omite."
		return
	fi

	log_info "Empaquetando ${app}…"
	cp -R "$source_app" "$PACKAGE_DIR/"

	log_info "  Ejecutando macdeployqt…"
	macdeployqt "$target_app" -verbose=1 -always-overwrite

	if [[ -d "$target_app/Contents/PlugIns" ]]; then
		rm -rf "$target_app/Contents/PlugIns"
	fi
	mkdir -p "$target_app/Contents/PlugIns"

	if [[ -d "$QT5_PLUGINS" ]]; then
		log_info "  Copiando plugins de Qt 5…"
		for dir in platforms styles imageformats iconengines; do
			copy_if_exists "${QT5_PLUGINS}/${dir}" "$target_app/Contents/PlugIns"
		done
	else
		log_warn "No se encontraron plugins de Qt 5 en ${QT5_PLUGINS}"
	fi

	log_info "  Copiando frameworks adicionales de Qt…"
	for fw in "${QT_EXTRA_FRAMEWORKS[@]}"; do
		local source="${QT5_FRAMEWORKS_DIR}/${fw}.framework"
		if [[ -d "$source" ]]; then
			copy_if_exists "$source" "$target_app/Contents/Frameworks"
		fi
	done

	if [[ -d "$QT_HTTP_FRAMEWORK_SOURCE" ]]; then
		for fw in "${QT_HTTP_FRAMEWORKS[@]}"; do
			copy_if_exists "${QT_HTTP_FRAMEWORK_SOURCE}/${fw}" "$target_app/Contents/Frameworks"
		done
	else
		log_warn "No se encontró ${QT_HTTP_FRAMEWORK_SOURCE}; QtHttpServer/QtSslServer no se copiarán."
	fi

	if [[ -d "$QCA_FRAMEWORK" ]]; then
		copy_if_exists "$QCA_FRAMEWORK" "$target_app/Contents/Frameworks"
	else
		log_warn "qca-qt5.framework no encontrado en ${QCA_FRAMEWORK}"
	fi

	for lib_path in "${BREW_LIBS[@]}"; do
		if [[ -f "$lib_path" ]]; then
			copy_if_exists "$lib_path" "$target_app/Contents/Frameworks"
		else
			log_warn "Dependencia opcional no encontrada: ${lib_path}"
		fi
	done

	if [[ -d "$OPENSSL_LIB_DIR" ]]; then
		mkdir -p "$target_app/Contents/Frameworks/openssl"
		for lib in "${OPENSSL_LIBS[@]}"; do
			local src="${OPENSSL_LIB_DIR}/${lib}"
			if [[ -f "$src" ]]; then
				cp "$src" "$target_app/Contents/Frameworks/openssl/"
			else
				log_warn "OpenSSL ${lib} no encontrado en ${OPENSSL_LIB_DIR}"
			fi
		done
		if [[ -d "$OPENSSL_MODULES_DIR" ]]; then
			cp -R "$OPENSSL_MODULES_DIR" "$target_app/Contents/Frameworks/openssl/"
		fi
	else
		log_warn "No se encontró ${OPENSSL_LIB_DIR}; OpenSSL no se copiará."
	fi

	local icon="${SCRIPT_DIR}/resources/icons/${app}.icns"
	if [[ -f "$icon" ]]; then
		cp "$icon" "$target_app/Contents/Resources/"
	else
		log_warn "Icono ${icon} no encontrado (opcional)."
	fi

	log_info "  Ajustando rutas básicas con install_name_tool…"
	local exe_path="$target_app/Contents/MacOS/${app}"
	if [[ -f "$exe_path" ]]; then
		install_name_tool -change "@rpath/libveyon-core.dylib" "@executable_path/../lib/veyon/libveyon-core.dylib" "$exe_path" 2>/dev/null || true
		install_name_tool -change "/usr/local/lib/qca-qt5.framework/Versions/2/qca-qt5" "@executable_path/../Frameworks/qca-qt5.framework/Versions/2/qca-qt5" "$exe_path" 2>/dev/null || true
		for lib in "${OPENSSL_LIBS[@]}"; do
			install_name_tool -change "${OPENSSL_LIB_DIR}/${lib}" "@executable_path/../Frameworks/openssl/${lib}" "$exe_path" 2>/dev/null || true
		done
	fi
}

for app in "${MAIN_APPS[@]}"; do
	package_app "$app"
done

if [[ -f "$FIX_SCRIPT" ]]; then
	log_info "Ejecutando fix_bundle_deps.py…"
	if ! python3 "$FIX_SCRIPT" "$PACKAGE_DIR/veyon-master.app" "$PACKAGE_DIR/veyon-configurator.app"; then
		log_warn "fix_bundle_deps.py devolvió un error. Revisa la salida anterior."
	fi
else
	log_warn "No se encontró ${FIX_SCRIPT}; se omite la corrección de dependencias."
fi

log_info "Empaquetado finalizado. Bundles disponibles en ${PACKAGE_DIR}"
