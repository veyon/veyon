#!/usr/bin/env bash

set -e

BASEDIR=$(pwd)
BUILDDIR=/tmp/build-$1

$BASEDIR/.ci/common/strip-ultravnc-sources.sh

rm -rf $BUILDDIR
mkdir $BUILDDIR
cd $BUILDDIR

/usr/$1-w64-mingw32/bin/qt-cmake $BASEDIR -G Ninja -DWITH_QT6=ON $CMAKE_FLAGS

if [ -z "$2" ] ; then
	ninja windows-binaries
else
	ninja ${@:2}
fi

mv veyon-*win* $BASEDIR

