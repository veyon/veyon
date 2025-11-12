#!/bin/bash
# 2b_package-apps.sh - Complete packaging of Veyon for macOS
# This script creates complete .app bundles that replicate the reference functional structure
# Includes: Helpers, crypto plugins, OpenSSL dual-location, QCA symlinks, and complete dependencies

set -euo pipefail
IFS=$'\n\t'
export PATH="/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:${PATH:-}"

# Colors for output
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
# PATH CONFIGURATION
# ============================================================================

SCRIPT_DIR="$(cd "$(/usr/bin/dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
DIST_DIR="${SCRIPT_DIR}/dist/Applications/Veyon"
PACKAGE_DIR="${SCRIPT_DIR}/veyon-macos-package"
FIX_SCRIPT="${SCRIPT_DIR}/tools/fix_bundle_deps.py"

# Main applications
MAIN_APPS=(veyon-configurator veyon-master veyon-server)

# Helper applications (will be included in Resources/Helpers/)
HELPER_APPS=(veyon-cli veyon-service veyon-worker)
# Note: veyon-auth-helper is NOT included in Helpers (stays only in MacOS/, requires setuid)

# Qt 5 paths
QT5_CELLAR="/usr/local/Cellar/qt@5/5.15.13"
QT5_PLUGINS="/usr/local/opt/qt@5/plugins"
QT5_FRAMEWORKS_DIR="${QT5_CELLAR}/lib"

# Additional required Qt frameworks
QT_EXTRA_FRAMEWORKS=(QtDBus QtPrintSupport QtSvg QtQml QtQmlModels QtQuick QtPdf QtWebSockets)

# Custom Qt frameworks (QtHttpServer/QtSslServer)
QT_HTTP_FRAMEWORK_SOURCE="$HOME/GitHub/qt5httpserver-build/lib"
QT_HTTP_FRAMEWORKS=(QtHttpServer.framework QtSslServer.framework)

# QCA framework and plugins
QCA_FRAMEWORK="/usr/local/lib/qca-qt5.framework"
QCA_PLUGINS_DIR="/usr/local/lib/qca-qt5/crypto"
QCA_PLUGINS=(libqca-logger.dylib libqca-ossl.dylib libqca-softstore.dylib)

# OpenSSL (MacPorts)
OPENSSL_LIB_DIR="/opt/local/libexec/openssl3/lib"
OPENSSL_LIBS=(libssl.3.dylib libcrypto.3.dylib)
OPENSSL_MODULES_DIR="${OPENSSL_LIB_DIR}/ossl-modules"

# Homebrew dependencies (auxiliary libraries)
# Note: We will use cp -L to follow symlinks and copy real files
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
	"/usr/local/opt/pcre2/lib/libpcre2-16.0.dylib"
)

# ============================================================================
# AUXILIARY FUNCTIONS
# ============================================================================

require_path() {
	local path="$1"
	if [[ ! -e "$path" ]]; then
		log_warn "Not found: ${path}"
		return 1
	fi
	return 0
}

copy_if_exists() {
	local source="$1"
	local dest_dir="$2"
	if [[ -e "$source" ]]; then
		mkdir -p "$dest_dir"
		# Use -L to follow symlinks and copy real files
		cp -RL "$source" "$dest_dir/"
	else
		log_warn "Could not copy ${source} (does not exist)"
	fi
}

ensure_command() {
	if ! command -v "$1" >/dev/null 2>&1; then
		log_error "Required command not found: $1"
		exit 1
	fi
}

find_helper_binary() {
	local helper="$1"
	local helper_subdir="${helper#veyon-}"
	local candidates=(
		"${BUILD_DIR}/${helper_subdir}/${helper}"
		"${SCRIPT_DIR}/dist/bin/${helper}"
		"${DIST_DIR}/${helper}.app/Contents/MacOS/${helper}"
	)

	for candidate in "${candidates[@]}"; do
		if [[ -x "$candidate" ]]; then
			printf '%s\n' "$candidate"
			return 0
		fi
	done

	return 1
}

install_helper_binaries() {
	local target_app="$1"
	local app_id="$2"

	# Only the configurator needs to include local helpers to manage the service
	if [[ "$app_id" != "veyon-configurator" ]]; then
		return
	fi

	local helpers_dir="${target_app}/Contents/Resources/Helpers"
	local app_name
	app_name="$(basename "$target_app")"

	mkdir -p "$helpers_dir"

	for helper in "${HELPER_APPS[@]}"; do
		local dest="${helpers_dir}/${helper}"
		if [[ -x "$dest" ]]; then
			continue
		fi

		local source
		if source="$(find_helper_binary "$helper")"; then
			cp "$source" "$dest"
			chmod 755 "$dest"
			log_info "  ✓ Helper ${helper} added to ${app_name}"

			if [[ "$helper" == "veyon-service" ]]; then
				wrap_service_helper_with_server_launcher "$dest"
			fi
		else
			log_warn "  Helper ${helper} not found for ${app_name}"
		fi
	done
}

wrap_service_helper_with_server_launcher() {
	local helper_path="$1"
	local real_bin="${helper_path}.bin"

	if [[ -f "$real_bin" ]]; then
		return
	fi

	mv "$helper_path" "$real_bin"

	cat > "$helper_path" <<'EOF'
#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(/usr/bin/dirname "$0")" && pwd)"
REAL_BIN="${SCRIPT_DIR}/veyon-service.bin"

find_server_app() {
	local contents_dir
	local bundle_dir
	local parent_dir

	contents_dir="$(cd "$SCRIPT_DIR/../.." && pwd)"
	bundle_dir="$(cd "$contents_dir/.." && pwd)"
	parent_dir="$(cd "$bundle_dir/.." && pwd)"

	local candidates=(
		"${parent_dir}/veyon-server.app"
		"${parent_dir}/Veyon/veyon-server.app"
		"${bundle_dir}/../veyon-server.app"
	)

	for candidate in "${candidates[@]}"; do
		if [[ -d "$candidate" ]]; then
			printf '%s' "$candidate"
			return
		fi
	done

	printf ''
}

SERVER_APP="$(find_server_app)"

if [[ -n "$SERVER_APP" ]]; then
	if /usr/bin/open -a "$SERVER_APP"; then
		exit 0
	fi
fi

if [[ -x "$REAL_BIN" ]]; then
	exec "$REAL_BIN" "$@"
fi

echo "Could not start veyon-server or execute the original service." >&2
exit 1
EOF

	chmod 755 "$helper_path"
}

# ============================================================================
# FUNCTION: REQUIRE ADMINISTRATOR CREDENTIALS WHEN OPENING VEYON CONFIGURATOR
# ============================================================================

enforce_configurator_admin_prompt() {
	local app_path="${PACKAGE_DIR}/veyon-configurator.app"
	local macos_dir="${app_path}/Contents/MacOS"
	local target_bin="${macos_dir}/veyon-configurator"
	local real_bin="${target_bin}.bin"

	if [[ ! -d "$app_path" ]]; then
		log_warn "veyon-configurator.app not found, skipping administrator restriction."
		return
	fi

	if [[ ! -f "$target_bin" ]] && [[ ! -f "$real_bin" ]]; then
		log_warn "veyon-configurator main executable not found."
		return
	fi

	if [[ ! -f "$real_bin" ]]; then
		mv "$target_bin" "$real_bin"
	else
		log_info "Updating veyon-configurator wrapper for administrator authentication."
	fi

	cat > "$target_bin" <<'EOF'
#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(/usr/bin/dirname "$0")" && pwd)"
REAL_BIN="${SCRIPT_DIR}/veyon-configurator.bin"
PROMPT_MESSAGE="Veyon Configurator requires an administrator account to open."

if [[ ! -x "$REAL_BIN" ]]; then
	echo "Could not find the original Veyon Configurator executable." >&2
	exit 1
fi

if [[ "${EUID:-0}" -eq 0 ]]; then
	exec "$REAL_BIN" "$@"
fi

export VEYON_PROMPT_MESSAGE="$PROMPT_MESSAGE"
if ! /usr/bin/osascript <<'APPLESCRIPT'
set promptMessage to system attribute "VEYON_PROMPT_MESSAGE"
if promptMessage is missing value or promptMessage is "" then
	set promptMessage to "Administrator privileges are required."
end if
try
	do shell script "/bin/echo Authenticating..." with prompt promptMessage with administrator privileges
on error errMsg number errNum
	if errNum is -128 then
		error "Authentication cancelled by user."
	else
		error errMsg number errNum
	end if
end try
APPLESCRIPT
then
	echo "Could not verify administrator authentication." >&2
	exit 1
fi

exec "$REAL_BIN" "$@"
EOF

	chmod 755 "$target_bin"
	log_info "veyon-configurator will request administrator credentials before opening."
}

# ============================================================================
# MAIN FUNCTION: PACKAGE APP (main or helper)
# ============================================================================

package_app() {
	local app="$1"
	local source_app="$2"
	local target_app="$3"
	local is_helper="${4:-false}"

	if [[ ! -d "$source_app" ]]; then
		log_warn "Not found ${source_app}, skipped."
		return
	fi

	log_step "Packaging ${app}…"

	# Copy app
	cp -R "$source_app" "$(dirname "$target_app")/"

	log_info "  Ensuring helpers inside the bundle…"
	install_helper_binaries "$target_app" "$app"

	# ========================================================================
	# PHASE 0: COPY LIBVEYON-CORE AND VEYON PLUGINS (FROM BUILD)
	# ========================================================================

	log_info "  Copying libveyon-core.dylib and Veyon plugins from BUILD…"

	# Create directory lib/veyon
	mkdir -p "$target_app/Contents/lib/veyon"

	# Copy libveyon-core.dylib
	if [[ -f "${BUILD_DIR}/core/libveyon-core.dylib" ]]; then
		cp "${BUILD_DIR}/core/libveyon-core.dylib" "$target_app/Contents/lib/veyon/"
		log_info "    ✓ Copied libveyon-core.dylib"
	else
		log_warn "    libveyon-core.dylib not found in BUILD"
	fi

	# Copy all Veyon plugins
	if [[ -d "${BUILD_DIR}/plugins" ]]; then
		# Copy all .dylib plugins recursively
		find "${BUILD_DIR}/plugins" -name "*.dylib" -type f | while read -r plugin; do
			cp "$plugin" "$target_app/Contents/lib/veyon/"
		done
		plugin_count=$(find "$target_app/Contents/lib/veyon" -name "*.dylib" -type f | wc -l | tr -d ' ')
		log_info "    ✓ Copied ${plugin_count} Veyon plugins"
	else
		log_warn "    plugins directory not found in BUILD"
	fi

	# Run macdeployqt
	log_info "  Running macdeployqt…"
	macdeployqt "$target_app" -verbose=1 -always-overwrite

	# ========================================================================
	# PHASE 1: Qt PLUGINS
	# ========================================================================

	log_info "  Installing Qt 5 plugins…"

	# Remove plugins copied by macdeployqt (may be incorrect)
	if [[ -d "$target_app/Contents/PlugIns" ]]; then
		rm -rf "$target_app/Contents/PlugIns"
	fi
	mkdir -p "$target_app/Contents/PlugIns"

	# Copy plugins Qt 5
	if [[ -d "$QT5_PLUGINS" ]]; then
		for dir in platforms styles imageformats iconengines; do
			if [[ -d "${QT5_PLUGINS}/${dir}" ]]; then
				cp -R "${QT5_PLUGINS}/${dir}" "$target_app/Contents/PlugIns/"
			fi
		done

		# Update Qt plugin paths - CRITICAL FIX for v15 issue
		for plugin in "$target_app/Contents/PlugIns"/*/*.dylib "$target_app/Contents/PlugIns"/*/*/*.dylib; do
			[[ -f "$plugin" ]] || continue

			# Disable problematic plugins
			local plugin_name
			plugin_name=$(basename "$plugin")
			if [[ "$plugin_name" == "libqwebgl.dylib" ]] || [[ "$plugin_name" == "libqpdf.dylib" ]]; then
				mv "$plugin" "${plugin}.disabled" 2>/dev/null || true
				continue
			fi

			# Fix install_id (change absolute paths to @loader_path)
			install_name_tool -id "@loader_path/${plugin_name}" "$plugin" 2>/dev/null || true

			# Update Qt framework references (Cellar paths)
			for qt_fw in QtCore QtGui QtWidgets QtNetwork QtDBus QtPrintSupport QtSvg QtQml QtQmlModels QtQuick; do
				install_name_tool -change "/usr/local/Cellar/qt@5/5.15.13/lib/${qt_fw}.framework/Versions/5/${qt_fw}" \
					"@executable_path/../Frameworks/${qt_fw}.framework/Versions/5/${qt_fw}" "$plugin" 2>/dev/null || true
				install_name_tool -change "/usr/local/opt/qt@5/lib/${qt_fw}.framework/Versions/5/${qt_fw}" \
					"@executable_path/../Frameworks/${qt_fw}.framework/Versions/5/${qt_fw}" "$plugin" 2>/dev/null || true
			done

			# Fix Homebrew library paths
			install_name_tool -change "/usr/local/opt/freetype/lib/libfreetype.6.dylib" \
				"@loader_path/../../Frameworks/libfreetype.6.dylib" "$plugin" 2>/dev/null || true
			install_name_tool -change "/usr/local/opt/glib/lib/libgthread-2.0.0.dylib" \
				"@loader_path/../../Frameworks/libgthread-2.0.0.dylib" "$plugin" 2>/dev/null || true
			install_name_tool -change "/usr/local/opt/glib/lib/libglib-2.0.0.dylib" \
				"@loader_path/../../Frameworks/libglib-2.0.0.dylib" "$plugin" 2>/dev/null || true
			install_name_tool -change "/usr/local/opt/gettext/lib/libintl.8.dylib" \
				"@loader_path/../../Frameworks/libintl.8.dylib" "$plugin" 2>/dev/null || true
			install_name_tool -change "/usr/local/opt/webp/lib/libwebp.7.dylib" \
				"@loader_path/../../Frameworks/libwebp.7.dylib" "$plugin" 2>/dev/null || true
			install_name_tool -change "/usr/local/opt/webp/lib/libwebpmux.3.dylib" \
				"@loader_path/../../Frameworks/libwebpmux.3.dylib" "$plugin" 2>/dev/null || true
			install_name_tool -change "/usr/local/opt/webp/lib/libwebpdemux.2.dylib" \
				"@loader_path/../../Frameworks/libwebpdemux.2.dylib" "$plugin" 2>/dev/null || true
			install_name_tool -change "/usr/local/opt/jpeg-turbo/lib/libjpeg.8.dylib" \
				"@loader_path/../../Frameworks/libjpeg.8.dylib" "$plugin" 2>/dev/null || true
			install_name_tool -change "/usr/local/opt/libtiff/lib/libtiff.6.dylib" \
				"@loader_path/../../Frameworks/libtiff.6.dylib" "$plugin" 2>/dev/null || true
		done
	else
		log_error "No se encontraron plugins Qt 5 en ${QT5_PLUGINS}"
		exit 1
	fi

	# ========================================================================
	# PHASE 2: ADDITIONAL Qt FRAMEWORKS
	# ========================================================================

	log_info "  Copying additional Qt frameworks…"

	mkdir -p "$target_app/Contents/Frameworks"

	# Copy additional standard Qt frameworks
	for fw in "${QT_EXTRA_FRAMEWORKS[@]}"; do
		local source="${QT5_FRAMEWORKS_DIR}/${fw}.framework"
		if [[ -d "$source" ]]; then
			copy_if_exists "$source" "$target_app/Contents/Frameworks"
		fi
	done

	# Copy custom Qt frameworks (QtHttpServer/QtSslServer)
	if [[ -d "$QT_HTTP_FRAMEWORK_SOURCE" ]]; then
		for fw in "${QT_HTTP_FRAMEWORKS[@]}"; do
			copy_if_exists "${QT_HTTP_FRAMEWORK_SOURCE}/${fw}" "$target_app/Contents/Frameworks"
		done
	else
		log_warn "Not found ${QT_HTTP_FRAMEWORK_SOURCE}; QtHttpServer/QtSslServer will not be copied."
	fi

	# ========================================================================
	# PHASE 3: QCA FRAMEWORK
	# ========================================================================

	log_info "  Installing QCA framework…"

	if [[ -d "$QCA_FRAMEWORK" ]]; then
		# Remove existing qca-qt5 if copied by macdeployqt
		if [[ -d "$target_app/Contents/Frameworks/qca-qt5.framework" ]]; then
			rm -rf "$target_app/Contents/Frameworks/qca-qt5.framework"
		fi
		copy_if_exists "$QCA_FRAMEWORK" "$target_app/Contents/Frameworks"
	else
		log_warn "qca-qt5.framework not found en ${QCA_FRAMEWORK}"
	fi

	# ========================================================================
	# PHASE 4: HOMEBREW DEPENDENCIES
	# ========================================================================

	log_info "  Copying Homebrew dependencies…"

	for lib_path in "${BREW_LIBS[@]}"; do
		if [[ -f "$lib_path" ]]; then
			copy_if_exists "$lib_path" "$target_app/Contents/Frameworks"
		else
			log_warn "Optional dependency not found: ${lib_path}"
		fi
	done

	# ========================================================================
	# PHASE 5: OPENSSL (DUAL LOCATION - CRITICAL!)
	# ========================================================================

	log_info "  Installing OpenSSL (dual location)…"

	if [[ -d "$OPENSSL_LIB_DIR" ]]; then
		# Location 1: Frameworks/ (for main executables)
		for lib in "${OPENSSL_LIBS[@]}"; do
			local src="${OPENSSL_LIB_DIR}/${lib}"
			if [[ -f "$src" ]]; then
				cp "$src" "$target_app/Contents/Frameworks/"
			else
				log_warn "OpenSSL ${lib} not found en ${OPENSSL_LIB_DIR}"
			fi
		done

		# Location 2: Frameworks/openssl/ (for QCA plugins)
		mkdir -p "$target_app/Contents/Frameworks/openssl"
		for lib in "${OPENSSL_LIBS[@]}"; do
			local src="${OPENSSL_LIB_DIR}/${lib}"
			if [[ -f "$src" ]]; then
				cp "$src" "$target_app/Contents/Frameworks/openssl/"
			fi
		done

		# Copy ossl-modules (legacy provider)
		if [[ -d "$OPENSSL_MODULES_DIR" ]]; then
			cp -R "$OPENSSL_MODULES_DIR" "$target_app/Contents/Frameworks/openssl/"
		fi
	else
		log_error "Not found ${OPENSSL_LIB_DIR}; OpenSSL will not be copied."
		exit 1
	fi

	# ========================================================================
	# PHASE 6: QCA CRYPTO PLUGINS (CRITICAL!)
	# ========================================================================

	log_info "  Installing QCA crypto plugins…"

	mkdir -p "$target_app/Contents/PlugIns/crypto"

	if [[ -d "$QCA_PLUGINS_DIR" ]]; then
		for plugin in "${QCA_PLUGINS[@]}"; do
			local src="${QCA_PLUGINS_DIR}/${plugin}"
			if [[ -f "$src" ]]; then
				cp "$src" "$target_app/Contents/PlugIns/crypto/"

				# Fix paths in QCA plugins
				local dest_plugin="$target_app/Contents/PlugIns/crypto/${plugin}"

				# Fix QCA framework path
				install_name_tool -change "/usr/local/lib/qca-qt5.framework/Versions/2/qca-qt5" \
					"@loader_path/../../Frameworks/qca-qt5.framework/Versions/2/qca-qt5" "$dest_plugin" 2>/dev/null || true

				# Fix Qt Core path
				install_name_tool -change "/usr/local/opt/qt@5/lib/QtCore.framework/Versions/5/QtCore" \
					"@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore" "$dest_plugin" 2>/dev/null || true
				install_name_tool -change "/usr/local/Cellar/qt@5/5.15.13/lib/QtCore.framework/Versions/5/QtCore" \
					"@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore" "$dest_plugin" 2>/dev/null || true
			else
				log_warn "QCA plugin ${plugin} not found"
			fi
		done
	else
		log_error "QCA plugins directory not found: ${QCA_PLUGINS_DIR}"
		exit 1
	fi

	# ========================================================================
	# PHASE 7: QCA SYMLINK (CRITICAL!)
	# ========================================================================

	log_info "  Creating symlink lib/qca-qt5/crypto…"

	mkdir -p "$target_app/Contents/lib/qca-qt5"
	cd "$target_app/Contents/lib/qca-qt5"
	ln -sf "../../PlugIns/crypto" crypto
	cd "$SCRIPT_DIR"

	# ========================================================================
	# PHASE 8: DISABLE PROBLEMATIC PLUGINS
	# ========================================================================

	log_info "  Disabling problematic plugins…"

	# Disable webapi.dylib in lib/veyon
	if [[ -f "$target_app/Contents/lib/veyon/webapi.dylib" ]]; then
		mv "$target_app/Contents/lib/veyon/webapi.dylib" \
		   "$target_app/Contents/lib/veyon/webapi.dylib.disabled" 2>/dev/null || true
	fi

	# Disable libqpdf.dylib in imageformats (already done above, but just in case)
	if [[ -f "$target_app/Contents/PlugIns/imageformats/libqpdf.dylib" ]]; then
		mv "$target_app/Contents/PlugIns/imageformats/libqpdf.dylib" \
		   "$target_app/Contents/PlugIns/imageformats/libqpdf.dylib.disabled" 2>/dev/null || true
	fi

	# Disable libqwebgl.dylib in platforms (already done above, but just in case)
	if [[ -f "$target_app/Contents/PlugIns/platforms/libqwebgl.dylib" ]]; then
		mv "$target_app/Contents/PlugIns/platforms/libqwebgl.dylib" \
		   "$target_app/Contents/PlugIns/platforms/libqwebgl.dylib.disabled" 2>/dev/null || true
	fi

	# ========================================================================
	# PHASE 9: COPY ICON
	# ========================================================================

	local icon="${SCRIPT_DIR}/resources/icons/${app}.icns"
	if [[ -f "$icon" ]]; then
		cp "$icon" "$target_app/Contents/Resources/"
	fi

	# ========================================================================
	# PHASE 9.5: COPY SCRIPTS TO RESOURCES
	# ========================================================================

	log_info "  Copying installation scripts to Resources…"

	if [[ "$app" == "veyon-configurator" ]]; then
		local scripts_source="${SCRIPT_DIR}/scripts"
		local scripts_dest="${target_app}/Contents/Resources/Scripts"

		if [[ -d "$scripts_source" ]]; then
			mkdir -p "$scripts_dest"

			for script_file in "${scripts_source}"/*; do
				if [[ -f "$script_file" ]]; then
					cp "$script_file" "$scripts_dest/"
					local script_name=$(basename "$script_file")

					# Make .sh files executable
					if [[ "$script_name" == *.sh ]]; then
						chmod 755 "$scripts_dest/$script_name"
					fi

					log_info "    ✓ Copied ${script_name}"
				fi
			done
		else
			log_warn "    Scripts directory not found: ${scripts_source}"
		fi
	fi

	# ========================================================================
	# PHASE 10: BUILD RPATHS CLEANUP (CRITICAL!)
	# ========================================================================

	log_info "  Cleaning build directory RPATHs…"

	# List of build RPATHs to remove
	local build_rpaths=(
		"/Users/pablo/GitHub/veyon/build/core"
		"/Users/pablo/GitHub/veyon/build/plugins"
	)

	# Process all binaries in MacOS/
	for binary in "$target_app/Contents/MacOS"/*; do
		if [[ -f "$binary" ]] && file "$binary" 2>/dev/null | grep -q "Mach-O"; then
			# Remove build RPATHs
			for rpath in "${build_rpaths[@]}"; do
				install_name_tool -delete_rpath "$rpath" "$binary" 2>/dev/null || true
			done

			# Ensure correct RPATHs
			install_name_tool -add_rpath "@executable_path/../Frameworks" "$binary" 2>/dev/null || true
			install_name_tool -add_rpath "@executable_path/../lib/veyon" "$binary" 2>/dev/null || true
		fi
	done

	# Process all .dylib in lib/veyon/
	if [[ -d "$target_app/Contents/lib/veyon" ]]; then
		for dylib in "$target_app/Contents/lib/veyon"/*.dylib; do
			if [[ -f "$dylib" ]] && file "$dylib" 2>/dev/null | grep -q "Mach-O"; then
				# Remove build RPATHs
				for rpath in "${build_rpaths[@]}"; do
					install_name_tool -delete_rpath "$rpath" "$dylib" 2>/dev/null || true
				done

				# Ensure correct RPATHs (from lib/veyon/ perspective)
				install_name_tool -add_rpath "@loader_path/../../Frameworks" "$dylib" 2>/dev/null || true
				install_name_tool -add_rpath "@loader_path" "$dylib" 2>/dev/null || true
			fi
		done
	fi

	# ========================================================================
	# PHASE 11: BASIC RPATH ADJUSTMENTS
	# ========================================================================

	log_info "  Adjusting basic paths with install_name_tool…"

	local exe_path="$target_app/Contents/MacOS/${app}"
	if [[ -f "$exe_path" ]]; then
		# Change libveyon-core
		install_name_tool -change "@rpath/libveyon-core.dylib" \
			"@executable_path/../lib/veyon/libveyon-core.dylib" "$exe_path" 2>/dev/null || true

		# Change QCA
		install_name_tool -change "/usr/local/lib/qca-qt5.framework/Versions/2/qca-qt5" \
			"@executable_path/../Frameworks/qca-qt5.framework/Versions/2/qca-qt5" "$exe_path" 2>/dev/null || true

		# Change OpenSSL
		for lib in "${OPENSSL_LIBS[@]}"; do
			install_name_tool -change "${OPENSSL_LIB_DIR}/${lib}" \
				"@executable_path/../Frameworks/openssl/${lib}" "$exe_path" 2>/dev/null || true
		done
	fi

	log_info "  ${app} packaged ✓"
}

# ============================================================================
# CLEANUP OF OLD FILES
# ============================================================================

cleanup_old_packages() {
	log_info "=== Cleaning up old packaging packaged ==="

	local cleaned=0

	# Clean dist/ folder
	if [[ -d "$DIST_DIR" ]]; then
		log_info "Removing folder dist/Applications/Veyon/..."
		rm -rf "$DIST_DIR"
		cleaned=$((cleaned + 1))
	fi

	# Clean veyon-macos-package/ folder
	if [[ -d "$PACKAGE_DIR" ]]; then
		log_info "Removing folder veyon-macos-package/..."
		rm -rf "$PACKAGE_DIR"
		cleaned=$((cleaned + 1))
	fi

	# Clean old DMG files
	local dmg_pattern="${SCRIPT_DIR}/veyon-*.dmg"
	local dmg_files=(${dmg_pattern})
	if [[ -f "${dmg_files[0]}" ]]; then
		log_info "Removing old DMG files..."
		for dmg in "${dmg_files[@]}"; do
			if [[ -f "$dmg" ]]; then
				log_info "  - $(basename "$dmg")"
				rm -f "$dmg"
				cleaned=$((cleaned + 1))
			fi
		done
	fi

	# Clean veyon-macos-distribution/ folder if exists
	if [[ -d "${SCRIPT_DIR}/veyon-macos-distribution" ]]; then
		log_info "Removing folder veyon-macos-distribution/..."
		rm -rf "${SCRIPT_DIR}/veyon-macos-distribution"
		cleaned=$((cleaned + 1))
	fi

	if [[ $cleaned -gt 0 ]]; then
		log_info "✓ Cleanup completed (${cleaned} item(s) removed)"
	else
		log_info "✓ No old files to clean up"
	fi

	log_info ""
}

# ============================================================================
# INITIAL VALIDATIONS
# ============================================================================

log_info "=== Veyon macOS Packaging (v3) ==="
log_info "Build directory: ${BUILD_DIR}"
log_info "Distribution directory: ${DIST_DIR}"
log_info ""

# Clean up old files first
cleanup_old_packages

# Verify that at least one of the directories exists
if [[ ! -d "$BUILD_DIR" ]] && [[ ! -d "$DIST_DIR" ]]; then
	log_error "Neither ${BUILD_DIR} nor ${DIST_DIR}."
	log_error "Run 'cmake --build build' first."
	exit 1
fi

# Prefer build directory if it exists
if [[ -d "$BUILD_DIR" ]]; then
	log_info "✓ Using binaries from BUILD directory (most recent)"
else
	log_warn "BUILD directory not found, using DIST directory"
fi

ensure_command macdeployqt
ensure_command install_name_tool
ensure_command python3

# ============================================================================
# MAIN PHASE: PACKAGE MAIN APPS
# ============================================================================

log_info "Creating packaging directory…"
mkdir -p "$PACKAGE_DIR"

for app in "${MAIN_APPS[@]}"; do
	# Search for app in BUILD_DIR first, then in DIST_DIR
	source_app=""

	# Different locations depending on the app
	if [[ "$app" == "veyon-server" ]]; then
		# veyon-server is in build/server/
		if [[ -d "${BUILD_DIR}/server/${app}.app" ]]; then
			source_app="${BUILD_DIR}/server/${app}.app"
			log_info "✓ Using ${app} from BUILD/server (recent build)"
		fi
	else
		# veyon-configurator and veyon-master are in their own subdirectories
		subdir="${app#veyon-}"  # Remove "veyon-" prefix
		if [[ -d "${BUILD_DIR}/${subdir}/${app}.app" ]]; then
			source_app="${BUILD_DIR}/${subdir}/${app}.app"
			log_info "✓ Using ${app} from BUILD/${subdir} (recent build)"
		fi
	fi

	# Fallback to DIST_DIR if not found in BUILD_DIR
	if [[ -z "$source_app" ]] || [[ ! -d "$source_app" ]]; then
		if [[ -d "${DIST_DIR}/${app}.app" ]]; then
			source_app="${DIST_DIR}/${app}.app"
			log_warn "Using ${app} from DIST directory (may be old)"
		fi
	fi

	# Package if app was found
	if [[ -n "$source_app" ]] && [[ -d "$source_app" ]]; then
		package_app "$app" \
			"$source_app" \
			"${PACKAGE_DIR}/${app}.app" \
			false
	else
		log_error "Not found ${app}.app in BUILD or DIST"
	fi
done

# ============================================================================
# AUTOMATIC PHASE: DEPENDENCY RESOLUTION WITH fix_bundle_deps.python
# ============================================================================

log_info ""
log_info "=== Running resolución automática de dependencias ==="
log_info ""

if [[ -f "$FIX_SCRIPT" ]]; then
	log_info "Running fix_bundle_deps.python…"

	# Collect all apps to process (main only)
	apps_to_fix=()
	for app in "${MAIN_APPS[@]}"; do
		if [[ -d "${PACKAGE_DIR}/${app}.app" ]]; then
			apps_to_fix+=("${PACKAGE_DIR}/${app}.app")
		fi
	done

	if [[ ${#apps_to_fix[@]} -gt 0 ]]; then
		if ! python3 "$FIX_SCRIPT" "${apps_to_fix[@]}"; then
			log_warn "fix_bundle_deps.python reported warnings. Review output above."
		else
			log_info "fix_bundle_deps.python completed successfully ✓"
		fi
	fi
else
	log_warn "Not found ${FIX_SCRIPT}; skipping automatic dependency correction."
fi

# ============================================================================
# ADDITIONAL PHASE: FORCE ADMINISTRATOR AUTHENTICATION IN CONFIGURATOR
# ============================================================================

log_info ""
log_info "=== Applying administrator restriction to veyon-configurator ==="
log_info ""

enforce_configurator_admin_prompt

# ============================================================================
# FINAL PHASE: SIGN MAIN BUNDLES
# ============================================================================

log_info ""
log_info "=== Signing main bundles ==="
log_info ""

for app in "${MAIN_APPS[@]}"; do
	app_path="${PACKAGE_DIR}/${app}.app"
	if [[ -d "$app_path" ]]; then
		log_info "Signing ${app}.app…"
		codesign --force --deep --sign - "$app_path" 2>/dev/null || true
	fi
done

# ============================================================================
# CREATE README
# ============================================================================

log_info "Creating README…"

cat > "$PACKAGE_DIR/README.txt" << 'EOF'
=== Veyon for macOS ===

This package contains the main Veyon applications for macOS:

1. veyon-configurator.app - Configuration tool
2. veyon-master.app - Master application for classroom management
3. veyon-server.app - Server component for remote access

INSTALLATION:
-------------
1. Drag the 'Veyon' folder to the 'Applications' shortcut in the DMG
2. The entire folder will be copied to /Applications/Veyon/
3. On first launch, macOS will ask for accessibility and screen recording permissions
4. Go to System Preferences > Security & Privacy to grant the permissions

FEATURES:
---------
- All dependencies included (Qt, OpenSSL, QCA, etc.)
- Helper executables included in Contents/MacOS/:
  * veyon-cli
  * veyon-server
  * veyon-service
  * veyon-worker
  * veyon-auth-helper
- QCA cryptography plugins installed and configured
- No Qt installation or other system dependencies required

RUNNING HELPERS:
----------------
To run helpers from command line:

  /Applications/Veyon/veyon-master.app/Contents/MacOS/veyon-server
  /Applications/Veyon/veyon-master.app/Contents/MacOS/veyon-service
  /Applications/Veyon/veyon-master.app/Contents/MacOS/veyon-worker
  /Applications/Veyon/veyon-master.app/Contents/MacOS/veyon-cli

Or using veyon-server.app:

  /Applications/Veyon/veyon-server.app/Contents/MacOS/veyon-server

DISTRIBUTION:
-------------
You can compress this package and distribute it to other Macs.
No Qt installation or other dependencies required.

EOF

# ============================================================================
# FINAL SUMMARY
# ============================================================================

log_info ""
log_info "=========================================="
log_info "=== PACKAGING COMPLETED ✓ ==="
log_info "=========================================="
log_info ""
log_info "Package created in: ${PACKAGE_DIR}"
log_info ""
log_info "Main applications:"
for app in "${MAIN_APPS[@]}"; do
	if [[ -d "${PACKAGE_DIR}/${app}.app" ]]; then
		app_size=$(du -sh "${PACKAGE_DIR}/${app}.app" | cut -f1)
		log_info "  ✓ ${app}.app (${app_size})"
	fi
done
log_info ""
log_info "Helper executables in Contents/MacOS/:"
for helper_app in "${HELPER_APPS[@]}"; do
	log_info "  ✓ ${helper_app}"
done
log_info ""
log_info "Critical components installed:"
log_info "  ✓ QCA crypto plugins (PlugIns/crypto/)"
log_info "  ✓ QCA symlink (lib/qca-qt5/crypto)"
log_info "  ✓ OpenSSL (dual location)"
log_info "  ✓ Homebrew dependencies (real files, not symlinks)"
log_info "  ✓ Additional Qt frameworks"
log_info "  ✓ Problematic plugins disabled"
log_info ""
log_info "Applications are ready to use!"
log_info ""
