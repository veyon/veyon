#!/usr/bin/env bash

set -e

cd $2

# add distribution name to file name
rename ".x86_64" ".${3}.x86_64" *.rpm

# show files
rpm -qlp *.rpm

# show dependencies
rpm -qpR *.rpm

# move to Docker volume
mv -v *.rpm $1
