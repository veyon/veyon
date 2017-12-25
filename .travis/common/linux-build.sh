#!/usr/bin/env bash

set -e

SRC=$1
BUILD=$2
CPUS=$(cat /proc/cpuinfo | grep bogo | wc -l)

mkdir -p $BUILD
cd $BUILD

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=/usr $SRC

echo Building on $CPUS CPUs
make -j$((CPUS+1))

fakeroot make package

LIBDIR=$(grep VEYON_LIB_DIR CMakeCache.txt |cut -d "=" -f2)
BUILD_PWD=$(pwd)

mkdir -p $LIBDIR
cd $LIBDIR
find $BUILD_PWD/plugins -name "*.so" -exec ln -s '{}' ';'
cd $BUILD_PWD

./ctl/veyon-ctl help
./ctl/veyon-ctl about

