#!/usr/bin/env bash

set -e

BASEDIR=$(pwd)
BUILDDIR=/tmp/build-$1

$BASEDIR/.ci/common/strip-ultravnc-sources.sh

rm -rf $BUILDDIR
mkdir $BUILDDIR
cd $BUILDDIR

PREFIX=/usr/$1-w64-mingw32
$PREFIX/bin/qt-cmake $BASEDIR \
	-G Ninja \
	-DWITH_QT6=ON \
	-DQT_TRANSLATIONS_DIR=$PREFIX/translations \
	$CMAKE_FLAGS

if [ -z "$2" ] ; then
	ninja create-windows-installer
else
	ninja ${@:2}
fi

mv *setup.exe $BASEDIR || true
mv InsightTeacher-* $BASEDIR || true
mv veyon-*win* $BASEDIR || true

