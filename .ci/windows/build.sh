#!/usr/bin/env bash

set -e

BASEDIR=$(pwd)
BUILDDIR=/tmp/build-$1

$BASEDIR/.ci/common/strip-ultravnc-sources.sh

rm -rf $BUILDDIR
mkdir $BUILDDIR
cd $BUILDDIR

cmake $BASEDIR -DCMAKE_TOOLCHAIN_FILE=$BASEDIR/cmake/modules/Win${1}Toolchain.cmake -DCMAKE_MODULE_PATH=$BASEDIR/cmake/modules/ -G Ninja $CMAKE_FLAGS

if [ -z "$2" ] ; then
	ninja windows-binaries
else
	ninja ${@:2}
fi

mv veyon-*win* $BASEDIR

