#!/usr/bin/env bash

set -e

SRC="$1"
BUILD="${2:-/tmp/build-nixos}"

mkdir -p "$BUILD"

# Enable nix flakes and nix-command if not already configured
mkdir -p /etc/nix
grep -q "experimental-features" /etc/nix/nix.conf 2>/dev/null || \
	echo "experimental-features = nix-command flakes" >> /etc/nix/nix.conf

if [ -n "$CI_COMMIT_TAG" ] ; then
	CMAKE_BUILD_TYPE="RelWithDebInfo"
	LTO="ON"
	TRANSLATIONS="ON"
else
	CMAKE_BUILD_TYPE="Debug"
	LTO="OFF"
	TRANSLATIONS="OFF"
fi

CMAKE_FLAGS="-DWITH_QT6=OFF -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DCMAKE_INSTALL_PREFIX=/usr -DWITH_LTO=$LTO -DWITH_TRANSLATIONS=$TRANSLATIONS"

# Use the nix development environment to get all build dependencies,
# then build from the local source tree (including checked-out submodules)
nix develop "$SRC" --command bash -c "
	cmake -G Ninja $CMAKE_FLAGS -S \"$SRC\" -B \"$BUILD\" && \
	cmake --build \"$BUILD\"
"

# Verify the build works
LIBDIR=$(grep VEYON_LIB_DIR "$BUILD/CMakeCache.txt" | cut -d '=' -f2)
if [ -z "$LIBDIR" ] ; then
	echo "ERROR: VEYON_LIB_DIR not found in CMakeCache.txt" >&2
	exit 1
fi
mkdir -p "$BUILD/$LIBDIR"
cd "$BUILD/$LIBDIR"
find "$BUILD/plugins" -name '*.so' -exec ln -sf '{}' ';'
cd "$BUILD"

"$BUILD/cli/veyon-cli" help
"$BUILD/cli/veyon-cli" about

# Install into a staging area and create an archive artifact
DESTDIR="$BUILD/install" nix develop "$SRC" --command bash -c "cmake --install \"$BUILD\""

VERSION=$(echo "$CI_COMMIT_TAG" | sed -e 's/^v//g')
if [ -z "$VERSION" ] ; then
	VERSION=$(echo "$GITHUB_REF_NAME" | sed -e 's/^v//g')
fi
if [ -z "$VERSION" ] ; then
	VERSION=$(git -C "$SRC" describe --tags --abbrev=0 | sed -e 's/^v//g')
fi

tar -czf "$SRC/veyon-$VERSION-nixos.tar.gz" -C "$BUILD/install" .
