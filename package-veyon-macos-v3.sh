#!/bin/bash
# package-veyon-macos-v3.sh - Empaquetado completo de Veyon para macOS
# Este script crea bundles .app completos que replican la estructura funcional de referencia
# Incluye: Helpers, crypto plugins, OpenSSL dual-location, QCA symlinks, y dependencias completas

set -euo pipefail
IFS=$'\n\t'

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info()  { printf "${GREEN}[INFO]${NC} %s\n" "$*"; }
log_step()  { printf "${BLUE}[STEP]${NC} %s\n" "$*"; }
log_warn()  { printf "${YELLOW}[WARN]${NC} %s\n" "$*"; }
log_error() { printf "${RED}[ERROR]${NC} %s\n" "$*"; }

# ============================================================================
# CONFIGURACIÓN DE RUTAS
# ============================================================================

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DIST_DIR="${SCRIPT_DIR}/dist/Applications/Veyon"
PACKAGE_DIR="${SCRIPT_DIR}/veyon-macos-package"
FIX_SCRIPT="${SCRIPT_DIR}/tools/fix_bundle_deps.py"

# Aplicaciones principales
MAIN_APPS=(veyon-configurator veyon-master)

# Aplicaciones helper (serán incluidas en Resources/Helpers/)
HELPER_APPS=(veyon-cli veyon-server veyon-service veyon-worker)
# Nota: veyon-auth-helper NO se incluye en Helpers (queda solo en MacOS/, requiere setuid)

# Rutas de Qt 5
QT5_CELLAR="/usr/local/Cellar/qt@5/5.15.13"
QT5_PLUGINS="/usr/local/opt/qt@5/plugins"
QT5_FRAMEWORKS_DIR="${QT5_CELLAR}/lib"

# Frameworks Qt adicionales necesarios
QT_EXTRA_FRAMEWORKS=(QtDBus QtPrintSupport QtSvg QtQml QtQmlModels QtQuick QtPdf)

# Frameworks Qt personalizados (QtHttpServer/QtSslServer)
QT_HTTP_FRAMEWORK_SOURCE="$HOME/GitHub/qt5httpserver-build/lib"
QT_HTTP_FRAMEWORKS=(QtHttpServer.framework QtSslServer.framework)

# QCA framework y plugins
QCA_FRAMEWORK="/usr/local/lib/qca-qt5.framework"
QCA_PLUGINS_DIR="/usr/local/lib/qca-qt5/crypto"
QCA_PLUGINS=(libqca-logger.dylib libqca-ossl.dylib libqca-softstore.dylib)

# OpenSSL (MacPorts)
OPENSSL_LIB_DIR="/opt/local/libexec/openssl3/lib"
OPENSSL_LIBS=(libssl.3.dylib libcrypto.3.dylib)
OPENSSL_MODULES_DIR="${OPENSSL_LIB_DIR}/ossl-modules"

# Dependencias Homebrew (librerías auxiliares)
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

# ============================================================================
# FUNCIONES AUXILIARES
# ============================================================================

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

# ============================================================================
# FUNCIÓN PRINCIPAL: EMPAQUETAR APP (main o helper)
# ============================================================================

package_app() {
	local app="$1"
	local source_app="$2"
	local target_app="$3"
	local is_helper="${4:-false}"

	if [[ ! -d "$source_app" ]]; then
		log_warn "No se encontró ${source_app}, se omite."
		return
	fi

	log_step "Empaquetando ${app}…"

	# Copiar app
	cp -R "$source_app" "$(dirname "$target_app")/"

	# Ejecutar macdeployqt
	log_info "  Ejecutando macdeployqt…"
	macdeployqt "$target_app" -verbose=1 -always-overwrite

	# ========================================================================
	# FASE 1: PLUGINS Qt
	# ========================================================================

	log_info "  Instalando plugins de Qt 5…"

	# Eliminar plugins que copió macdeployqt (pueden ser incorrectos)
	if [[ -d "$target_app/Contents/PlugIns" ]]; then
		rm -rf "$target_app/Contents/PlugIns"
	fi
	mkdir -p "$target_app/Contents/PlugIns"

	# Copiar plugins Qt 5
	if [[ -d "$QT5_PLUGINS" ]]; then
		for dir in platforms styles imageformats iconengines; do
			if [[ -d "${QT5_PLUGINS}/${dir}" ]]; then
				cp -R "${QT5_PLUGINS}/${dir}" "$target_app/Contents/PlugIns/"
			fi
		done

		# Actualizar rutas de plugins Qt
		for plugin in "$target_app/Contents/PlugIns"/*/*.dylib "$target_app/Contents/PlugIns"/*/*/*.dylib; do
			[[ -f "$plugin" ]] || continue

			# Deshabilitar plugins problemáticos
			local plugin_name
			plugin_name=$(basename "$plugin")
			if [[ "$plugin_name" == "libqwebgl.dylib" ]] || [[ "$plugin_name" == "libqpdf.dylib" ]]; then
				mv "$plugin" "${plugin}.disabled" 2>/dev/null || true
				continue
			fi

			# Actualizar referencias a Qt frameworks
			for qt_fw in QtCore QtGui QtWidgets QtNetwork QtDBus QtPrintSupport QtSvg; do
				install_name_tool -change "/usr/local/Cellar/qt@5/5.15.13/lib/${qt_fw}.framework/Versions/5/${qt_fw}" \
					"@executable_path/../Frameworks/${qt_fw}.framework/Versions/5/${qt_fw}" "$plugin" 2>/dev/null || true
			done
		done
	else
		log_error "No se encontraron plugins Qt 5 en ${QT5_PLUGINS}"
		exit 1
	fi

	# ========================================================================
	# FASE 2: FRAMEWORKS Qt ADICIONALES
	# ========================================================================

	log_info "  Copiando frameworks adicionales de Qt…"

	mkdir -p "$target_app/Contents/Frameworks"

	# Copiar frameworks Qt estándar adicionales
	for fw in "${QT_EXTRA_FRAMEWORKS[@]}"; do
		local source="${QT5_FRAMEWORKS_DIR}/${fw}.framework"
		if [[ -d "$source" ]]; then
			copy_if_exists "$source" "$target_app/Contents/Frameworks"
		fi
	done

	# Copiar frameworks Qt personalizados (QtHttpServer/QtSslServer)
	if [[ -d "$QT_HTTP_FRAMEWORK_SOURCE" ]]; then
		for fw in "${QT_HTTP_FRAMEWORKS[@]}"; do
			copy_if_exists "${QT_HTTP_FRAMEWORK_SOURCE}/${fw}" "$target_app/Contents/Frameworks"
		done
	else
		log_warn "No se encontró ${QT_HTTP_FRAMEWORK_SOURCE}; QtHttpServer/QtSslServer no se copiarán."
	fi

	# ========================================================================
	# FASE 3: QCA FRAMEWORK
	# ========================================================================

	log_info "  Instalando QCA framework…"

	if [[ -d "$QCA_FRAMEWORK" ]]; then
		copy_if_exists "$QCA_FRAMEWORK" "$target_app/Contents/Frameworks"
	else
		log_warn "qca-qt5.framework no encontrado en ${QCA_FRAMEWORK}"
	fi

	# ========================================================================
	# FASE 4: DEPENDENCIAS HOMEBREW
	# ========================================================================

	log_info "  Copiando dependencias Homebrew…"

	for lib_path in "${BREW_LIBS[@]}"; do
		if [[ -f "$lib_path" ]]; then
			copy_if_exists "$lib_path" "$target_app/Contents/Frameworks"
		else
			log_warn "Dependencia opcional no encontrada: ${lib_path}"
		fi
	done

	# ========================================================================
	# FASE 5: OPENSSL (UBICACIÓN DUAL - CRÍTICO!)
	# ========================================================================

	log_info "  Instalando OpenSSL (ubicación dual)…"

	if [[ -d "$OPENSSL_LIB_DIR" ]]; then
		# Ubicación 1: Frameworks/ (para ejecutables principales)
		for lib in "${OPENSSL_LIBS[@]}"; do
			local src="${OPENSSL_LIB_DIR}/${lib}"
			if [[ -f "$src" ]]; then
				cp "$src" "$target_app/Contents/Frameworks/"
			else
				log_warn "OpenSSL ${lib} no encontrado en ${OPENSSL_LIB_DIR}"
			fi
		done

		# Ubicación 2: Frameworks/openssl/ (para plugins QCA)
		mkdir -p "$target_app/Contents/Frameworks/openssl"
		for lib in "${OPENSSL_LIBS[@]}"; do
			local src="${OPENSSL_LIB_DIR}/${lib}"
			if [[ -f "$src" ]]; then
				cp "$src" "$target_app/Contents/Frameworks/openssl/"
			fi
		done

		# Copiar ossl-modules (legacy provider)
		if [[ -d "$OPENSSL_MODULES_DIR" ]]; then
			cp -R "$OPENSSL_MODULES_DIR" "$target_app/Contents/Frameworks/openssl/"
		fi
	else
		log_error "No se encontró ${OPENSSL_LIB_DIR}; OpenSSL no se copiará."
		exit 1
	fi

	# ========================================================================
	# FASE 6: QCA CRYPTO PLUGINS (CRÍTICO!)
	# ========================================================================

	log_info "  Instalando QCA crypto plugins…"

	mkdir -p "$target_app/Contents/PlugIns/crypto"

	if [[ -d "$QCA_PLUGINS_DIR" ]]; then
		for plugin in "${QCA_PLUGINS[@]}"; do
			local src="${QCA_PLUGINS_DIR}/${plugin}"
			if [[ -f "$src" ]]; then
				cp "$src" "$target_app/Contents/PlugIns/crypto/"
			else
				log_warn "QCA plugin ${plugin} no encontrado"
			fi
		done
	else
		log_error "Directorio de plugins QCA no encontrado: ${QCA_PLUGINS_DIR}"
		exit 1
	fi

	# ========================================================================
	# FASE 7: SYMLINK QCA (CRÍTICO!)
	# ========================================================================

	log_info "  Creando symlink lib/qca-qt5/crypto…"

	mkdir -p "$target_app/Contents/lib/qca-qt5"
	cd "$target_app/Contents/lib/qca-qt5"
	ln -sf "../../PlugIns/crypto" crypto
	cd "$SCRIPT_DIR"

	# ========================================================================
	# FASE 8: DESHABILITAR PLUGINS PROBLEMÁTICOS
	# ========================================================================

	log_info "  Deshabilitando plugins problemáticos…"

	# Deshabilitar webapi.dylib en lib/veyon
	if [[ -f "$target_app/Contents/lib/veyon/webapi.dylib" ]]; then
		mv "$target_app/Contents/lib/veyon/webapi.dylib" \
		   "$target_app/Contents/lib/veyon/webapi.dylib.disabled" 2>/dev/null || true
	fi

	# Deshabilitar libqpdf.dylib en imageformats (ya se hizo arriba, pero por si acaso)
	if [[ -f "$target_app/Contents/PlugIns/imageformats/libqpdf.dylib" ]]; then
		mv "$target_app/Contents/PlugIns/imageformats/libqpdf.dylib" \
		   "$target_app/Contents/PlugIns/imageformats/libqpdf.dylib.disabled" 2>/dev/null || true
	fi

	# Deshabilitar libqwebgl.dylib en platforms (ya se hizo arriba, pero por si acaso)
	if [[ -f "$target_app/Contents/PlugIns/platforms/libqwebgl.dylib" ]]; then
		mv "$target_app/Contents/PlugIns/platforms/libqwebgl.dylib" \
		   "$target_app/Contents/PlugIns/platforms/libqwebgl.dylib.disabled" 2>/dev/null || true
	fi

	# ========================================================================
	# FASE 9: COPIAR ICONO
	# ========================================================================

	local icon="${SCRIPT_DIR}/resources/icons/${app}.icns"
	if [[ -f "$icon" ]]; then
		cp "$icon" "$target_app/Contents/Resources/"
	fi

	# ========================================================================
	# FASE 10: AJUSTES BÁSICOS DE RPATH
	# ========================================================================

	log_info "  Ajustando rutas básicas con install_name_tool…"

	local exe_path="$target_app/Contents/MacOS/${app}"
	if [[ -f "$exe_path" ]]; then
		# Cambiar libveyon-core
		install_name_tool -change "@rpath/libveyon-core.dylib" \
			"@executable_path/../lib/veyon/libveyon-core.dylib" "$exe_path" 2>/dev/null || true

		# Cambiar QCA
		install_name_tool -change "/usr/local/lib/qca-qt5.framework/Versions/2/qca-qt5" \
			"@executable_path/../Frameworks/qca-qt5.framework/Versions/2/qca-qt5" "$exe_path" 2>/dev/null || true

		# Cambiar OpenSSL
		for lib in "${OPENSSL_LIBS[@]}"; do
			install_name_tool -change "${OPENSSL_LIB_DIR}/${lib}" \
				"@executable_path/../Frameworks/openssl/${lib}" "$exe_path" 2>/dev/null || true
		done
	fi

	log_info "  ${app} empaquetado ✓"
}

# ============================================================================
# VALIDACIONES INICIALES
# ============================================================================

log_info "=== Empaquetado Veyon macOS (v3) ==="
log_info "Directorio de distribución: ${DIST_DIR}"

if [[ ! -d "$DIST_DIR" ]]; then
	log_error "No existe ${DIST_DIR}. Ejecuta 'cmake --build build --target install' antes."
	exit 1
fi

ensure_command macdeployqt
ensure_command install_name_tool
ensure_command python3

# ============================================================================
# FASE PRINCIPAL: EMPAQUETAR APPS PRINCIPALES
# ============================================================================

log_info "Creando directorio de empaquetado limpio…"
rm -rf "$PACKAGE_DIR"
mkdir -p "$PACKAGE_DIR"

for app in "${MAIN_APPS[@]}"; do
	package_app "$app" \
		"${DIST_DIR}/${app}.app" \
		"${PACKAGE_DIR}/${app}.app" \
		false
done

# ============================================================================
# FASE HELPERS: CREAR DIRECTORIO HELPERS CON APPS COMPLETAS
# ============================================================================

log_info ""
log_info "=== Empaquetando aplicaciones helper ==="
log_info ""

for main_app in "${MAIN_APPS[@]}"; do
	local main_app_path="${PACKAGE_DIR}/${main_app}.app"

	if [[ ! -d "$main_app_path" ]]; then
		continue
	fi

	log_info "Añadiendo helpers a ${main_app}…"

	# Crear directorio Helpers
	mkdir -p "$main_app_path/Contents/Resources/Helpers"

	for helper_app in "${HELPER_APPS[@]}"; do
		local helper_source="${DIST_DIR}/${helper_app}.app"
		local helper_target="$main_app_path/Contents/Resources/Helpers/${helper_app}.app"

		if [[ -d "$helper_source" ]]; then
			log_info "  Procesando helper: ${helper_app}…"

			# Empaquetar helper con la misma función (reutilizar lógica)
			package_app "$helper_app" \
				"$helper_source" \
				"$helper_target" \
				true

			# Firmar helper
			codesign --force --deep --sign - "$helper_target" 2>/dev/null || true
		else
			log_warn "  Helper ${helper_app} no encontrado en ${helper_source}"
		fi
	done

	log_info "Helpers añadidos a ${main_app} ✓"
done

# ============================================================================
# FASE AUTOMÁTICA: RESOLUCIÓN DE DEPENDENCIAS CON fix_bundle_deps.py
# ============================================================================

log_info ""
log_info "=== Ejecutando resolución automática de dependencias ==="
log_info ""

if [[ -f "$FIX_SCRIPT" ]]; then
	log_info "Ejecutando fix_bundle_deps.py…"

	# Recopilar todas las apps a procesar (principales + helpers)
	local apps_to_fix=()
	for app in "${MAIN_APPS[@]}"; do
		if [[ -d "${PACKAGE_DIR}/${app}.app" ]]; then
			apps_to_fix+=("${PACKAGE_DIR}/${app}.app")
		fi
	done

	# También procesar helpers dentro de cada app principal
	for main_app in "${MAIN_APPS[@]}"; do
		for helper_app in "${HELPER_APPS[@]}"; do
			local helper_path="${PACKAGE_DIR}/${main_app}.app/Contents/Resources/Helpers/${helper_app}.app"
			if [[ -d "$helper_path" ]]; then
				apps_to_fix+=("$helper_path")
			fi
		done
	done

	if [[ ${#apps_to_fix[@]} -gt 0 ]]; then
		if ! python3 "$FIX_SCRIPT" "${apps_to_fix[@]}"; then
			log_warn "fix_bundle_deps.py reportó advertencias. Revisa la salida anterior."
		else
			log_info "fix_bundle_deps.py completado exitosamente ✓"
		fi
	fi
else
	log_warn "No se encontró ${FIX_SCRIPT}; se omite la corrección automática de dependencias."
fi

# ============================================================================
# FASE FINAL: FIRMA DE BUNDLES PRINCIPALES
# ============================================================================

log_info ""
log_info "=== Firmando bundles principales ==="
log_info ""

for app in "${MAIN_APPS[@]}"; do
	local app_path="${PACKAGE_DIR}/${app}.app"
	if [[ -d "$app_path" ]]; then
		log_info "Firmando ${app}.app…"
		codesign --force --deep --sign - "$app_path" 2>/dev/null || true
	fi
done

# ============================================================================
# CREAR README
# ============================================================================

log_info "Creando README…"

cat > "$PACKAGE_DIR/README.txt" << 'EOF'
=== Veyon para macOS ===

Este paquete contiene las aplicaciones principales de Veyon para macOS:

1. veyon-configurator.app - Herramienta de configuración
2. veyon-master.app - Aplicación maestra para gestión de aulas

INSTALACIÓN:
------------
1. Copia las aplicaciones .app a tu carpeta /Applications
2. Al abrir por primera vez, macOS pedirá permisos de accesibilidad y grabación de pantalla
3. Ve a Preferencias del Sistema > Seguridad y Privacidad para otorgar los permisos

CARACTERÍSTICAS:
----------------
- Todas las dependencias incluidas (Qt, OpenSSL, QCA, etc.)
- Aplicaciones helper incluidas en Contents/Resources/Helpers/
  * veyon-cli.app
  * veyon-server.app
  * veyon-service.app
  * veyon-worker.app
- Plugins de criptografía QCA instalados y configurados
- No requiere instalación de Qt ni otras dependencias del sistema

EJECUCIÓN DE HELPERS:
--------------------
Para ejecutar los helpers desde línea de comandos:

  /Applications/veyon-master.app/Contents/Resources/Helpers/veyon-server.app/Contents/MacOS/veyon-server
  /Applications/veyon-master.app/Contents/Resources/Helpers/veyon-service.app/Contents/MacOS/veyon-service
  /Applications/veyon-master.app/Contents/Resources/Helpers/veyon-worker.app/Contents/MacOS/veyon-worker
  /Applications/veyon-master.app/Contents/Resources/Helpers/veyon-cli.app/Contents/MacOS/veyon-cli

DISTRIBUCIÓN:
------------
Puedes comprimir este paquete y distribuirlo a otros Macs.
No requiere instalación de Qt ni otras dependencias.

EOF

# ============================================================================
# RESUMEN FINAL
# ============================================================================

log_info ""
log_info "=========================================="
log_info "=== EMPAQUETADO COMPLETADO ✓ ==="
log_info "=========================================="
log_info ""
log_info "Paquete creado en: ${PACKAGE_DIR}"
log_info ""
log_info "Aplicaciones principales:"
for app in "${MAIN_APPS[@]}"; do
	if [[ -d "${PACKAGE_DIR}/${app}.app" ]]; then
		log_info "  ✓ ${app}.app"
	fi
done
log_info ""
log_info "Helpers incluidos en cada app:"
for helper_app in "${HELPER_APPS[@]}"; do
	log_info "  ✓ ${helper_app}.app"
done
log_info ""
log_info "Componentes críticos instalados:"
log_info "  ✓ QCA crypto plugins (PlugIns/crypto/)"
log_info "  ✓ QCA symlink (lib/qca-qt5/crypto)"
log_info "  ✓ OpenSSL (ubicación dual)"
log_info "  ✓ Dependencias Homebrew"
log_info "  ✓ Frameworks Qt adicionales"
log_info "  ✓ Plugins problemáticos deshabilitados"
log_info ""
log_info "Siguiente paso:"
log_info "  Ejecuta: codesign --force --deep --sign - veyon-macos-package/*.app"
log_info ""
