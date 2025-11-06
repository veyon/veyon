# Guía de Compilación y Empaquetado de Veyon para macOS

Esta guía documenta cómo compilar, empaquetar y distribuir Veyon en macOS.

## Requisitos Previos

### Dependencias de Homebrew
```bash
brew install qt@5 qthttpserver openldap cmake openssl qca
```

### Xcode Command Line Tools
```bash
xcode-select --install
```

## Scripts Disponibles

### 1. `configure-cmake.sh` - Configurar CMake
Recrea la configuración de CMake si borras el directorio `build/`:

```bash
./configure-cmake.sh
```

**Qué hace:**
- Verifica si `build/` existe y pregunta si quieres recrearlo
- Configura CMake con Qt5 (no Qt6)
- Establece todas las rutas necesarias de OpenLDAP
- Configura build en modo Release

### 2. `build-and-package.sh` - Compilar, Empaquetar y Crear DMG (TODO EN UNO)
Script completo que ejecuta todo el proceso:

```bash
./build-and-package.sh
```

**Qué hace:**
1. Configura CMake (si `build/` no existe)
2. Compila todo el proyecto
3. Instala en `dist/`
4. Copia dylibs a los app bundles
5. Empaqueta las aplicaciones (ejecuta `package-veyon-macos-v3.sh`)
6. Crea el DMG de distribución (ejecuta `create-distribution.sh`)

**Resultado final:**
- `veyon-macos-distribution/Veyon-macOS.dmg` (250MB)

### 3. `package-veyon-macos-v3.sh` - Solo Empaquetar
Solo ejecuta el empaquetado (requiere que `dist/` exista):

```bash
./package-veyon-macos-v3.sh
```

**Qué hace:**
- Copia las aplicaciones desde `dist/` a `veyon-macos-package/`
- Ejecuta `macdeployqt` en cada app
- Copia frameworks Qt, QCA, OpenSSL
- Instala plugins y dependencias Homebrew
- Ejecuta `fix_bundle_deps.py` para resolver todas las dependencias
- Firma los bundles con ad-hoc signature

**Resultado:**
- `veyon-macos-package/veyon-configurator.app` (179MB)
- `veyon-macos-package/veyon-master.app` (179MB)
- `veyon-macos-package/veyon-server.app` (178MB) ⭐

### 4. `create-distribution.sh` - Solo Crear DMG
Solo crea el DMG (requiere que `veyon-macos-package/` exista):

```bash
./create-distribution.sh
```

**Qué hace:**
- Copia las 3 aplicaciones a un directorio temporal
- Copia README.txt
- Crea el DMG comprimido

**Resultado:**
- `veyon-macos-distribution/Veyon-macOS.dmg` (250MB)

## Estructura de Directorios

```
veyon/
├── build/                           # Build directory (git-ignored, regenerable)
├── dist/                            # Install directory (git-ignored, regenerable)
├── veyon-macos-package/            # Apps empaquetadas (git-ignored, regenerable)
│   ├── veyon-configurator.app
│   ├── veyon-master.app
│   └── veyon-server.app            # ⭐ NUEVO - App bundle completo
├── veyon-macos-distribution/       # DMG final (git-ignored, regenerable)
│   └── Veyon-macOS.dmg
├── configure-cmake.sh              # Configurar CMake
├── build-and-package.sh            # Todo en uno
├── package-veyon-macos-v3.sh       # Solo empaquetar
├── create-distribution.sh          # Solo crear DMG
└── install-dylibs-to-bundles.sh    # Helper: copiar dylibs
```

## Flujo de Trabajo Común

### Primera Compilación (desde cero)
```bash
./build-and-package.sh
```

### Después de Cambios en el Código
```bash
# Solo recompilar e instalar
cmake --build build --parallel
cmake --build build --target install
./install-dylibs-to-bundles.sh dist

# Luego empaquetar
./package-veyon-macos-v3.sh
./create-distribution.sh
```

### Si Borras `build/` Accidentalmente
```bash
# Opción 1: Usar el script
./configure-cmake.sh

# Opción 2: Usar build-and-package.sh (detecta ausencia de build/)
./build-and-package.sh
```

## Cambios Importantes para macOS

### veyon-server.app es Ahora un App Bundle Completo
En versiones anteriores, `veyon-server` era un binario simple dentro de los otros apps. Ahora es un app bundle completo porque:

1. **macOS 12.3+ requiere app bundles para Screen Recording**
   - ScreenCaptureKit API solo funciona con apps firmadas en app bundles
   - TCC (Transparency, Consent, and Control) solo reconoce app bundles

2. **Cambios en CMake:**
   - [server/CMakeLists.txt](server/CMakeLists.txt:5): Removido `NO_BUNDLE`

3. **Cambios en Packaging:**
   - [package-veyon-macos-v3.sh](package-veyon-macos-v3.sh:31): Movido de `HELPER_APPS` a `MAIN_APPS`
   - [create-distribution.sh](create-distribution.sh:32): Añadida línea para copiar al DMG
   - Icono añadido: [resources/icons/veyon-server.icns](resources/icons/veyon-server.icns)

### Copiar dylibs a App Bundles
El script [install-dylibs-to-bundles.sh](install-dylibs-to-bundles.sh) copia los dylibs desde `dist/lib/veyon/` a `Contents/lib/veyon/` dentro de cada app bundle. Esto es necesario para que las apps funcionen sin variables de entorno.

## Distribución

### Para Usuarios Finales
1. Distribuir el archivo `Veyon-macOS.dmg`
2. Los usuarios montan el DMG (doble clic)
3. Arrastran las 3 aplicaciones a `/Applications`
4. **IMPORTANTE**: Dar permisos de Screen Recording a `veyon-server.app`:
   - System Preferences → Security & Privacy → Privacy → Screen Recording
   - Agregar `/Applications/veyon-server.app`

### Para Desarrollo
NO distribuyas las apps directamente desde Finder. Siempre usa el DMG creado por `create-distribution.sh`.

## Solución de Problemas

### Error: "Cannot find Qt6"
El proyecto requiere Qt5, no Qt6. Usa `./configure-cmake.sh` que fuerza Qt5.

### Error: LDAP linking error
Las rutas de OpenLDAP deben estar expandidas (no usar `$(brew ...)`). El script `configure-cmake.sh` usa rutas absolutas.

### Error: "Library not loaded: @rpath/libveyon-core.dylib"
Los dylibs no están en el app bundle. Ejecuta:
```bash
./install-dylibs-to-bundles.sh dist
```

### Las apps no tienen íconos
Los iconos deben estar en `resources/icons/` con el nombre correcto:
- `veyon-configurator.icns`
- `veyon-master.icns`
- `veyon-server.icns` ⭐

## Archivos a NO Commitear (ya en .gitignore)

- `build/` - Directorio de compilación
- `dist/` - Binarios instalados
- `veyon-macos-package/` - Apps empaquetadas
- `veyon-macos-distribution/` - DMG final
- `dmg-temp/` - Directorio temporal del DMG

## Archivos Importantes a Commitear

- ✅ `configure-cmake.sh`
- ✅ `build-and-package.sh`
- ✅ `package-veyon-macos-v3.sh`
- ✅ `create-distribution.sh`
- ✅ `install-dylibs-to-bundles.sh`
- ✅ `resources/icons/veyon-server.icns` ⭐
- ✅ Cambios en `server/CMakeLists.txt`
- ✅ `.gitignore` actualizado

## Contacto y Soporte

Para problemas específicos de macOS, consulta:
- [Documentación oficial de Veyon](https://veyon.readthedocs.io/)
- [GitHub Issues](https://github.com/veyon/veyon/issues)
