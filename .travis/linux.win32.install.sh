#!/usr/bin/env bash

PACKAGES="nsis tofrodos g++-mingw-w64 qt5base-mingw-w64 qt5tools-mingw-w64
	openssl-mingw-w64 libz-mingw-w64-dev libpng-mingw-w64 libjpeg-mingw-w64"

sudo apt-get install -y --force-yes $PACKAGES
