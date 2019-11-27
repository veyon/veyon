#!/usr/bin/env bash

set -e

$1/.ci/common/linux-build.sh $1 $2
$1/.ci/common/finalize-deb.sh $1 $2 "debian-stretch"

# generate source tarball
cd $1
VERSION=$(git describe --tags --abbrev=0 | sed -e 's/^v//g')
cp $2/CONTRIBUTORS .

$1/.ci/common/strip-kitemmodels-sources.sh
$1/.ci/common/strip-kldap-sources.sh
$1/.ci/common/strip-libvncserver-sources.sh
$1/.ci/common/strip-ultravnc-sources.sh
$1/.ci/common/strip-x11vnc-sources.sh

cd /
tar --transform "s,^veyon,veyon-$VERSION," --exclude=".git" --exclude="*.deb" -cjf $2/veyon-$VERSION-src.tar.bz2 veyon

mv -v $2/*.tar.bz2 $1
