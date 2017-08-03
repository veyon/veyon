#!/usr/bin/env bash

CPUS=$(cat /proc/cpuinfo | grep bogo | wc -l)

cd /veyon

mkdir build
cd build

../cmake/build_mingw$1

echo Building on $CPUS CPUs
make -j$((CPUS+1))

make win-nsi
mv -v veyon*setup.exe /veyon

