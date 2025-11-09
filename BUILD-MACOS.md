# Veyon macOS Build and Packaging Guide

This guide documents how to build, package, and distribute Veyon on macOS.

## Prerequisites

### Homebrew Dependencies
```bash
brew install qt@5 qthttpserver openldap cmake openssl qca
```

### Xcode Command Line Tools
```bash
xcode-select --install
```

## Available Scripts

The build and packaging process is organized into numbered scripts that follow a logical workflow:

### 1️⃣ `1_configure-cmake.sh` - CMake Configuration (Optional)

Configure or reconfigure CMake if you delete the `build/` directory:

```bash
./1_configure-cmake.sh
```

**What it does:**
- Checks if `build/` exists and prompts to recreate it
- Configures CMake with Qt5 (not Qt6)
- Sets all necessary OpenLDAP paths
- Configures build in Release mode

**When to use:**
- First time setup
- After deleting `build/` directory
- To reconfigure build settings

---

### 2️⃣ `2_build-package-distribution.sh` - Complete Build Process (ALL-IN-ONE)

**This is the main script** that executes the entire build, packaging, and distribution process:

```bash
./2_build-package-distribution.sh
```

**What it does:**
1. **STEP 1: BUILD**
   - Configures CMake (if `build/` doesn't exist)
   - Compiles the entire project in parallel
   - Installs binaries to `dist/`
   - Runs `2a_install-dylibs-to-bundles.sh` to copy libraries

2. **STEP 2: PACKAGING**
   - Runs `2b_package-apps.sh` to create complete app bundles
   - Includes all dependencies, frameworks, and plugins
   - Signs bundles with ad-hoc signature

3. **STEP 3: CREATE DISTRIBUTION**
   - Runs `2c_create-distribution.sh` to create DMG
   - Packages all apps with README

**Final result:**
- `veyon-macos-distribution/Veyon-macOS.dmg` (~250MB)

---

### Sub-Scripts (Called Automatically)

These scripts are called by `2_build-package-distribution.sh` but can also be run individually:

#### 2a️⃣ `2a_install-dylibs-to-bundles.sh` - Copy Libraries

Copies Veyon dylibs from `dist/lib/veyon/` to each app bundle's `Contents/lib/veyon/`:

```bash
./2a_install-dylibs-to-bundles.sh dist
```

#### 2b️⃣ `2b_package-apps.sh` - Package Applications

Creates complete, self-contained app bundles:

```bash
./2b_package-apps.sh
```

**What it does:**
- **Cleanup**: Removes old packaging artifacts automatically
- **Phase 0**: Copies libveyon-core.dylib and all Veyon plugins from BUILD
- **Phase 1**: Installs Qt 5 plugins (platforms, styles, imageformats, iconengines)
- **Phase 2**: Copies additional Qt frameworks (QtDBus, QtPrintSupport, QtSvg, QtPdf, etc.)
- **Phase 3**: Installs QCA framework
- **Phase 4**: Copies Homebrew dependencies (libpng, libjpeg, libtiff, etc.)
- **Phase 5**: Installs OpenSSL in dual location (for main executables and QCA plugins)
- **Phase 6**: Installs QCA crypto plugins (libqca-ossl.dylib, etc.)
- **Phase 7**: Creates QCA symlink (lib/qca-qt5/crypto)
- **Phase 8**: Disables problematic plugins (webapi, libqpdf, libqwebgl)
- **Phase 9**: Copies application icons
- **Phase 10**: Cleans build directory RPATHs (critical!)
- **Phase 11**: Adjusts basic RPATHs with install_name_tool
- Runs `fix_bundle_deps.py` for automatic dependency resolution
- Signs bundles with ad-hoc signature
- Creates README.txt with installation instructions

**Output:**
- `veyon-macos-package/veyon-configurator.app` (~179MB)
- `veyon-macos-package/veyon-master.app` (~179MB)
- `veyon-macos-package/veyon-server.app` (~178MB) ⭐

#### 2c️⃣ `2c_create-distribution.sh` - Create DMG

Creates a distributable DMG image:

```bash
./2c_create-distribution.sh
```

**What it does:**
- Copies all 3 applications to a temporary directory
- Copies README.txt
- Creates compressed DMG image

**Output:**
- `veyon-macos-distribution/Veyon-macOS.dmg` (~250MB)

---

## Directory Structure

```
veyon/
├── build/                           # Build directory (git-ignored, regenerable)
├── dist/                            # Install directory (git-ignored, regenerable)
├── veyon-macos-package/            # Packaged apps (git-ignored, regenerable)
│   ├── veyon-configurator.app
│   ├── veyon-master.app
│   └── veyon-server.app            # ⭐ Complete app bundle (required for Screen Recording)
├── veyon-macos-distribution/       # Final DMG (git-ignored, regenerable)
│   └── Veyon-macOS.dmg
├── 1_configure-cmake.sh            # [1] Configure CMake (optional)
├── 2_build-package-distribution.sh # [2] Main script - ALL-IN-ONE
├── 2a_install-dylibs-to-bundles.sh # [2a] Helper: copy dylibs
├── 2b_package-apps.sh              # [2b] Helper: package apps
└── 2c_create-distribution.sh       # [2c] Helper: create DMG
```

---

## Common Workflows

### First Build (from scratch)

```bash
./2_build-package-distribution.sh
```

This single command does everything: configure, build, package, and create DMG.

---

### After Code Changes

If you've made changes to the source code:

```bash
# Quick recompile and reinstall
cmake --build build --parallel
cmake --build build --target install
./2a_install-dylibs-to-bundles.sh dist

# Then repackage
./2b_package-apps.sh
./2c_create-distribution.sh
```

Or simply run the complete process again:
```bash
./2_build-package-distribution.sh
```

---

### If You Delete `build/` Accidentally

```bash
# Option 1: Use the configuration script
./1_configure-cmake.sh

# Option 2: Use the main script (auto-detects missing build/)
./2_build-package-distribution.sh
```

---

### Packaging Only (without rebuild)

If you already have compiled binaries and just want to repackage:

```bash
./2b_package-apps.sh
./2c_create-distribution.sh
```

---

## Important Changes for macOS

### veyon-server.app is Now a Complete App Bundle

In previous versions, `veyon-server` was a simple binary inside other apps. Now it's a complete app bundle because:

1. **macOS 12.3+ requires app bundles for Screen Recording**
   - ScreenCaptureKit API only works with signed apps in bundles
   - TCC (Transparency, Consent, and Control) only recognizes app bundles

2. **CMake changes:**
   - `server/CMakeLists.txt`: Removed `NO_BUNDLE` flag

3. **Packaging changes:**
   - `2b_package-apps.sh`: Moved from `HELPER_APPS` to `MAIN_APPS`
   - `2c_create-distribution.sh`: Added line to copy to DMG
   - Icon added: `resources/icons/veyon-server.icns`

### Helper Executables

Only `veyon-configurator.app` includes helper executables in `Contents/Resources/Helpers/`:
- `veyon-cli`
- `veyon-service` (wrapper that launches veyon-server.app)
- `veyon-worker`

Note: `veyon-auth-helper` stays only in `MacOS/` as it requires setuid.

### Administrator Authentication

`veyon-configurator` now requires administrator privileges to launch. A bash wrapper prompts for credentials using AppleScript before launching the main executable.

---

## Distribution

### For End Users

1. Distribute the `Veyon-macOS.dmg` file
2. Users mount the DMG (double click)
3. Drag all 3 applications to `/Applications`
4. **IMPORTANT**: Grant Screen Recording permission to `veyon-server.app`:
   - System Preferences → Security & Privacy → Privacy → Screen Recording
   - Add `/Applications/veyon-server.app`

### For Development

**DO NOT** distribute apps directly from Finder. Always use the DMG created by `2c_create-distribution.sh`.

---

## Troubleshooting

### Error: "Cannot find Qt6"

The project requires Qt5, not Qt6. Use `./1_configure-cmake.sh` which forces Qt5.

### Error: LDAP linking error

OpenLDAP paths must be expanded (don't use `$(brew ...)`). The `1_configure-cmake.sh` script uses absolute paths.

### Error: "Library not loaded: @rpath/libveyon-core.dylib"

Dylibs are not in the app bundle. Run:
```bash
./2a_install-dylibs-to-bundles.sh dist
```

Or repackage completely:
```bash
./2b_package-apps.sh
```

### Apps don't have icons

Icons must be in `resources/icons/` with correct names:
- `veyon-configurator.icns`
- `veyon-master.icns`
- `veyon-server.icns` ⭐

### Automatic Cleanup

The `2b_package-apps.sh` script now automatically cleans up old files before packaging:
- `dist/Applications/Veyon/`
- `veyon-macos-package/`
- Old DMG files (`veyon-*.dmg`)
- `veyon-macos-distribution/`

This ensures every packaging run starts with a clean slate.

---

## Files to NOT Commit (already in .gitignore)

- `build/` - Build directory
- `dist/` - Installed binaries
- `veyon-macos-package/` - Packaged apps
- `veyon-macos-distribution/` - Final DMG
- `dmg-temp/` - Temporary DMG directory

---

## Important Files to Commit

- ✅ `1_configure-cmake.sh`
- ✅ `2_build-package-distribution.sh`
- ✅ `2a_install-dylibs-to-bundles.sh`
- ✅ `2b_package-apps.sh`
- ✅ `2c_create-distribution.sh`
- ✅ `tools/fix_bundle_deps.py` - Automatic dependency resolver
- ✅ `resources/icons/veyon-server.icns` ⭐
- ✅ Changes in `server/CMakeLists.txt`
- ✅ `.gitignore` updated

---

## Script Naming Convention

Scripts follow a numbered convention for clarity:

- **1_** prefix: Initial configuration (optional, run once)
- **2_** prefix: Main build/package script
- **2a_, 2b_, 2c_** prefixes: Sub-scripts called by main script

This makes the workflow easy to understand and follow.

---

## Contact and Support

For macOS-specific issues, consult:
- [Official Veyon Documentation](https://veyon.readthedocs.io/)
- [GitHub Issues](https://github.com/veyon/veyon/issues)
