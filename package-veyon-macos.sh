#!/bin/bash
# Script para empaquetar Veyon para macOS con todas sus dependencias
# Este script crea bundles .app autónomos que pueden distribuirse sin dependencias externas

set -e  # Salir si hay algún error

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

echo_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

echo_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Directorio base
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DIST_DIR="${SCRIPT_DIR}/dist/Applications/Veyon"
PACKAGE_DIR="${SCRIPT_DIR}/veyon-macos-package"

# Aplicaciones principales que serán distribuidas como .app
MAIN_APPS=("veyon-configurator" "veyon-master")

# Aplicaciones auxiliares (serán incluidas dentro de los bundles principales)
AUX_APPS=("veyon-server" "veyon-service" "veyon-worker" "veyon-cli")

echo_info "=== Empaquetando Veyon para macOS ==="
echo_info "Directorio de distribución: $DIST_DIR"

# Verificar que existe el directorio de distribución
if [ ! -d "$DIST_DIR" ]; then
    echo_error "No se encuentra el directorio de distribución: $DIST_DIR"
    echo_error "Primero debes compilar el proyecto con: make install"
    exit 1
fi

# Verificar que macdeployqt está disponible
if ! command -v macdeployqt &> /dev/null; then
    echo_error "macdeployqt no está disponible. Instálalo con: brew install qt@5"
    exit 1
fi

# Crear directorio de empaquetado limpio
echo_info "Creando directorio de empaquetado..."
rm -rf "$PACKAGE_DIR"
mkdir -p "$PACKAGE_DIR"

# Función para empaquetar una aplicación
package_app() {
    local app_name="$1"
    local app_path="$DIST_DIR/${app_name}.app"

    if [ ! -d "$app_path" ]; then
        echo_warn "No se encuentra ${app_name}.app, saltando..."
        return
    fi

    echo_info "Empaquetando ${app_name}..."

    # Copiar la app al directorio de empaquetado
    cp -R "$app_path" "$PACKAGE_DIR/"

    local target_app="$PACKAGE_DIR/${app_name}.app"

    # Ejecutar macdeployqt para incluir frameworks de Qt y plugins
    echo_info "  Ejecutando macdeployqt para ${app_name}..."
    macdeployqt "$target_app" -verbose=1 -always-overwrite

    # FORZAR el uso de plugins de Qt 5 (eliminar lo que copió macdeployqt y copiar Qt 5)
    echo_info "  Forzando el uso de plugins de Qt 5..."

    # Eliminar cualquier plugin que haya copiado macdeployqt (pueden ser de Qt 6)
    rm -rf "$target_app/Contents/PlugIns"

    # Crear directorio de plugins y copiar SOLO desde Qt 5
    mkdir -p "$target_app/Contents/PlugIns"
    QT_PLUGIN_PATH="/usr/local/opt/qt@5/plugins"

    if [ -d "$QT_PLUGIN_PATH/platforms" ]; then
        echo_info "    Copiando plugins de Qt 5 desde: $QT_PLUGIN_PATH"
        cp -R "$QT_PLUGIN_PATH/platforms" "$target_app/Contents/PlugIns/"
        cp -R "$QT_PLUGIN_PATH/styles" "$target_app/Contents/PlugIns/" 2>/dev/null || true
        cp -R "$QT_PLUGIN_PATH/imageformats" "$target_app/Contents/PlugIns/" 2>/dev/null || true
        cp -R "$QT_PLUGIN_PATH/iconengines" "$target_app/Contents/PlugIns/" 2>/dev/null || true
        echo_info "    Plugins de Qt 5 copiados correctamente"

        # Actualizar las rutas de los plugins para que usen los frameworks del bundle
        echo_info "    Actualizando rutas de dependencias de TODOS los plugins de Qt..."
        for plugin in "$target_app/Contents/PlugIns"/*/*.dylib "$target_app/Contents/PlugIns"/*/*/*.dylib; do
            [ -f "$plugin" ] || continue

            # Deshabilitar plugins que requieren frameworks Qt no disponibles
            plugin_name=$(basename "$plugin")
            if [[ "$plugin_name" == "libqwebgl.dylib" ]] || [[ "$plugin_name" == "libqpdf.dylib" ]]; then
                echo_info "      Deshabilitando $plugin_name (requiere QtQuick/QtPdf no disponibles)..."
                mv "$plugin" "${plugin}.disabled" 2>/dev/null || true
                continue
            fi

            # Actualizar TODAS las referencias a Qt frameworks
            for qt_fw in QtCore QtGui QtWidgets QtNetwork QtDBus QtPrintSupport QtSvg; do
                install_name_tool -change "/usr/local/Cellar/qt@5/5.15.13/lib/${qt_fw}.framework/Versions/5/${qt_fw}" \
                    "@executable_path/../Frameworks/${qt_fw}.framework/Versions/5/${qt_fw}" "$plugin" 2>/dev/null || true
            done
        done
    else
        echo_error "    No se encontraron los plugins de Qt 5 en: $QT_PLUGIN_PATH"
        exit 1
    fi

    # Copiar dependencias adicionales (OpenSSL, QCA, etc.)
    echo_info "  Copiando dependencias adicionales..."

    # Crear directorio para frameworks adicionales si no existe
    mkdir -p "$target_app/Contents/Frameworks"

    # Copiar frameworks Qt adicionales requeridos por el plugin libqcocoa
    echo_info "    Copiando frameworks Qt adicionales para el plugin libqcocoa..."
    for qt_framework in QtDBus QtPrintSupport QtSvg; do
        if [ -d "/usr/local/Cellar/qt@5/5.15.13/lib/${qt_framework}.framework" ]; then
            if [ ! -d "$target_app/Contents/Frameworks/${qt_framework}.framework" ]; then
                cp -R "/usr/local/Cellar/qt@5/5.15.13/lib/${qt_framework}.framework" "$target_app/Contents/Frameworks/" 2>/dev/null || true

                # Actualizar las rutas para usar los frameworks del bundle
                install_name_tool -id "@rpath/${qt_framework}.framework/Versions/5/${qt_framework}" \
                    "$target_app/Contents/Frameworks/${qt_framework}.framework/Versions/5/${qt_framework}" 2>/dev/null || true
                install_name_tool -change "/usr/local/Cellar/qt@5/5.15.13/lib/QtCore.framework/Versions/5/QtCore" \
                    "@rpath/QtCore.framework/Versions/5/QtCore" \
                    "$target_app/Contents/Frameworks/${qt_framework}.framework/Versions/5/${qt_framework}" 2>/dev/null || true
                # QtPrintSupport y QtSvg también dependen de QtGui y QtWidgets
                for dep_fw in QtGui QtWidgets; do
                    install_name_tool -change "/usr/local/Cellar/qt@5/5.15.13/lib/${dep_fw}.framework/Versions/5/${dep_fw}" \
                        "@rpath/${dep_fw}.framework/Versions/5/${dep_fw}" \
                        "$target_app/Contents/Frameworks/${qt_framework}.framework/Versions/5/${qt_framework}" 2>/dev/null || true
                done
            fi
        fi
    done

    # Copiar QCA framework
    if [ -d "/usr/local/lib/qca-qt5.framework" ]; then
        if [ ! -d "$target_app/Contents/Frameworks/qca-qt5.framework" ]; then
            echo_info "    Copiando qca-qt5.framework..."
            cp -R "/usr/local/lib/qca-qt5.framework" "$target_app/Contents/Frameworks/"
        fi
    fi

    # Copiar OpenSSL libraries
    mkdir -p "$target_app/Contents/Frameworks/openssl"
    if [ -f "/opt/local/libexec/openssl3/lib/libssl.3.dylib" ]; then
        echo_info "    Copiando librerías OpenSSL..."
        cp "/opt/local/libexec/openssl3/lib/libssl.3.dylib" "$target_app/Contents/Frameworks/openssl/"
        cp "/opt/local/libexec/openssl3/lib/libcrypto.3.dylib" "$target_app/Contents/Frameworks/openssl/"
    fi

    # Copiar otras dependencias
    if [ -f "/usr/local/opt/libpng/lib/libpng16.16.dylib" ]; then
        echo_info "    Copiando libpng..."
        cp "/usr/local/opt/libpng/lib/libpng16.16.dylib" "$target_app/Contents/Frameworks/"
    fi

    if [ -f "/usr/local/opt/jpeg-turbo/lib/libjpeg.8.dylib" ]; then
        echo_info "    Copiando libjpeg..."
        cp "/usr/local/opt/jpeg-turbo/lib/libjpeg.8.dylib" "$target_app/Contents/Frameworks/"
    fi

    # Copiar icono de la aplicación
    echo_info "  Añadiendo icono de la aplicación..."
    local icon_file="${SCRIPT_DIR}/resources/icons/${app_name}.icns"
    if [ -f "$icon_file" ]; then
        cp "$icon_file" "$target_app/Contents/Resources/"
        # Actualizar Info.plist para que use el icono
        /usr/libexec/PlistBuddy -c "Set :CFBundleIconFile ${app_name}.icns" "$target_app/Contents/Info.plist" 2>/dev/null || true
        echo_info "    Icono ${app_name}.icns añadido ✓"
    else
        echo_info "    Icono no encontrado en: $icon_file (opcional)"
    fi

    # Actualizar rutas de dependencias en el ejecutable principal
    echo_info "  Actualizando rutas de dependencias..."
    local exe_path="$target_app/Contents/MacOS/${app_name}"

    # Cambiar rpath para libveyon-core
    install_name_tool -change "@rpath/libveyon-core.dylib" \
        "@executable_path/../lib/veyon/libveyon-core.dylib" "$exe_path" 2>/dev/null || true

    # Cambiar rutas de QCA
    install_name_tool -change "/usr/local/lib/qca-qt5.framework/Versions/2/qca-qt5" \
        "@executable_path/../Frameworks/qca-qt5.framework/Versions/2/qca-qt5" "$exe_path" 2>/dev/null || true

    # Cambiar rutas de OpenSSL
    install_name_tool -change "/opt/local/libexec/openssl3/lib/libssl.3.dylib" \
        "@executable_path/../Frameworks/openssl/libssl.3.dylib" "$exe_path" 2>/dev/null || true

    install_name_tool -change "/opt/local/libexec/openssl3/lib/libcrypto.3.dylib" \
        "@executable_path/../Frameworks/openssl/libcrypto.3.dylib" "$exe_path" 2>/dev/null || true

    # Cambiar rutas de libpng y libjpeg
    install_name_tool -change "/usr/local/opt/libpng/lib/libpng16.16.dylib" \
        "@executable_path/../Frameworks/libpng16.16.dylib" "$exe_path" 2>/dev/null || true

    install_name_tool -change "/usr/local/opt/jpeg-turbo/lib/libjpeg.8.dylib" \
        "@executable_path/../Frameworks/libjpeg.8.dylib" "$exe_path" 2>/dev/null || true

    # Actualizar dependencias en libveyon-core.dylib
    if [ -f "$target_app/Contents/lib/veyon/libveyon-core.dylib" ]; then
        local core_lib="$target_app/Contents/lib/veyon/libveyon-core.dylib"

        install_name_tool -change "/usr/local/lib/qca-qt5.framework/Versions/2/qca-qt5" \
            "@executable_path/../Frameworks/qca-qt5.framework/Versions/2/qca-qt5" "$core_lib" 2>/dev/null || true

        install_name_tool -change "/opt/local/libexec/openssl3/lib/libssl.3.dylib" \
            "@executable_path/../Frameworks/openssl/libssl.3.dylib" "$core_lib" 2>/dev/null || true

        install_name_tool -change "/opt/local/libexec/openssl3/lib/libcrypto.3.dylib" \
            "@executable_path/../Frameworks/openssl/libcrypto.3.dylib" "$core_lib" 2>/dev/null || true

        install_name_tool -change "/usr/local/opt/libpng/lib/libpng16.16.dylib" \
            "@executable_path/../Frameworks/libpng16.16.dylib" "$core_lib" 2>/dev/null || true

        install_name_tool -change "/usr/local/opt/jpeg-turbo/lib/libjpeg.8.dylib" \
            "@executable_path/../Frameworks/libjpeg.8.dylib" "$core_lib" 2>/dev/null || true
    fi

    # Actualizar dependencias en todos los plugins
    if [ -d "$target_app/Contents/lib/veyon" ]; then
        for plugin in "$target_app/Contents/lib/veyon"/*.dylib; do
            [ -f "$plugin" ] || continue

            install_name_tool -change "@rpath/libveyon-core.dylib" \
                "@executable_path/../lib/veyon/libveyon-core.dylib" "$plugin" 2>/dev/null || true

            install_name_tool -change "/usr/local/lib/qca-qt5.framework/Versions/2/qca-qt5" \
                "@executable_path/../Frameworks/qca-qt5.framework/Versions/2/qca-qt5" "$plugin" 2>/dev/null || true

            install_name_tool -change "/opt/local/libexec/openssl3/lib/libssl.3.dylib" \
                "@executable_path/../Frameworks/openssl/libssl.3.dylib" "$plugin" 2>/dev/null || true

            install_name_tool -change "/opt/local/libexec/openssl3/lib/libcrypto.3.dylib" \
                "@executable_path/../Frameworks/openssl/libcrypto.3.dylib" "$plugin" 2>/dev/null || true

            # WORKAROUND: webapi.dylib tiene referencias a QtHttpServer y QtSslServer que no existen
            # En lugar de intentar arreglarlas, simplemente deshabilitamos el plugin renombrándolo
            if [[ "$(basename "$plugin")" == "webapi.dylib" ]]; then
                echo_info "    Deshabilitando webapi.dylib (tiene dependencias no disponibles)..."
                mv "$plugin" "${plugin}.disabled" 2>/dev/null || true
            fi
        done
    fi

    # Actualizar dependencias en OpenSSL (libssl depende de libcrypto)
    if [ -f "$target_app/Contents/Frameworks/openssl/libssl.3.dylib" ]; then
        install_name_tool -change "/opt/local/libexec/openssl3/lib/libcrypto.3.dylib" \
            "@executable_path/../Frameworks/openssl/libcrypto.3.dylib" \
            "$target_app/Contents/Frameworks/openssl/libssl.3.dylib" 2>/dev/null || true
    fi

    # Actualizar dependencias en QCA
    if [ -f "$target_app/Contents/Frameworks/qca-qt5.framework/Versions/2/qca-qt5" ]; then
        install_name_tool -change "/opt/local/libexec/openssl3/lib/libssl.3.dylib" \
            "@executable_path/../Frameworks/openssl/libssl.3.dylib" \
            "$target_app/Contents/Frameworks/qca-qt5.framework/Versions/2/qca-qt5" 2>/dev/null || true

        install_name_tool -change "/opt/local/libexec/openssl3/lib/libcrypto.3.dylib" \
            "@executable_path/../Frameworks/openssl/libcrypto.3.dylib" \
            "$target_app/Contents/Frameworks/qca-qt5.framework/Versions/2/qca-qt5" 2>/dev/null || true
    fi

    # Firmar el bundle (ad-hoc signature)
    echo_info "  Firmando ${app_name}..."
    codesign --force --deep --sign - "$target_app"

    echo_info "  ${app_name} empaquetado completamente ✓"
}

# Empaquetar aplicaciones principales
for app in "${MAIN_APPS[@]}"; do
    package_app "$app"
done

# Copiar aplicaciones auxiliares dentro de cada app principal
echo_info "Añadiendo aplicaciones auxiliares a los bundles principales..."
for main_app in "${MAIN_APPS[@]}"; do
    main_app_path="$PACKAGE_DIR/${main_app}.app"

    if [ ! -d "$main_app_path" ]; then
        continue
    fi

    # Crear directorio para recursos auxiliares
    mkdir -p "$main_app_path/Contents/Resources/Helpers"

    for aux_app in "${AUX_APPS[@]}"; do
        aux_app_path="$DIST_DIR/${aux_app}.app"

        if [ -d "$aux_app_path" ]; then
            echo_info "  Copiando ${aux_app} a ${main_app}..."
            cp -R "$aux_app_path" "$main_app_path/Contents/Resources/Helpers/"

            # Procesar el auxiliar también con macdeployqt
            helper_app="$main_app_path/Contents/Resources/Helpers/${aux_app}.app"
            macdeployqt "$helper_app" -verbose=1 -always-overwrite

            # FORZAR el uso de plugins de Qt 5 para helpers
            rm -rf "$helper_app/Contents/PlugIns"
            mkdir -p "$helper_app/Contents/PlugIns"
            QT_PLUGIN_PATH="/usr/local/opt/qt@5/plugins"
            if [ -d "$QT_PLUGIN_PATH/platforms" ]; then
                cp -R "$QT_PLUGIN_PATH/platforms" "$helper_app/Contents/PlugIns/" 2>/dev/null || true
                cp -R "$QT_PLUGIN_PATH/styles" "$helper_app/Contents/PlugIns/" 2>/dev/null || true
                cp -R "$QT_PLUGIN_PATH/imageformats" "$helper_app/Contents/PlugIns/" 2>/dev/null || true
                cp -R "$QT_PLUGIN_PATH/iconengines" "$helper_app/Contents/PlugIns/" 2>/dev/null || true

                # Actualizar rutas de TODOS los plugins (no solo platforms)
                for plugin in "$helper_app/Contents/PlugIns"/*/*.dylib "$helper_app/Contents/PlugIns"/*/*/*.dylib; do
                    [ -f "$plugin" ] || continue

                    # Deshabilitar plugins que requieren frameworks Qt no disponibles
                    plugin_name=$(basename "$plugin")
                    if [[ "$plugin_name" == "libqwebgl.dylib" ]] || [[ "$plugin_name" == "libqpdf.dylib" ]]; then
                        mv "$plugin" "${plugin}.disabled" 2>/dev/null || true
                        continue
                    fi

                    # Actualizar TODAS las referencias a Qt frameworks
                    for qt_fw in QtCore QtGui QtWidgets QtNetwork QtDBus QtPrintSupport QtSvg; do
                        install_name_tool -change "/usr/local/Cellar/qt@5/5.15.13/lib/${qt_fw}.framework/Versions/5/${qt_fw}" \
                            "@executable_path/../Frameworks/${qt_fw}.framework/Versions/5/${qt_fw}" "$plugin" 2>/dev/null || true
                    done
                done
            fi

            # Copiar frameworks si es necesario
            mkdir -p "$helper_app/Contents/Frameworks"

            # Copiar QtDBus framework (requerido por el plugin libqcocoa)
            if [ -d "/usr/local/Cellar/qt@5/5.15.13/lib/QtDBus.framework" ]; then
                cp -R "/usr/local/Cellar/qt@5/5.15.13/lib/QtDBus.framework" "$helper_app/Contents/Frameworks/" 2>/dev/null || true

                # Actualizar las rutas de QtDBus
                install_name_tool -id "@rpath/QtDBus.framework/Versions/5/QtDBus" \
                    "$helper_app/Contents/Frameworks/QtDBus.framework/Versions/5/QtDBus" 2>/dev/null || true
                install_name_tool -change "/usr/local/Cellar/qt@5/5.15.13/lib/QtCore.framework/Versions/5/QtCore" \
                    "@rpath/QtCore.framework/Versions/5/QtCore" \
                    "$helper_app/Contents/Frameworks/QtDBus.framework/Versions/5/QtDBus" 2>/dev/null || true
            fi

            if [ -d "/usr/local/lib/qca-qt5.framework" ]; then
                cp -R "/usr/local/lib/qca-qt5.framework" "$helper_app/Contents/Frameworks/" 2>/dev/null || true
            fi

            mkdir -p "$helper_app/Contents/Frameworks/openssl"
            cp "/opt/local/libexec/openssl3/lib/libssl.3.dylib" "$helper_app/Contents/Frameworks/openssl/" 2>/dev/null || true
            cp "/opt/local/libexec/openssl3/lib/libcrypto.3.dylib" "$helper_app/Contents/Frameworks/openssl/" 2>/dev/null || true

            # Actualizar rutas en el helper
            helper_exe="$helper_app/Contents/MacOS/${aux_app}"

            install_name_tool -change "@rpath/libveyon-core.dylib" \
                "@executable_path/../lib/veyon/libveyon-core.dylib" "$helper_exe" 2>/dev/null || true

            install_name_tool -change "/usr/local/lib/qca-qt5.framework/Versions/2/qca-qt5" \
                "@executable_path/../Frameworks/qca-qt5.framework/Versions/2/qca-qt5" "$helper_exe" 2>/dev/null || true

            install_name_tool -change "/opt/local/libexec/openssl3/lib/libssl.3.dylib" \
                "@executable_path/../Frameworks/openssl/libssl.3.dylib" "$helper_exe" 2>/dev/null || true

            install_name_tool -change "/opt/local/libexec/openssl3/lib/libcrypto.3.dylib" \
                "@executable_path/../Frameworks/openssl/libcrypto.3.dylib" "$helper_exe" 2>/dev/null || true

            # Actualizar dependencias en los plugins de Veyon del helper
            if [ -d "$helper_app/Contents/lib/veyon" ]; then
                for helper_plugin in "$helper_app/Contents/lib/veyon"/*.dylib; do
                    [ -f "$helper_plugin" ] || continue

                    install_name_tool -change "@rpath/libveyon-core.dylib" \
                        "@executable_path/../lib/veyon/libveyon-core.dylib" "$helper_plugin" 2>/dev/null || true

                    install_name_tool -change "/usr/local/lib/qca-qt5.framework/Versions/2/qca-qt5" \
                        "@executable_path/../Frameworks/qca-qt5.framework/Versions/2/qca-qt5" "$helper_plugin" 2>/dev/null || true

                    install_name_tool -change "/opt/local/libexec/openssl3/lib/libssl.3.dylib" \
                        "@executable_path/../Frameworks/openssl/libssl.3.dylib" "$helper_plugin" 2>/dev/null || true

                    install_name_tool -change "/opt/local/libexec/openssl3/lib/libcrypto.3.dylib" \
                        "@executable_path/../Frameworks/openssl/libcrypto.3.dylib" "$helper_plugin" 2>/dev/null || true

                    # Deshabilitar webapi.dylib también en helpers
                    if [[ "$(basename "$helper_plugin")" == "webapi.dylib" ]]; then
                        mv "$helper_plugin" "${helper_plugin}.disabled" 2>/dev/null || true
                    fi
                done
            fi

            # Firmar el helper
            codesign --force --deep --sign - "$helper_app"
        fi
    done

    # Firmar de nuevo el bundle principal después de añadir los helpers
    codesign --force --deep --sign - "$main_app_path"
done

# Crear un README con instrucciones
echo_info "Creando README de instalación..."
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

Las aplicaciones auxiliares (veyon-server, veyon-service, veyon-worker, veyon-cli)
están incluidas dentro de cada bundle en:
  <App>.app/Contents/Resources/Helpers/

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

echo_info "Creando archivo .dmg para distribución..."
# Crear un DMG para fácil distribución
DMG_NAME="Veyon-macOS-$(date +%Y%m%d).dmg"

if command -v create-dmg &> /dev/null; then
    create-dmg \
        --volname "Veyon" \
        --window-pos 200 120 \
        --window-size 600 400 \
        --icon-size 100 \
        --hide-extension "veyon-configurator.app" \
        --hide-extension "veyon-master.app" \
        --app-drop-link 450 150 \
        "$SCRIPT_DIR/$DMG_NAME" \
        "$PACKAGE_DIR"

    echo_info "DMG creado: $DMG_NAME"
else
    echo_warn "create-dmg no está instalado. Creando DMG básico..."
    hdiutil create -volname "Veyon" -srcfolder "$PACKAGE_DIR" -ov -format UDZO "$SCRIPT_DIR/$DMG_NAME"
    echo_info "DMG creado: $DMG_NAME"
fi

echo_info ""
echo_info "=== Empaquetado completado ==="
echo_info "Paquete creado en: $PACKAGE_DIR"
echo_info "DMG creado en: $SCRIPT_DIR/$DMG_NAME"
echo_info ""
echo_info "Aplicaciones incluidas:"
for app in "${MAIN_APPS[@]}"; do
    if [ -d "$PACKAGE_DIR/${app}.app" ]; then
        echo_info "  ✓ ${app}.app"
    fi
done
echo_info ""
echo_info "Helpers incluidos en cada app:"
for aux_app in "${AUX_APPS[@]}"; do
    echo_info "  ✓ ${aux_app}.app"
done
echo_info ""
echo_info "Ahora puedes:"
echo_info "  1. Probar las apps en: $PACKAGE_DIR"
echo_info "  2. Distribuir el DMG: $DMG_NAME"
echo_info "  3. Copiar el paquete a otro Mac sin Qt instalado"
