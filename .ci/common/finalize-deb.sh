#!/usr/bin/env bash

set -e

cd $2

# add distribution name to file name
rename "s/_amd64/-${3}_amd64/g" *.deb

# show content
dpkg -c *.deb

# show package information and dependencies
dpkg -I *.deb

# move to Docker volume
mv -v *.deb $1
