#!/usr/bin/env bash

PACKAGES="cmake make nsis tofrodos g++-mingw-w64 qt5base-mingw-w64 qt5tools-mingw-w64
	openssl-mingw-w64 libz-mingw-w64-dev libpng-mingw-w64 libjpeg-turbo-mingw-w64
	interception-mingw-w64 openldap-mingw-w64 lzo2-mingw-w64"

sudo apt-get install -y --force-yes $PACKAGES
