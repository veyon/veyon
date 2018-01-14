#!/usr/bin/env bash

CPUS=$(nproc)

cd /veyon

mkdir build
cd build

../cmake/build_mingw$1

echo Building on $CPUS CPUs
make -j$((CPUS))

make win-nsi
mv -v veyon*setup.exe /veyon

