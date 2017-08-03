#!/usr/bin/env bash

set -e

/veyon/.travis/common/linux-build.sh /veyon /build

cd /build

# move generated DEB to mounted Docker volume
rename "s/_amd64/-debian-stretch_amd64/g" *.deb

dpkg -I *.deb

# generate source tarball
cd /veyon
VERSION=$(git describe --tags --abbrev=0)
cp /build/CONTRIBUTORS .

cd /
tar --transform "s,^veyon,^veyon-$VERSION," --exclude=".git" --exclude="*.deb" -cjf /build/veyon-$VERSION-src.tar.bz2 veyon

mv -v /build/*.deb /build/*.tar.bz2 /veyon


