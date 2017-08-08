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

make package

mkdir -p lib/veyon
cd lib/veyon
find ../../plugins -name "*.so" -exec ln -s '{}' ';'
cd ../../

./ctl/veyon-ctl help
./ctl/veyon-ctl about

