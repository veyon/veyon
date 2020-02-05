#!/usr/bin/env bash

set -e

CPUS=$(nproc)
BASEDIR=$(pwd)
BUILDDIR=/tmp/build-$1

$BASEDIR/.ci/common/strip-ultravnc-sources.sh

rm -rf $BUILDDIR
mkdir $BUILDDIR
cd $BUILDDIR

cmake $BASEDIR -DCMAKE_TOOLCHAIN_FILE=$BASEDIR/cmake/modules/Win${1}Toolchain.cmake -DCMAKE_MODULE_PATH=$BASEDIR/cmake/modules/

echo Building on $CPUS CPUs

if [ -z "$2" ] ; then
	make windows-binaries -j$CPUS
else
	make ${@:2} -j$CPUS
fi

mv veyon-*win* $BASEDIR

