#!/usr/bin/env bash

/veyon/.travis/common/linux-build.sh /veyon /build

cd /build

# add distribution name to file name
rename '.x86_64' '.fc26.x86_64' *.rpm

# show dependencies
rpm -qpR *.rpm

# move to Docker volume
mv -v *.rpm /veyon

