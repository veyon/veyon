#!/usr/bin/env bash

/veyon/.travis/common/linux-build.sh /veyon /build

cd /build

rename "s/_amd64/-ubuntu-xenial_amd64/g" *.deb

dpkg -I *.deb

mv -v *.deb /veyon

